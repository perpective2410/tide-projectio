#include "WeatherService.h"
#include "Shared.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

static const unsigned long FETCH_INTERVAL_MS = 20UL * 60UL * 1000UL;  // 20 min — plenty for a daily forecast

static void fetchDailyWeather()
{
  WiFiClientSecure client;
  client.setInsecure();   // skip TLS cert validation — acceptable for a public, non-sensitive read-only API on a hobby device

  HTTPClient http;
  char url[320];
  snprintf(url, sizeof(url),
           "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f"
           "&daily=weathercode,windspeed_10m_max,winddirection_10m_dominant"
           "&hourly=windspeed_10m,winddirection_10m&timezone=auto&forecast_days=%d",
           LE_PALAIS_LAT, LE_PALAIS_LON, WEATHER_DAYS);

  if (!http.begin(client, url)) {
    Serial.println("[Weather] http.begin() failed");
    weatherErrorText = "Weather: request failed";
    redrawRequested = true;
    return;
  }

  int code = http.GET();
  if (code != 200) {
    Serial.printf("[Weather] GET failed: HTTP %d\n", code);
    http.end();
    weatherErrorText = code > 0 ? ("Weather: HTTP " + String(code)) : "Weather: no response";
    redrawRequested = true;
    return;
  }

  String payload = http.getString();
  http.end();

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.printf("[Weather] JSON parse failed: %s\n", err.c_str());
    weatherErrorText = "Weather: bad JSON";
    redrawRequested = true;
    return;
  }

  JsonArray codes = doc["daily"]["weathercode"];
  JsonArray winds = doc["daily"]["windspeed_10m_max"];
  JsonArray windDirs = doc["daily"]["winddirection_10m_dominant"];
  if (codes.isNull() || winds.isNull() || windDirs.isNull()) {
    Serial.println("[Weather] Response missing daily.weathercode / windspeed_10m_max / winddirection_10m_dominant");
    weatherErrorText = "Weather: bad response";
    redrawRequested = true;
    return;
  }

  int n = min((int)codes.size(), WEATHER_DAYS);
  for (int i = 0; i < n; i++) {
    dailyWeatherCode[i] = codes[i].as<int>();
    dailyWindSpeedMax[i] = winds[i].as<float>();
    dailyWindDirection[i] = windDirs[i].as<float>();
  }

  JsonArray hourlySpeed = doc["hourly"]["windspeed_10m"];
  JsonArray hourlyDir = doc["hourly"]["winddirection_10m"];
  if (!hourlySpeed.isNull() && !hourlyDir.isNull()) {
    int hn = min((int)hourlySpeed.size(), WEATHER_DAYS * WEATHER_HOURLY_PER_DAY);
    for (int i = 0; i < hn; i++) {
      hourlyWindSpeed[i] = hourlySpeed[i].as<float>();
      hourlyWindDirection[i] = hourlyDir[i].as<float>();
    }
  } else {
    Serial.println("[Weather] Response missing hourly.windspeed_10m / winddirection_10m");
  }

  weatherDataValid = true;
  weatherErrorText = "";
  redrawRequested = true;
  Serial.printf("[Weather] Daily forecast updated (%d days), today: code=%d wind=%.0fkm/h from %.0f deg\n",
                n, dailyWeatherCode[0], dailyWindSpeedMax[0], dailyWindDirection[0]);
}

