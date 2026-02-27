#pragma once
#include <Arduino.h>

// One harmonic constituent as stored in the compiled station data.
struct HarmonicConst {
    const char* name;       // constituent name (must match TideHarmonics table)
    double      amplitude;  // station amplitude (metres)
    double      phase;      // station phase lag (degrees)
};

// All data that defines a tidal station.
struct StationDef {
    const char*          id;             // used by setStation()
    double               z0;             // datum offset (metres)
    double               refLow;         // reserved
    double               refHigh;        // tidal range at reference point (e.g. 6.1 for Brest)
    const HarmonicConst* harmonics;      // pointer to compile-time array
    int                  harmonicCount;
};

// Station lookup is provided by StationRegistry.cpp.
// Use setStation(id) from Tides.h API instead of calling findStation directly.
extern const StationDef* findStation(const char* id);
