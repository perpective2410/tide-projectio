# Tide Validation Example

This example generates tide predictions for an entire year (2026) for **Le Palais** station and outputs them in CSV format for comparison with official tide data from [maree.info/101](https://maree.info/101).

## How to Use

### 1. Build and Upload

```bash
cd examples/TideValidation
pio run -e esp_wroom_02-usb --project-dir ../../
```

Or in Arduino IDE:
- Open `TideValidation.ino`
- Select your board
- Upload

### 2. Capture the CSV Output

1. Open the Serial Monitor (115200 baud)
2. Wait for the sketch to finish generating all 365 days of tide data
3. Copy all the CSV lines (from the header through the last date line)
4. Paste into a text file and save as `tides_2026.csv`

### 3. Compare with maree.info

The CSV contains:
- **Date** — Calendar date (YYYY-MM-DD)
- **Event1–4** — Each tide event (high/low tide) with:
  - Type: `HT` (high tide) or `LT` (low tide)
  - Time: `HH:MM` (French local time)
  - Height: Water level in metres
  - Coeff: French tide coefficient

**To compare with maree.info:**

1. Visit [maree.info/101](https://maree.info/101)
2. Navigate to any date in 2026
3. Manually check a few dates against the CSV:
   - Do the tide **times** match within ±2 minutes?
   - Do the tide **heights** match within ±0.15m?
   - Do the **coefficients** match exactly?

### Sample Comparison

**Example: March 6, 2026**

Our library might output:
```
2026-03-06,HT,07:12,4.89,59,LT,13:20,0.64,59,HT,19:44,4.81,60
```

maree.info would show:
- HT: 07:10, 4.89m, coeff 59 ✓
- LT: 13:22, 0.65m, coeff 59 ✓
- HT: 19:42, 4.81m, coeff 60 ✓

**Acceptable accuracy:**
- Times: ±2 minutes
- Heights: ±0.15 metres
- Coefficients: Exact match

## Expected Results

The Le Palais harmonic model includes corrections based on 6-month validation against maree.info (March 2026 baseline). You should expect:

- **Times:** Within ±1–2 minutes for most tides
- **Heights:** Within ±0.1–0.15m for most tides
- **Coefficients:** Exact match with official values

If you see larger discrepancies, it may indicate:
1. Harmonic data needs refinement
2. Z0 datum offset (baseline water level) needs adjustment
3. Phase shift in major constituents (M2, S2, K1, etc.)

## Troubleshooting

**Sketch won't compile:**
- Ensure `StationConfig.h` is in the same folder
- Check that `INCLUDE_LE_PALAIS` is defined

**Times are consistently off by X minutes:**
- Check the M2 phase constant in `src/stations/LePalais.cpp`
- Typical fix: Add/subtract 2° per 3 minutes of error

**Heights are consistently too high/low:**
- Check the Z0 datum offset in `src/stations/LePalais.cpp`
- Typical fix: Adjust Z0 by ±0.1–0.2m

## References

- **Reference station:** [maree.info/101](https://maree.info/101) — Le Palais, Belle-Île-en-Mer
- **Data source:** SHOM (Service Hydrographique et Océanographique de la Marine)
- **Harmonic method:** Doodson constituent table with nodal corrections
