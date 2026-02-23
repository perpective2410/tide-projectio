// Tide Display — M5Stack Tab5
// Target hardware: M5Stack Tab5 (ESP32-P4, 1280×720 MIPI-DSI)
//
// Displays current tide state, rising/falling indicator, next events
// with coefficients, and a 3-day coefficient forecast.
//
// Today and tomorrow events are shown side by side.
// Time source: WiFi + NTP (primary), millis-tracked fallback.
//
// DISCLAIMER: This library is designed for French Atlantic tidal stations.
// Results are approximations based on harmonic analysis and DO NOT replace
// official tide tables published by SHOM (https://www.shom.fr).
// Do not use this data for navigation or safety-critical purposes.

#include <M5Unified.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <TimeLib.h>
#include <Tides.h>

// ─── Configuration ────────────────────────────────────────────────────────────


const char* WIFI_SSID     = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";
const char* STATION_ID    = "Le Palais";

// ─── Display alias ────────────────────────────────────────────────────────────

// M5GFX is the underlying display driver; alias to keep drawing helpers generic.
auto& tft = M5.Display;

// ─── Layout ───────────────────────────────────────────────────────────────────
// M5Stack Tab5: 1280×720 landscape
//
// FreeMonoBold24pt7b: monospace, xAdvance=28px, yAdvance=47px, ascent=29px
// setCursor y = top-of-line + FONT_ASCENT  (baseline offset for GFXFont)

#define SCREEN_W    1280
#define SCREEN_H     720

#define BAR_X          6
#define BAR_Y          0
#define BAR_W         26
#define BAR_H        720

#define PANEL_X       50    // content starts after bar + gap
#define LINE_H        70    // px per line — generous spacing on a 5" screen
#define FONT_ASCENT   29    // baseline offset for FreeMonoBold24pt7b

// Header x positions — xAdv=28:
//   "Le Palais" (9×28=252) from 50 → 302
//   DATE_X: centred in remaining space
//   TIME_X: right-aligned (5×28=140 → 1136+140=1276)
#define DATE_X       580
#define TIME_X      1136

// Two-column layout for today / tomorrow
// Available: 1280-50 = 1230px → each col 615px
// Longest event: "^ 20:23(95)" = 11 chars × 28px = 308px  ✓ (307px margin)
#define COL_MID      665    // split point  (PANEL_X + 615)
#define COL_L        (PANEL_X + 4)   //  54 — left column x
#define COL_R        (COL_MID  + 4)  // 669 — right column x

// ─── Colors ───────────────────────────────────────────────────────────────────

#define COL_BG          0x0000U
#define COL_WHITE       0xFFFFU
#define COL_GREEN       0x07E0U
#define COL_RED         0xF800U
#define COL_CYAN        0x07FFU
#define COL_ORANGE      0xFD20U
#define COL_DARK        0x528AU
#define COL_DIM         0xAD75U
#define COL_BAR_FILL    0x07BFU
#define COL_BAR_EMPTY   0x000FU

// ─── Globals ──────────────────────────────────────────────────────────────────

WiFiUDP   ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 86400000);

// Sliding window: days[0]=today, days[1]=tomorrow, days[2..4]=+2..+4 days.
// Shifted by 1 at midnight; only days[4] is recomputed each day.
TideInfo      days[5];
TideInfo&     today    = days[0];
TideInfo&     tomorrow = days[1];
int           lastDay         = -1;
unsigned long lastRefresh     = 0;
time_t        lastKnownEpoch  = 0;
unsigned long lastKnownMillis = 0;

// ─── Helpers ──────────────────────────────────────────────────────────────────

// Print text with y=top-of-line (handles GFXFont baseline offset).
void printAt(int x, int y, const char* s) {
    tft.setCursor(x, y + FONT_ASCENT);
    tft.print(s);
}

String formatHM(float h) {
    while (h >= 24.0f) h -= 24.0f;
    while (h <   0.0f) h += 24.0f;
    int hh = (int)h;
    int mm = (int)((h - hh) * 60.0f + 0.5f);
    if (mm == 60) { hh++; mm = 0; }
    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d", hh, mm);
    return String(buf);
}

const char* dayNameFR(time_t epoch) {
    const char* days[] = { "Dim.", "Lun.", "Mar.", "Mer.", "Jeu.", "Ven.", "Sam." };
    return days[weekday(epoch) - 1];
}

// ─── Tide computation ─────────────────────────────────────────────────────────

void getTidalRange(float& minH, float& maxH) {
    minH = 99.0f; maxH = -99.0f;
    for (int d = 0; d < 2; d++) {
        const TideInfo& ti = (d == 0) ? today : tomorrow;
        for (int i = 0; i < ti.numEvents; i++) {
            if (ti.events[i].amplitude < minH) minH = ti.events[i].amplitude;
            if (ti.events[i].amplitude > maxH) maxH = ti.events[i].amplitude;
        }
    }
    if (maxH <= minH) maxH = minH + 1.0f;
}

