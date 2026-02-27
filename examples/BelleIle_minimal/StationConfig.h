// ─── Station Configuration ────────────────────────────────────────────────────
// Edit this file to select which tidal stations to compile into your firmware.
// Uncomment the stations you need. Brest is always included.
// Only the selected stations will be compiled, saving memory on small boards.
//
// See the complete list of available stations at:
// src/stations/

#ifndef STATION_CONFIG_H
#define STATION_CONFIG_H

// ─── French Atlantic Stations ─────────────────────────────────────────────────
#define INCLUDE_LE_PALAIS           // Belle-Île-en-Mer
// #define INCLUDE_SHEERNESS        // UK (test station)
// #define INCLUDE_DOVER            // UK (test station)
// ... (170+ more stations available)

#endif // STATION_CONFIG_H
