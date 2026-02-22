#pragma once
#include <Arduino.h>
#include <TimeLib.h>
#include "TideHarmonics.h"

struct TideEvent {
    double amplitude;
    float  time;     // local time, decimal hours (e.g. 14.5 = 14h30)
    bool   isPeak;   // true = high tide, false = low tide
};

struct TideInfo {
    TideEvent events[4];   // up to 2 peaks + 2 troughs per day
    int    numEvents;
    int    morningCoefficient;    // French tide coefficient (Brest-based), morning range
    int    afternoonCoefficient;  // French tide coefficient, afternoon range
    time_t epoch;

    float getEventTime(int index) const {
        if (index >= 0 && index < numEvents) return events[index].time;
        return 0.0f;
    }

    TideInfo() : numEvents(0), morningCoefficient(0), afternoonCoefficient(0), epoch(-1) {
        for (int i = 0; i < 4; i++) events[i] = TideEvent();
    }
};

class TideStack {
private:
    int       stackSize;
    TideInfo* stack;
    int       count;

public:
    TideStack(int daysToCalculate);
    ~TideStack();

    void      push(TideInfo& tideInfo);
    TideInfo* getStack();
    int       getCount();
    int       getTop();
    TideInfo& peek(int index);
};

// ── Public API ──────────────────────────────────────────────────────────────

// Select a station by id.  Station data is compiled into the firmware
// (see src/stations/).  Brest is loaded automatically as the French
// coefficient reference.
// minAmplitude (metres): harmonics smaller than this are skipped for speed.
//   0.005 (default) skips ~30% of minor harmonics with <5 mm accuracy impact.
//   Pass 0.0 for full precision with all harmonics included.
// Example: setStation("Le Palais")
bool setStation(const char* id, double minAmplitude = 0.005);

// Calculate tides for the calendar day that contains 'epoch' (UTC).
TideInfo tides(time_t epoch);

// Convenience overload — builds a noon-UTC epoch for the given date.
TideInfo tides(int year, int month, int day);

// France timezone offset in minutes (+60 winter / +120 summer DST)
int getFranceTimezoneOffset(time_t epoch);
