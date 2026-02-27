#include <M5GFX.h>
#include <Tides.h>
#include "FreeSansBold48pt7b.h"

M5GFX display;
M5Canvas canvas(&display);

static const int W = 1280;
static const int H = 720;

const int FRAME_TIME = 33;
unsigned long lastFrame = 0;

// -------- Display Tide Data --------
struct DisplayTideEvent {
  int hour;
  int minute;
  bool isHigh;
  int coefficient;
  double amplitude;  // Store actual amplitude from library
};

DisplayTideEvent today[4] = {};  // Will be populated from Tides Library
TideInfo tideInfoToday;  // Store complete TideInfo for amplitude data

// -------- Load tide data from Tides Library --------
void loadTideDataForToday(int year, int month, int day)
{
  tideInfoToday = tides(year, month, day);

  // Clear the array
  for (int i = 0; i < 4; i++) {
    today[i] = {0, 0, false, 0, 0.0};
  }

  // Fill with tide events
  for (int i = 0; i < tideInfoToday.numEvents && i < 4; i++) {
    float h = tideInfoToday.events[i].time;
    int hh = (int)h;
    int mm = (int)((h - hh) * 60.0f + 0.5f);
    if (mm == 60) { hh++; mm = 0; }

    today[i].hour = hh;
    today[i].minute = mm;
    today[i].isHigh = tideInfoToday.events[i].isPeak;
    today[i].amplitude = tideInfoToday.events[i].amplitude;

    // Use afternoon coefficient if PM, morning if AM
    today[i].coefficient = (hh >= 12) ? tideInfoToday.afternoonCoefficient : tideInfoToday.morningCoefficient;
  }
}

// -------- Get tide amplitude at a specific time by interpolating pre-calculated amplitude points --------
double getTideAmplitudeAtTime(int timeMin)
{
  if (!tideInfoToday.amplitudeCalculated) return 0.0;

  // Convert minutes since midnight to amplitude sample index
  float minutesPerSample = 1440.0f / TIDE_AMPLITUDE_SAMPLES;  // minutes between samples
  float index = (float)timeMin / minutesPerSample;
  int idx1 = (int)index;
  int idx2 = (idx1 + 1) % TIDE_AMPLITUDE_SAMPLES;
  float frac = index - idx1;

  // Linear interpolation between amplitude points
  double amp1 = tideInfoToday.amplitudePoints[idx1];
  double amp2 = tideInfoToday.amplitudePoints[idx2];

  return amp1 + (amp2 - amp1) * frac;
}

const char* dayNames[] = {"Dimanche", "Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi"};
const char* shortDayNames[] = {"Dim", "Lun", "Mar", "Mer", "Jeu", "Ven", "Sam"};

// -------- Animation --------
float animatedProgress = 0;
float targetProgress   = 0;

// -------- Time Simulation --------
int getMinutesNow()
{
  unsigned long sec = millis() / 1000;
  int base = 8 * 60 + 0;  // 8:00 AM
  return (base + sec / 60) % 1440;
}

// Get current time with hours, minutes, seconds
void getCurrentTime(int& hour, int& minute, int& second)
{
  unsigned long totalSeconds = millis() / 1000;
  int baseSeconds = (8 * 60 + 0) * 60;  // 8:00 AM in seconds
  totalSeconds = (baseSeconds + totalSeconds) % 86400;  // 86400 = seconds in a day

  hour = totalSeconds / 3600;
  minute = (totalSeconds % 3600) / 60;
  second = totalSeconds % 60;
}

// Get current date (static: Feb 27, 2026)
void getCurrentDate(int& year, int& month, int& day)
{
  year = 2026;
  month = 2;
  day = 27;
}

