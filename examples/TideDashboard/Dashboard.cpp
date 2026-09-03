#include "Dashboard.h"
#include "Shared.h"
#include "TemperatureWidget.h"
#include "WeatherService.h"
#include "StationConfig.h"
#include <Tides.h>
#include <SolarCalculator.h>
#include <time.h>

// Single-screen layout (no tabs) mimicking a maree.info-style design (dark
// theme, per the user's choice): a 7-day forecast strip (coefficient, an
// Open-Meteo weather icon and max wind speed per day) drives which day's
// tide curve is shown below, with events labeled directly on the curve
// instead of a separate list, plus sunrise/sunset as a small sun icon +
// time right above the chart's top border (kept deliberately minimal — no
// separate row for it, so the chart gets that space instead). No
// moonrise/moonset — no data source for that yet. The header's IN/OUT
// widgets are both DS18B20-probe-related (OUT is a placeholder — only one
// physical probe exists so far). Weather (icon/wind) is Open-Meteo, via
// WeatherService.

struct DisplayTideEvent {
  int hour;
  int minute;
  bool isHigh;
};

static const int FORECAST_DAYS = 7;
struct ForecastDay {
  int year, month, day;
  int coefficient;   // max(morning, afternoon) — a single representative number for the strip
  int dow;           // dayOfWeekSunday0() result, cached once per refresh instead of recomputed every frame
};
static ForecastDay forecast[FORECAST_DAYS];
static int forecastBaseDay = -1;     // local day the strip was last computed for, -1 = not yet loaded
static int selectedDayOffset = 0;    // index into forecast[] — which day's curve is shown below

static DisplayTideEvent selectedEvents[4] = {};
static TideInfo selectedTideInfo;
static int lastLoadedOffset = -1;

// Sunrise/sunset/transit/peak-position for the selected day — these only
// depend on the date (not "now"), so loadSelectedDay() computes them once
// per day-selection instead of every render frame like the rest of the
// SOLEIL row used to (this was the single most expensive avoidable bit of
// per-frame work: 3 trig-heavy calcHorizontalCoordinates/calcSunriseSunset
// calls that gave the same answer on every one of the ~5 redraws/sec during
// the weather-loading window).
struct SelectedDaySunInfo {
  double transit = 0, sunrise = 0, sunset = 0;
  int tz = 1;
  int peakHour = 0, peakMin = 0;
  double peakAz = 0, peakEl = 0;
};
static SelectedDaySunInfo selectedSunInfo;

static const int TIDE_CHART_L  = 56;
static const int TIDE_CHART_R  = 1224;
static const int TIDE_CHART_T  = 304;
static const int TIDE_CHART_B  = 628;   // closer to the hourly-wind row below it (which stays anchored near the card's bottom edge — see windIconY)
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

// windDirFromDeg is meteorological convention (direction the wind blows
// FROM, 0=N, clockwise) — the arrow is drawn pointing the way it blows
// TOWARD, which is the more intuitive reading at a glance. polarPoint's
// angle convention is 0=3 o'clock/east clockwise, hence the -90 offset to
// turn a compass bearing into a polarPoint angle.
static void drawWindArrow(int cx, int cy, int r, float windDirFromDeg, uint16_t color)
{
  const float STROKE = 2.0f;   // drawWideLine's radius, not diameter — makes the arrow read as bold rather than hairline
  float toward = fmodf(windDirFromDeg + 180.0f, 360.0f);
  float ang = fmodf(toward - 90.0f + 360.0f, 360.0f);
  int tipX, tipY, tailX, tailY;
  polarPoint(cx, cy, r, ang, tipX, tipY);
  polarPoint(cx, cy, r, fmodf(ang + 180.0f, 360.0f), tailX, tailY);
  canvas.drawWideLine(tailX, tailY, tipX, tipY, STROKE, color);
  int b1X, b1Y, b2X, b2Y;
  polarPoint(tipX, tipY, r * 0.7f, fmodf(ang + 150.0f, 360.0f), b1X, b1Y);
  polarPoint(tipX, tipY, r * 0.7f, fmodf(ang + 210.0f, 360.0f), b2X, b2Y);
  canvas.drawWideLine(tipX, tipY, b1X, b1Y, STROKE, color);
  canvas.drawWideLine(tipX, tipY, b2X, b2Y, STROKE, color);
}