float currentTideHeight(float nowH) {
    int before = -1, after = -1;
    for (int i = 0; i < today.numEvents; i++) {
        if (today.events[i].time <= nowH) before = i;
        else if (after == -1)             after  = i;
    }
    if (before == -1) { before = 0; after = min(1, today.numEvents - 1); }
    if (after  == -1) { before = max(0, today.numEvents - 2); after = today.numEvents - 1; }
    float t0 = today.events[before].time,      t1 = today.events[after].time;
    float h0 = today.events[before].amplitude, h1 = today.events[after].amplitude;
    float frac = (t1 > t0) ? constrain((nowH - t0) / (t1 - t0), 0.0f, 1.0f) : 0.0f;
    return h0 + (h1 - h0) * (1.0f - cosf(frac * M_PI)) / 2.0f;
}

bool isTideRising(float nowH) {
    for (int i = 0; i < today.numEvents; i++)
        if (today.events[i].time > nowH) return today.events[i].isPeak;
    return today.numEvents > 0 ? !today.events[today.numEvents - 1].isPeak : false;
}

int minutesToNextEvent(float nowH, bool& nextIsPeak) {
    for (int i = 0; i < today.numEvents; i++) {
        if (today.events[i].time > nowH) {
            nextIsPeak = today.events[i].isPeak;
            return (int)((today.events[i].time - nowH) * 60.0f + 0.5f);
        }
    }
    for (int i = 0; i < tomorrow.numEvents; i++) {
        nextIsPeak = tomorrow.events[i].isPeak;
        return (int)((tomorrow.events[i].time + 24.0f - nowH) * 60.0f + 0.5f);
    }
    return -1;
}

// ─── Drawing ──────────────────────────────────────────────────────────────────

void drawBar(float curH, float minH, float maxH) {
    tft.fillRect(BAR_X, BAR_Y, BAR_W, BAR_H, COL_BAR_EMPTY);
    float frac  = constrain((curH - minH) / (maxH - minH), 0.0f, 1.0f);
    int   fillH = (int)(frac * BAR_H);
    int   fillY = BAR_Y + BAR_H - fillH;
    tft.fillRect(BAR_X, fillY, BAR_W, fillH, COL_BAR_FILL);
    // Triangle marker ◄ pointing right at water level
    int tx = BAR_X + BAR_W + 2;
    int ty = fillY;
    tft.fillTriangle(tx, ty - 12, tx, ty + 12, tx + 16, ty, COL_WHITE);
}

void drawSep(int y) {
    tft.drawFastHLine(PANEL_X, y, SCREEN_W - PANEL_X - 8, COL_DARK);
}

// Draw one event into a column; coeff shown inline as "(N)" if > 0.
void drawEventInline(int x, int y, const TideEvent& ev, bool past, int coeff = -1) {
    if (y + LINE_H > SCREEN_H) return;
    tft.setTextColor(past ? COL_DARK : (ev.isPeak ? COL_CYAN : COL_ORANGE));
    char sym = past ? '.' : (ev.isPeak ? '^' : 'v');
    char buf[16];
    if (coeff > 0)
        snprintf(buf, sizeof(buf), "%c %s(%d)", sym, formatHM(ev.time).c_str(), coeff);
    else
        snprintf(buf, sizeof(buf), "%c %s", sym, formatHM(ev.time).c_str());
    printAt(x, y, buf);
}

