#include <M5Unified.h>
#include <M5GFX.h>
#include <WiFi.h>
#include <time.h>
#include <esp_sntp.h>

// Station configuration is in StationConfig.h — edit that file to select stations.
#include "StationConfig.h"
#include <Tides.h>
#include "FreeSansBold48pt7b.h"

// WiFi & NTP Configuration — add all known networks here
struct WiFiCredential { const char* ssid; const char* pass; };
const WiFiCredential WIFI_NETWORKS[] = {
 
  { "SFR_5360",   "0123456789"   },
   { "Chez-nous",   "yyy" }
  // { "Hotspot",     "password3"   },
};
const int WIFI_NETWORK_COUNT = sizeof(WIFI_NETWORKS) / sizeof(WIFI_NETWORKS[0]);

const char* NTP_TIMEZONE = "CET-1CEST,M3.5.0,M10.5.0/3";  // France timezone
const char* NTP_SERVER1 = "pool.ntp.org";
const char* NTP_SERVER2 = "time.nist.gov";

// WiFi state
int currentNetworkIndex = 0;
int lastSuccessfulNetworkIndex = -1;  // Remember which network last worked (-1 = none yet)
unsigned long lastWiFiCheckTime = 0;
unsigned long lastNTPSyncTime = 0;
unsigned long lastWiFiReconnectTime = 0;
const unsigned long WIFI_CHECK_INTERVAL = 5000;      // Check WiFi status every 5 seconds
const unsigned long NTP_SYNC_INTERVAL = 24 * 3600000; // Sync NTP every 24 hours

// WiFi scan state (scan-before-connect)
bool wifiAvailable[WIFI_NETWORK_COUNT] = {};  // which configured networks were found in scan
bool wifiScanInProgress = false;
int  wifiScanTargetIndex = -1;                // index to connect to after scan completes
unsigned long lastScanTime = 0;
const unsigned long SCAN_INTERVAL = 30000;    // rescan every 30s when not connected

M5Canvas canvas(&M5.Display);

static const int W = 1280;
static const int H = 720;


// -------- Display Tide Data --------
struct DisplayTideEvent {
  int hour;
  int minute;
  bool isHigh;
  int coefficient;
};

DisplayTideEvent today[4]    = {};
DisplayTideEvent tomorrow[4] = {};
TideInfo tideInfoToday;
TideInfo tideInfoTomorrow;

bool showTomorrow = false;  // Toggle with touch: false=Aujourd'hui, true=Demain

// -------- Chart layout constants (must match drawUI() values) --------
static const int CHART_L  = 560;
static const int CHART_R  = 1224;  // INNER_R = CARD_X + CARD_W - 36 = 20+1240-36
static const int CHART_T  = 380;   // LIST_TOP - 50 = 430 - 50
static const int CHART_B  = 670;   // LIST_TOP + 4*ROW_HEIGHT - 20 = 430+260-20
static const int CHART_CW = CHART_R - CHART_L;           // 664 pixels wide
static const int CHART_CH = CHART_B - CHART_T;           // 290 pixels tall
static const int CHART_CY = CHART_T + CHART_CH / 2;      // vertical centre = 525

// Pre-computed Y pixel for each chart column — represents the daily tidal pattern
int chartYToday[CHART_CW + 1];

// -------- Get tide amplitude at a specific time by interpolating pre-calculated amplitude points --------
double getTideAmplitudeAtTime(TideInfo& info, int timeMin)
{
  if (!info.amplitudeCalculated) return 0.0;

  float minutesPerSample = 1440.0f / TIDE_AMPLITUDE_SAMPLES;
  float index = (float)timeMin / minutesPerSample;
  int idx1 = (int)index;
  int idx2 = (idx1 + 1) % TIDE_AMPLITUDE_SAMPLES;
  float frac = index - idx1;

  return info.amplitudePoints[idx1] + (info.amplitudePoints[idx2] - info.amplitudePoints[idx1]) * frac;
}

void computeChartYCache(TideInfo& info, int* cache)
{
  // Use fixed 0-6m scale (matches grid labels)
  double minAmp = 0.0;
  double maxAmp = 6.0;
  double range = maxAmp - minAmp;

  // Second pass: compute cache with correct min/max
  for (int i = 0; i < CHART_CW; i++) {
    int timeMin = (int)((float)i / CHART_CW * 1440.0f);
    double amp = getTideAmplitudeAtTime(info, timeMin);
    float norm = constrain((float)(amp - minAmp) / range * 2.0f - 1.0f, -1.0f, 1.0f);
    cache[i] = CHART_CY - (int)(norm * CHART_CH / 2.4f);
  }
  // Fill last pixel with same value as previous to avoid boundary artifact
  if (CHART_CW > 0) cache[CHART_CW] = cache[CHART_CW - 1];
}

// -------- Populate a DisplayTideEvent array from a TideInfo --------
void populateTideEvents(TideInfo& info, DisplayTideEvent* events)
{
  for (int i = 0; i < 4; i++) events[i] = {0, 0, false, 0};
  for (int i = 0; i < info.numEvents && i < 4; i++) {
    float h = info.events[i].time;
    int hh = (int)h;
    int mm = (int)((h - hh) * 60.0f + 0.5f);
    if (mm == 60) { hh++; mm = 0; }
    events[i].hour      = hh;
    events[i].minute    = mm;
    events[i].isHigh    = info.events[i].isPeak;
    events[i].coefficient = (hh >= 12) ? info.afternoonCoefficient : info.morningCoefficient;
  }
}

// -------- Load tide data from Tides Library --------
void loadTideDataForToday(int year, int month, int day)
{
  tideInfoToday = tides(year, month, day);
  populateTideEvents(tideInfoToday, today);
  // Load tomorrow so we can show its first tide after today's last passes
  tideInfoTomorrow = tides(tideInfoToday.epoch + 86400);
  populateTideEvents(tideInfoTomorrow, tomorrow);
  // Pre-compute chart Y pixels using today's sinusoid (same pattern repeats daily)
  computeChartYCache(tideInfoToday, chartYToday);
}

