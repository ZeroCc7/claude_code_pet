#pragma once
#include "Adafruit_GFX.h"

#define INITR_BLACKTAB   0x00
#define INITR_GREENTAB   0x01
#define INITR_REDTAB     0x02

class Adafruit_ST7735 : public Adafruit_GFX {
public:
    Adafruit_ST7735(int8_t cs, int8_t dc, int8_t rst);
    Adafruit_ST7735(int8_t cs, int8_t dc, int8_t mosi, int8_t sclk, int8_t rst);
    void initR(uint8_t options);
    void setSPISpeed(uint32_t speed);
    void setRotation(uint8_t r);
    void invertDisplay(bool i);
    void setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
};