// A "sun on the horizon" glyph for the sunset marker — deliberately not the
// plain sun-with-rays icon (drawWeatherIcon's weatherCode 0, used for
// sunrise) so the two read as visually distinct rather than two identical
// suns next to each other. Shifted down slightly from cy: a half-disc's
// visual weight sits above its flat edge, so drawing it centered on cy like
// the (symmetric) full sun icon made it look like it was floating higher —
// nudging the flat edge down brings its apparent center in line with the
// sunrise icon's.
static void drawSunsetIcon(int cx, int cy, int r, uint16_t color)
{
  int flatY = cy + r * 2 / 5;
  canvas.fillArc(cx, flatY, 0, (int)(r * 0.85f), 180.0f, 360.0f, color);   // upper half of the disc
  canvas.drawFastHLine(cx - r - 2, flatY, (r + 2) * 2, color);             // horizon line
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
    forecast[i].dow = dayOfWeekSunday0(y, m, d);
  }
}

static void loadSelectedDay()
{
  ForecastDay& fd = forecast[selectedDayOffset];
  selectedTideInfo = tides(fd.year, fd.month, fd.day);
  populateEvents(selectedTideInfo, selectedEvents);
  computeTideChartYCache(selectedTideInfo, tideChartY);

  SelectedDaySunInfo& sun = selectedSunInfo;
  calcSunriseSunset(fd.year, fd.month, fd.day, LE_PALAIS_LAT, LE_PALAIS_LON, sun.transit, sun.sunrise, sun.sunset);
  sun.tz = isEuropeDST(fd.year, fd.month, fd.day) ? 2 : 1;

  double transitLocal = fmod(sun.transit + sun.tz + 24.0, 24.0);
  sun.peakHour = (int)transitLocal;
  sun.peakMin = (int)round((transitLocal - sun.peakHour) * 60.0);
  if (sun.peakMin == 60) { sun.peakMin = 0; sun.peakHour = (sun.peakHour + 1) % 24; }
  int thh = (int)sun.transit, tmm = (int)((sun.transit - thh) * 60.0);
  int tss = (int)(((sun.transit - thh) * 60.0 - tmm) * 60.0);
  calcHorizontalCoordinates(fd.year, fd.month, fd.day, thh, tmm, tss,
                             LE_PALAIS_LAT, LE_PALAIS_LON, sun.peakAz, sun.peakEl);

  lastLoadedOffset = selectedDayOffset;
}

// -------- Layout shared between drawing and touch hit-testing --------
static const int STRIP_Y = 55;
static const int STRIP_H = 170;   // bigger cards — room freed up by shrinking the chart a bit further (see TIDE_CHART_T/B above)
static const int STRIP_X0 = 20;
static const int STRIP_CONTENT_W = W - 40;
static const int STRIP_GAP = 10;
static const int STRIP_CARD_W = (STRIP_CONTENT_W - (FORECAST_DAYS - 1) * STRIP_GAP) / FORECAST_DAYS;

void dashboardInit()
{
  bool stationSet = setStation("Le Palais");
  Serial.printf("[Tides] Station set: %s\n", stationSet ? "SUCCESS" : "FAILED");
}

void dashboardHandleTouch(int touchX, int touchY)
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

