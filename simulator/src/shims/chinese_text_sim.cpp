// Simulator replacement for chinese_text.cpp
// Uses the shim U8G2_FOR_ADAFRUIT_GFX for Chinese text rendering.

#include "chinese_text.h"

void ChineseText::begin(DisplayDevice& display) {
  begin(display.raw());
}

void ChineseText::begin(Adafruit_GFX& target) {
  text_.begin(target);
  text_.setFontMode(1);
  text_.setFontDirection(0);
  text_.setFont(u8g2_font_wqy12_t_gb2312b);
  text_.setForegroundColor(ST77XX_WHITE);
}

void ChineseText::color(uint16_t foreground) {
  text_.setForegroundColor(foreground);
}

void ChineseText::draw(int16_t x, int16_t baseline, const char* text) {
  text_.drawUTF8(x, baseline, text);
}
