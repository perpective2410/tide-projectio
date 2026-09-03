#pragma once
#include <cstdint>
// WiFi (WiFiManager, same approach as TFT_TideDisplay) + periodic daily
// fetches from two Open-Meteo APIs for Le Palais, same 7-day window as
// Dashboard.cpp's forecast strip: the Forecast API (weather code, max wind
// speed/direction — dailyWeatherCode[]/dailyWindSpeedMax[]/weatherDataValid,
// Shared.h) and the Marine API (max wave height — dailyWaveHeightMax[]/
// marineDataValid, Shared.h; a separate host/request, see fetchDailyMarine()).
//
// First run: connect a phone/laptop to the "TideDashboardSetup" WiFi
// network it starts (192.168.4.1) to enter your real WiFi credentials.
// Already-saved ESP32 WiFi credentials (e.g. from running the tide display
// example) are tried automatically first.
void weatherServiceTask(void* params);

// Draws a small vector weather icon (sun / partly cloudy / cloud / rain /
// snow / storm / fog) for the given WMO weather code, centered at (cx, cy).
void drawWeatherIcon(int cx, int cy, int size, int weatherCode);

// Draws a small "ocean waves" icon (two wavy strokes), centered at (cx, cy).
void drawWaveIcon(int cx, int cy, int size, uint16_t color);