// -------- Animation --------
float targetProgress = 0;  // Progress in current tidal cycle (0-1)

// -------- RTC Time Functions --------
// Returns UTC time directly from RTC (used for NTP sync and tide calculations)
void getRTCDateTime(int& year, int& month, int& day, int& hour, int& minute, int& second)
{
  auto dt = M5.Rtc.getDateTime();
  year = dt.date.year;
  month = dt.date.month;
  day = dt.date.date;
  hour = dt.time.hours;
  minute = dt.time.minutes;
  second = dt.time.seconds;
}

// Returns France local time (applies timezone offset from configTzTime)
void getLocalDateTime(int& year, int& month, int& day, int& hour, int& minute, int& second)
{
  // If NTP has configured the timezone, use localtime() for correct France time
  // Otherwise fall back to RTC UTC time
  time_t t = time(nullptr);
  if (t > 0) {
    struct tm* local = localtime(&t);
    year   = local->tm_year + 1900;
    month  = local->tm_mon + 1;
    day    = local->tm_mday;
    hour   = local->tm_hour;
    minute = local->tm_min;
    second = local->tm_sec;
  } else {
    getRTCDateTime(year, month, day, hour, minute, second);
  }
}

int getMinutesNow()
{
  int year, month, day, hour, minute, second;
  getLocalDateTime(year, month, day, hour, minute, second);
  return hour * 60 + minute;
}

// Get current time with hours, minutes, seconds in France local time
void getCurrentTime(int& hour, int& minute, int& second)
{
  int year, month, day;
  getLocalDateTime(year, month, day, hour, minute, second);
}

// Get current date in France local time
void getCurrentDate(int& year, int& month, int& day)
{
  int hour, minute, second;
  getLocalDateTime(year, month, day, hour, minute, second);
}

// -------- Non-blocking WiFi & NTP Functions --------

// Start an asynchronous WiFi network scan
void startWiFiScan()
{
  Serial.println("[WiFi] Starting scan...");
  WiFi.scanNetworks(/*async=*/true);
  wifiScanInProgress = true;
}

// Process WiFi scan results; returns 1 if at least one match found, 0 if no match, -1 if still scanning
// Populates wifiAvailable[] — does NOT set currentNetworkIndex
int processScanResults()
{
  int scanStatus = WiFi.scanComplete();
  if (scanStatus < 0) {
    // Still scanning (scanStatus returns -1 while in progress, -2 if not initiated)
    return -1;
  }

  // Scan completed — process results
  int numNetworks = scanStatus;
  Serial.printf("[WiFi] Scan found %d networks\n", numNetworks);

  // Reset availability array
  for (int i = 0; i < WIFI_NETWORK_COUNT; i++) {
    wifiAvailable[i] = false;
  }

  // Check each scanned network against configured networks
  int matchCount = 0;
  for (int i = 0; i < numNetworks; i++) {
    String scannedSSID = WiFi.SSID(i);
    Serial.printf("  [%d] %s\n", i, scannedSSID.c_str());

    for (int j = 0; j < WIFI_NETWORK_COUNT; j++) {
      if (scannedSSID == WIFI_NETWORKS[j].ssid) {
        wifiAvailable[j] = true;
        matchCount++;
        Serial.printf("    → Matches WIFI_NETWORKS[%d]\n", j);
      }
    }
  }

  WiFi.scanDelete();

  if (matchCount > 0) {
    Serial.printf("[WiFi] Scan result: %d configured network(s) available\n", matchCount);
    return 1;
  }

  Serial.println("[WiFi] Scan result: no matching configured networks");
  return 0;  // No match found
}

// Returns the next available network index after afterIndex, or -1 if none left
int findNextAvailableNetwork(int afterIndex)
{
  for (int i = afterIndex + 1; i < WIFI_NETWORK_COUNT; i++) {
    if (wifiAvailable[i]) return i;
  }
  return -1;
}

void updateWiFiStatus()
{
  unsigned long now = millis();
  if (now - lastWiFiCheckTime < WIFI_CHECK_INTERVAL) return;
  lastWiFiCheckTime = now;

  wl_status_t status = WiFi.status();

  if (status == WL_CONNECTED) {
    if (wifiScanInProgress) {
      // Device auto-connected (ESP-Hosted saved credentials) before our scan completed
      WiFi.scanDelete();
      wifiScanInProgress = false;
      Serial.printf("[WiFi] Auto-connected to: %s (scan superseded)\n", WiFi.SSID().c_str());
    } else if (lastSuccessfulNetworkIndex < 0) {
      // First time we observe a connection — log it
      Serial.printf("[WiFi] Connected to: %s\n", WiFi.SSID().c_str());
    }
    // Find the matching index in WIFI_NETWORKS for the connected SSID
    String connectedSSID = WiFi.SSID();
    for (int i = 0; i < WIFI_NETWORK_COUNT; i++) {
      if (connectedSSID == WIFI_NETWORKS[i].ssid) {
        currentNetworkIndex = i;
        lastSuccessfulNetworkIndex = i;
        break;
      }
    }
    lastWiFiReconnectTime = 0;
    wifiScanTargetIndex = -1;
    return;
  }

  // Not connected: handle scan-driven reconnection logic

  if (wifiScanInProgress) {
    // Scan is in progress — check if it completed
    int result = processScanResults();
    if (result == 1) {
      // At least one matching network found — try the first one
      int idx = findNextAvailableNetwork(-1);
      currentNetworkIndex = idx;
      wifiScanTargetIndex = idx;
      Serial.printf("[WiFi] Trying WIFI_NETWORKS[%d]: %s\n", idx, WIFI_NETWORKS[idx].ssid);
      WiFi.begin(WIFI_NETWORKS[idx].ssid, WIFI_NETWORKS[idx].pass);
      lastWiFiReconnectTime = now;
      wifiScanInProgress = false;
    } else if (result == 0) {
      // Scan completed but no match found — show "No WiFi" in display
      wifiScanInProgress = false;
      wifiScanTargetIndex = -1;
      lastScanTime = now;
    }
    // If result == -1, scan still in progress, wait for next check

  } else if (wifiScanTargetIndex >= 0) {
    // Scan done, currently attempting to connect to a network from scan results
    wl_status_t s = status;
    if (s == WL_CONNECT_FAILED || s == WL_NO_SSID_AVAIL) {
      // This network failed — try the next one from scan results
      int next = findNextAvailableNetwork(currentNetworkIndex);
      if (next >= 0) {
        currentNetworkIndex = next;
        wifiScanTargetIndex = next;
        Serial.printf("[WiFi] Failed, trying next: WIFI_NETWORKS[%d]: %s\n",
                      next, WIFI_NETWORKS[next].ssid);
        WiFi.begin(WIFI_NETWORKS[next].ssid, WIFI_NETWORKS[next].pass);
        lastWiFiReconnectTime = now;
      } else {
        // All scan matches exhausted — give up and rescan after interval
        Serial.println("[WiFi] All scan candidates failed, will rescan");
        wifiScanTargetIndex = -1;
        lastScanTime = now;
      }
    }
    // Otherwise still connecting — wait

  } else {
    // No scan in progress and no pending connection — check if time to rescan
    if ((now - lastScanTime > SCAN_INTERVAL) || (lastScanTime == 0)) {
      for (int i = 0; i < WIFI_NETWORK_COUNT; i++) wifiAvailable[i] = false;
      startWiFiScan();
      lastScanTime = now;
    }
  }
}

