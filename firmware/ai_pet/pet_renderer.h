#pragma once

#include <Adafruit_GFX.h>

#include "game_types.h"

class PetRenderer {
 public:
  void draw(Adafruit_GFX& target, PetForm form, int16_t x, int16_t y,
            uint32_t now);

 private:
  uint16_t bodyColor(PetForm form) const;
};
