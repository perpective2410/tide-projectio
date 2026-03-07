// ======== TIDE VALIDATION SKETCH ========
// Generates tide predictions for entire year and outputs in CSV format
// Compare with: https://maree.info/101 (Le Palais)

#include <TimeLib.h>
#include "StationConfig.h"
#include <Tides.h>

// Reuse single static TideInfo buffer to avoid stack issues
static TideInfo ti;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n========== TIDE VALIDATION FOR LE PALAIS ==========\n");

  // Initialize station
  bool stationSet = setStation("Le Palais");
  if (!stationSet) {
    Serial.println("ERROR: Failed to load Le Palais station!");
    while(1) delay(1000);
  }

  Serial.println("Station loaded: Le Palais");
  Serial.println("Generating tide data for 2026...\n");

  // CSV Header
  Serial.println("Date,Event1_Type,Event1_Time,Event1_Height,Event1_Coeff,"
                 "Event2_Type,Event2_Time,Event2_Height,Event2_Coeff,"
                 "Event3_Type,Event3_Time,Event3_Height,Event3_Coeff,"
                 "Event4_Type,Event4_Time,Event4_Height,Event4_Coeff");

  int dayCounter = 0;

  // Generate tides for entire year 2026
  for (int month = 1; month <= 12; month++) {
    int daysInMonth = getDaysInMonth(month, 2026);

    for (int day = 1; day <= daysInMonth; day++) {
      // Calculate tides for this day (reuse static ti buffer)
      ti = tides(2026, month, day);

      // Format date
      char dateStr[11];
      snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d", 2026, month, day);
      Serial.print(dateStr);

      // Output each tide event
      int coeff_am = ti.morningCoefficient;
      int coeff_pm = ti.afternoonCoefficient;

      for (int i = 0; i < 4; i++) {
        Serial.print(",");

        if (i < ti.numEvents) {
          // Event type (HT = high tide, LT = low tide)
          Serial.print(ti.events[i].isPeak ? "HT" : "LT");
          Serial.print(",");

          // Event time (decimal hours -> HH:MM format)
          int hour = (int)ti.events[i].time;
          int minute = (int)((ti.events[i].time - hour) * 60.0f + 0.5f);
          if (minute == 60) { hour++; minute = 0; }
          Serial.printf("%02d:%02d", hour, minute);
          Serial.print(",");

          // Height (amplitude)
          Serial.printf("%.2f", ti.events[i].amplitude);
          Serial.print(",");

          // Coefficient (use appropriate one based on time of day)
          int event_hour = (int)ti.events[i].time;
          int coeff = (event_hour >= 12) ? coeff_pm : coeff_am;
          Serial.print(coeff);
        } else {
          // No event for this slot
          Serial.print(",");
          Serial.print(",");
          Serial.print(",");
        }
      }

      Serial.println();
      dayCounter++;
    }
  }

  Serial.println("\n========== GENERATION COMPLETE ==========");
  Serial.printf("Total days processed: %d\n", dayCounter);
  Serial.println("Copy the CSV output above and compare with maree.info/101");
  Serial.println("Focus on:");
  Serial.println("  - Time accuracy (±2 minutes is good)");
  Serial.println("  - Height accuracy (±0.15m is good)");
  Serial.println("  - Coefficient values (should match exactly)");
}

void loop() {
  delay(10000);
}

// Helper function to get days in month
int getDaysInMonth(int month, int year) {
  static const int daysInMonths[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  int days = daysInMonths[month - 1];

  // Check for leap year
  if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) {
    days = 29;
  }

  return days;
}