void syncNTPTime()
{
  static bool ntpConfigured = false;
  static unsigned long wifiLostTime = 0;
  unsigned long now = millis();

  // Only reset NTP if WiFi has been disconnected for more than 5 seconds
  // (don't reset on brief status 6 glitches)
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiLostTime == 0) {
      wifiLostTime = now;
    }
    if (now - wifiLostTime > 5000) {  // 5 second threshold
      ntpConfigured = false;
    }
    return;
  }

  // WiFi is connected - clear the lost timer
  wifiLostTime = 0;

  // If already successfully synced, check interval before trying again
  if (lastNTPSyncTime > 0 && now - lastNTPSyncTime < NTP_SYNC_INTERVAL) {
    return;  // Wait for interval to pass before syncing again
  }

  // First time: configure NTP servers
  if (!ntpConfigured) {
    Serial.println("[NTP] Configuring NTP servers...");
    configTzTime(NTP_TIMEZONE, NTP_SERVER1, NTP_SERVER2);
    ntpConfigured = true;
    return;  // Wait for next loop to check if sync completed
  }

  // Check if NTP has actually completed a real sync from the internet
  if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
    time_t t = time(nullptr);
    struct tm* timeInfo = gmtime(&t);

    // Update RTC with synced UTC time
    auto rtcTime = M5.Rtc.getDateTime();
    rtcTime.date.year = timeInfo->tm_year + 1900;
    rtcTime.date.month = timeInfo->tm_mon + 1;
    rtcTime.date.date = timeInfo->tm_mday;
    rtcTime.time.hours = timeInfo->tm_hour;
    rtcTime.time.minutes = timeInfo->tm_min;
    rtcTime.time.seconds = timeInfo->tm_sec;
    M5.Rtc.setDateTime(rtcTime);

    Serial.printf("[NTP] RTC synced to: %04d/%02d/%02d %02d:%02d:%02d UTC\n",
                  rtcTime.date.year, rtcTime.date.month, rtcTime.date.date,
                  rtcTime.time.hours, rtcTime.time.minutes, rtcTime.time.seconds);

    lastNTPSyncTime = now;
    ntpConfigured = false;
  } else {
    static unsigned long lastWaitLog = 0;
    if (now - lastWaitLog > 5000) {  // Log at most every 5 seconds
      Serial.printf("[NTP] Waiting for sync... (status: %d)\n", sntp_get_sync_status());
      lastWaitLog = now;
    }
  }
}

DisplayTideEvent getNextTide(int nowMin)
{
  for (int i = 0; i < 4; i++) {
    int t = today[i].hour * 60 + today[i].minute;
    if (t > nowMin) return today[i];
  }
  // All of today's tides have passed — return tomorrow's first tide
  return tomorrow[0];
}

float computeProgress(int nowMin, DisplayTideEvent prev, DisplayTideEvent next)
{
  int prevMin = prev.hour * 60 + prev.minute;
  int nextMin = next.hour * 60 + next.minute;

  // Calculate total time between prev and next tide
  int totalMinutes = nextMin - prevMin;
  if (totalMinutes <= 0) totalMinutes += 1440;  // Handle day wrap-around

  // Calculate elapsed time from prev tide to now
  int elapsedMinutes = nowMin - prevMin;
  if (elapsedMinutes < 0) elapsedMinutes += 1440;  // Handle day wrap-around

  // Progress = how far we are between prev and next tide
  return constrain((float)elapsedMinutes / totalMinutes, 0.0f, 1.0f);
}

bool isMontante(int nowMin, DisplayTideEvent next)
{
  return next.isHigh;
}