DisplayTideEvent getNextTide(int nowMin)
{
  for (int i = 0; i < 4; i++) {
    int t = today[i].hour * 60 + today[i].minute;
    if (t > nowMin) return today[i];
  }
  return today[0];
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
  animatedProgress += (targetProgress - animatedProgress) * 0.12f;

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
  // Background strip
  canvas.fillRect(0, 0, W, 44, canvas.color565(18, 24, 38));

  // Location icon (bigger pin) — left side
  int locIconX = 30;
  int locIconY = 24;  // 2px down from original (5px down - 3px up)
  // Pin head (circle) — bigger
  canvas.fillCircle(locIconX, locIconY - 4, 9, canvas.color565(0, 180, 220));
  // Pin tail (triangle pointing down) — bigger
  canvas.fillTriangle(locIconX,     locIconY + 10,
                      locIconX - 6, locIconY - 2,
                      locIconX + 6, locIconY - 2,
                      canvas.color565(0, 180, 220));

  // "Le Palais" text — right of location icon
  canvas.setFont(&fonts::FreeSansBold18pt7b);
  canvas.setTextColor(canvas.color565(200, 220, 240));
  canvas.setCursor(locIconX + 22, 12);  // moved up 4px
  canvas.print("Le Palais");

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
  canvas.drawCircle(clockIconX, clockIconY, 12, canvas.color565(100, 120, 150));
  // Clock hands — bigger
  canvas.drawLine(clockIconX, clockIconY, clockIconX,     clockIconY - 8, canvas.color565(100, 120, 150)); // 12 o'clock
  canvas.drawLine(clockIconX, clockIconY, clockIconX + 7, clockIconY,     canvas.color565(100, 120, 150)); // 3 o'clock

  // Time and date inline
  canvas.setFont(&fonts::FreeSansBold18pt7b);
  canvas.setTextColor(canvas.color565(160, 180, 200));
  canvas.setCursor(clockIconX + 22, 12);  // same gap as location icon (22px)
  canvas.printf("%02d:%02d:%02d  %02d/%02d/%d", currHour, currMinute, currSecond, currDay, currMonth, currYear);

  // Thin separator line below header
  canvas.drawFastHLine(0, 44, W, canvas.color565(35, 50, 75));

  // ===================== ZONE B — MAIN CARD ========================
  // Outer rounded rectangle card, 10px margin left/right, 10px below header
  const int CARD_X = 20;
  const int CARD_Y = 54;
  const int CARD_W = W - 40;
  const int CARD_H = H - 64;   // bottom at Y=710
  const int CARD_R = 18;       // corner radius

  canvas.fillRoundRect(CARD_X, CARD_Y, CARD_W, CARD_H, CARD_R,
                       canvas.color565(20, 28, 42));
  canvas.drawRoundRect(CARD_X, CARD_Y, CARD_W, CARD_H, CARD_R,
                       canvas.color565(48, 68, 98));

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

  // Coefficient number (same size as 15:36) — on the right side, vertically centered
  canvas.setFont(&FreeSansBold48pt7b);
  canvas.setTextColor(canvas.color565(255, 175, 50));  // orange
  canvas.setCursor(INNER_R - 160, 155);  // vertically centered, right side
  canvas.printf("%d", next.coefficient);

  // Big arrow circle (right side, vertically centered in top panel)
  uint16_t arrowColor = isRising ? canvas.color565(80, 220, 100) : canvas.color565(0, 200, 255);
  drawBigCenterArrow(ARROW_CX, ARROW_CY, isRising, arrowColor, ARROW_R);


  // Large next-tide time — FreeSansBold48pt7b, baseline at Y=90
  // TOP OF TIME TEXT at ~Y=25, baseline at Y=90, bottom at ~Y=95
  canvas.setFont(&FreeSansBold48pt7b);
  canvas.setTextColor(canvas.color565(240, 245, 255));
  canvas.setCursor(INNER_X, 90);   // baseline at Y=90 (moved higher)
  canvas.printf("%02d:%02d", next.hour, next.minute);

  // PM/BM label beside the time — use same big font, closer to time, aligned baseline
  canvas.setFont(&FreeSansBold48pt7b);
  canvas.setTextColor(canvas.color565(130, 155, 185));
  canvas.setCursor(INNER_X + 310, 90);  // same baseline as time, same font size
  canvas.print(next.isHigh ? "PM" : "BM");

  // Status text (MONTANTE / DESCENDANTE) — baseline at Y=190 (100px below time)
  canvas.setFont(&fonts::FreeSansBold18pt7b);
  const char* statusText = isRising ? "MONTANTE" : "DESCENDANTE";
  uint16_t statusColor   = isRising ? canvas.color565(80, 220, 100) : canvas.color565(0, 200, 255);
  canvas.setTextColor(statusColor);
  canvas.setCursor(INNER_X, 190);   // baseline at Y=190 — clear gap from time

  canvas.print(statusText);

  // Countdown "Xh YYm" — baseline at Y=245 (55px below status)
  canvas.setFont(&fonts::FreeSansBold18pt7b);
  canvas.setTextColor(canvas.color565(255, 175, 50));
  int countHours = diff / 60;
  int countMins  = diff % 60;
  canvas.setCursor(INNER_X, 245);   // baseline at Y=245
  canvas.printf("%dh%02dm", countHours, countMins);

  // Thin horizontal divider between top panel and progress bar zone
  canvas.drawFastHLine(CARD_X + 10, 290, CARD_W - 20, canvas.color565(35, 50, 75));

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

  // Previous tide direction arrow — left side
  drawSmallArrow(BAR_X + 20, 345, prev.isHigh, canvas.color565(80, 100, 130));

  // Next tide direction arrow — right side
  drawSmallArrow(INNER_R - 30, 345, next.isHigh,
                 next.isHigh ? canvas.color565(0, 180, 220) : canvas.color565(100, 120, 150));

  // Bar background (dark)
  canvas.fillRoundRect(BAR_X, BAR_Y, BAR_W, BAR_H, BAR_R,
                       canvas.color565(28, 38, 55));
  canvas.drawRoundRect(BAR_X, BAR_Y, BAR_W, BAR_H, BAR_R,
                       canvas.color565(70, 90, 120));

  // Filled portion (cyan progress) - static, not animated
  int fillW = (int)(BAR_W * targetProgress);
  if (fillW > BAR_R * 2) {
    canvas.fillRoundRect(BAR_X, BAR_Y, fillW, BAR_H, BAR_R,
                         canvas.color565(0, 180, 220));
  } else if (fillW > 0) {
    canvas.fillRect(BAR_X, BAR_Y, fillW, BAR_H, canvas.color565(0, 180, 220));
  }

  // White slider knob (circle) at progress position
  int knobRadius = BAR_H / 2 + 2;  // Reduced to keep within bar bounds
  int knobX = BAR_X + fillW;
  knobX = constrain(knobX, BAR_X + knobRadius, BAR_X + BAR_W - knobRadius);
  canvas.fillCircle(knobX, BAR_Y + BAR_H / 2, knobRadius,
                    canvas.color565(255, 255, 255));
  canvas.fillCircle(knobX, BAR_Y + BAR_H / 2, knobRadius - 4,
                    canvas.color565(0, 180, 220));

  // Thin horizontal divider between progress zone and list
  canvas.drawFastHLine(CARD_X + 10, 368, CARD_W - 20, canvas.color565(35, 50, 75));

  // ===================== ZONE E — SECTION TITLE ====================
  // "Aujourd'hui" label, Y=375..420
  canvas.setFont(&fonts::FreeSansBold18pt7b);
  canvas.setTextColor(canvas.color565(0, 180, 220));
  canvas.setCursor(INNER_X, 410);   // baseline at Y=410
  canvas.print("Aujourd'hui");

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

  for (int i = 0; i < 4; i++) {
    int rowTopY  = LIST_TOP + i * ROW_HEIGHT;
    int baselineY = rowTopY + 40;   // text baseline within the row (centered)
    int arrowCY   = baselineY + 10; // arrow vertically centered with text baseline, moved down 10px

    int tideMin = today[i].hour * 60 + today[i].minute;
    bool isPassed = tideMin < nowMin;

    // Check if this is the "active" (next upcoming) row for special styling
    bool isNextRow = (today[i].hour == next.hour && today[i].minute == next.minute);

    // Arrow color: muted gray for passed tides, colored for upcoming
    uint16_t arrowClr;
    if (isPassed) {
      arrowClr = canvas.color565(60, 75, 95);
    } else if (today[i].isHigh) {
      arrowClr = canvas.color565(0, 180, 220);    // cyan for high tide
    } else {
      arrowClr = canvas.color565(120, 140, 165);  // muted blue-gray for low tide
    }
    drawSmallArrow(ARROW_COL_X, arrowCY, today[i].isHigh, arrowClr);

    // Time text (properly aligned with arrow)
    canvas.setFont(&fonts::FreeSansBold18pt7b);
    uint16_t timeClr;
    if (isPassed) {
      timeClr = canvas.color565(65, 80, 100);  // muted gray for passed
    } else if (isNextRow) {
      timeClr = canvas.color565(0, 220, 255);  // bright cyan for active/next
    } else {
      timeClr = canvas.color565(200, 220, 240);  // normal light color
    }
    canvas.setTextColor(timeClr);
    canvas.setCursor(TIME_COL_X, baselineY);
    canvas.printf("%02d:%02d", today[i].hour, today[i].minute);

    // PM / BM badge next to time
    canvas.setFont(&fonts::FreeSans18pt7b);
    if (today[i].isHigh) {
      // "PM coeff" — right next to time
      uint16_t labelClr, coeffClr;
      if (isPassed) {
        labelClr = canvas.color565(65, 80, 100);
        coeffClr = canvas.color565(65, 80, 100);
      } else if (isNextRow) {
        labelClr = canvas.color565(0, 220, 255);  // bright cyan for active
        coeffClr = canvas.color565(255, 200, 0);   // bright orange for active
      } else {
        labelClr = canvas.color565(0, 180, 220);
        coeffClr = canvas.color565(255, 175, 50);
      }

      canvas.setTextColor(labelClr);
      canvas.setCursor(TIME_COL_X + 140, baselineY);
      canvas.print("PM");

      canvas.setTextColor(coeffClr);
      canvas.setCursor(TIME_COL_X + 200, baselineY);
      canvas.printf("%d", today[i].coefficient);
    } else {
      // "BM" — right next to time
      uint16_t labelClr;
      if (isPassed) {
        labelClr = canvas.color565(65, 80, 100);
      } else if (isNextRow) {
        labelClr = canvas.color565(0, 220, 255);  // bright cyan for active
      } else {
        labelClr = canvas.color565(130, 150, 175);
      }
      canvas.setTextColor(labelClr);
      canvas.setCursor(TIME_COL_X + 140, baselineY);
      canvas.print("BM");
    }
  }

  // ===================== TIDE CHART (Maree.info Style - 24 Hour) =======================
  // Show 24-hour tide curve (2 complete tidal cycles: 2 peaks and 2 troughs)
  const int CHART_LEFT   = 560;   // start of chart area (extend further left)
  const int CHART_RIGHT  = INNER_R;  // end of chart area (extend to edge)
  const int CHART_TOP    = LIST_TOP - 50;  // top of chart (extend above) — moved up 30px total
  const int CHART_BOT    = LIST_TOP + 4 * ROW_HEIGHT - 20;  // bottom of chart (extend below) — moved up 20px
  const int CHART_H      = CHART_BOT - CHART_TOP;
  const int CHART_W      = CHART_RIGHT - CHART_LEFT;

  // Vertical center for the sinusoid
  int chart_center_y = CHART_TOP + CHART_H / 2;

  // Draw grid lines and height labels — equally distributed from bottom (0) to top (6)
  canvas.setFont(&fonts::FreeSansBold9pt7b);
  canvas.setTextColor(canvas.color565(100, 120, 150));
  for (int h = 0; h <= 6; h++) {
    // Position from bottom (CHART_BOT) to top (CHART_TOP), equally distributed
    int labelY = CHART_BOT - (h * (CHART_BOT - CHART_TOP) / 6);

    // Draw horizontal grid line
    canvas.drawFastHLine(CHART_LEFT, labelY, CHART_W, canvas.color565(40, 60, 90));

    // Draw height number on the left
    canvas.setCursor(CHART_LEFT - 35, labelY + 4);
    canvas.printf("%d", h);
  }

  // Draw vertical grid lines for hours
  for (int hour = 0; hour <= 24; hour += 2) {
    float hourProgress = hour / 24.0f;
    int gridX = CHART_LEFT + (hourProgress * CHART_W);
    canvas.drawFastVLine(gridX, CHART_TOP, CHART_H, canvas.color565(40, 60, 90));
  }

  // Draw tide curve using actual amplitude data from library
  // Calculate current position within 24-hour day (0-1)
  float dayProgress = (float)nowMin / 1440.0f;  // 1440 = 24 * 60 minutes

  // Find min/max amplitudes for scaling
  double minAmplitude = 10.0, maxAmplitude = -10.0;
  for (int i = 0; i < tideInfoToday.numEvents; i++) {
    minAmplitude = min(minAmplitude, tideInfoToday.events[i].amplitude);
    maxAmplitude = max(maxAmplitude, tideInfoToday.events[i].amplitude);
  }
  double amplitudeRange = maxAmplitude - minAmplitude;
  if (amplitudeRange < 0.1) amplitudeRange = 1.0;  // Avoid division by zero

  // Draw tide curve with smooth lines between points
  int prevY = -1;
  int prevX = CHART_LEFT;
  for (int x = CHART_LEFT; x <= CHART_RIGHT; x++) {
    // Convert x position to time of day in minutes
    float progress = (float)(x - CHART_LEFT) / CHART_W;
    int timeMin = (int)(progress * 1440.0f);  // 0 to 1440 minutes in 24 hours

    // Get actual tide amplitude at this time
    double amplitude = getTideAmplitudeAtTime(timeMin);

    // Normalize amplitude to chart height (-1 to 1 range)
    float normalizedAmplitude = (amplitude - minAmplitude) / amplitudeRange * 2.0f - 1.0f;
    normalizedAmplitude = constrain(normalizedAmplitude, -1.0f, 1.0f);

    // Convert to Y pixel position (invert: high tide at top, centered vertically)
    int y = chart_center_y - (int)(normalizedAmplitude * CHART_H / 2.4f);

    // Color: bright if before current time, muted after
    bool is_past = (progress < dayProgress);
    uint16_t lineColor = is_past
      ? canvas.color565(0, 200, 255)     // bright cyan for past
      : canvas.color565(50, 80, 120);    // muted for future

    // Draw line from previous point to current point for smooth curve
    if (prevY != -1) {
      canvas.drawLine(prevX, prevY, x, y, lineColor);
    }

    prevY = y;
    prevX = x;
  }

  // Fill area under curve
  for (int x = CHART_LEFT; x < CHART_RIGHT; x++) {
    float progress = (float)(x - CHART_LEFT) / CHART_W;
    int timeMin = (int)(progress * 1440.0f);

    double amplitude = getTideAmplitudeAtTime(timeMin);
    float normalizedAmplitude = (amplitude - minAmplitude) / amplitudeRange * 2.0f - 1.0f;
    normalizedAmplitude = constrain(normalizedAmplitude, -1.0f, 1.0f);

    int y = chart_center_y - (int)(normalizedAmplitude * CHART_H / 2.4f);

    bool is_past = (progress < dayProgress);
    uint16_t fillColor = is_past
      ? canvas.color565(0, 120, 180)     // semi-transparent bright cyan
      : canvas.color565(30, 50, 80);     // dark blue

    canvas.drawLine(x, y, x, CHART_BOT, fillColor);
  }

  // Draw current position vertical line and knob
  int current_x = CHART_LEFT + (dayProgress * CHART_W);
  canvas.drawLine(current_x, CHART_TOP, current_x, CHART_BOT, canvas.color565(255, 200, 0));

  // Draw knob at current tide height position
  double currentAmplitude = getTideAmplitudeAtTime(nowMin);
  float normalizedCurrent = (currentAmplitude - minAmplitude) / amplitudeRange * 2.0f - 1.0f;
  normalizedCurrent = constrain(normalizedCurrent, -1.0f, 1.0f);
  int currentY = chart_center_y - (int)(normalizedCurrent * CHART_H / 2.4f);
  canvas.fillCircle(current_x, currentY, 6, canvas.color565(255, 200, 0));  // yellow knob
  canvas.drawCircle(current_x, currentY, 6, canvas.color565(255, 255, 255));  // white outline

  // Draw chart border
  canvas.drawRect(CHART_LEFT, CHART_TOP, CHART_W, CHART_H, canvas.color565(50, 80, 120));

  // Add hour labels on horizontal axis (0 to 24 hours) — below the chart
  canvas.setFont(&fonts::FreeSansBold9pt7b);
  canvas.setTextColor(canvas.color565(100, 120, 150));
  for (int hour = 0; hour <= 24; hour += 2) {
    float hourProgress = hour / 24.0f;
    int labelX = CHART_LEFT + (hourProgress * CHART_W);
    canvas.setCursor(labelX - 8, CHART_BOT + 18);
    canvas.printf("%d", hour);
  }

  canvas.pushSprite(0, 0);
}

