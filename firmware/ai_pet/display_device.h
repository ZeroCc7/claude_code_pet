#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

#include "board_config.h"

class DisplayDevice {
 public:
  DisplayDevice();

  void begin();
  void setBacklight(uint8_t value);
  void drawSolid(uint16_t color, const char* label);
  void drawColorBars();
  void drawGrid();
  Adafruit_ST7735& raw();

 private:
  Adafruit_ST7735 tft_;
};