// -------- Draw big center arrow in circle --------
// centerX, centerY: center of the circle
// radius: circle radius (use 85 for standard size)
void drawBigCenterArrow(int centerX, int centerY, bool isUp, uint16_t color, int radius = 85)
{
  // Background circle (dark navy)
  canvas.fillCircle(centerX, centerY, radius, canvas.color565(25, 38, 58));
  canvas.drawCircle(centerX, centerY, radius, canvas.color565(70, 95, 125));
  // Second inner ring for depth
  canvas.drawCircle(centerX, centerY, radius - 3, canvas.color565(50, 70, 100));

  int arrowHalfWidth = radius * 50 / 100;   // 50% of radius
  int arrowHeight    = radius * 80 / 100;   // 80% of radius (tip-to-base span)

  if (isUp) {
    int tipY  = centerY - arrowHeight / 2;
    int baseY = centerY + arrowHeight / 2;
    canvas.fillTriangle(centerX,              tipY,
                        centerX - arrowHalfWidth, baseY,
                        centerX + arrowHalfWidth, baseY,
                        color);
  } else {
    int tipY  = centerY + arrowHeight / 2;
    int baseY = centerY - arrowHeight / 2;
    canvas.fillTriangle(centerX,              tipY,
                        centerX - arrowHalfWidth, baseY,
                        centerX + arrowHalfWidth, baseY,
                        color);
  }
}

// -------- Draw small arrow icon for tide list --------
// Draws a filled triangle arrow centered at (x, y), height ~18px
void drawSmallArrow(int x, int y, bool isUp, uint16_t color)
{
  if (isUp) {
    // Triangle pointing up: tip at top, base at bottom
    canvas.fillTriangle(x, y - 9,
                        x - 10, y + 9,
                        x + 10, y + 9,
                        color);
  } else {
    // Triangle pointing down: tip at bottom, base at top
    canvas.fillTriangle(x, y + 9,
                        x - 10, y - 9,
                        x + 10, y - 9,
                        color);
  }
}