void setup()
{
  display.begin();
  display.setRotation(1);

  canvas.setColorDepth(16);
  canvas.createSprite(W, H);

  // Initialize Tides Library
  setStation("Le Palais");

  // Load tide data for today (February 27, 2026 — static for now)
  loadTideDataForToday(2026, 2, 27);

  // ========== Performance Test: Calculate tides for next 10 days ==========
  Serial.begin(115200);
  delay(100);
  Serial.println("\n========== TIDE CALCULATION PERFORMANCE TEST ==========");
  Serial.println("Calculating tides for 10 consecutive days...\n");

  unsigned long totalStartTime = millis();
  int totalEvents = 0;
  double totalMemoryUsed = 0.0;

  for (int day = 0; day < 10; day++) {
    unsigned long dayStart = millis();

    TideInfo ti = tides(2026, 2, 27 + day);

    unsigned long dayTime = millis() - dayStart;
    double dayMemory = sizeof(TideInfo) / 1024.0;  // KB
    totalMemoryUsed += dayMemory;
    totalEvents += ti.numEvents;

    Serial.printf("Day %d (Feb %d): ", day + 1, 27 + day);
    Serial.printf("%d tides, ", ti.numEvents);
    Serial.printf("%lu ms, ", dayTime);
    Serial.printf("%.2f KB\n", dayMemory);

    // Print tide details
    for (int i = 0; i < ti.numEvents; i++) {
      int h = (int)ti.events[i].time;
      int m = (int)((ti.events[i].time - h) * 60);
      Serial.printf("  - %02d:%02d %s %.2f m\n",
        h, m,
        ti.events[i].isPeak ? "HM" : "BM",
        ti.events[i].amplitude);
    }
    Serial.printf("  Morning Coeff: %d, Afternoon Coeff: %d\n",
      ti.morningCoefficient, ti.afternoonCoefficient);
  }

  unsigned long totalTime = millis() - totalStartTime;

  Serial.println("\n========== SUMMARY ==========");
  Serial.printf("Total time: %lu ms\n", totalTime);
  Serial.printf("Average per day: %.1f ms\n", (float)totalTime / 10.0);
  Serial.printf("Total events: %d\n", totalEvents);
  Serial.printf("Total memory per day: %.2f KB\n", totalMemoryUsed / 10.0);
  Serial.printf("Amplitude samples: %d (every %.1f minutes)\n",
    TIDE_AMPLITUDE_SAMPLES, 1440.0 / TIDE_AMPLITUDE_SAMPLES);
  Serial.println("============================================\n");
}

void loop()
{
  if (millis() - lastFrame >= FRAME_TIME) {
    lastFrame = millis();
    drawUI();
  }
}
