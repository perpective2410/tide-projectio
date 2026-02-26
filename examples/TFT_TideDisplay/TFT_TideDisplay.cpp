#include <M5GFX.h>

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
  int base = 12 * 60 + 0; // Changed to show some tides as passed
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

  float cycle = 6.0f * 60.0f;
  // Progress shows how far through the cycle we are
  // If tide is in 35 minutes, we're near the end of the cycle
  return constrain((cycle - diff) / cycle, 0.0f, 1.0f);
}

bool isMontante(int nowMin, TideEvent next)
{
  return next.isHigh;
}

// -------- Draw arrow icon --------
void drawArrow(int x, int y, bool isUp, uint16_t color)
{
  if (isUp) {
    canvas.fillTriangle(x, y+8, x-5, y, x+5, y, color);
  } else {
    canvas.fillTriangle(x, y, x-5, y+8, x+5, y+8, color);
  }
}

// -------- Draw big center arrow --------
void drawBigCenterArrow(int centerX, int centerY, bool isUp, uint16_t color)
{
  int arrowSize = 120; // Bigger arrow
  if (isUp) {
    // Up arrow (Montante)
    canvas.fillTriangle(centerX, centerY - arrowSize,
                        centerX - arrowSize, centerY + arrowSize/2,
                        centerX + arrowSize, centerY + arrowSize/2, color);
    // Outline
    canvas.drawTriangle(centerX, centerY - arrowSize,
                        centerX - arrowSize, centerY + arrowSize/2,
                        centerX + arrowSize, centerY + arrowSize/2,
                        canvas.color565(0,240,255));
  } else {
    // Down arrow (Descendante)
    canvas.fillTriangle(centerX, centerY + arrowSize,
                        centerX - arrowSize, centerY - arrowSize/2,
                        centerX + arrowSize, centerY - arrowSize/2, color);
    // Outline
    canvas.drawTriangle(centerX, centerY + arrowSize,
                        centerX - arrowSize, centerY - arrowSize/2,
                        centerX + arrowSize, centerY - arrowSize/2,
                        canvas.color565(255,100,80));
  }
}