void drawDashboard(bool timeValid)
{
  // The RTC read (and its isEnabled()/year>=2020 sanity check, done by the
  // caller) can glitch for a frame or two around a USB power replug. That
  // used to blank the whole screen to a separate "RTC not synced" layout,
  // which looked like a spurious popup window. Now: once there's been one
  // good read, a momentary bad one just keeps showing the last known time/
  // date (the forecast/tide data don't change second to second anyway) and
  // flags it through the status pill below instead of swapping the whole UI.
  static int lastGoodYear = 0, lastGoodMonth = 0, lastGoodDay = 0;
  static int lastGoodHour = 0, lastGoodMinute = 0, lastGoodSecond = 0;
  static bool everSynced = false;

  int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
  if (timeValid) {
    getLocalFromRTC(year, month, day, hour, minute, second);
    lastGoodYear = year; lastGoodMonth = month; lastGoodDay = day;
    lastGoodHour = hour; lastGoodMinute = minute; lastGoodSecond = second;
    everSynced = true;
  } else if (everSynced) {
    year = lastGoodYear; month = lastGoodMonth; day = lastGoodDay;
    hour = lastGoodHour; minute = lastGoodMinute; second = lastGoodSecond;
  }

  // ===================== HEADER ========================
  // Title on the left; current local time centered; IN/OUT temperature
  // readouts on the right (this is the whole screen now — no tabs — so they
  // live here instead of their own tab). Sun info (icon, azimuth/elevation/
  // peak) is all in the SOLEIL row and its moving marker further down — no
  // need to duplicate it up here. Drawn unconditionally (even without a
  // valid clock) — WiFi/weather/RTC status all live in the pill below the
  // title (see TITLE_ROW_CY further down) instead of cluttering this bar.
  // All header widgets share this one vertical center so title/time/
  // IN/OUT all line up — drawTemperatureLabeled (IN/OUT) also centers on it.
  const int HEADER_CY = 35;

  canvas.setFont(&fonts::FreeSansBold18pt7b);
  canvas.setTextColor(canvas.color565(0, 180, 220));
  const char* title = "Le Palais (Belle-Ile)";
  canvas.setTextDatum(textdatum_t::middle_left);
  canvas.drawString(title, 20, HEADER_CY);

  if (everSynced) {
    char timeBuf[6];
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", hour, minute);
    canvas.setFont(&fonts::FreeSansBold18pt7b);
    canvas.setTextColor(canvas.color565(220, 235, 250));
    canvas.setTextDatum(textdatum_t::middle_center);
    canvas.drawString(timeBuf, PANEL_CX, HEADER_CY);
  }
  canvas.setTextDatum(textdatum_t::top_left);

  int outLeftX = drawTemperatureLabeled(1260, HEADER_CY, "OUT", 18.5f, true);   // dummy — only one DS18B20 probe exists so far, reads "IN"
  drawTemperatureLabeled(outLeftX - 25, HEADER_CY, "IN", lastTempC, lastReadOk);

  if (!everSynced) {
    // Genuinely never synced (first-ever boot, dead RTC battery, etc.) —
    // no date at all yet to compute a forecast from, so there's nothing to
    // show but this message. A momentary bad read after having been synced
    // before does NOT hit this path — see the caching above.
    canvas.setTextDatum(textdatum_t::middle_center);
    canvas.setFont(&fonts::FreeSansBold18pt7b);
    canvas.setTextColor(COLOR_ERROR);
    canvas.drawString("RTC not synced", PANEL_CX, H / 2 - 10);
    canvas.setFont(&fonts::FreeSans12pt7b);
    canvas.setTextColor(canvas.color565(130, 150, 175));
    canvas.drawString("Run the tide display once to set the RTC via NTP", PANEL_CX, H / 2 + 30);
    canvas.setTextDatum(textdatum_t::top_left);
    return;
  }

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
    int dow = forecast[i].dow;
    bool hasWeather = weatherDataValid;

    canvas.setTextDatum(textdatum_t::middle_center);

    // Day name + date on one compact line, to leave room below for the icon/wind.
    char dayLineBuf[16];
    snprintf(dayLineBuf, sizeof(dayLineBuf), "%s %02d", DAY_NAMES_FR[dow], forecast[i].day);
    canvas.setFont(&fonts::FreeSansBold9pt7b);
    canvas.setTextColor(dayLabelColor);
    canvas.drawString(dayLineBuf, cardCX, STRIP_Y + 22);

    // Icon slot is reserved even before the fetch completes, so a spinner
    // shows where the icon will appear instead of the layout jumping once
    // weather data arrives.
    int coeffCX = cardCX + 26;
    if (hasWeather) {
      drawWeatherIcon(cardCX - 32, STRIP_Y + 70, 24, dailyWeatherCode[i]);
    } else {
      drawSpinner(cardCX - 32, STRIP_Y + 70, 12, canvas.color565(90, 100, 115));
    }
    canvas.setFont(&fonts::FreeSansBold18pt7b);
    canvas.setTextColor(colorForCoefficient(forecast[i].coefficient));
    canvas.drawString(String(forecast[i].coefficient), coeffCX, STRIP_Y + 70);

    if (hasWeather) {
      uint16_t windColor = gaugeColorForWind(dailyWindSpeedMax[i]);
      char windBuf[10];
      snprintf(windBuf, sizeof(windBuf), "%d km/h", (int)round(dailyWindSpeedMax[i]));
      int windTextX = cardCX - 12;
      canvas.setFont(&fonts::FreeSans12pt7b);
      canvas.setTextColor(windColor);
      canvas.setTextDatum(textdatum_t::middle_left);
      canvas.drawString(windBuf, windTextX, STRIP_Y + 118);
      drawWindArrow(windTextX - 16, STRIP_Y + 118, 10, dailyWindDirection[i], windColor);
      canvas.setTextDatum(textdatum_t::middle_center);
    }

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

  // Title row: the selected day's date, then WiFi/weather status (moved
  // here from the top bar, which felt too busy with it), then a
  // "Coefficient: NN" pill on the right. TITLE_ROW_CY matches the pill's
  // own vertical center (pillY + pillH/2 below) so everything on this row
  // lines up.
  const int TITLE_ROW_CY = CARD_Y + 36;

  char cardTitleBuf[32];
  snprintf(cardTitleBuf, sizeof(cardTitleBuf), "%s %02d %s",
           DAY_NAMES_FR[selectedDate.dow], selectedDate.day, MONTH_NAMES_FR[selectedDate.month - 1]);
  canvas.setFont(&fonts::FreeSansBold18pt7b);
  canvas.setTextColor(canvas.color565(220, 235, 250));
  canvas.setTextDatum(textdatum_t::middle_left);
  canvas.drawString(cardTitleBuf, INNER_X, TITLE_ROW_CY);
  int titleRight = INNER_X + canvas.textWidth(cardTitleBuf);
  canvas.setTextDatum(textdatum_t::top_left);

  // Sunrise/sunset, right next to the date on the same line, in their own
  // box — same styling (bg/border/height) as the WiFi and coefficient
  // pills on this row, so it reads as one of that same family rather than
  // bare floating icons+text. Sunrise keeps the plain sun glyph; sunset
  // uses drawSunsetIcon so the two don't look like two identical suns.
  if (!isnan(selectedSunInfo.sunrise) && !isnan(selectedSunInfo.sunset)) {
    uint16_t sunColor = canvas.color565(255, 195, 60);
    canvas.setFont(&fonts::FreeSansBold12pt7b);
    String riseStr = formatLocalTime(selectedSunInfo.sunrise, selectedSunInfo.tz);
    String setStr = formatLocalTime(selectedSunInfo.sunset, selectedSunInfo.tz);
    int riseTextW = canvas.textWidth(riseStr);
    int setTextW = canvas.textWidth(setStr);

    const int SUN_BOX_H = 36;      // matches PILL_H (the WiFi/coefficient pills), declared later in this function
    const int PAD = 14;            // box left/right padding
    const int ICON_TO_TEXT = 16;   // icon center -> text start
    const int PAIR_GAP = 24;       // gap between the rise and set pairs
    int boxX = titleRight + 20;
    int boxW = PAD + ICON_TO_TEXT + riseTextW + PAIR_GAP + ICON_TO_TEXT + setTextW + PAD;
    int boxY = TITLE_ROW_CY - SUN_BOX_H / 2;

    canvas.fillRoundRect(boxX, boxY, boxW, SUN_BOX_H, 18, canvas.color565(28, 38, 55));
    canvas.drawRoundRect(boxX, boxY, boxW, SUN_BOX_H, 18, sunColor);

    canvas.setTextColor(sunColor);
    canvas.setTextDatum(textdatum_t::middle_left);

    int sx = boxX + PAD;
    drawWeatherIcon(sx, TITLE_ROW_CY, 11, 0);   // weatherCode 0 = clear-sky sun glyph
    sx += ICON_TO_TEXT;
    canvas.drawString(riseStr, sx, TITLE_ROW_CY);
    sx += riseTextW + PAIR_GAP;

    drawSunsetIcon(sx, TITLE_ROW_CY, 11, sunColor);
    sx += ICON_TO_TEXT;
    canvas.drawString(setStr, sx, TITLE_ROW_CY);

    titleRight = boxX + boxW;
    canvas.setTextDatum(textdatum_t::top_left);
  }

  char coeffPillBuf[24];
  snprintf(coeffPillBuf, sizeof(coeffPillBuf), "Coefficient: %d", selectedDate.coefficient);
  canvas.setFont(&fonts::FreeSansBold12pt7b);
  int coeffPillW = canvas.textWidth(coeffPillBuf) + 32;
  const int PILL_H = 36;
  int coeffPillX = INNER_R - coeffPillW;
  int coeffPillY = TITLE_ROW_CY - PILL_H / 2;

  // WiFi/weather status pill: same box styling and vertical center as the
  // coefficient pill, centered horizontally in the gap between the title
  // and that pill (only drawn if it actually fits there).
  {
    // A momentary RTC read glitch (see the caching at the top of this
    // function) takes priority here — everything else keeps showing
    // cached/last-known data, so this pill is the only visible sign of it.
    String statusLine;
    uint16_t statusColor;
    if (!timeValid) {
      statusLine = "RTC not synced";
      statusColor = COLOR_ERROR;
    } else {
      statusLine = wifiConnected
          ? ("WiFi: " + (wifiSSID.length() > 16 ? (wifiSSID.substring(0, 15) + "...") : wifiSSID))
          : String("WiFi: not connected");
      if (weatherErrorText.length() > 0) statusLine += "   " + weatherErrorText;

      statusColor = weatherErrorText.length() > 0 ? COLOR_ERROR
                  : wifiConnected              ? COLOR_OK
                                                : COLOR_WARN;
    }

    canvas.setFont(&fonts::FreeSansBold12pt7b);
    int statusPillW = canvas.textWidth(statusLine) + 32;
    int gapStart = titleRight + 20;
    int gapEnd = coeffPillX - 20;

    if (statusPillW <= gapEnd - gapStart) {
      int statusPillX = gapStart + (gapEnd - gapStart - statusPillW) / 2;
      int statusPillY = TITLE_ROW_CY - PILL_H / 2;
      canvas.fillRoundRect(statusPillX, statusPillY, statusPillW, PILL_H, 18, canvas.color565(28, 38, 55));
      canvas.drawRoundRect(statusPillX, statusPillY, statusPillW, PILL_H, 18, statusColor);
      canvas.setTextDatum(textdatum_t::middle_center);
      canvas.setTextColor(statusColor);
      canvas.drawString(statusLine, statusPillX + statusPillW / 2, statusPillY + PILL_H / 2);
      canvas.setTextDatum(textdatum_t::top_left);
    }
  }

  canvas.setFont(&fonts::FreeSansBold12pt7b);
  canvas.fillRoundRect(coeffPillX, coeffPillY, coeffPillW, PILL_H, 18, canvas.color565(28, 38, 55));
  canvas.drawRoundRect(coeffPillX, coeffPillY, coeffPillW, PILL_H, 18, colorForCoefficient(selectedDate.coefficient));
  canvas.setTextDatum(textdatum_t::middle_center);
  canvas.setTextColor(colorForCoefficient(selectedDate.coefficient));
  canvas.drawString(coeffPillBuf, coeffPillX + coeffPillW / 2, coeffPillY + PILL_H / 2);
  canvas.setTextDatum(textdatum_t::top_left);

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

  // ===================== HOURLY WIND ========================
  // One arrow every 2 hours (matches the chart's vertical gridlines), same
  // time axis as the chart above. Direction only — see drawWindArrow. Sits
  // near the card's bottom edge rather than right under the chart, with
  // room to spare now the chart itself is shorter.
  if (weatherDataValid) {
    int windIconY = CARD_Y + CARD_H - 24;
    for (int hr = 0; hr < 24; hr += 2) {
      int idx = selectedDayOffset * WEATHER_HOURLY_PER_DAY + hr;
      int x = TIDE_CHART_L + (int)(hr / 24.0f * TIDE_CHART_CW);
      drawWindArrow(x, windIconY, 14, hourlyWindDirection[idx], gaugeColorForWind(hourlyWindSpeed[idx]));
    }
  }

}
