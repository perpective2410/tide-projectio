#pragma once
// Shared display/layout constants and drawing helpers. See TideDashboard.ino
// for the overall architecture (tasks, render loop, setup/loop). There's a
// single screen now (Dashboard.cpp), which composes a compact temperature
// readout (TemperatureWidget.cpp) and a compact sun icon (SunWidget.cpp) into its
// header — no tab bar / tab switching anymore.

#include <M5Unified.h>
#include <M5GFX.h>

// -------- Shared canvas / layout --------
// Logical drawing space is landscape (1280x720); the physical sprite buffer
// is native-portrait with its own setRotation(1) for a fast pushSprite path
// — see the comment above `canvas`'s definition in TideDashboard.ino.
extern M5Canvas canvas;
static const int W = 1280;
static const int H = 720;
static const int PANEL_CX = W / 2;   // shared horizontal center for centered fallback messages

extern volatile bool redrawRequested;

// -------- Shared status colors --------
// The same red/orange/green triad shows up for RTC/WiFi/weather status
// (header pill, DS18B20 fallback text) and gauge extremes — centralized so
// they stay in sync instead of each call site picking its own color565().
static const uint16_t COLOR_ERROR = M5Canvas::color565(255, 90, 90);
static const uint16_t COLOR_WARN  = M5Canvas::color565(255, 150, 60);
static const uint16_t COLOR_OK    = M5Canvas::color565(120, 220, 140);

// -------- DS18B20 (written by sensorTask in TideDashboard.ino, read by TemperatureWidget) --------
extern float lastTempC;
extern bool lastReadOk;

// -------- Daily forecast, Open-Meteo (written by WeatherService.cpp) --------
// Same 7-day window as Dashboard's forecast strip (today + 6 days out).
static const int WEATHER_DAYS = 7;
extern int dailyWeatherCode[WEATHER_DAYS];       // WMO weather code — see drawWeatherIcon()
extern float dailyWindSpeedMax[WEATHER_DAYS];    // km/h
extern float dailyWindDirection[WEATHER_DAYS];   // degrees, meteorological convention (direction wind blows FROM)
extern bool weatherDataValid;

// Hourly wind, flat-indexed as [dayOffset * WEATHER_HOURLY_PER_DAY + hourOfDay]
// (timezone=auto in the Open-Meteo request means index 0 of each day is that
// day's local midnight).
static const int WEATHER_HOURLY_PER_DAY = 24;
extern float hourlyWindSpeed[WEATHER_DAYS * WEATHER_HOURLY_PER_DAY];      // km/h
extern float hourlyWindDirection[WEATHER_DAYS * WEATHER_HOURLY_PER_DAY];  // degrees, meteorological convention

// -------- Daily wave height, Open-Meteo Marine API (written by WeatherService.cpp) --------
// Separate API/host from the atmospheric forecast above (marine-api.open-meteo.com
// vs api.open-meteo.com), fetched as its own request — see fetchDailyMarine().
// The dwd_ewam model pinned there only forecasts ~3 days out, unlike the
// 7-day window everything else here uses — dailyWaveHeightValid[] is what
// tells the day-strip which of the 7 slots actually got a real value back,
// so it can leave the row blank instead of showing a false "0.0m" for a
// day the model just didn't cover.
extern float dailyWaveHeightMax[WEATHER_DAYS];   // meters
extern bool dailyWaveHeightValid[WEATHER_DAYS];
extern bool marineDataValid;

// -------- WiFi / weather status, for the header (written by WeatherService.cpp) --------
extern volatile bool wifiConnected;
extern String wifiSSID;          // valid once wifiConnected is true
extern String weatherErrorText;  // "" if the last fetch attempt succeeded (or none has run yet);
                                  // otherwise a short description of the last failure, e.g. "HTTP 404"

// -------- Le Palais (Belle-Ile-en-Mer) — used for sun (SunWidget, Dashboard) and weather (WeatherService) --------
static const double LE_PALAIS_LAT = 47.3483;
static const double LE_PALAIS_LON = -3.1553;  // positive = east

// -------- DST / local time (shared by SunWidget and Dashboard) --------
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
uint16_t gaugeColorForWind(float kmh);
void polarPoint(int cx, int cy, float r, float angleDeg, int& x, int& y);

// A "loading" indicator: three dots, one highlighted at a time, cycling one
// step per full-screen redraw (spinnerFrameCounter, bumped in drawUI(),
// TideDashboard.ino). Deliberately NOT a continuously-rotating arc — this UI's
// redraws are capped at ~4-5/sec (each repaints the whole screen), and a
// smooth-rotation angle sampled that infrequently either aliases into
// looking frozen or turns into flicker. A discrete step-to-step animation
// (like a text "..." indicator) has no such minimum frame rate to look right.
extern volatile uint32_t spinnerFrameCounter;
void drawSpinner(int cx, int cy, int radius, uint16_t color);
