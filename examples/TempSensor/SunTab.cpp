#include "SunTab.h"
#include "Shared.h"
#include <SolarCalculator.h>

// Sun position comes from the SolarCalculator library (NOAA/Meeus
// algorithm). No hand-rolled astronomy here.
void drawSunIcon(int cx, int cy, int radius,
                  int year, int month, int day, int hour, int minute, int second,
                  bool showLabel)
{
  double azimuth, elevation;
  calcHorizontalCoordinates(year, month, day, hour, minute, second,
                             NICE_LAT, NICE_LON, azimuth, elevation);

  uint16_t color = gaugeColorForElevation((float)elevation);

  for (int a = 0; a < 360; a += 45) {
    int x1, y1, x2, y2;
    polarPoint(cx, cy, radius + 4, (float)a, x1, y1);
    polarPoint(cx, cy, radius + 9, (float)a, x2, y2);
    canvas.drawLine(x1, y1, x2, y2, color);
  }
  canvas.fillCircle(cx, cy, radius, color);
  canvas.drawCircle(cx, cy, radius, canvas.color565(255, 255, 255));

  if (showLabel) {
    canvas.setFont(&fonts::FreeSansBold12pt7b);
    canvas.setTextColor(color);
    canvas.setTextDatum(textdatum_t::middle_left);
    String elevStr = String((int)round(elevation));
    int textX = cx + radius + 14;
    canvas.drawString(elevStr, textX, cy);
    drawSmallDegreeAfter(elevStr, textX, cy, color);
    canvas.setTextDatum(textdatum_t::top_left);
  }
}
