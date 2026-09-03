#include "TemperatureWidget.h"
#include "Shared.h"

int drawTemperatureLabeled(int rightX, int centerY, const char* label, float value, bool valid)
{
  char buf[16];
  uint16_t color;
  if (valid) {
    snprintf(buf, sizeof(buf), "%s %.1fC", label, value);
    color = gaugeColorForTemp(value);
  } else {
    snprintf(buf, sizeof(buf), "%s --", label);
    color = canvas.color565(90, 100, 115);
  }

  canvas.setFont(&fonts::FreeSansBold18pt7b);
  canvas.setTextColor(color);
  canvas.setTextDatum(textdatum_t::middle_right);
  canvas.drawString(buf, rightX, centerY);
  canvas.setTextDatum(textdatum_t::top_left);

  return rightX - canvas.textWidth(buf);
}
