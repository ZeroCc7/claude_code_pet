#pragma once

#include <Arduino.h>

namespace board {

constexpr uint8_t kTftSckPin = 2;
constexpr uint8_t kTftMosiPin = 3;
constexpr uint8_t kTftDcPin = 4;
constexpr uint8_t kTftCsPin = 5;
constexpr uint8_t kTftResetPin = 6;
constexpr uint8_t kBacklightPin = 7;

constexpr uint8_t kButtonPins[] = {8, 9, 10, 11};
constexpr size_t kButtonCount = 4;

constexpr uint16_t kScreenWidth = 128;
constexpr uint16_t kScreenHeight = 160;
constexpr uint32_t kUsbBaud = 115200;
constexpr uint32_t kInitialSpiHz = 8000000;
constexpr uint8_t kBacklightInitial = 0;
constexpr uint8_t kBacklightNormal = 180;

static_assert(kButtonCount == 4, "UI requires exactly four buttons");
static_assert(kTftSckPin != kTftMosiPin, "SPI pins must be unique");

}  // namespace board

