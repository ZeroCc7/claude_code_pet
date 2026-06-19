#pragma once

#include <U8g2_for_Adafruit_GFX.h>

#include "display_device.h"

class ChineseText {
 public:
  void begin(DisplayDevice& display);
  void color(uint16_t foreground);
  void draw(int16_t x, int16_t baseline, const char* text);

 private:
  U8G2_FOR_ADAFRUIT_GFX text_;
};

