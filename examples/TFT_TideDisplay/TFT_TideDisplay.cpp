#include <M5GFX.h>
#include "FreeSansBold48pt7b.h"

M5GFX display;
M5Canvas canvas(&display);

static const int W = 1280;
static const int H = 720;

const int FRAME_TIME = 33;
unsigned long lastFrame = 0;

// -------- Tide Data --------
struct TideEvent {
  int hour;
  int minute;
  bool isHigh;
  int coefficient;
};

TideEvent today[] = {
  {3, 9,  false, 0},
  {9, 2,  true,  60},
  {15,36, false, 0},
  {21,22, true,  52}
};

TideEvent day1[] = {
  {4,14,false,0},{10,35,true,46},{16,45,false,0},{23,30,true,41}
};
TideEvent day2[] = {
  {5,10,false,0},{11,20,true,39},{17,50,false,0},{0,15,true,38}
};
TideEvent day3[] = {
  {6,0,false,0},{12,10,true,42},{18,40,false,0},{1,10,true,47}
};

const char* dayNames[] = {"Dimanche", "Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi"};
const char* shortDayNames[] = {"Dim", "Lun", "Mar", "Mer", "Jeu", "Ven", "Sam"};

// -------- Animation --------
float animatedProgress = 0;
float targetProgress   = 0;

// -------- Time Simulation --------
int getMinutesNow()
{
  unsigned long sec = millis() / 1000;
  int base = 12 * 60 + 0;
  return (base + sec / 60) % 1440;
}

TideEvent getNextTide(int nowMin)
{
  for (int i = 0; i < 4; i++) {
    int t = today[i].hour * 60 + today[i].minute;
    if (t > nowMin) return today[i];
  }
  return today[0];
}

float computeProgress(int nowMin, TideEvent next)
{
  int nextMin = next.hour * 60 + next.minute;
  int diff = nextMin - nowMin;
  if (diff < 0) diff += 1440;

  // Use 10 hours (600 minutes) as reference so that 36 minutes shows as 94% (close to end)
  float reference = 600.0f;
  // Progress shows how close we are to the next tide (inverse of time remaining)
  return constrain(1.0f - (diff / reference), 0.0f, 1.0f);
}

bool isMontante(int nowMin, TideEvent next)
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
// Draws a filled triangle arrow centered at (x, y), height ~14px
void drawSmallArrow(int x, int y, bool isUp, uint16_t color)
{
  if (isUp) {
    // Triangle pointing up: tip at top, base at bottom
    canvas.fillTriangle(x, y - 7,
                        x - 8, y + 7,
                        x + 8, y + 7,
                        color);
  } else {
    // Triangle pointing down: tip at bottom, base at top
    canvas.fillTriangle(x, y + 7,
                        x - 8, y - 7,
                        x + 8, y - 7,
                        color);
  }
}

