#pragma once
// Shared display/layout constants and drawing helpers. See TempSensor.ino
// for the overall architecture (tasks, render loop, setup/loop). There's a
// single screen now (TideTab.cpp), which composes a compact temperature
// readout (TemperatureTab.cpp) and a compact sun icon (SunTab.cpp) into its
// header — no tab bar / tab switching anymore.

#include <M5Unified.h>
#include <M5GFX.h>

// -------- Shared canvas / layout --------
// Logical drawing space is landscape (1280x720); the physical sprite buffer
// is native-portrait with its own setRotation(1) for a fast pushSprite path
// — see the comment above `canvas`'s definition in TempSensor.ino.
extern M5Canvas canvas;
static const int W = 1280;
static const int H = 720;
static const int PANEL_CX = W / 2;   // shared horizontal center for centered fallback messages

extern volatile bool redrawRequested;

// -------- DS18B20 (written by sensorTask in TempSensor.ino, read by TemperatureTab) --------
extern float lastTempC;
extern bool lastReadOk;

// -------- Nice, France (SunTab) --------
static const double NICE_LAT = 43.7102;
static const double NICE_LON = 7.2620;  // positive = east

// -------- DST / local time (shared by SunTab and TideTab) --------
// No NTP/configTzTime in this sketch, so these work directly off the RTC.
bool isEuropeDST(int year, int month, int day);
void getLocalFromRTC(int& year, int& month, int& day, int& hour, int& minute, int& second);
String formatLocalTime(double utcHours, int tzOffsetHours);   // SolarCalculator-style decimal UTC hours -> "HH:MM"
int dayOfWeekSunday0(int year, int month, int day);            // 0=Sunday .. 6=Saturday (Sakamoto's algorithm)

// -------- Shared drawing helpers --------
// The FreeSansBold*pt7b fonts are 7-bit ASCII only, so ° isn't in them —
// drawSmallDegreeAfter draws it as a small ring instead, right after text
// already printed at (x, y) in the current font/color.
void drawSmallDegreeAfter(const String& text, int x, int y, uint16_t color);

uint16_t gaugeColorForTemp(float t);
uint16_t gaugeColorForElevation(float e);
void polarPoint(int cx, int cy, float r, float angleDeg, int& x, int& y);