void drawScreen(time_t localEpoch) {
    float nowH   = hour(localEpoch) + minute(localEpoch) / 60.0f;
    float minH, maxH;
    getTidalRange(minH, maxH);
    float curH   = currentTideHeight(nowH);
    bool  rising = isTideRising(nowH);
    bool  nextIsPeak;
    int   mins   = minutesToNextEvent(nowH, nextIsPeak);

    tft.fillScreen(COL_BG);
    tft.setFont(&fonts::FreeMonoBold24pt7b);
    tft.setTextSize(1);
    tft.setTextWrap(false);

    // ── Bar ───────────────────────────────────────────────────────────────────
    drawBar(curH, minH, maxH);

    // ── Header: station | date | time (single line) ───────────────────────────
    int y = 4;
    tft.setTextColor(COL_WHITE);
    printAt(PANEL_X, y, STATION_ID);

    char dateBuf[12];
    snprintf(dateBuf, sizeof(dateBuf), "%02d/%02d/%04d",
             day(localEpoch), month(localEpoch), year(localEpoch));
    printAt(DATE_X, y, dateBuf);

    char timeBuf[6];
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d",
             hour(localEpoch), minute(localEpoch));
    printAt(TIME_X, y, timeBuf);
    y += LINE_H;

    // ── MONTANTE / DESCENDANTE + countdown (single line) ─────────────────────
    tft.setTextColor(rising ? COL_GREEN : COL_RED);
    printAt(PANEL_X, y, rising ? "^ MONTANTE" : "v DESCEND.");

    if (mins >= 0) {
        char cntBuf[22];
        if (mins < 60)
            snprintf(cntBuf, sizeof(cntBuf), "%d min avant %s", mins, nextIsPeak ? "HM" : "BM");
        else
            snprintf(cntBuf, sizeof(cntBuf), "%dh%02d avant %s", mins / 60, mins % 60, nextIsPeak ? "HM" : "BM");
        tft.setTextColor(COL_DIM);
        printAt(DATE_X, y, cntBuf);
    }
    y += LINE_H;

    drawSep(y); y += 3;

    // ── Today | Tomorrow (side by side) ───────────────────────────────────────
    if (y + LINE_H > SCREEN_H) return;
    time_t tomorrowEpoch = localEpoch + SECS_PER_DAY;
    tft.setTextColor(COL_WHITE);
    printAt(COL_L, y, "Auj.");
    printAt(COL_R, y, dayNameFR(tomorrowEpoch));
    y += LINE_H;

    int maxEv = max(today.numEvents, tomorrow.numEvents);
    for (int i = 0; i < maxEv && y + LINE_H <= SCREEN_H; i++) {
        if (i < today.numEvents) {
            bool past = (today.events[i].time < nowH);
            int  coeff = -1;
            if (today.events[i].isPeak && !past)
                coeff = (today.events[i].time < 12.0f)
                      ? today.morningCoefficient : today.afternoonCoefficient;
            drawEventInline(COL_L, y, today.events[i], past, coeff);
        }
        if (i < tomorrow.numEvents) {
            int coeff = -1;
            if (tomorrow.events[i].isPeak)
                coeff = (tomorrow.events[i].time < 12.0f)
                      ? tomorrow.morningCoefficient : tomorrow.afternoonCoefficient;
            drawEventInline(COL_R, y, tomorrow.events[i], false, coeff);
        }
        y += LINE_H;
    }

    // ── Future days ───────────────────────────────────────────────────────────
    y += 2;
    if (y + LINE_H > SCREEN_H) return;
    drawSep(y); y += 3;

    tft.setTextColor(COL_DIM);
    for (int d = 2; d <= 4 && y + LINE_H <= SCREEN_H; d++) {
        time_t futEpoch = localEpoch + (time_t)d * SECS_PER_DAY;
        char line[28];
        snprintf(line, sizeof(line), "%s mat %d  soir %d",
                 dayNameFR(futEpoch), days[d].morningCoefficient, days[d].afternoonCoefficient);
        printAt(PANEL_X, y, line);
        y += LINE_H;
    }
}

// ─── Setup ────────────────────────────────────────────────────────────────────

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);  // initialises display, touch, power, serial automatically

    tft.setTextWrap(false);
    tft.fillScreen(COL_BG);
    tft.setFont(&fonts::FreeMonoBold24pt7b);
    tft.setTextSize(1);
    tft.setTextColor(COL_WHITE);
    printAt(PANEL_X, 4, "Connexion WiFi...");

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    unsigned long wifiStart = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 10000UL) { delay(500); }

    time_t epoch = 0;
    if (WiFi.status() == WL_CONNECTED) {
        printAt(PANEL_X, 4 + LINE_H, "Sync NTP...");
        timeClient.begin();
        timeClient.update();
        epoch = timeClient.getEpochTime();
    }

    if (epoch < 1000000000UL) epoch = 1740312000UL;  // fallback: 2026-02-23 12:00 UTC
    setTime(epoch);
    lastKnownEpoch  = epoch;
    lastKnownMillis = millis();

    setStation(STATION_ID);

    int    tzOffset   = getFranceTimezoneOffset(epoch);
    time_t localEpoch = epoch + (time_t)tzOffset * 60;
    lastDay = day(localEpoch);
    for (int d = 0; d < 5; d++)
        days[d] = tides(localEpoch + (time_t)d * SECS_PER_DAY);

    drawScreen(localEpoch);
}

// ─── Loop ─────────────────────────────────────────────────────────────────────

void loop() {
    M5.update();  // keeps M5Unified internals (touch, power) responsive

    unsigned long now = millis();
    if (now - lastRefresh < 60000UL) return;
    lastRefresh = now;

    time_t epoch;
    if (WiFi.status() == WL_CONNECTED) {
        timeClient.update();
        epoch = timeClient.getEpochTime();
        setTime(epoch);
        lastKnownEpoch  = epoch;
        lastKnownMillis = now;
    } else {
        epoch = lastKnownEpoch + (now - lastKnownMillis) / 1000UL;
        setTime(epoch);
    }

    int    tzOffset   = getFranceTimezoneOffset(epoch);
    time_t localEpoch = epoch + (time_t)tzOffset * 60;

    if (day(localEpoch) != lastDay) {
        lastDay = day(localEpoch);
        // Slide the window: promote each day, compute only the new +4 tail.
        for (int d = 0; d < 4; d++) days[d] = days[d + 1];
        days[4] = tides(localEpoch + (time_t)4 * SECS_PER_DAY);
    }

    drawScreen(localEpoch);
}
