# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP32 firmware (Arduino/PlatformIO) for a coastal monitoring station in Bangor, France (Lat: 47.32275, Long: -3.16908). Tracks tides, solar position, and temperature, exposing data via Virtuino IoT (TCP), ThingSpeak, and a Wokwi simulation.

## Build & Flash

```bash
# Build
pio run

# Flash via USB
pio run -t upload

# Flash via OTA (to 192.168.1.90)
pio run -e esp_wroom_02-ota -t upload
```

Two environments are defined in `platformio.ini`: `esp_wroom_02-usb` (default) and `esp_wroom_02-ota`.

## Simulation (Wokwi)

Open in VS Code with the Wokwi extension installed, then run **Wokwi: Start Simulator** (F1). The simulator uses `.pio/build/esp_wroom_02-usb/firmware.bin` and `diagram.json` for the hardware layout.

## Architecture

The codebase is split across four source files that interact at runtime:

### `src/main.cpp`
Central orchestration. Runs a loop every 10 seconds that:
1. Reads two DS18B20 temperature sensors (OneWire on GPIO 4, addresses hardcoded) with 5-reading rolling averages.
2. Calls `Tides.cpp` to compute 4 days of tide data; recalculates at midnight.
3. Calls `TimeLord` and `SolarPosition` for sunrise/sunset and solar elevation/azimuth.
4. Serves results over a Virtuino TCP server (port 8002): 250 float variables (`V[]`) and 22 text variables (`Text_1`–`Text_22`).
5. Pushes temperature to ThingSpeak channel 1275039 (fields 4 & 5).

**Virtuino variable map (important for UI work):**
- `Text_1`/`Text_2` — sunrise/sunset times
- `Text_3` — solar elevation/azimuth at noon
- `Text_4`–`Text_11` — tide times and amplitudes for 4 days
- `Text_12`–`Text_19` — tide coefficients (morning/afternoon)
- `V[200–204]` — arrow indicators for next low tide
- `V[210–214]` — arrow indicators for next high tide

### `src/Tides.cpp` / `Tides.h`
Harmonic tide engine using 60+ tidal constituents with nodal modulation (u/f functions). Key entry point: `run_calculations(epoch)` returns a `TideStack` containing `TideInfo` (per-day) and `TideEvent` (individual high/low) objects.

### `src/TimeLord.cpp` / `TimeLord.h`
Computes sunrise/sunset and moon phase. Configured per instance with latitude, longitude, and UTC offset (hardcoded to 2 = UTC+2 for French summer time).

### `src/SolarPosition.cpp` / `SolarPosition.h`
David R. Brooks' algorithm for solar elevation and azimuth given epoch time and coordinates. Used separately from TimeLord for precise position data.

## Key Hardcoded Values

| Value | Location | Notes |
|---|---|---|
| Lat/Long | `main.cpp` `LATITUDE`/`LONGITUDE` | 47.32275, -3.16908 (Bangor, FR) — `float`, not `int` |
| UTC offset | auto-detected | `getFranceTimezoneOffset()` in `Tides.cpp` handles DST automatically |
| OneWire GPIO | `main.cpp` `ONEWIRE_PIN` | Pin 4 |
| Sensor addresses | `main.cpp` `SENSOR_IN`/`SENSOR_OUT` | Two 8-byte DS18B20 addresses |
| Virtuino port | `main.cpp` `VIRTUINO_PORT` | 8002 |
| ThingSpeak channel | `main.cpp` `THINGSPEAK_CHANNEL_ID` | 1275039 |
| OTA target IP | `platformio.ini` | 192.168.1.90 |

## Tide Verification

Harmonics in `Tides.cpp` are verified against [maree.info port 101](https://maree.info/101) (Belle-Île, Le Palais). Bangor is on Belle-Île island so Le Palais data is the correct reference. Two harmonic sets are used:
- `nonRefHarmonics` (Z0 = 3.0 m) — tide times and heights for Belle-Île
- `refHarmonics` (Z0 = 4.1364 m) — Brest reference, used only for tidal coefficient calculation (French coeff system always references Brest)

## Notes

- All code comments are in French.
- The WiFi SSID is `Wokwi-GUEST` — change to production credentials before deploying.
- `hour()` / `day()` etc. from TimeLib always return UTC (set via `setTime(utcEpoch)`). Tide event times stored in `TideInfo` are local time (offset applied inside `findTidesAndTimes`). Keep this distinction in mind when comparing times.
