#include "TideTab.h"
#include "Shared.h"
#include "SunTab.h"
#include "TemperatureTab.h"
#include "StationConfig.h"
#include <Tides.h>
#include <SolarCalculator.h>
#include <time.h>

// Single-screen layout (no tabs) mimicking a maree.info-style design (dark
// theme, per the user's choice): a 7-day forecast strip (coefficient only —
// no weather source wired up yet) drives which day's tide curve is shown
// below, with events labeled directly on the curve instead of a separate
// list, plus a sunrise/sunset timeline bar. No moonrise/moonset — no data
// source for that yet. Temperature and current sun elevation are folded
// into the header as compact widgets (drawTemperatureCompact/drawSunIcon).

struct DisplayTideEvent {
  int hour;
  int minute;
  bool isHigh;
};

static const int FORECAST_DAYS = 7;
struct ForecastDay {
  int year, month, day;
  int coefficient;   // max(morning, afternoon) — a single representative number for the strip
};
static ForecastDay forecast[FORECAST_DAYS];
static int forecastBaseDay = -1;     // local day the strip was last computed for, -1 = not yet loaded
static int selectedDayOffset = 0;    // index into forecast[] — which day's curve is shown below

static DisplayTideEvent selectedEvents[4] = {};
static TideInfo selectedTideInfo;
static int lastLoadedOffset = -1;

static const int TIDE_CHART_L  = 56;
static const int TIDE_CHART_R  = 1224;
static const int TIDE_CHART_T  = 230;
static const int TIDE_CHART_B  = 550;
static const int TIDE_CHART_CW = TIDE_CHART_R - TIDE_CHART_L;
static const int TIDE_CHART_CH = TIDE_CHART_B - TIDE_CHART_T;
static const int TIDE_CHART_CY = TIDE_CHART_T + TIDE_CHART_CH / 2;

static int tideChartY[TIDE_CHART_CW + 1];

static const char* DAY_NAMES_FR[7]   = {"Dim.", "Lun.", "Mar.", "Mer.", "Jeu.", "Ven.", "Sam."};   // 0=Sunday, matches dayOfWeekSunday0
static const char* MONTH_NAMES_FR[12] = {"jan", "fev", "mar", "avr", "mai", "juin",
                                          "juil", "aout", "sept", "oct", "nov", "dec"};

// Adds deltaDays to a calendar date via an epoch round-trip, which correctly
// normalizes month/year rollover (mktime() does the normalization; we don't
// need the actual epoch value, just the recomputed calendar fields).
static void addDays(int year, int month, int day, int deltaDays, int& outYear, int& outMonth, int& outDay)
{
  struct tm tmDate = {};
  tmDate.tm_year = year - 1900;
  tmDate.tm_mon = month - 1;
  tmDate.tm_mday = day + deltaDays;
  time_t t = mktime(&tmDate);
  struct tm* norm = gmtime(&t);
  outYear = norm->tm_year + 1900;
  outMonth = norm->tm_mon + 1;
  outDay = norm->tm_mday;
}

static double getTideAmplitudeAtTime(TideInfo& info, int timeMin)
{
  if (!info.amplitudeCalculated) return 0.0;
  float minutesPerSample = 1440.0f / TIDE_AMPLITUDE_SAMPLES;
  float index = (float)timeMin / minutesPerSample;
  int idx1 = (int)index;
  int idx2 = (idx1 + 1) % TIDE_AMPLITUDE_SAMPLES;
  float frac = index - idx1;
  return info.amplitudePoints[idx1] + (info.amplitudePoints[idx2] - info.amplitudePoints[idx1]) * frac;
}

static void computeTideChartYCache(TideInfo& info, int* cache)
{
  double minAmp = 0.0;
  double maxAmp = 6.0;
  double range = maxAmp - minAmp;
  for (int i = 0; i < TIDE_CHART_CW; i++) {
    int timeMin = (int)((float)i / TIDE_CHART_CW * 1440.0f);
    double amp = getTideAmplitudeAtTime(info, timeMin);
    float norm = constrain((float)(amp - minAmp) / range * 2.0f - 1.0f, -1.0f, 1.0f);
    cache[i] = TIDE_CHART_CY - (int)(norm * TIDE_CHART_CH / 2.4f);
  }
  if (TIDE_CHART_CW > 0) cache[TIDE_CHART_CW] = cache[TIDE_CHART_CW - 1];
}

