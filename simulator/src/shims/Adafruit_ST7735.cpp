#include "Adafruit_ST7735.h"

Adafruit_ST7735::Adafruit_ST7735(int8_t cs, int8_t dc, int8_t rst)
    : Adafruit_GFX(128, 160) {
    (void)cs; (void)dc; (void)rst;
}

Adafruit_ST7735::Adafruit_ST7735(int8_t cs, int8_t dc, int8_t mosi, int8_t sclk, int8_t rst)
    : Adafruit_GFX(128, 160) {
    (void)cs; (void)dc; (void)mosi; (void)sclk; (void)rst;
}

void Adafruit_ST7735::initR(uint8_t options) {
    (void)options;
    width_ = WIDTH_;
    height_ = HEIGHT_;
}

void Adafruit_ST7735::setSPISpeed(uint32_t speed) {
    (void)speed;
}

void Adafruit_ST7735::setRotation(uint8_t r) {
    rotation_ = r & 3;
    switch (rotation_) {
    case 0:
        width_ = 128;
        height_ = 160;
        break;
    case 1:
        width_ = 160;
        height_ = 128;
        break;
    case 2:
        width_ = 128;
        height_ = 160;
        break;
    case 3:
        width_ = 160;
        height_ = 128;
        break;
    }
}

void Adafruit_ST7735::invertDisplay(bool i) {
    (void)i;
}

void Adafruit_ST7735::setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    (void)x; (void)y; (void)w; (void)h;
}
