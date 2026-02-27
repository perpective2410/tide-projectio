// ─────────────────────────────────────────────────────────────────────────────
// Station Configuration Template
// ─────────────────────────────────────────────────────────────────────────────
//
// This file is a TEMPLATE and DOCUMENTATION. It is used by PlatformIO as a
// fallback if no StationConfig.h is found in your sketch directory.
//
// To customize which stations are compiled:
//
// ═══ For PlatformIO Users ═══════════════════════════════════════════════════
// Create examples/BelleIle_minimal/StationConfig.h (or your example's directory)
// with the station definitions you want. The build will automatically find it.
//
// ═══ For Arduino IDE Users ═════════════════════════════════════════════════
// 1. In your sketch folder, create a new file called: StationConfig.h
// 2. Add your station #define statements (see examples below)
// 3. Make sure it's in the same folder as your .ino file
// 4. Arduino IDE will automatically include it
//
// ═══ Available Stations ════════════════════════════════════════════════════
// Station names use underscores to replace spaces and hyphens.
// Examples: LE_PALAIS, SAINT_MALO, BOULOGNE_SUR_MER, SAINT_VAAST_LA_HOUGUE
//
// ═══ Brest ═════════════════════════════════════════════════════════════════
// Always included by default — it's used as the reference station for French
// tidal coefficients. You don't need to explicitly define it.
//
// ═══ Example Configuration ══════════════════════════════════════════════════
// Uncomment the stations you need:
//
// #define INCLUDE_LE_PALAIS              // Belle-Île-en-Mer
// #define INCLUDE_SAINT_MALO             // Brittany coast
// #define INCLUDE_BREST                  // Brittany (optional, always included)
//
// ═══ Memory Considerations ══════════════════════════════════════════════════
// Each station adds ~5-15 KB to your firmware depending on the number of
// harmonic constituents. Small boards (< 1MB flash) should only include
// essential stations. Brest is ~8 KB.
//
// ═════════════════════════════════════════════════════════════════════════════

#ifndef STATION_CONFIG_H
#define STATION_CONFIG_H

// ─── DEFAULT: Only Brest ──────────────────────────────────────────────────
// If you're seeing this, it means:
// - PlatformIO: Create examples/BelleIle_minimal/StationConfig.h
// - Arduino IDE: Create StationConfig.h in your sketch folder
//
// Without a custom StationConfig.h, only Brest is compiled.
// Uncomment stations below if you want them compiled by default:

// #define INCLUDE_LE_PALAIS
// #define INCLUDE_SAINT_MALO
// #define INCLUDE_SHEERNESS
// #define INCLUDE_DOVER

#endif // STATION_CONFIG_H
