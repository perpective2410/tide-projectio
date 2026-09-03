#include "Shared.h"
#include <time.h>

M5Canvas canvas(&M5.Display);

volatile bool redrawRequested = true;

float lastTempC = NAN;
bool lastReadOk = false;

int dailyWeatherCode[WEATHER_DAYS] = {};
float dailyWindSpeedMax[WEATHER_DAYS] = {};
float dailyWindDirection[WEATHER_DAYS] = {};
bool weatherDataValid = false;

float hourlyWindSpeed[WEATHER_DAYS * WEATHER_HOURLY_PER_DAY] = {};
float hourlyWindDirection[WEATHER_DAYS * WEATHER_HOURLY_PER_DAY] = {};

float dailyWaveHeightMax[WEATHER_DAYS] = {};
bool marineDataValid = false;

volatile bool wifiConnected = false;
String wifiSSID = "";
String weatherErrorText = "";

// -------- Europe (France) DST — last Sunday of March 01:00 UTC to last Sunday of October 01:00 UTC --------
int dayOfWeekSunday0(int year, int month, int day)
{
  static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  int y = year;
  if (month < 3) y -= 1;
  return (y + y / 4 - y / 100 + y / 400 + t[month - 1] + day) % 7;
}

static int lastSundayOfMonth(int year, int month)
{
  static const int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  int lastDay = daysInMonth[month - 1];
  if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) lastDay = 29;
  return lastDay - dayOfWeekSunday0(year, month, lastDay);
}

bool isEuropeDST(int year, int month, int day)
{
  if (month < 3 || month > 10) return false;
  if (month > 3 && month < 10) return true;
  int ls = lastSundayOfMonth(year, month);
  return (month == 3) ? (day >= ls) : (day < ls);
}

// France local civil date/time from the RTC (UTC) — day boundaries matter
// for the tide tab (which calendar day's tides to load), so this goes
// through an epoch round-trip rather than a simple hour%24. No
// configTzTime()/NTP in this sketch, so TZ is unset (UTC default) and
// mktime()/gmtime() work directly as UTC<->epoch conversions.
void getLocalFromRTC(int& year, int& month, int& day, int& hour, int& minute, int& second)
{
  auto dt = M5.Rtc.getDateTime();
  struct tm tmUtc = {};
  tmUtc.tm_year = dt.date.year - 1900;
  tmUtc.tm_mon = dt.date.month - 1;
  tmUtc.tm_mday = dt.date.date;
  tmUtc.tm_hour = dt.time.hours;
  tmUtc.tm_min = dt.time.minutes;
  tmUtc.tm_sec = dt.time.seconds;
  tmUtc.tm_isdst = 0;
  time_t utcEpoch = mktime(&tmUtc);

  int tz = isEuropeDST(dt.date.year, dt.date.month, dt.date.date) ? 2 : 1;
  time_t localEpoch = utcEpoch + (time_t)tz * 3600;
  struct tm* localTm = gmtime(&localEpoch);

  year = localTm->tm_year + 1900;
  month = localTm->tm_mon + 1;
  day = localTm->tm_mday;
  hour = localTm->tm_hour;
  minute = localTm->tm_min;
  second = localTm->tm_sec;
}

// SolarCalculator returns times of day in decimal UTC hours (e.g. 5.75 = 05:45).
String formatLocalTime(double utcHours, int tzOffsetHours)
{
  if (isnan(utcHours)) return "--:--";
  double localMin = fmod(utcHours * 60.0 + tzOffsetHours * 60.0 + 1440.0, 1440.0);
  int hh = (int)(localMin / 60.0);
  int mm = ((int)localMin) % 60;
  char buf[8];
  snprintf(buf, sizeof(buf), "%02d:%02d", hh, mm);
  return String(buf);
}

// ==================== Drawing ====================

void drawSmallDegreeAfter(const String& text, int x, int y, uint16_t color)
{
  int w = canvas.textWidth(text);
  canvas.drawCircle(x + w + 6, y - 9, 3, color);
}

uint16_t gaugeColorForTemp(float t)
{
  if (t < 10) return canvas.color565(80, 160, 255);   // cold — blue
  if (t < 20) return canvas.color565(0, 220, 255);    // cool — cyan
  if (t < 28) return canvas.color565(80, 220, 120);   // comfortable — green
  if (t < 34) return canvas.color565(255, 175, 50);   // warm — orange
  return canvas.color565(255, 80, 80);                // hot — red
}

// Wind speed thresholds loosely follow the Beaufort scale (km/h).
uint16_t gaugeColorForWind(float kmh)
{
  if (kmh < 10) return canvas.color565(100, 180, 255);   // light air/breeze — blue
  if (kmh < 20) return canvas.color565(80, 220, 160);    // moderate breeze — green
  if (kmh < 35) return canvas.color565(255, 205, 70);    // fresh/strong breeze — gold
  if (kmh < 50) return canvas.color565(255, 140, 60);    // near gale/gale — orange
  return canvas.color565(255, 70, 70);                   // storm — red
}

uint16_t gaugeColorForElevation(float e)
{
  if (e < 0) return canvas.color565(90, 100, 130);    // below horizon — muted
  if (e < 10) return canvas.color565(255, 130, 60);   // dawn/dusk — orange
  if (e < 30) return canvas.color565(255, 205, 70);   // low sun — gold
  if (e < 55) return canvas.color565(255, 230, 130);  // high sun — bright yellow
  return canvas.color565(255, 250, 220);              // near-zenith — warm white
}

void polarPoint(int cx, int cy, float r, float angleDeg, int& x, int& y)
{
  float rad = angleDeg * (PI / 180.0f);
  x = cx + (int)(r * cosf(rad));
  y = cy + (int)(r * sinf(rad));
}

volatile uint32_t spinnerFrameCounter = 0;

void drawSpinner(int cx, int cy, int radius, uint16_t color)
{
  uint16_t dimColor = canvas.color565(55, 62, 74);
  int spacing = max(4, radius);
  int dotR = max(2, radius / 3);
  int activePhase = spinnerFrameCounter % 3;
  for (int i = 0; i < 3; i++) {
    int dx = cx + (i - 1) * spacing;
    bool active = (i == activePhase);
    canvas.fillCircle(dx, cy, active ? dotR + 2 : dotR, active ? color : dimColor);
  }
}