// -------- Main UI --------
void drawUI()
{
  canvas.fillScreen(canvas.color565(12,16,28));

  int nowMin = getMinutesNow();
  TideEvent next = getNextTide(nowMin);
  targetProgress = computeProgress(nowMin, next);
  animatedProgress += (targetProgress - animatedProgress) * 0.12f;

  int hour = nowMin/60;
  int minute = nowMin%60;

  int nextMin = next.hour*60 + next.minute;
  int diff = nextMin - nowMin;
  if (diff < 0) diff += 1440;
  bool isRising = isMontante(nowMin, next);


  // ========== MAIN CARD (FULL SCREEN) ==========
  canvas.fillRoundRect(20, 20, W-40, H-40, 30, canvas.color565(20,28,40));
  canvas.drawRoundRect(20, 20, W-40, H-40, 30, canvas.color565(70,90,120));

  // ========== LOCATION AT TOP (CENTERED, BIG) ==========
  canvas.setFont(&fonts::FreeSans18pt7b);
  canvas.setTextColor(canvas.color565(120,140,160));
  canvas.setCursor(W/2 - 180, 50);
  canvas.print("Le Palais - Belle Ile");

  // ========== TOP SECTION IN CARD ==========
  // "PROCHAINE MAREE" label (LEFT) - BIGGER
  canvas.setFont(&fonts::FreeSans18pt7b);
  canvas.setTextColor(canvas.color565(120,140,160));
  canvas.setCursor(60, 75);
  canvas.print("PROCHAINE MAREE");

  // Next tide time + type (VERY LARGE, LEFT) - CLOSE TOGETHER
  canvas.setFont(&fonts::FreeSansBold24pt7b);
  canvas.setTextColor(canvas.color565(200,220,240));
  canvas.setCursor(60, 145);
  canvas.printf("%02d:%02d", next.hour, next.minute);

  // Tide type (BM/PM) - RIGHT NEXT TO TIME
  canvas.setFont(&fonts::FreeSans18pt7b);
  canvas.setTextColor(canvas.color565(150,170,190));
  canvas.setCursor(310, 145);
  canvas.print(next.isHigh ? "PM" : "BM");

  // "Dans" label (RIGHT)
  canvas.setFont(&fonts::FreeSans18pt7b);
  canvas.setTextColor(canvas.color565(120,140,160));
  canvas.setCursor(W-250, 75);
  canvas.print("Dans");

  // Countdown (VERY LARGE) - CLOSE TO LABEL
  canvas.setFont(&fonts::FreeSansBold24pt7b);
  canvas.setTextColor(canvas.color565(255,180,60)); // Orange
  canvas.setCursor(W-250, 145);
  canvas.printf("%dm", diff % 60);

  // DESCENDANTE/MONTANTE (CENTER) - BIGGER
  canvas.setFont(&fonts::FreeSansBold24pt7b);
  const char* statusText = isRising ? "MONTANTE" : "DESCENDANTE";
  uint16_t statusColor = isRising ? canvas.color565(80,220,100) : canvas.color565(0,200,255);
  canvas.setTextColor(statusColor);
  canvas.setCursor(W/2 - 140, 145);
  canvas.print(statusText);

  // ========== PROGRESS BAR (SQUARE, NO ROUNDED, NO KNOB) ==========
  int barY = 190;
  int barW = W - 120;
  int barH = 35;
  canvas.fillRect(60, barY, barW, barH, canvas.color565(30,40,55));
  canvas.drawRect(60, barY, barW, barH, canvas.color565(80,100,130));

  // Progress fill
  int fillW = barW * animatedProgress;
  if (fillW > 0) {
    canvas.fillRect(60, barY, fillW, barH, canvas.color565(0,200,255));
  }

  // ========== SEPARATOR LINE ==========
  canvas.drawLine(60, 250, W-60, 250, canvas.color565(60,80,110));

  // ========== LEFT SIDE: TODAY'S TIDE EVENTS (NO BOX, NO SMALL ARROWS) ==========
  // All 4 tide events vertically centered
  for (int i = 0; i < 4; i++) {
    int eventY = 340 + i * 85;
    int tideMin = today[i].hour * 60 + today[i].minute;
    bool isPassed = tideMin < nowMin;

    // Time + Type CLOSE TOGETHER
    canvas.setFont(&fonts::FreeSansBold24pt7b);
    uint16_t timeColor = isPassed ? canvas.color565(80,90,110) : canvas.color565(200,220,240);
    canvas.setTextColor(timeColor);
    canvas.setCursor(80, eventY - 10);
    canvas.printf("%02d:%02d", today[i].hour, today[i].minute);

    // Type (LARGE) - RIGHT NEXT TO TIME
    canvas.setFont(&fonts::FreeSans18pt7b);
    if (today[i].isHigh) {
      uint16_t pmColor = isPassed ? canvas.color565(80,90,110) : canvas.color565(0,200,255);
      canvas.setTextColor(pmColor);
      canvas.setCursor(310, eventY - 10);
      canvas.print("PM");
    } else {
      uint16_t bmColor = isPassed ? canvas.color565(80,90,110) : canvas.color565(130,150,170);
      canvas.setTextColor(bmColor);
      canvas.setCursor(310, eventY - 10);
      canvas.print("BM");
    }

    // Coefficient (LARGE)
    if (today[i].isHigh) {
      uint16_t coefColor = isPassed ? canvas.color565(80,90,110) : canvas.color565(0,200,255);
      canvas.setTextColor(coefColor);
      canvas.setCursor(385, eventY - 10);
      canvas.printf("%d", today[i].coefficient);
    }
  }

  // ========== RIGHT SIDE: BIG ARROW (centered between tides and right edge) ==========
  uint16_t arrowColor = isRising ? canvas.color565(80,220,100) : canvas.color565(255,100,80);
  drawBigCenterArrow(W - 340, 430, isRising, arrowColor);

  // ========== COEFFICIENT NEXT TO ARROW (VERY BIG) ==========
  if (next.isHigh) {
    canvas.setFont(&fonts::FreeSansBold24pt7b);
    canvas.setTextColor(canvas.color565(0,200,255));
    canvas.setCursor(W - 480, 400);
    canvas.printf("%d", next.coefficient);
  }

  canvas.pushSprite(0,0);
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