void weatherServiceTask(void* params)
{
  WiFiManager wifiManager;
  WiFi.mode(WIFI_AP_STA);
  delay(500);

  wifiManager.setConfigPortalBlocking(false);
  wifiManager.setConnectTimeout(10);
  wifiManager.setConnectRetries(2);
  wifiManager.setConfigPortalTimeout(0);

  Serial.println("[Weather] Attempting WiFi autoConnect with saved credentials...");
  wifiManager.autoConnect("TideDashboardSetup");

  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP("TideDashboardSetup");
  wifiManager.startWebPortal();
  Serial.println("[Weather] AP 'TideDashboardSetup' active at 192.168.4.1 for WiFi setup if needed");

  unsigned long lastFetch = 0;
  for (;;) {
    wifiManager.process();

    bool nowConnected = WiFi.isConnected();
    if (nowConnected != wifiConnected) {
      wifiConnected = nowConnected;
      if (nowConnected) {
        wifiSSID = WiFi.SSID();
        Serial.printf("[WiFi] Connected to \"%s\"\n", wifiSSID.c_str());
      } else {
        Serial.println("[WiFi] Disconnected");
      }
      redrawRequested = true;
    }

    if (nowConnected && (lastFetch == 0 || millis() - lastFetch >= FETCH_INTERVAL_MS)) {
      fetchDailyWeather();
      lastFetch = millis();
    }

    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

// -------- Icon --------
// WMO weather codes (https://open-meteo.com/en/docs, "WMO Weather interpretation
// codes") bucketed into a handful of simple vector glyphs — no bitmap assets.
void drawWeatherIcon(int cx, int cy, int size, int weatherCode)
{
  uint16_t sunColor = canvas.color565(255, 195, 60);
  uint16_t cloudColor = canvas.color565(170, 185, 200);
  uint16_t rainColor = canvas.color565(90, 160, 255);
  uint16_t snowColor = canvas.color565(230, 235, 245);
  uint16_t stormColor = canvas.color565(255, 205, 60);

  bool isClear = (weatherCode == 0);
  bool isPartlyCloudy = (weatherCode >= 1 && weatherCode <= 3);
  bool isFog = (weatherCode == 45 || weatherCode == 48);
  bool isRain = (weatherCode >= 51 && weatherCode <= 67) || (weatherCode >= 80 && weatherCode <= 82);
  bool isSnow = (weatherCode >= 71 && weatherCode <= 77) || (weatherCode >= 85 && weatherCode <= 86);
  bool isStorm = (weatherCode >= 95);

  if (isClear) {
    for (int a = 0; a < 360; a += 45) {
      int x1, y1, x2, y2;
      polarPoint(cx, cy, size * 0.6f, (float)a, x1, y1);
      polarPoint(cx, cy, size * 0.95f, (float)a, x2, y2);
      canvas.drawLine(x1, y1, x2, y2, sunColor);
    }
    canvas.fillCircle(cx, cy, (int)(size * 0.5f), sunColor);
    return;
  }

  // Everything else is cloud-based: a puff of overlapping circles, with a
  // partial sun behind it for "partly cloudy" and drops/flakes below for
  // rain/snow/storm.
  int cloudCY = cy + (isPartlyCloudy ? size / 6 : 0);

  if (isPartlyCloudy) {
    canvas.fillCircle(cx - size / 2, cy - size / 2, (int)(size * 0.32f), sunColor);
  }

  uint16_t cColor = isFog ? canvas.color565(140, 150, 160) : cloudColor;
  canvas.fillCircle(cx - size / 3, cloudCY, (int)(size * 0.38f), cColor);
  canvas.fillCircle(cx + size / 4, cloudCY - size / 8, (int)(size * 0.42f), cColor);
  canvas.fillCircle(cx, cloudCY + size / 10, (int)(size * 0.40f), cColor);
  canvas.fillRoundRect(cx - size / 2, cloudCY, size, (int)(size * 0.36f), (int)(size * 0.18f), cColor);

  if (isRain || isStorm) {
    uint16_t dropColor = isStorm ? stormColor : rainColor;
    for (int i = -1; i <= 1; i++) {
      int dx = cx + i * (size / 3);
      int dy0 = cloudCY + (int)(size * 0.45f);
      canvas.drawLine(dx, dy0, dx - 2, dy0 + 8, dropColor);
    }
  } else if (isSnow) {
    for (int i = -1; i <= 1; i++) {
      int dx = cx + i * (size / 3);
      canvas.fillCircle(dx, cloudCY + (int)(size * 0.5f), 2, snowColor);
    }
  }
}
