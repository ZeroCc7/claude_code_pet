#include "U8g2_for_Adafruit_GFX.h"

#include <cstring>

const uint8_t u8g2_font_wqy12_t_gb2312b[] = {0};

void U8G2_FOR_ADAFRUIT_GFX::begin(Adafruit_GFX& gfx) {
  gfx_ = &gfx;
}

void U8G2_FOR_ADAFRUIT_GFX::setFont(const uint8_t* font) {
  (void)font;
}

void U8G2_FOR_ADAFRUIT_GFX::setForegroundColor(uint16_t fg) {
  fg_ = fg;
}

void U8G2_FOR_ADAFRUIT_GFX::setBackgroundColor(uint16_t bg) {
  bg_ = bg;
}

void U8G2_FOR_ADAFRUIT_GFX::setFontDirection(uint8_t dir) {
  (void)dir;
}

void U8G2_FOR_ADAFRUIT_GFX::setFontMode(uint8_t mode) {
  (void)mode;
}

void U8G2_FOR_ADAFRUIT_GFX::drawUTF8(int16_t x, int16_t y, const char* str) {
  if (!gfx_ || !str) return;

  const uint8_t* p = reinterpret_cast<const uint8_t*>(str);
  int16_t cx = x;

  while (*p) {
    uint32_t codepoint = 0;
    int byteCount = 0;

    if ((*p & 0x80) == 0) {
      // ASCII: single byte
      codepoint = *p;
      byteCount = 1;
    } else if ((*p & 0xE0) == 0xC0) {
      codepoint = (*p & 0x1F);
      byteCount = 2;
    } else if ((*p & 0xF0) == 0xE0) {
      codepoint = (*p & 0x0F);
      byteCount = 3;
    } else if ((*p & 0xF8) == 0xF0) {
      codepoint = (*p & 0x07);
      byteCount = 4;
    } else {
      // Invalid UTF-8 lead byte; skip
      p++;
      continue;
    }

    for (int i = 1; i < byteCount; ++i) {
      if ((p[i] & 0xC0) != 0x80) {
        // Invalid continuation byte; treat lead byte alone
        byteCount = 1;
        codepoint = *p;
        break;
      }
      codepoint = (codepoint << 6) | (p[i] & 0x3F);
    }

    p += byteCount;

    if (codepoint < 128) {
      // ASCII: draw with GFX drawChar at size 2 (10x14 effective pixels)
      gfx_->setTextColor(fg_);
      gfx_->setTextSize(2);
      gfx_->setCursor(cx, y - 12);
      gfx_->drawChar(cx, y - 12, static_cast<char>(codepoint), fg_, bg_, 2);
      cx += 12;
    } else {
      // CJK or other multi-byte: draw a filled 12x12 rectangle as placeholder
      gfx_->fillRect(cx, y - 12, 12, 12, fg_);
      cx += 12;
    }
  }
}

int16_t U8G2_FOR_ADAFRUIT_GFX::getUTF8Width(const char* str) {
  if (!str) return 0;

  int16_t width = 0;
  const uint8_t* p = reinterpret_cast<const uint8_t*>(str);

  while (*p) {
    if ((*p & 0x80) == 0) {
      p += 1;
    } else if ((*p & 0xE0) == 0xC0) {
      p += 2;
    } else if ((*p & 0xF0) == 0xE0) {
      p += 3;
    } else if ((*p & 0xF8) == 0xF0) {
      p += 4;
    } else {
      p += 1;
    }
    width += 12;
  }

  return width;
}
