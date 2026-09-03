// Le Palais tide/weather/sun dashboard for M5Stack Tab5 — single screen (no
// tabs): the tide chart (ported from TFT_TideDisplay, mimicking a
// maree.info-style layout) — Dashboard.cpp — drives the whole screen, with
// a 7-day Open-Meteo forecast strip, a sunrise/sunset timeline, and a
// compact DS18B20 temperature readout (TemperatureWidget.cpp) and sun icon
// (SunWidget.cpp) folded into its header.
//
// DS18B20 wiring:
//   3V3 ───────── DS18B20 VCC
//   GND ───────── DS18B20 GND
//   G49 ───────── DS18B20 DATA  (with internal pull-up enabled below)
//
// The clock (RTC) uses UTC directly — no NTP needed for that, and the Tab5's
// RTC keeps time across reflashes, so run the tide display example once
// first if it hasn't been synced yet. WiFi is used only for the 7-day
// forecast strip's weather icon/wind (Open-Meteo, see WeatherService.cpp)
// — first run, connect to the "TideDashboardSetup" AP it starts to
// configure it. The header's IN/OUT readouts are DS18B20-probe-related.
//
// File layout: this .ino only owns orchestration (tasks, setup/loop) and the
// top-level render call. Screen-specific data/drawing lives in Dashboard.cpp
// (which composes the compact widgets from TemperatureWidget.cpp/SunWidget.cpp);
// Shared.h/.cpp holds cross-file state and drawing helpers (canvas,
// degree-mark drawing, DST/local-time helpers); WeatherService.cpp owns
// WiFi + the daily forecast fetch.

#include <M5Unified.h>
#include <M5GFX.h>
#include <WiFi.h>               // setup()'s WiFi.setPins() call — see comment there
#include <OneWire.h>
#include <DallasTemperature.h>
#include "Shared.h"
#include "Dashboard.h"
#include "WeatherService.h"

#define ONE_WIRE_PIN 49

OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);

// The sprite's physical buffer is native-portrait (720x1280) with its own
// setRotation(1) so its logical drawing space is landscape (W x H, see
// Shared.h) — see the fast-pushSprite comment in setup() for why.
static const int PANEL_NATIVE_W = 720;
static const int PANEL_NATIVE_H = 1280;

// -------- Screenshot dump (debug aid, triggered over Serial) --------
// Sending the single byte 'S' over the serial/USB-CDC connection dumps the
// current framebuffer as raw RGB565: a 4-byte "SSHT" magic, width/height as
// little-endian uint16, then W*H*2 bytes of pixel data, row-major.
//
// This used to suspend every other task for the whole transfer (to stop
// their Serial.printf calls from splicing into the binary stream — see
// below) and run from renderTask. That deadlocked the entire board once: a
// screenshot client that dies mid-transfer (e.g. a killed script) leaves
// Serial.write() blocked forever waiting for a reader that's gone, and
// since renderTask was the one blocked inside it, touch/sensor/weather
// stayed suspended forever too — nothing left running to even notice.
//
// Fixed by splitting the two concerns: the *readRect() capture* into a
// PSRAM buffer is what needs the other tasks suspended (so their prints
// can't land mid-frame), and that's fast (a memory copy, no I/O) — done
// with everything else briefly paused, then immediately resumed. The
// *Serial.write()* of that buffer — the slow, potentially-blocking-forever
// part — happens afterwards from loop() (the Arduino main task) with every
// other task running normally. A stuck client can now only ever freeze the
// screenshot feature (and loop()'s 'S' polling) rather than the whole board.
static TaskHandle_t touchTaskHandle = nullptr;
static TaskHandle_t renderTaskHandle = nullptr;
static TaskHandle_t sensorTaskHandle = nullptr;
static TaskHandle_t weatherTaskHandle = nullptr;
static uint16_t* screenshotBuf = nullptr;   // W*H pixels, lazily allocated in PSRAM

