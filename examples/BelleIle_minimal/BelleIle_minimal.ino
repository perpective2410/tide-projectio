// Minimal example — no WiFi, no NTP.
// Calculates tides for a hardcoded date and prints to Serial.
//
// DISCLAIMER: This library is designed for French Atlantic tidal stations only.
// Results are approximations based on harmonic analysis and DO NOT replace
// official tide tables published by the SHOM (Service Hydrographique et
// Océanographique de la Marine — https://www.shom.fr).
// Do not use this data for navigation or safety-critical purposes.

#include <Tides.h>

void setup() {
    Serial.begin(115200);
    while (!Serial);

    setStation("Le Palais");

    TideInfo ti = tides(2026, 2, 21);

    Serial.println("21/02/2026");
    Serial.print("AM coeff: "); Serial.println(ti.morningCoefficient);
    Serial.print("PM coeff: "); Serial.println(ti.afternoonCoefficient);

    for (int i = 0; i < ti.numEvents; i++) {
        float h  = ti.events[i].time;
        int   hh = (int)h;
        int   mm = (int)((h - hh) * 60.0f + 0.5f);
        if (mm == 60) { hh++; mm = 0; }
        char buf[20];
        snprintf(buf, sizeof(buf), "  %s %02dh%02d  %.2f m",
                 ti.events[i].isPeak ? "HM" : "BM", hh, mm,
                 ti.events[i].amplitude);
        Serial.println(buf);
    }
}

void loop() {}
