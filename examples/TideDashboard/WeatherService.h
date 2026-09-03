#pragma once
// WiFi (WiFiManager, same approach as TFT_TideDisplay) + periodic daily
// forecast fetch from the Open-Meteo Forecast API (free, no API key) for
// Le Palais — weather code and max wind speed for the same 7-day window as
// Dashboard.cpp's forecast strip. Updates dailyWeatherCode[]/dailyWindSpeedMax[]/
// weatherDataValid (Shared.h).
//
// First run: connect a phone/laptop to the "TideDashboardSetup" WiFi
// network it starts (192.168.4.1) to enter your real WiFi credentials.
// Already-saved ESP32 WiFi credentials (e.g. from running the tide display
// example) are tried automatically first.
void weatherServiceTask(void* params);

// Draws a small vector weather icon (sun / partly cloudy / cloud / rain /
// snow / storm / fog) for the given WMO weather code, centered at (cx, cy).
void drawWeatherIcon(int cx, int cy, int size, int weatherCode);