static void populateEvents(TideInfo& info, DisplayTideEvent* events)
{
  for (int i = 0; i < 4; i++) events[i] = {0, 0, false};
  for (int i = 0; i < info.numEvents && i < 4; i++) {
    float h = info.events[i].time;
    int hh = (int)h;
    int mm = (int)((h - hh) * 60.0f + 0.5f);
    if (mm == 60) { hh++; mm = 0; }
    events[i].hour = hh;
    events[i].minute = mm;
    events[i].isHigh = info.events[i].isPeak;
  }
}

static uint16_t colorForCoefficient(int c)
{
  if (c < 45) return canvas.color565(80, 160, 255);   // low — blue
  if (c < 70) return canvas.color565(0, 220, 255);    // moderate — cyan
  if (c < 90) return canvas.color565(255, 205, 70);   // strong — gold
  return canvas.color565(255, 120, 60);               // very strong — orange
}

static void refreshForecastStrip(int baseYear, int baseMonth, int baseDay)
{
  for (int i = 0; i < FORECAST_DAYS; i++) {
    int y, m, d;
    addDays(baseYear, baseMonth, baseDay, i, y, m, d);
    TideInfo info = tides(y, m, d);
    forecast[i].year = y;
    forecast[i].month = m;
    forecast[i].day = d;
    forecast[i].coefficient = max(info.morningCoefficient, info.afternoonCoefficient);
  }
}

static void loadSelectedDay()
{
  ForecastDay& fd = forecast[selectedDayOffset];
  selectedTideInfo = tides(fd.year, fd.month, fd.day);
  populateEvents(selectedTideInfo, selectedEvents);
  computeTideChartYCache(selectedTideInfo, tideChartY);
  lastLoadedOffset = selectedDayOffset;
}

// -------- Layout shared between drawing and touch hit-testing --------
static const int STRIP_Y = 55;
static const int STRIP_H = 90;
static const int STRIP_X0 = 20;
static const int STRIP_CONTENT_W = W - 40;
static const int STRIP_GAP = 10;
static const int STRIP_CARD_W = (STRIP_CONTENT_W - (FORECAST_DAYS - 1) * STRIP_GAP) / FORECAST_DAYS;

void tideTabInit()
{
  bool stationSet = setStation("Le Palais");
  Serial.printf("[Tides] Station set: %s\n", stationSet ? "SUCCESS" : "FAILED");
}

void tideTabHandleTouch(int touchX, int touchY)
{
  if (touchY < STRIP_Y || touchY >= STRIP_Y + STRIP_H) return;
  int relX = touchX - STRIP_X0;
  if (relX < 0) return;
  int card = relX / (STRIP_CARD_W + STRIP_GAP);
  if (card < 0 || card >= FORECAST_DAYS) return;
  int cardLocalX = relX - card * (STRIP_CARD_W + STRIP_GAP);
  if (cardLocalX >= STRIP_CARD_W) return;   // tapped the gap between cards
  if (card != selectedDayOffset) {
    selectedDayOffset = card;
    redrawRequested = true;
  }
}

