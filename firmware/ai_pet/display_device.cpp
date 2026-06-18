#include "display_device.h"

DisplayDevice::DisplayDevice()
    : tft_(board::kTftCsPin, board::kTftDcPin, board::kTftResetPin) {
}

void DisplayDevice::begin() {
  pinMode(board::kBacklightPin, OUTPUT);
  analogWrite(board::kBacklightPin, board::kBacklightInitial);

  SPI.setSCK(board::kTftSckPin);
  SPI.setTX(board::kTftMosiPin);
  SPI.begin();

  tft_.initR(INITR_BLACKTAB);
  tft_.setSPISpeed(board::kInitialSpiHz);
  tft_.setRotation(0);
  tft_.fillScreen(ST77XX_BLACK);
  setBacklight(board::kBacklightNormal);
}

void DisplayDevice::setBacklight(uint8_t value) {
  analogWrite(board::kBacklightPin, value);
}

void DisplayDevice::drawSolid(uint16_t color, const char* label) {
  tft_.fillScreen(color);
  tft_.setTextColor(color == ST77XX_WHITE ? ST77XX_BLACK : ST77XX_WHITE);
  tft_.setTextSize(2);
  tft_.setCursor(4, 4);
  tft_.print(label);
}

void DisplayDevice::drawColorBars() {
  constexpr uint16_t colors[] = {
      ST77XX_RED,
      ST77XX_GREEN,
      ST77XX_BLUE,
      ST77XX_WHITE,
      ST77XX_BLACK,
  };
  constexpr int count = sizeof(colors) / sizeof(colors[0]);
  const int barWidth = board::kScreenWidth / count;

  for (int i = 0; i < count; ++i) {
    const int x = i * barWidth;
    const int width =
        i == count - 1 ? board::kScreenWidth - x : barWidth;
    tft_.fillRect(x, 0, width, board::kScreenHeight, colors[i]);
  }
}

void DisplayDevice::drawGrid() {
  tft_.fillScreen(ST77XX_BLACK);
  for (int x = 0; x < board::kScreenWidth; x += 8) {
    tft_.drawFastVLine(x, 0, board::kScreenHeight, ST77XX_BLUE);
  }
  for (int y = 0; y < board::kScreenHeight; y += 8) {
    tft_.drawFastHLine(0, y, board::kScreenWidth, ST77XX_GREEN);
  }
  tft_.drawRect(
      0, 0, board::kScreenWidth, board::kScreenHeight, ST77XX_RED);
}

Adafruit_ST7735& DisplayDevice::raw() {
  return tft_;
}

