#pragma once
// Compact "<LABEL> XX.XC" temperature readout, right-aligned at (rightX,
// centerY), for the Tide screen's header. Shows "<LABEL> --" in a muted
// color when invalid (e.g. no sensor reading yet). Returns the X of the
// text's left edge, so a caller placing another readout to its left can
// measure the real width instead of guessing a fixed offset.
int drawTemperatureLabeled(int rightX, int centerY, const char* label, float value, bool valid);