void drawTideTab(bool timeValid)
{
  if (!timeValid) {
    canvas.setTextDatum(textdatum_t::middle_center);
    canvas.setFont(&fonts::FreeSansBold18pt7b);
    canvas.setTextColor(canvas.color565(255, 90, 90));
    canvas.drawString("Clock not synced", PANEL_CX, H / 2 - 10);
    canvas.setFont(&fonts::FreeSans12pt7b);
    canvas.setTextColor(canvas.color565(130, 150, 175));
    canvas.drawString("Run the tide display once to set the RTC via NTP", PANEL_CX, H / 2 + 30);
    canvas.setTextDatum(textdatum_t::top_left);
    return;
  }

  int year, month, day, hour, minute, second;
  getLocalFromRTC(year, month, day, hour, minute, second);

  if (day != forecastBaseDay) {
    refreshForecastStrip(year, month, day);
    forecastBaseDay = day;
    selectedDayOffset = 0;
    lastLoadedOffset = -1;
  }
  if (selectedDayOffset != lastLoadedOffset) {
    loadSelectedDay();
  }

  bool isToday = (selectedDayOffset == 0);
  ForecastDay& selectedDate = forecast[selectedDayOffset];

  // ===================== HEADER ========================
  // Title on the left; IN/OUT temperature readouts on the right (this is
  // the whole screen now — no tabs — so they live here instead of their own
  // tab). Sun info (icon, azimuth/elevation/peak) is all in the SOLEIL row
  // and its moving marker further down — no need to duplicate it up here.
  canvas.setFont(&fonts::FreeSansBold18pt7b);
  canvas.setTextColor(canvas.color565(0, 180, 220));
  canvas.setCursor(20, 12);
  canvas.print("Le Palais (Belle-Ile)");

  auto rtcNow = M5.Rtc.getDateTime();   // SolarCalculator wants UTC, unlike the local time used below — reused further down
  int outLeftX = drawTemperatureLabeled(1260, 35, "OUT", 18.5f, true);   // dummy — no outdoor sensor wired up yet
  drawTemperatureLabeled(outLeftX - 25, 35, "IN", lastTempC, lastReadOk);

  // ===================== 7-DAY FORECAST STRIP ========================
  uint16_t stripBg = canvas.color565(20, 28, 42);
  uint16_t stripBorder = canvas.color565(48, 68, 98);
  uint16_t stripSelectedBorder = canvas.color565(0, 200, 255);
  uint16_t stripSelectedBg = canvas.color565(28, 42, 58);
  uint16_t dayLabelColor = canvas.color565(190, 205, 225);
  uint16_t dateLabelColor = canvas.color565(110, 130, 160);

  for (int i = 0; i < FORECAST_DAYS; i++) {
    int cx0 = STRIP_X0 + i * (STRIP_CARD_W + STRIP_GAP);
    bool selected = (i == selectedDayOffset);

    canvas.fillRoundRect(cx0, STRIP_Y, STRIP_CARD_W, STRIP_H, 12, selected ? stripSelectedBg : stripBg);
    canvas.drawRoundRect(cx0, STRIP_Y, STRIP_CARD_W, STRIP_H, 12, selected ? stripSelectedBorder : stripBorder);

    int cardCX = cx0 + STRIP_CARD_W / 2;
    int dow = dayOfWeekSunday0(forecast[i].year, forecast[i].month, forecast[i].day);

    canvas.setTextDatum(textdatum_t::middle_center);
    canvas.setFont(&fonts::FreeSansBold12pt7b);
    canvas.setTextColor(dayLabelColor);
    canvas.drawString(DAY_NAMES_FR[dow], cardCX, STRIP_Y + 20);

    char dateBuf[12];
    snprintf(dateBuf, sizeof(dateBuf), "%02d %s", forecast[i].day, MONTH_NAMES_FR[forecast[i].month - 1]);
    canvas.setFont(&fonts::FreeSans9pt7b);
    canvas.setTextColor(dateLabelColor);
    canvas.drawString(dateBuf, cardCX, STRIP_Y + 40);

    canvas.setFont(&fonts::FreeSansBold18pt7b);
    canvas.setTextColor(colorForCoefficient(forecast[i].coefficient));
    canvas.drawString(String(forecast[i].coefficient), cardCX, STRIP_Y + 68);

    canvas.setTextDatum(textdatum_t::top_left);
  }

  // ===================== MAIN CARD ========================
  const int CARD_X = 20;
  const int CARD_Y = STRIP_Y + STRIP_H + 10;   // 155
  const int CARD_W = W - 40;
  const int CARD_H = H - CARD_Y - 10;          // bottom at H-10=710
  const int CARD_R = 18;
  uint16_t cardBgColor = canvas.color565(20, 28, 42);
  uint16_t cardBorderColor = canvas.color565(48, 68, 98);

  canvas.fillRoundRect(CARD_X, CARD_Y, CARD_W, CARD_H, CARD_R, cardBgColor);
  canvas.drawRoundRect(CARD_X, CARD_Y, CARD_W, CARD_H, CARD_R, cardBorderColor);

  const int INNER_X = CARD_X + 36;
  const int INNER_R = CARD_X + CARD_W - 36;

  // Title row: "Marees du <day>" + a "Coefficient: NN" pill on the right.
  canvas.setFont(&fonts::FreeSansBold18pt7b);
  canvas.setTextColor(canvas.color565(220, 235, 250));
  canvas.setCursor(INNER_X, CARD_Y + 30);
  canvas.printf("Marees du %s %02d %s", DAY_NAMES_FR[dayOfWeekSunday0(selectedDate.year, selectedDate.month, selectedDate.day)],
                selectedDate.day, MONTH_NAMES_FR[selectedDate.month - 1]);

  {
    char pillBuf[24];
    snprintf(pillBuf, sizeof(pillBuf), "Coefficient: %d", selectedDate.coefficient);
    canvas.setFont(&fonts::FreeSansBold12pt7b);
    int pillTextW = canvas.textWidth(pillBuf);
    int pillW = pillTextW + 32;
    int pillH = 36;
    int pillX = INNER_R - pillW;
    int pillY = CARD_Y + 18;
    canvas.fillRoundRect(pillX, pillY, pillW, pillH, 18, canvas.color565(28, 38, 55));
    canvas.drawRoundRect(pillX, pillY, pillW, pillH, 18, colorForCoefficient(selectedDate.coefficient));
    canvas.setTextDatum(textdatum_t::middle_center);
    canvas.setTextColor(colorForCoefficient(selectedDate.coefficient));
    canvas.drawString(pillBuf, pillX + pillW / 2, pillY + pillH / 2);
    canvas.setTextDatum(textdatum_t::top_left);
  }

  // ===================== TIDE CURVE ========================
  uint16_t chartGridColor = canvas.color565(40, 60, 90);
  uint16_t chartLabelColor = canvas.color565(100, 120, 150);
  uint16_t chartFillColor = canvas.color565(0, 120, 180);
  uint16_t chartCurveColor = canvas.color565(0, 200, 255);
  uint16_t chartBorderColor = canvas.color565(50, 80, 120);
  uint16_t eventDotColor = canvas.color565(80, 170, 255);
  uint16_t eventTimeColor = canvas.color565(230, 240, 250);
  uint16_t nowColor = canvas.color565(255, 150, 60);

  canvas.setFont(&fonts::FreeSansBold9pt7b);
  canvas.setTextColor(chartLabelColor);
  for (int h = 0; h <= 6; h++) {
    int labelY = TIDE_CHART_B - (h * TIDE_CHART_CH / 6);
    canvas.drawFastHLine(TIDE_CHART_L, labelY, TIDE_CHART_CW, chartGridColor);
    canvas.setCursor(TIDE_CHART_L - 35, labelY - 4);
    canvas.printf("%d", h);
  }

  for (int hr = 0; hr <= 24; hr += 2) {
    int gridX = TIDE_CHART_L + (int)(hr / 24.0f * TIDE_CHART_CW);
    canvas.drawFastVLine(gridX, TIDE_CHART_T, TIDE_CHART_CH, chartGridColor);
  }

  for (int x = TIDE_CHART_L; x < TIDE_CHART_R; x++) {
    int i = x - TIDE_CHART_L;
    if (i >= TIDE_CHART_CW) break;
    int y = tideChartY[i];
    canvas.drawFastVLine(x, y, TIDE_CHART_B - y, chartFillColor);
    if (i > 0) canvas.drawLine(x - 1, tideChartY[i - 1], x, y, chartCurveColor);
  }

  canvas.drawRect(TIDE_CHART_L, TIDE_CHART_T, TIDE_CHART_CW, TIDE_CHART_CH, chartBorderColor);

  canvas.setTextColor(chartLabelColor);
  for (int hr = 0; hr <= 24; hr += 2) {
    int labelX = TIDE_CHART_L + (int)(hr / 24.0f * TIDE_CHART_CW);
    canvas.setCursor(labelX - 8, TIDE_CHART_B + 18);
    canvas.printf("%d", hr);
  }

  // Event markers: a dot on the curve plus its time, above the dot for high
  // tides and below for low tides.
  canvas.setTextDatum(textdatum_t::middle_center);
  canvas.setFont(&fonts::FreeSansBold18pt7b);
  for (int i = 0; i < selectedTideInfo.numEvents && i < 4; i++) {
    int eventMin = selectedEvents[i].hour * 60 + selectedEvents[i].minute;
    int idx = constrain((int)((float)eventMin / 1440.0f * TIDE_CHART_CW), 0, TIDE_CHART_CW - 1);
    int ex = TIDE_CHART_L + idx;
    int ey = tideChartY[idx];

    canvas.fillCircle(ex, ey, 5, eventDotColor);
    canvas.drawCircle(ex, ey, 5, canvas.color565(255, 255, 255));

    char timeBuf[6];
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", selectedEvents[i].hour, selectedEvents[i].minute);
    int labelY = selectedEvents[i].isHigh ? (ey - 28) : (ey + 28);
    canvas.setTextColor(eventTimeColor);
    canvas.drawString(timeBuf, ex, labelY);
  }

  // "Now" marker — only meaningful for today. Just the dot/line, no caption.
  if (isToday) {
    int nowMin = hour * 60 + minute;
    float dayProgress = (float)nowMin / 1440.0f;
    int nowIdx = constrain((int)(dayProgress * TIDE_CHART_CW), 0, TIDE_CHART_CW - 1);
    int nowX = TIDE_CHART_L + nowIdx;
    int nowY = tideChartY[nowIdx];

    canvas.drawFastVLine(nowX, TIDE_CHART_T, TIDE_CHART_CH, nowColor);
    canvas.fillCircle(nowX, nowY, 6, nowColor);
    canvas.drawCircle(nowX, nowY, 6, canvas.color565(255, 255, 255));
  }
  canvas.setTextDatum(textdatum_t::top_left);

  // ===================== SUN TIMELINE ========================
  // Bottom-anchored in the card, using the extra room below the chart.
  // Same X scale as the chart above (TIDE_CHART_L..TIDE_CHART_R = 0h..24h),
  // so the rise/set positions line up with that time axis.
  double transit, sunrise, sunset;
  calcSunriseSunset(selectedDate.year, selectedDate.month, selectedDate.day, NICE_LAT, NICE_LON, transit, sunrise, sunset);
  int tz = isEuropeDST(selectedDate.year, selectedDate.month, selectedDate.day) ? 2 : 1;

  uint16_t sunTrackColor = canvas.color565(40, 50, 65);
  uint16_t sunBarColor = canvas.color565(255, 195, 60);
  uint16_t sunLabelColor = canvas.color565(100, 120, 150);

  canvas.setFont(&fonts::FreeSansBold12pt7b);
  canvas.setTextColor(sunLabelColor);
  canvas.setCursor(TIDE_CHART_L, 615);
  canvas.print("SOLEIL");

  // Azimuth/elevation right now, and the day's peak (solar noon) time +
  // elevation, on the rest of the SOLEIL row — plenty of unused width there.
  {
    double nowAz, nowEl;
    calcHorizontalCoordinates(rtcNow.date.year, rtcNow.date.month, rtcNow.date.date,
                               rtcNow.time.hours, rtcNow.time.minutes, rtcNow.time.seconds,
                               NICE_LAT, NICE_LON, nowAz, nowEl);

    double transitLocal = fmod(transit + tz + 24.0, 24.0);
    int peakHour = (int)transitLocal;
    int peakMin = (int)round((transitLocal - peakHour) * 60.0);
    if (peakMin == 60) { peakMin = 0; peakHour = (peakHour + 1) % 24; }
    int thh = (int)transit, tmm = (int)((transit - thh) * 60.0);
    int tss = (int)(((transit - thh) * 60.0 - tmm) * 60.0);
    double peakAz, peakEl;
    calcHorizontalCoordinates(selectedDate.year, selectedDate.month, selectedDate.day, thh, tmm, tss,
                               NICE_LAT, NICE_LON, peakAz, peakEl);

    canvas.setFont(&fonts::FreeSansBold12pt7b);
    canvas.setTextColor(sunBarColor);

    char azBuf[12];
    snprintf(azBuf, sizeof(azBuf), "Az %d", (int)round(nowAz));
    canvas.setCursor(TIDE_CHART_L + 160, 615);
    canvas.print(azBuf);
    drawSmallDegreeAfter(azBuf, TIDE_CHART_L + 160, 615, sunBarColor);

    char elBuf[12];
    snprintf(elBuf, sizeof(elBuf), "El %d", (int)round(nowEl));
    canvas.setCursor(TIDE_CHART_L + 360, 615);
    canvas.print(elBuf);
    drawSmallDegreeAfter(elBuf, TIDE_CHART_L + 360, 615, sunBarColor);

    char peakBuf[24];
    snprintf(peakBuf, sizeof(peakBuf), "Pic %dh%02d %d", peakHour, peakMin, (int)round(peakEl));
    canvas.setCursor(TIDE_CHART_L + 560, 615);
    canvas.print(peakBuf);
    drawSmallDegreeAfter(peakBuf, TIDE_CHART_L + 560, 615, sunBarColor);
  }

  int sunBarY = 655;
  canvas.fillRoundRect(TIDE_CHART_L, sunBarY - 3, TIDE_CHART_CW, 6, 3, sunTrackColor);

  if (!isnan(sunrise) && !isnan(sunset)) {
    // calcSunriseSunset() returns UTC hours, but the chart's X axis (and the
    // tide curve on it) is local French time — same conversion formatLocalTime()
    // already applies for the text labels below, needed here too for the dot
    // positions or they land 1-2h off from where their own labels say they are.
    double sunriseLocal = fmod(sunrise + tz + 24.0, 24.0);
    double sunsetLocal = fmod(sunset + tz + 24.0, 24.0);
    int riseX = TIDE_CHART_L + (int)((sunriseLocal / 24.0) * TIDE_CHART_CW);
    int setX  = TIDE_CHART_L + (int)((sunsetLocal / 24.0) * TIDE_CHART_CW);
    riseX = constrain(riseX, TIDE_CHART_L, TIDE_CHART_R);
    setX  = constrain(setX, TIDE_CHART_L, TIDE_CHART_R);

    canvas.fillRoundRect(riseX, sunBarY - 5, setX - riseX, 10, 5, sunBarColor);
    canvas.fillCircle(riseX, sunBarY, 10, sunBarColor);
    canvas.drawCircle(riseX, sunBarY, 10, canvas.color565(255, 255, 255));
    canvas.fillCircle(setX, sunBarY, 10, sunBarColor);
    canvas.drawCircle(setX, sunBarY, 10, canvas.color565(255, 255, 255));

    // Sun icon at the current time's position on the bar — same rtcNow
    // fetched for the header icon, same "now" scale as the tide curve above.
    if (isToday) {
      int nowMin = hour * 60 + minute;
      int nowBarIdx = constrain((int)((float)nowMin / 1440.0f * TIDE_CHART_CW), 0, TIDE_CHART_CW - 1);
      int nowBarX = TIDE_CHART_L + nowBarIdx;
      drawSunIcon(nowBarX, sunBarY, 12, rtcNow.date.year, rtcNow.date.month, rtcNow.date.date,
                  rtcNow.time.hours, rtcNow.time.minutes, rtcNow.time.seconds, false);
    }

    canvas.setTextDatum(textdatum_t::top_center);
    canvas.setFont(&fonts::FreeSansBold12pt7b);
    canvas.setTextColor(sunBarColor);
    canvas.drawString(formatLocalTime(sunrise, tz), riseX, sunBarY + 18);
    canvas.drawString(formatLocalTime(sunset, tz), setX, sunBarY + 18);
    canvas.setTextDatum(textdatum_t::top_left);
  }
}