// -------- Main UI --------
void drawUI()
{
  canvas.fillScreen(canvas.color565(12, 16, 28));

  int nowMin = getMinutesNow();
  TideEvent next = getNextTide(nowMin);
  targetProgress = computeProgress(nowMin, next);
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

  // "Marées" — left side, vertically centered in header
  canvas.setFont(&fonts::DejaVu18);
  canvas.setTextColor(canvas.color565(100, 120, 150));
  canvas.setCursor(40, 14);     // baseline ~Y=32 for 18pt font
  canvas.print("Mar\xC3\xA9" "es");  // "Marées" UTF-8

  // Location icon (simple pin: filled circle + tail) — center area
  // Icon center at (W/2 - 80, 22), text "Le Palais" to its right
  int locIconX = W / 2 - 80;
  int locIconY = 22;
  // Pin head (circle)
  canvas.fillCircle(locIconX, locIconY - 4, 6, canvas.color565(0, 180, 220));
  // Pin tail (small filled triangle pointing down)
  canvas.fillTriangle(locIconX,     locIconY + 7,
                      locIconX - 4, locIconY - 1,
                      locIconX + 4, locIconY - 1,
                      canvas.color565(0, 180, 220));

  canvas.setFont(&fonts::DejaVu18);
  canvas.setTextColor(canvas.color565(200, 220, 240));
  canvas.setCursor(locIconX + 14, 14);
  canvas.print("Le Palais");

  // Clock icon + time — right side
  // Icon center at (W - 190, 22), text to its right
  int clockIconX = W - 190;
  int clockIconY = 22;
  // Clock face (circle outline)
  canvas.drawCircle(clockIconX, clockIconY, 9, canvas.color565(100, 120, 150));
  // Clock hands
  canvas.drawLine(clockIconX, clockIconY, clockIconX,     clockIconY - 6, canvas.color565(100, 120, 150)); // 12 o'clock
  canvas.drawLine(clockIconX, clockIconY, clockIconX + 5, clockIconY,     canvas.color565(100, 120, 150)); // 3 o'clock

  canvas.setFont(&fonts::DejaVu18);
  canvas.setTextColor(canvas.color565(160, 180, 200));
  canvas.setCursor(clockIconX + 16, 14);
  canvas.print("11:02");

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

  // Find the previous tide (the one just before current next tide in the today[] array)
  int prevIndex = -1;
  for (int i = 0; i < 4; i++) {
    int t = today[i].hour * 60 + today[i].minute;
    if (t <= nowMin) prevIndex = i;
  }
  TideEvent prev = (prevIndex >= 0) ? today[prevIndex] : today[3];

  const int BAR_X = INNER_X;
  const int BAR_Y = 318;   // vertically centered between divider lines (290 to 368)
  const int BAR_W = INNER_R - INNER_X;  // full inner width
  const int BAR_H = 45;    // thicker progress bar (was 30)
  const int BAR_R = 14;    // corner radius

  // Previous tide direction arrow — left side
  drawSmallArrow(BAR_X + 20, 335, prev.isHigh, canvas.color565(80, 100, 130));

  // Next tide direction arrow — right side
  drawSmallArrow(INNER_R - 30, 335, next.isHigh,
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
  int knobX = BAR_X + fillW;
  knobX = constrain(knobX, BAR_X + BAR_H / 2, BAR_X + BAR_W - BAR_H / 2);
  canvas.fillCircle(knobX, BAR_Y + BAR_H / 2, BAR_H / 2 + 4,
                    canvas.color565(255, 255, 255));
  canvas.fillCircle(knobX, BAR_Y + BAR_H / 2, BAR_H / 2 - 2,
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
    int arrowCY   = baselineY;      // arrow vertically centered with text baseline

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
  const int CHART_TOP    = LIST_TOP - 10;  // top of chart (extend above)
  const int CHART_BOT    = LIST_TOP + 4 * ROW_HEIGHT + 10;  // bottom of chart (extend below)
  const int CHART_H      = CHART_BOT - CHART_TOP;
  const int CHART_W      = CHART_RIGHT - CHART_LEFT;

  // Vertical center for the sinusoid
  int chart_center_y = CHART_TOP + CHART_H / 2;

  // Draw smooth tide curve - 24 hour period showing 2 complete cycles
  // Current tide determines the starting phase
  float tidePhase = isRising ? 0.0f : 3.14159f;  // 0 for rising, π for falling

  for (int x = CHART_LEFT; x < CHART_RIGHT; x++) {
    // Normalize x position to 0-1 (representing 24 hours)
    float progress = (float)(x - CHART_LEFT) / CHART_W;

    // Calculate tide height using sine for full 24-hour cycle (2π radians)
    // This shows 2 complete peaks and 2 complete troughs
    float wavePhase = tidePhase + (progress * 6.28318f);  // 2π for 24-hour cycle
    float tideHeight = sin(wavePhase);  // -1 to 1

    // Convert to Y pixel position (invert: high tide at top, centered vertically)
    int y = chart_center_y - (tideHeight * CHART_H / 2.4f);

    // Color: bright if before current time, muted after
    bool is_past = (progress < targetProgress);
    uint16_t lineColor = is_past
      ? canvas.color565(0, 200, 255)     // bright cyan for past
      : canvas.color565(50, 80, 120);    // muted for future

    canvas.drawPixel(x, y, lineColor);
  }

  // Fill area under curve
  for (int x = CHART_LEFT; x < CHART_RIGHT; x++) {
    float progress = (float)(x - CHART_LEFT) / CHART_W;
    float wavePhase = tidePhase + (progress * 6.28318f);  // 2π for full cycle
    float tideHeight = sin(wavePhase);

    int y = chart_center_y - (tideHeight * CHART_H / 2.4f);

    bool is_past = (progress < targetProgress);
    uint16_t fillColor = is_past
      ? canvas.color565(0, 120, 180)     // semi-transparent bright cyan
      : canvas.color565(30, 50, 80);     // dark blue

    canvas.drawLine(x, y, x, CHART_BOT, fillColor);
  }

  // Draw current position vertical line
  int current_x = CHART_LEFT + (targetProgress * CHART_W);
  canvas.drawLine(current_x, CHART_TOP, current_x, CHART_BOT, canvas.color565(255, 200, 0));

  // Draw chart border
  canvas.drawRect(CHART_LEFT, CHART_TOP, CHART_W, CHART_H, canvas.color565(50, 80, 120));

  canvas.pushSprite(0, 0);
}

void setup()
{
  display.begin();
  display.setRotation(1);

  canvas.setColorDepth(16);
  canvas.createSprite(W, H);
}

void loop()
{
  if (millis() - lastFrame >= FRAME_TIME) {
    lastFrame = millis();
    drawUI();
  }
}