// -------- Main UI --------
void drawUI()
{


  canvas.fillScreen(canvas.color565(12, 16, 28));

  // Select data source based on view mode
  DisplayTideEvent* displayTides = showTomorrow ? tomorrow : today;
  TideInfo& displayInfo = showTomorrow ? tideInfoTomorrow : tideInfoToday;

  int nowMin = getMinutesNow();
  DisplayTideEvent next = getNextTide(nowMin);

  // Find the previous tide (the one just before current next tide in the today[] array)
  int prevIndex = -1;
  for (int i = 0; i < 4; i++) {
    int t = today[i].hour * 60 + today[i].minute;
    if (t <= nowMin) prevIndex = i;
  }
  DisplayTideEvent prev = (prevIndex >= 0) ? today[prevIndex] : today[3];

  targetProgress = computeProgress(nowMin, prev, next);

  int hour   = nowMin / 60;
  int minute = nowMin % 60;

  int nextMin = next.hour * 60 + next.minute;
  int diff    = nextMin - nowMin;
  if (diff < 0) diff += 1440;
  bool isRising = isMontante(nowMin, next);

  // ================================================================
  // LAYOUT ZONES  (all Y values are absolute canvas coordinates)
  // ----------------------------------------------------------------
  //  Zone A  Y=  0 .. 44   Header bar (45px)
  //  Zone B  Y= 50 .. 710  Main card outer border
  //  Zone C  Y= 65 .. 285  Top panel: big time + status + arrow + coeff
  //  Zone D  Y=295 .. 370  Progress bar + flanking tide labels
  //  Zone E  Y=385 .. 420  "Aujourd'hui" section title
  //  Zone F  Y=425 .. 700  Tide event list (4 rows × 68px)
  // ================================================================

  // ===================== ZONE A — HEADER BAR =======================
  // Pre-calculate header bar colors
  uint16_t headerBgColor = canvas.color565(18, 24, 38);
  uint16_t locationIconColor = canvas.color565(0, 180, 220);
  uint16_t locationTextColor = canvas.color565(200, 220, 240);
  uint16_t clockIconColor = canvas.color565(100, 120, 150);
  uint16_t timeTextColor = canvas.color565(160, 180, 200);
  uint16_t headerDividerColor = canvas.color565(35, 50, 75);

  // Background strip
  canvas.fillRect(0, 0, W, 44, headerBgColor);

  // Location icon (bigger pin) — left side
  int locIconX = 30;
  int locIconY = 24;  // 2px down from original (5px down - 3px up)
  // Pin head (circle) — bigger
  canvas.fillCircle(locIconX, locIconY - 4, 9, locationIconColor);
  // Pin tail (triangle pointing down) — bigger
  canvas.fillTriangle(locIconX,     locIconY + 10,
                      locIconX - 6, locIconY - 2,
                      locIconX + 6, locIconY - 2,
                      locationIconColor);

  // "Le Palais" text — right of location icon
  canvas.setFont(&fonts::FreeSansBold18pt7b);
  canvas.setTextColor(locationTextColor);
  canvas.setCursor(locIconX + 22, 12);  // moved up 4px
  canvas.print("Le Palais");

  // WiFi icon + SSID — center of header
  wl_status_t wifiStatus = WiFi.status();
  bool wifiOk = (wifiStatus == WL_CONNECTED);
  // Show "No WiFi" only when: not connected AND scan done AND no configured network found
  bool noWiFiAvailable = (wifiStatus != WL_CONNECTED && !wifiScanInProgress && wifiScanTargetIndex == -1 && WiFi.scanComplete() >= 0);

  // Signal strength: 1/2/3 bars based on RSSI, colored green/yellow/orange
  int signalBars = 0;
  uint16_t wifiColor = canvas.color565(80, 90, 110);  // Grey when not connected
  if (wifiOk) {
    int rssi = WiFi.RSSI();
    if (rssi >= -55) {
      signalBars = 3;
      wifiColor = canvas.color565(80, 220, 100);   // Strong: green
    } else if (rssi >= -70) {
      signalBars = 2;
      wifiColor = canvas.color565(255, 200, 50);   // Medium: yellow
    } else {
      signalBars = 1;
      wifiColor = canvas.color565(255, 120, 50);   // Weak: orange
    }
  }
  uint16_t arcDimColor = canvas.color565(40, 50, 65);  // Inactive arc slots

  int wifiIconX = W / 2 - 60;
  int wifiIconY = 26;

  // Draw WiFi icon: dot + 3 arcs, lit arcs reflect signal strength
  canvas.fillCircle(wifiIconX, wifiIconY + 2, 3, wifiColor);
  canvas.drawArc(wifiIconX, wifiIconY + 2,  7,  5, 225, 315,
                 (wifiOk && signalBars >= 1) ? wifiColor : (wifiOk ? arcDimColor : wifiColor));
  canvas.drawArc(wifiIconX, wifiIconY + 2, 13, 11, 225, 315,
                 (wifiOk && signalBars >= 2) ? wifiColor : (wifiOk ? arcDimColor : wifiColor));
  canvas.drawArc(wifiIconX, wifiIconY + 2, 19, 17, 225, 315,
                 (wifiOk && signalBars >= 3) ? wifiColor : (wifiOk ? arcDimColor : wifiColor));

  // SSID or status label
  canvas.setTextColor(wifiColor);
  canvas.setCursor(wifiIconX + 26, 12);
  if (wifiOk) {
    canvas.print(WiFi.SSID().c_str());
  } else if (!noWiFiAvailable) {
    canvas.print("Searching...");  // Blinks while scanning/connecting
  } else {
    canvas.print("No WiFi");
  }

  // Clock icon (bigger) + time + date — right side (inline)
  // Get current time and date
  int currHour, currMinute, currSecond;
  getCurrentTime(currHour, currMinute, currSecond);
  int currYear, currMonth, currDay;
  getCurrentDate(currYear, currMonth, currDay);

  // Icon center at (W - 380, 24), text to its right — moved 20px more left
  int clockIconX = W - 380;
  int clockIconY = 24;  // icons stay at same vertical position
  // Clock face (circle outline) — bigger
  canvas.drawCircle(clockIconX, clockIconY, 12, clockIconColor);
  // Clock hands — bigger
  canvas.drawLine(clockIconX, clockIconY, clockIconX,     clockIconY - 8, clockIconColor); // 12 o'clock
  canvas.drawLine(clockIconX, clockIconY, clockIconX + 7, clockIconY,     clockIconColor); // 3 o'clock

  // Time and date inline
  canvas.setTextColor(timeTextColor);
  canvas.setCursor(clockIconX + 22, 12);  // same gap as location icon (22px)
  canvas.printf("%02d:%02d:%02d  %02d/%02d/%d", currHour, currMinute, currSecond, currDay, currMonth, currYear);

  // Thin separator line below header
  canvas.drawFastHLine(0, 44, W, headerDividerColor);

  // ===================== ZONE B — MAIN CARD ========================
  // Outer rounded rectangle card, 10px margin left/right, 10px below header
  const int CARD_X = 20;
  const int CARD_Y = 54;
  const int CARD_W = W - 40;
  const int CARD_H = H - 64;   // bottom at Y=710
  const int CARD_R = 18;       // corner radius
  uint16_t cardBgColor = canvas.color565(20, 28, 42);
  uint16_t cardBorderColor = canvas.color565(48, 68, 98);
  uint16_t dividerColor = canvas.color565(35, 50, 75);

  canvas.fillRoundRect(CARD_X, CARD_Y, CARD_W, CARD_H, CARD_R, cardBgColor);
  canvas.drawRoundRect(CARD_X, CARD_Y, CARD_W, CARD_H, CARD_R, cardBorderColor);

  // Inner content left/right margins (relative to card)
  const int INNER_X = CARD_X + 36;       // X=56  — left edge of inner content
  const int INNER_R = CARD_X + CARD_W - 36; // X=1224 — right edge of inner content

  // ===================== ZONE C — TOP PANEL ========================
  // Height: Y=65 to Y=285  (220px)
  //
  //  Left column  (X=56..850):   big time, PM/BM label, status, countdown
  //  Right column (X=900..1220): big arrow circle, COEFF badge above it
  //
  // Center-right column geometry — arrow circle
  const int ARROW_CX = W / 2 + 80;  // a bit more to the right
  const int ARROW_CY = 170;         // vertical center of big arrow circle
  const int ARROW_R  = 85;          // smaller radius

  // Pre-calculate top panel colors
  uint16_t topPanelCoeffColor = canvas.color565(255, 175, 50);   // orange
  uint16_t topPanelTimeColor = canvas.color565(240, 245, 255);
  uint16_t topPanelPmBmColor = canvas.color565(130, 155, 185);
  uint16_t topPanelCountdownColor = canvas.color565(255, 175, 50);
  uint16_t topPanelRisingColor = canvas.color565(80, 220, 100);
  uint16_t topPanelFallingColor = canvas.color565(0, 200, 255);

  // Coefficient number (same size as 15:36) — on the right side, vertically centered
  canvas.setFont(&FreeSansBold48pt7b);
  canvas.setTextColor(topPanelCoeffColor);
  canvas.setCursor(INNER_R - 160, 155);  // vertically centered, right side
  canvas.printf("%d", next.coefficient);

  // Big arrow circle (right side, vertically centered in top panel)
  uint16_t arrowColor = isRising ? topPanelRisingColor : topPanelFallingColor;
  drawBigCenterArrow(ARROW_CX, ARROW_CY, isRising, arrowColor, ARROW_R);

  // Large next-tide time — FreeSansBold48pt7b, baseline at Y=90
  // TOP OF TIME TEXT at ~Y=25, baseline at Y=90, bottom at ~Y=95
  canvas.setTextColor(topPanelTimeColor);
  canvas.setCursor(INNER_X, 90);   // baseline at Y=90 (moved higher)
  canvas.printf("%02d:%02d", next.hour, next.minute);

  // PM/BM label beside the time — use same big font, closer to time, aligned baseline
  canvas.setTextColor(topPanelPmBmColor);
  canvas.setCursor(INNER_X + 310, 90);  // same baseline as time, same font size
  canvas.print(next.isHigh ? "PM" : "BM");

  // Status text (MONTANTE / DESCENDANTE) — baseline at Y=190 (100px below time)
  canvas.setFont(&fonts::FreeSansBold18pt7b);
  const char* statusText = isRising ? "MONTANTE" : "DESCENDANTE";
  uint16_t statusColor   = isRising ? topPanelRisingColor : topPanelFallingColor;
  canvas.setTextColor(statusColor);
  canvas.setCursor(INNER_X, 190);   // baseline at Y=190 — clear gap from time
  canvas.print(statusText);

  // Countdown "Xh YYm" — baseline at Y=245 (55px below status)
  canvas.setTextColor(topPanelCountdownColor);
  int countHours = diff / 60;
  int countMins  = diff % 60;
  canvas.setCursor(INNER_X, 245);   // baseline at Y=245
  canvas.printf("%dh%02dm", countHours, countMins);

  // Thin horizontal divider between top panel and progress bar zone
  canvas.drawFastHLine(CARD_X + 10, 290, CARD_W - 20, dividerColor);

  // ===================== ZONE D — PROGRESS BAR =====================
  // Y=295..370
  //
  // Previous tide label (left) and next tide label (right) flank the bar.
  // Bar itself sits at Y=320, height=30.
  // Labels printed at Y=318 (baseline just at bar top edge for visual alignment).

  const int BAR_X = INNER_X;
  const int BAR_Y = 308;   // vertically centered between divider lines (290 to 368) — moved 10px higher
  const int BAR_W = INNER_R - INNER_X;  // full inner width
  const int BAR_H = 45;    // thicker progress bar (was 30)
  const int BAR_R = 14;    // corner radius

  // Pre-calculate bar colors
  uint16_t barArrowColor = canvas.color565(80, 100, 130);
  uint16_t barNextHighColor = canvas.color565(0, 180, 220);
  uint16_t barNextLowColor = canvas.color565(100, 120, 150);
  uint16_t barBgColor = canvas.color565(28, 38, 55);
  uint16_t barFillColor = canvas.color565(0, 180, 220);
  uint16_t barBorderColor = canvas.color565(70, 90, 120);
  uint16_t barKnobWhite = canvas.color565(255, 255, 255);
  uint16_t barKnobCyan = canvas.color565(0, 180, 220);

  // Arrows moved higher to avoid artifact at bottom of bar
  drawSmallArrow(BAR_X + 20, 330, prev.isHigh, barArrowColor);
  drawSmallArrow(INNER_R - 30, 330, next.isHigh,
                 next.isHigh ? barNextHighColor : barNextLowColor);

  // Bar background (dark) with rounded corners
  canvas.fillRoundRect(BAR_X, BAR_Y, BAR_W, BAR_H, BAR_R, barBgColor);

  // Filled portion (cyan progress) with rounded corners - static, not animated
  int fillW = (int)(BAR_W * targetProgress);
  if (fillW > BAR_R * 2) {
    canvas.fillRoundRect(BAR_X, BAR_Y, fillW, BAR_H, BAR_R, barFillColor);
  } else if (fillW > 0) {
    canvas.fillRect(BAR_X, BAR_Y, fillW, BAR_H, barFillColor);
  }

  // Draw rounded border
  canvas.drawRoundRect(BAR_X, BAR_Y, BAR_W, BAR_H, BAR_R, barBorderColor);

  // White slider knob (circle) at progress position - clipped to bar bounds
  int knobRadius = BAR_H / 2 - 2;  // Smaller radius to prevent overflow
  int knobX = BAR_X + fillW;
  knobX = constrain(knobX, BAR_X + knobRadius + 2, BAR_X + BAR_W - knobRadius - 2);
  int knobY = BAR_Y + BAR_H / 2;
  canvas.fillCircle(knobX, knobY, knobRadius, barKnobWhite);
  canvas.fillCircle(knobX, knobY, knobRadius - 3, barKnobCyan);

  // Thin horizontal divider between progress zone and list
  canvas.drawFastHLine(CARD_X + 10, 368, CARD_W - 20, dividerColor);

  // ===================== ZONE E — SECTION TITLE ====================
  // "Aujourd'hui" label, Y=375..420
  canvas.setFont(&fonts::FreeSansBold18pt7b);
  canvas.setTextColor(canvas.color565(0, 180, 220));
  canvas.setCursor(INNER_X, 410);   // baseline at Y=410
  canvas.print(showTomorrow ? "Demain" : "Aujourd'hui");

  // ===================== ZONE F — TIDE EVENT LIST ==================
  // 4 tides, each row 68px tall, starting at Y=430
  // Row layout:
  //   Left:   small arrow (16px wide)  at X=INNER_X
  //   Time:   FreeSansBold18pt7b       at X=INNER_X+30
  //   Right:  "PM coeff" / "BM"        at X=INNER_R-160
  //
  // Each row baseline: Y_base = 430 + i*68 + 25  (25px below row top for 18pt font)

  const int LIST_TOP      = 430;
  const int ROW_HEIGHT    = 65;
  const int ARROW_COL_X   = INNER_X + 15;    // center X of small arrow (aligned)
  const int TIME_COL_X    = INNER_X + 50;    // time text (properly spaced from arrow)
  const int BADGE_COL_X   = INNER_R - 180;

  // Pre-calculate list row colors
  uint16_t listArrowPassed = canvas.color565(60, 75, 95);
  uint16_t listArrowHigh = canvas.color565(0, 180, 220);
  uint16_t listArrowLow = canvas.color565(120, 140, 165);
  uint16_t listTimePassed = canvas.color565(65, 80, 100);
  uint16_t listTimeNext = canvas.color565(0, 220, 255);
  uint16_t listTimeNormal = canvas.color565(200, 220, 240);
  uint16_t listLabelNext = canvas.color565(0, 220, 255);
  uint16_t listLabelHigh = canvas.color565(0, 180, 220);
  uint16_t listLabelLow = canvas.color565(130, 150, 175);
  uint16_t listCoeffNext = canvas.color565(255, 200, 0);
  uint16_t listCoeffHigh = canvas.color565(255, 175, 50);

  for (int i = 0; i < 4; i++) {
    int rowTopY  = LIST_TOP + i * ROW_HEIGHT;
    int baselineY = rowTopY + 40;   // text baseline within the row (centered)
    int arrowCY   = baselineY + 10; // arrow vertically centered with text baseline, moved down 10px

    int tideMin = displayTides[i].hour * 60 + displayTides[i].minute;
    // Tides are "passed" only when viewing today and the tide time is before now
    bool isPassed = !showTomorrow && (tideMin < nowMin);
    // Highlight the first tide of tomorrow, or the next upcoming tide of today
    bool isNextRow = showTomorrow ? (i == 0)
                                  : (displayTides[i].hour == next.hour && displayTides[i].minute == next.minute);

    // Arrow color: muted gray for passed tides, colored for upcoming
    uint16_t arrowClr = isPassed ? listArrowPassed
                                 : (displayTides[i].isHigh ? listArrowHigh : listArrowLow);
    drawSmallArrow(ARROW_COL_X, arrowCY, displayTides[i].isHigh, arrowClr);

    // Time text
    canvas.setFont(&fonts::FreeSansBold18pt7b);
    uint16_t timeClr = isPassed ? listTimePassed
                                : (isNextRow ? listTimeNext : listTimeNormal);
    canvas.setTextColor(timeClr);
    canvas.setCursor(TIME_COL_X, baselineY);
    canvas.printf("%02d:%02d", displayTides[i].hour, displayTides[i].minute);

    // PM / BM badge next to time
    canvas.setFont(&fonts::FreeSans18pt7b);
    if (displayTides[i].isHigh) {
      uint16_t labelClr, coeffClr;
      if (isPassed) {
        labelClr = listTimePassed;
        coeffClr = listTimePassed;
      } else if (isNextRow) {
        labelClr = listLabelNext;
        coeffClr = listCoeffNext;
      } else {
        labelClr = listLabelHigh;
        coeffClr = listCoeffHigh;
      }
      canvas.setTextColor(labelClr);
      canvas.setCursor(TIME_COL_X + 140, baselineY);
      canvas.print("PM");
      canvas.setTextColor(coeffClr);
      canvas.setCursor(TIME_COL_X + 200, baselineY);
      canvas.printf("%d", displayTides[i].coefficient);
    } else {
      uint16_t labelClr = isPassed ? listTimePassed
                                   : (isNextRow ? listLabelNext : listLabelLow);
      canvas.setTextColor(labelClr);
      canvas.setCursor(TIME_COL_X + 140, baselineY);
      canvas.print("BM");
    }
  }

  // ===================== TIDE CHART (Maree.info Style - 24 Hour) =======================
  // Y values are pre-computed in chartYToday — no float math per frame.

  // Pre-calculate chart colors to avoid repeated color565() calls in loops
  uint16_t chartGridColor = canvas.color565(40, 60, 90);
  uint16_t chartLabelColor = canvas.color565(100, 120, 150);
  uint16_t chartFillColor = canvas.color565(0, 120, 180);
  uint16_t chartCurveColor = canvas.color565(0, 200, 255);
  uint16_t chartTimeLineColor = canvas.color565(255, 200, 0);
  uint16_t chartKnobFill = canvas.color565(255, 200, 0);
  uint16_t chartKnobBorder = canvas.color565(255, 255, 255);
  uint16_t chartBorderColor = canvas.color565(50, 80, 120);

  // Draw grid lines and height labels — equally distributed from bottom (0) to top (6)
  canvas.setFont(&fonts::FreeSansBold9pt7b);
  canvas.setTextColor(chartLabelColor);
  for (int h = 0; h <= 6; h++) {
    int labelY = CHART_B - (h * CHART_CH / 6);
    canvas.drawFastHLine(CHART_L, labelY, CHART_CW, chartGridColor);
    canvas.setCursor(CHART_L - 35, labelY - 4);  // Moved up 4 pixels to align with grid line
    canvas.printf("%d", h);
  }

  // Draw vertical grid lines for hours
  for (int hour = 0; hour <= 24; hour += 2) {
    int gridX = CHART_L + (int)(hour / 24.0f * CHART_CW);
    canvas.drawFastVLine(gridX, CHART_T, CHART_CH, chartGridColor);
  }

  // Draw tide curve using pre-computed Y cache (no float math per pixel)
  // Always use current time for yellow line position, regardless of which day is displayed
  float dayProgress = (float)nowMin / 1440.0f;
  int splitX = CHART_L + (int)(dayProgress * CHART_CW);
  int* activeChartY = chartYToday;

  // Single pass: fill under curve + draw curve line
  // Always use the same fill regardless of time or view mode, so chart looks identical
  for (int x = CHART_L; x < CHART_R; x++) {
    int i = x - CHART_L;
    if (i >= CHART_CW) break;  // Don't go past cache bounds
    int y = activeChartY[i];

    // Fill column from curve down to chart bottom (always the same color)
    canvas.drawFastVLine(x, y, CHART_B - y, chartFillColor);

    // Curve line segment to previous pixel (always the same color)
    if (i > 0) {
      canvas.drawLine(x - 1, activeChartY[i - 1], x, y, chartCurveColor);
    }
  }

  // Current time indicator: yellow vertical line and knob showing where we are in the tide
  int current_x = splitX;
  canvas.drawFastVLine(current_x, CHART_T, CHART_CH, chartTimeLineColor);
  int nowCacheIdx = constrain((int)(dayProgress * CHART_CW), 0, CHART_CW - 1);
  int currentY = activeChartY[nowCacheIdx];
  canvas.fillCircle(current_x, currentY, 6, chartKnobFill);
  canvas.drawCircle(current_x, currentY, 6, chartKnobBorder);

  // Draw chart border
  canvas.drawRect(CHART_L, CHART_T, CHART_CW, CHART_CH, chartBorderColor);

  // Add hour labels on horizontal axis (0 to 24 hours) — below the chart
  canvas.setTextColor(chartLabelColor);
  for (int hour = 0; hour <= 24; hour += 2) {
    int labelX = CHART_L + (int)(hour / 24.0f * CHART_CW);
    canvas.setCursor(labelX - 8, CHART_B + 18);
    canvas.printf("%d", hour);
  }

  canvas.pushSprite(0, 0);
}

