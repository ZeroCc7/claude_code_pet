#pragma once
#include "Adafruit_GFX.h"

class GFXcanvas16 : public Adafruit_GFX {
public:
    GFXcanvas16(uint16_t w, uint16_t h);
    ~GFXcanvas16() override;
    void drawPixel(int16_t x, int16_t y, uint16_t color) override;
    void fillScreen(uint16_t color) override;
    uint16_t* getBuffer();
};

class GFXcanvas1 : public Adafruit_GFX {
public:
    GFXcanvas1(uint16_t w, uint16_t h);
    ~GFXcanvas1() override;
    void drawPixel(int16_t x, int16_t y, uint16_t color) override;
    void fillScreen(uint16_t color) override;
    uint8_t* getBuffer();
private:
    uint8_t* buffer_1bit_ = nullptr;
};
