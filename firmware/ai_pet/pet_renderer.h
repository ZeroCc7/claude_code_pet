#pragma once

#include <Adafruit_GFX.h>

#include "game_types.h"

enum class PetEffect : uint8_t {
  None,
  Interaction,
  AiComplete,
  Evolution,
};

class PetRenderer {
 public:
  void draw(Adafruit_GFX& target, PetForm form, int16_t x, int16_t y,
            uint32_t now, PetEffect effect = PetEffect::None,
            uint32_t effectElapsed = 0);

 private:
  void drawEffect(Adafruit_GFX& target, PetForm form, int16_t x, int16_t y,
                  uint32_t effectElapsed);
};
