#pragma once
// Selects the "Le Palais" station and loads the Tides library reference
// data. Must be called once from setup() before drawTideTab().
void tideTabInit();
void drawTideTab(bool timeValid);

// Called by the touch task for any tap outside the tab bar while the Tide
// tab is active — handles day-strip card selection.
void tideTabHandleTouch(int touchX, int touchY);