static void captureScreenshot()
{
  if (!screenshotBuf) {
    screenshotBuf = (uint16_t*)ps_malloc((size_t)W * H * sizeof(uint16_t));
    if (!screenshotBuf) {
      Serial.println("[Screenshot] PSRAM allocation failed");
      return;
    }
  }

  if (touchTaskHandle) vTaskSuspend(touchTaskHandle);
  if (renderTaskHandle) vTaskSuspend(renderTaskHandle);
  if (sensorTaskHandle) vTaskSuspend(sensorTaskHandle);
  if (weatherTaskHandle) vTaskSuspend(weatherTaskHandle);
  for (int y = 0; y < H; y++) {
    canvas.readRect(0, y, W, 1, screenshotBuf + (size_t)y * W);
  }
  if (touchTaskHandle) vTaskResume(touchTaskHandle);
  if (renderTaskHandle) vTaskResume(renderTaskHandle);
  if (sensorTaskHandle) vTaskResume(sensorTaskHandle);
  if (weatherTaskHandle) vTaskResume(weatherTaskHandle);

  uint16_t w16 = (uint16_t)W, h16 = (uint16_t)H;
  Serial.write((const uint8_t*)"SSHT", 4);
  Serial.write((const uint8_t*)&w16, 2);
  Serial.write((const uint8_t*)&h16, 2);
  Serial.write((const uint8_t*)screenshotBuf, (size_t)W * H * 2);
  Serial.flush();
}

void drawUI(bool timeValid)
{
  unsigned long t0 = millis();
  spinnerFrameCounter = spinnerFrameCounter + 1;   // drives drawSpinner() — see its comment in Shared.h; avoids a "++ on volatile" deprecation warning
  canvas.fillScreen(canvas.color565(12, 16, 28));
  unsigned long t1 = millis();

  drawDashboard(timeValid);
  unsigned long t2 = millis();

  canvas.setTextDatum(textdatum_t::top_left);
  canvas.pushSprite(0, 0);
  unsigned long t3 = millis();

  Serial.printf("[Timing] fillScreen=%lu content=%lu pushSprite=%lu\n",
                t1 - t0, t2 - t1, t3 - t2);
}

