#include "pet_renderer.h"

#include <Adafruit_ST7735.h>

#include "assets/pet_sprites.h"

void PetRenderer::draw(Adafruit_GFX& tft, PetForm form, int16_t x, int16_t y,
                       uint32_t now) {
  const PetSpriteFrame* frames = nullptr;
  if (form == PetForm::Egg) {
    frames = kPet_egg_frames;
  } else if (form == PetForm::RookieA) {
    frames = kPet_rookie_a_frames;
  } else if (form == PetForm::RookieB) {
    frames = kPet_rookie_b_frames;
  }
  if (frames) {
    const PetSpriteFrame& frame =
        frames[(now / 400) % kPetSpriteFrameCount];
    tft.drawRGBBitmap(x, y, frame.pixels, frame.mask, kPetSpriteWidth,
                      kPetSpriteHeight);
    return;
  }

  const uint16_t body = bodyColor(form);
  const int16_t bob = ((now / 600) % 2) ? 1 : 0;
  y += bob;

  if (form >= PetForm::FinalA1) {
    tft.fillCircle(x + 20, y + 8, 6, 0xF5CC);
    tft.fillTriangle(x + 14, y + 6, x + 16, y, x + 19, y + 7, 0x6D2B);
    tft.fillTriangle(x + 21, y + 7, x + 25, y, x + 27, y + 7, 0x6D2B);
    tft.fillRect(x + 14, y + 14, 13, 18, body);
    tft.fillTriangle(x + 14, y + 20, x + 6, y + 34, x + 16, y + 30, body);
    tft.fillTriangle(x + 27, y + 20, x + 34, y + 34, x + 25, y + 30, body);
    tft.drawCircle(x + 20, y + 21, 14, 0xFFE0);
    tft.drawPixel(x + 18, y + 8, ST77XX_BLACK);
    tft.drawPixel(x + 22, y + 8, ST77XX_BLACK);
    return;
  }
}

uint16_t PetRenderer::bodyColor(PetForm form) const {
  switch (form) {
    case PetForm::Egg:
      return ST77XX_CYAN;
    case PetForm::RookieA:
      return ST77XX_GREEN;
    case PetForm::RookieB:
      return ST77XX_MAGENTA;
    case PetForm::FinalA1:
      return ST77XX_YELLOW;
    case PetForm::FinalA2:
      return ST77XX_BLUE;
    case PetForm::FinalB1:
      return ST77XX_RED;
    case PetForm::FinalB2:
      return ST77XX_WHITE;
  }
  return ST77XX_CYAN;
}
