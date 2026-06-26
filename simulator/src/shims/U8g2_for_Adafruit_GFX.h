#pragma once

#include <cstdint>
#include "Adafruit_GFX.h"

class U8G2_FOR_ADAFRUIT_GFX {
 public:
  void begin(Adafruit_GFX& gfx);
  void setFont(const uint8_t* font);
  void setForegroundColor(uint16_t fg);
  void setBackgroundColor(uint16_t bg);
  void setFontDirection(uint8_t dir);
  void setFontMode(uint8_t mode);
  void drawUTF8(int16_t x, int16_t y, const char* str);
  int16_t getUTF8Width(const char* str);

 private:
  Adafruit_GFX* gfx_ = nullptr;
  uint16_t fg_ = 0xFFFF;
  uint16_t bg_ = 0x0000;
};

extern const uint8_t u8g2_font_wqy12_t_gb2312b[];
