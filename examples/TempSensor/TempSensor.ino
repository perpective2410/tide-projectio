// Standalone DS18B20 temperature sensor example for M5Stack Tab5.
// Single screen (no tabs): the Le Palais tide display (ported from
// TFT_TideDisplay, mimicking a maree.info-style layout) — TideTab.cpp —
// with a compact temperature readout (TemperatureTab.cpp) and sun icon
// (SunTab.cpp) folded into its header.
//
// Wiring:
//   3V3 ───────── DS18B20 VCC
//   GND ───────── DS18B20 GND
//   G49 ───────── DS18B20 DATA  (with internal pull-up enabled below)
//
// Everything uses the RTC's UTC time directly (no NTP/WiFi in this sketch).
// The Tab5's RTC keeps time across reflashes, so run the tide display example
// once first if the clock hasn't been synced yet.
//
// File layout: this .ino only owns orchestration (tasks, setup/loop) and the
// top-level render call. Screen-specific data/drawing lives in TideTab.cpp
// (which composes the compact widgets from TemperatureTab.cpp/SunTab.cpp);
// Shared.h/.cpp holds cross-file state and drawing helpers (canvas,
// degree-mark drawing, DST/local-time helpers).

#include <M5Unified.h>
#include <M5GFX.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SolarCalculator.h>   // sensorTask's own diagnostic sun-position log
#include "Shared.h"
#include "TideTab.h"

#define ONE_WIRE_PIN 49

OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);

// The sprite's physical buffer is native-portrait (720x1280) with its own
// setRotation(1) so its logical drawing space is landscape (W x H, see
// Shared.h) — see the fast-pushSprite comment in setup() for why.
static const int PANEL_NATIVE_W = 720;
static const int PANEL_NATIVE_H = 1280;

void drawUI(bool timeValid)
{
  unsigned long t0 = millis();
  canvas.fillScreen(canvas.color565(12, 16, 28));
  unsigned long t1 = millis();

  drawTideTab(timeValid);
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
      tideTabHandleTouch(touchX, touchY);
    }
    prevTouching = isTouching;
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// -------- Render task --------
// Owns drawUI() exclusively, polling redrawRequested at 20ms. Kept separate
// from sensorTask so a day-strip tap is never stuck waiting behind a
// blocking DS18B20 conversion (~750ms).
void renderTask(void* params)
{
  for (;;) {
    if (redrawRequested) {
      redrawRequested = false;
      unsigned long t0 = millis();
      auto dt = M5.Rtc.getDateTime();
      bool timeValid = M5.Rtc.isEnabled() && dt.date.year >= 2020;
      drawUI(timeValid);
      Serial.printf("[Render] drawn in %lu ms\n", millis() - t0);
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// -------- Sensor task --------
// Handles the blocking DS18B20 read (and logs sun position) on its own
// ~1s cadence, isolated from touch/render responsiveness.
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

    auto dt = M5.Rtc.getDateTime();
    if (M5.Rtc.isEnabled() && dt.date.year >= 2020) {
      double azimuth, elevation;
      calcHorizontalCoordinates(dt.date.year, dt.date.month, dt.date.date,
                                 dt.time.hours, dt.time.minutes, dt.time.seconds,
                                 NICE_LAT, NICE_LON, azimuth, elevation);
      Serial.printf("[SUN] Elevation: %.1f  Azimuth: %.1f\n", elevation, azimuth);
    }

    redrawRequested = true;
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void setup()
{
  Serial.begin(115200);
  delay(500);

  Serial.println("\n\n========== TIDE DISPLAY + TEMP/SUN WIDGETS ==========");

  auto cfg = M5.config();
  M5.begin(cfg);
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

  tideTabInit();

  xTaskCreate(touchDetectionTask, "touch", 2048, nullptr, 5, nullptr);
  // 16KB: the tide harmonic calculation (loadSelectedDay -> tides(), 64
  // constituents) runs from here (unlike TFT_TideDisplay, where it ran from
  // Arduino's main loop() task, which has a much bigger default stack) and
  // overflowed a 4096-byte stack — confirmed via a "Stack protection fault"
  // panic in this task right after "[Tides] Calculating ...".
  xTaskCreate(renderTask, "render", 16384, nullptr, 4, nullptr);
  xTaskCreate(sensorTask, "sensor", 4096, nullptr, 2, nullptr);

  drawUI(false);

  Serial.println("Setup complete!\n");
}

void loop()
{
  delay(1000);
}
