#pragma once

#include <Adafruit_ST7735.h>

#include "game_types.h"

class PetRenderer {
 public:
  void draw(Adafruit_ST7735& tft, PetForm form, int16_t x, int16_t y,
            uint32_t now);

 private:
  uint16_t bodyColor(PetForm form) const;
};