// -------- Touch Detection Task (runs independently of render loop) --------
// Polls touch at 20 ms on a separate RTOS thread for responsive touch detection.
volatile bool touchTogglePending = false;

void touchDetectionTask(void* params)
{
  m5::touch_point_t pts[5];
  bool prevTouching = false;
  for (;;) {
    int n = M5.Lcd.getTouchRaw(pts, 5);
    bool isTouching = (n > 0);
    if (isTouching && !prevTouching) {
      touchTogglePending = true;
    }
    prevTouching = isTouching;
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// -------- Display Render Task (runs independently of main loop) --------
// Handles the expensive drawUI() operation in a background task.
// This keeps the main loop fast and responsive to touch.
volatile bool redrawRequested = true;  // Request initial redraw

void renderTask(void* params)
{
  unsigned long lastRender = 0;
  const unsigned long RENDER_INTERVAL = 800;  // Render every 800ms (hardware limit)

  for (;;) {
    unsigned long now = millis();
    if (redrawRequested || (now - lastRender >= RENDER_INTERVAL)) {
      drawUI();
      lastRender = now;
      redrawRequested = false;
    }
    vTaskDelay(pdMS_TO_TICKS(50));  // Check every 50ms if redraw needed
  }
}

void setup()
{
  Serial.begin(115200);
  delay(500);

  Serial.println("\n\n========== TFT TIDE DISPLAY SETUP ==========");

  // Initialize M5Unified (Tab5 built-in RTC is auto-detected)
  auto cfg = M5.config();
  M5.begin(cfg);

  Serial.println("M5Unified initialized");

  // Check RTC availability
  if (!M5.Rtc.isEnabled()) {
    Serial.println("[RTC] ERROR: RTC not found!");
    for (;;) { M5.delay(500); }
  }
  Serial.println("[RTC] RTC initialized");

  // Configure France timezone immediately (even before NTP/WiFi)
  // This ensures localtime() always applies France timezone to RTC time
  configTzTime(NTP_TIMEZONE, NTP_SERVER1, NTP_SERVER2);
  Serial.println("[TZ] France timezone configured");

  Serial.println("Initializing display...");

  // Set display rotation to landscape
  M5.Display.setRotation(1);

  canvas.setColorDepth(16);
  canvas.createSprite(W, H);

  Serial.println("Canvas created");

  // Initialize Tides Library
  Serial.println("Setting station: Le Palais");

  bool stationSet = setStation("Le Palais");
  Serial.print("Station set: ");
  Serial.println(stationSet ? "SUCCESS" : "FAILED");

  // Load tide data based on current RTC date
  int year, month, day, hour, minute, second;
  getRTCDateTime(year, month, day, hour, minute, second);
  Serial.printf("[RTC] Current date: %04d-%02d-%02d %02d:%02d:%02d\n",
                year, month, day, hour, minute, second);

  Serial.println("Loading tide data...");
  loadTideDataForToday(year, month, day);

  // Start WiFi scan (don't blindly connect to first network)
  // Tab5 uses ESP32-C6 WiFi coprocessor on SDIO2 with non-standard GPIO pins
  Serial.println("Starting WiFi scan...");
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  WiFi.setPins(GPIO_NUM_12, GPIO_NUM_13, GPIO_NUM_11,
               GPIO_NUM_10, GPIO_NUM_9, GPIO_NUM_8, GPIO_NUM_15);
#endif
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);  // Prevent auto-reconnect until our scan completes
  WiFi.disconnect(false);  // Disconnect from any current network (keep radio on)
  // Scan-based connection logic will take control of connection
  delay(500);  // Allow WiFi hardware time to initialize before first scan
  lastScanTime = 0;  // Force immediate scan
  startWiFiScan();

  // Trigger first NTP sync immediately when WiFi connects
  lastNTPSyncTime = 0;

  xTaskCreate(touchDetectionTask, "touch", 2048, nullptr, 5, nullptr);
  xTaskCreate(renderTask, "render", 4096, nullptr, 4, nullptr);

  Serial.println("Setup complete!\n");
}

void loop()
{
  // Touch toggle — set by touchDetectionTask independently of render task
  static int touchCount = 0;
  if (touchTogglePending) {
    touchTogglePending = false;
    showTomorrow = !showTomorrow;
    touchCount++;
    unsigned long now = millis();
    Serial.printf("[Touch] Toggled to: %s (touch #%d at %lu ms)\n",
                  showTomorrow ? "Demain" : "Aujourd'hui", touchCount, now);
    // Request immediate redraw on touch
    redrawRequested = true;
  }

  // Non-blocking WiFi & NTP updates
  updateWiFiStatus();
  syncNTPTime();

  // Check if date has changed and reload tide data if needed
  static int lastDay = -1;
  int year, month, day, hour, minute, second;
  getLocalDateTime(year, month, day, hour, minute, second);

  if (day != lastDay && lastDay != -1) {
    // Date changed, reload tide data
    Serial.printf("[Time] Date changed to %04d-%02d-%02d, reloading tide data...\n",
                  year, month, day);
    loadTideDataForToday(year, month, day);
    redrawRequested = true;
  }
  lastDay = day;

  // Keep loop fast and responsive — minimal delay
  delay(1);

}
