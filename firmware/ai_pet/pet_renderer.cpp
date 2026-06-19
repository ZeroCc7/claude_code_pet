#include "pet_renderer.h"

#include <Adafruit_ST7735.h>

void PetRenderer::draw(Adafruit_GFX& tft, PetForm form, int16_t x, int16_t y,
                       uint32_t now) {
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
  tft.fillRoundRect(x + 5, y + 8, 30, 25, 7, body);
  tft.fillTriangle(x + 8, y + 11, x + 11, y + 2, x + 16, y + 11, body);
  tft.fillTriangle(x + 24, y + 11, x + 29, y + 2, x + 32, y + 11, body);
  tft.fillRect(x + 12, y + 17, 4, 5, ST77XX_WHITE);
  tft.fillRect(x + 25, y + 17, 4, 5, ST77XX_WHITE);
  tft.fillRect(x + 14, y + 18, 2, 3, ST77XX_BLACK);
  tft.fillRect(x + 25, y + 18, 2, 3, ST77XX_BLACK);
  tft.drawFastHLine(x + 17, y + 27, 7, ST77XX_BLACK);
  tft.fillRect(x + 9, y + 32, 8, 4, body);
  tft.fillRect(x + 24, y + 32, 8, 4, body);
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