// -------- Touch handling (runs on its own task for responsive taps) --------
void touchDetectionTask(void* params)
{
  m5::touch_point_t pts[5];
  bool prevTouching = false;
  for (;;) {
    int n = M5.Lcd.getTouchRaw(pts, 5);
    bool isTouching = (n > 0);
    if (isTouching && !prevTouching) {
      // M5.Display itself stays at rotation=0 (native portrait, for the fast
      // pushSprite path) — only the sprite's own logical drawing space is
      // landscape. The physical touch panel doesn't know about that trick,
      // so raw touch coordinates still need the same swap+invert measured
      // for landscape earlier: displayX = rawY, displayY = H - rawX.
      int touchX = constrain(pts[0].y, 0, W - 1);
      int touchY = constrain(H - pts[0].x, 0, H - 1);
      Serial.printf("[Touch] Raw X=%d Y=%d -> screen X=%d Y=%d\n", pts[0].x, pts[0].y, touchX, touchY);
      dashboardHandleTouch(touchX, touchY);
    }
    prevTouching = isTouching;
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// -------- Render task --------
// Owns drawUI() exclusively, polling redrawRequested at 20ms. Kept separate
// from sensorTask so a day-strip tap is never stuck waiting behind a
// blocking DS18B20 conversion (~750ms).
//
// redrawRequested alone only gets set roughly once a second (sensorTask's
// loop) or on touch/weather events, which made the day-strip's loading
// spinner (drawSpinner(), animated off millis()) jump ~100 deg per frame
// instead of spinning smoothly. So: keep redrawing back-to-back, with no
// delay, for as long as the weather fetch hasn't landed yet — that's the
// only thing the spinner is shown for, and it's a short one-time window at
// boot, not a steady-state cost.
void renderTask(void* params)
{
  for (;;) {
    if (redrawRequested || !weatherDataValid) {
      redrawRequested = false;
      unsigned long t0 = millis();
      auto dt = M5.Rtc.getDateTime();
      bool timeValid = M5.Rtc.isEnabled() && dt.date.year >= 2020;
      drawUI(timeValid);
      Serial.printf("[Render] drawn in %lu ms\n", millis() - t0);
    }
    vTaskDelay(pdMS_TO_TICKS(weatherDataValid ? 20 : 10));
  }
}

// -------- Sensor task --------
// Handles the blocking DS18B20 read on its own ~1s cadence, isolated from
// touch/render responsiveness. (This used to also log sun azimuth/elevation
// every second as a diagnostic — dropped once the SOLEIL row started
// showing that live on screen every frame; keeping both meant paying for
// the same calcHorizontalCoordinates() call twice a second for no reason.)
void sensorTask(void* params)
{
  for (;;) {
    sensors.requestTemperatures();
    float tempC = sensors.getTempCByIndex(0);

    lastReadOk = (tempC != DEVICE_DISCONNECTED_C);
    if (lastReadOk) {
      lastTempC = tempC;
      Serial.printf("[DS18B20] Temperature: %.2f C\n", tempC);
    } else {
      Serial.println("[DS18B20] Error: could not read temperature (sensor disconnected?)");
    }

    redrawRequested = true;
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void setup()
{
  Serial.begin(115200);
  delay(500);

  Serial.println("\n\n========== TIDE/WEATHER/SUN DASHBOARD ==========");

  auto cfg = M5.config();
  M5.begin(cfg);

  // The ESP32-P4 has no onboard radio — Tab5 talks to a WiFi co-processor
  // over SDIO, which must be pin-configured before any WiFi call or it
  // fails to initialize (repeated "sdmmc_card_init failed" / "esp_wifi_init
  // ESP_FAIL" — confirmed by testing without this call). Matches
  // TFT_TideDisplay.ino's setup() exactly.
  WiFi.setPins(/* CLK */ 12, /* CMD */ 13, /* D0 */ 11, /* D1 */ 10,
               /* D2 */ 9, /* D3 */ 8, /* RST */ 15);
  delay(100);

  M5.Display.setRotation(0);  // native portrait — fast DMA path, see Shared.h comment above `canvas`

  canvas.setColorDepth(16);
  canvas.createSprite(PANEL_NATIVE_W, PANEL_NATIVE_H);  // physical buffer: native portrait
  canvas.setRotation(1);  // logical drawing space: landscape (W x H)

  pinMode(ONE_WIRE_PIN, INPUT_PULLUP);

  sensors.begin();
  int deviceCount = sensors.getDeviceCount();
  Serial.printf("[DS18B20] Devices found on bus: %d\n", deviceCount);
  if (deviceCount == 0) {
    Serial.println("[DS18B20] WARNING: No sensor detected — check wiring / pull-up.");
  }

  if (!M5.Rtc.isEnabled()) {
    Serial.println("[RTC] WARNING: RTC not found!");
  }

  dashboardInit();

  xTaskCreate(touchDetectionTask, "touch", 2048, nullptr, 5, &touchTaskHandle);
  // 16KB: the tide harmonic calculation (loadSelectedDay -> tides(), 64
  // constituents) runs from here (unlike TFT_TideDisplay, where it ran from
  // Arduino's main loop() task, which has a much bigger default stack) and
  // overflowed a 4096-byte stack — confirmed via a "Stack protection fault"
  // panic in this task right after "[Tides] Calculating ...".
  xTaskCreate(renderTask, "render", 16384, nullptr, 4, &renderTaskHandle);
  xTaskCreate(sensorTask, "sensor", 4096, nullptr, 2, &sensorTaskHandle);
  // 16KB: TLS handshakes (WiFiClientSecure, for the HTTPS weather fetch)
  // are stack-hungry — matching the render task's size defensively rather
  // than risk another "Stack protection fault" like the tide calc did at 4KB.
  xTaskCreate(weatherServiceTask, "weather", 16384, nullptr, 1, &weatherTaskHandle);

  drawUI(false);

  Serial.println("Setup complete!\n");
}

void loop()
{
  if (Serial.available() && Serial.read() == 'S') {
    captureScreenshot();   // runs from this task on purpose — see the comment above it
  }
  delay(50);
}
