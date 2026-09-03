#pragma once
// Selects the "Le Palais" station and loads the Tides library reference
// data. Must be called once from setup() before drawDashboard().
void dashboardInit();
void drawDashboard(bool timeValid);

// Called by the touch task for any tap on screen — handles day-strip card
// selection.
void dashboardHandleTouch(int touchX, int touchY);
