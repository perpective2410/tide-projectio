#pragma once
// Compact sun icon (small sun-with-rays glyph, color-coded by elevation —
// see gaugeColorForElevation), optionally with the elevation in degrees
// printed beside it (showLabel=false for a bare icon, e.g. as a marker on
// the sun timeline bar). Takes UTC date/time (matches SolarCalculator's
// expected input), unlike the rest of Dashboard.cpp which works in local time.
void drawSunIcon(int cx, int cy, int radius,
                  int year, int month, int day, int hour, int minute, int second,
                  bool showLabel = true);
