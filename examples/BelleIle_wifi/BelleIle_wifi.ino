// Tide station — Belle-Île-en-Mer (Le Palais)
// Calculates tidal events for DAYS_FORECAST days, prints to Serial.
//
// DISCLAIMER: This library is designed for French Atlantic tidal stations only.
// Results are approximations based on harmonic analysis and DO NOT replace
// official tide tables published by the SHOM (Service Hydrographique et
// Océanographique de la Marine — https://www.shom.fr).
// Do not use this data for navigation or safety-critical purposes.

// Station configuration is in StationConfig.h — edit that file to select stations.
#include "StationConfig.h"
#include <WiFi.h>
#include <TimeLib.h>
#include <NTPClient.h>
#include <Tides.h>

// ─── Configuration ────────────────────────────────────────────────────────────

const char* WIFI_SSID     = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

constexpr int  DAYS_FORECAST = 4;
constexpr long UPDATE_MS     = 10000;   // print tides every 10 s (demo)

// ─── Globals ──────────────────────────────────────────────────────────────────

WiFiUDP     ntpUDP;
NTPClient   timeClient(ntpUDP, "pool.ntp.org", 0, 86400000);
TideStack   tideStack(DAYS_FORECAST);

int           lastDay       = -1;
unsigned long prevUpdate    = 0;

// ─── Helpers ──────────────────────────────────────────────────────────────────

String formatHM(float h) {
    if (fabsf(h) < 0.0001f) return " ";
    while (h >= 24.0f) h -= 24.0f;
    while (h <   0.0f) h += 24.0f;
    int hh = (int)h;
    int mm = (int)((h - hh) * 60.0f + 0.5f);
    if (mm == 60) { hh++; mm = 0; }
    char buf[8];
    snprintf(buf, sizeof(buf), "%02dh%02d", hh, mm);
    return String(buf);
}

void printTides() {
    Serial.println("═══ Tide Belle-Île (Le Palais) ═══");
    for (int d = 0; d < DAYS_FORECAST; d++) {
        const TideInfo& ti = tideStack.peek(d);
        if (ti.epoch < 0) continue;

        char dateBuf[12];
        snprintf(dateBuf, sizeof(dateBuf), "%02d/%02d/%04d",
                 day(ti.epoch), month(ti.epoch), year(ti.epoch));
        Serial.print(dateBuf);
        Serial.print("  AM coefficient: ");  Serial.print(ti.morningCoefficient);
        Serial.print("  PM coefficient: "); Serial.println(ti.afternoonCoefficient);

        for (int e = 0; e < ti.numEvents; e++) {
            Serial.print("  ");
            Serial.print(ti.events[e].isPeak ? "HM " : "BM ");
            Serial.print(formatHM(ti.getEventTime(e)));
            Serial.print("  ");
            Serial.print(ti.events[e].amplitude, 2);
            Serial.println(" m");
        }
    }
}

// ─── setup ────────────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(9600);
    while (!Serial);

    // WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print('.'); }
    Serial.println("\nWiFi: " + WiFi.localIP().toString());

    setStation("Le Palais");

    // NTP time sync
    timeClient.begin();
    timeClient.update();
    time_t epoch = timeClient.getEpochTime();
    setTime(epoch);
    lastDay = day();

    // Pre-calculate DAYS_FORECAST days
    for (int i = 0; i < DAYS_FORECAST; i++) {
        TideInfo ti = tides(epoch + (time_t)i * SECS_PER_DAY);
        tideStack.push(ti);
    }

    printTides();
}

// ─── loop ─────────────────────────────────────────────────────────────────────

void loop() {
    unsigned long now = millis();
    if (now - prevUpdate < UPDATE_MS) return;
    prevUpdate = now;

    // At midnight: drop oldest day, add the next one
    if (day() != lastDay) {
        lastDay = day();
        time_t newEpoch = timeClient.getEpochTime()
                          + (time_t)(DAYS_FORECAST - 1) * SECS_PER_DAY;
        TideInfo ti = tides(newEpoch);
        tideStack.push(ti);
        printTides();
    }
}
