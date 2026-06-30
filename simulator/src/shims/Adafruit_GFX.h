#pragma once
#include "Print.h"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <algorithm>

#define ST77XX_BLACK   0x0000
#define ST77XX_RED     0xF800
#define ST77XX_GREEN   0x07E0
#define ST77XX_BLUE    0x001F
#define ST77XX_CYAN    0x07FF
#define ST77XX_MAGENTA 0xF81F
#define ST77XX_YELLOW  0xFFE0
#define ST77XX_WHITE   0xFFFF

class Adafruit_GFX : public Print {
public:
    Adafruit_GFX(int16_t w, int16_t h);
    virtual ~Adafruit_GFX();

    virtual void drawPixel(int16_t x, int16_t y, uint16_t color);
    virtual void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    virtual void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    virtual void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    virtual void fillScreen(uint16_t color);
    virtual void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
    virtual void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
    virtual void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
    virtual void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
    virtual void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
    virtual void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
    virtual void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color);
    virtual void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color);
    virtual void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color);
    virtual void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color, uint16_t bg);
    virtual void drawXBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color);
    virtual void drawRGBBitmap(int16_t x, int16_t y, const uint16_t *bitmap, int16_t w, int16_t h);
    virtual void drawRGBBitmap(int16_t x, int16_t y, const uint16_t *bitmap, const uint8_t *mask, int16_t w, int16_t h);
    virtual void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size_x = 1, uint8_t size_y = 1);

    void setCursor(int16_t x, int16_t y);
    void setTextColor(uint16_t c);
    void setTextColor(uint16_t c, uint16_t bg);
    void setTextSize(uint8_t s);
    void setTextSize(uint8_t sx, uint8_t sy);
    void setTextWrap(bool w);
    void setRotation(uint8_t r);

    size_t write(uint8_t c) override;

    int16_t width() const;
    int16_t height() const;
    uint8_t getRotation() const;
    int16_t getCursorX() const;
    int16_t getCursorY() const;

    const uint16_t* getFramebuffer() const;
    uint16_t* getFramebuffer();

protected:
    uint16_t *framebuffer_ = nullptr;
    bool owns_buffer_ = true;
    int16_t WIDTH_, HEIGHT_;
    int16_t width_, height_;
    int16_t cursor_x_ = 0, cursor_y_ = 0;
    uint16_t textcolor_ = 0xFFFF, textbgcolor_ = 0x0000;
    uint8_t textsize_x_ = 1, textsize_y_ = 1;
    bool wrap_ = true;
    uint8_t rotation_ = 0;
    bool _cp437 = false;
    bool text_bg_ = false;

    static const uint8_t font[];

    void drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornermask, uint16_t color);
    void fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t corners, int16_t delta, uint16_t color);
};

// In the real Adafruit_GFX library, GFXcanvas classes are declared alongside
// Adafruit_GFX. Include them here so firmware code that #includes <Adafruit_GFX.h>
// gets GFXcanvas16/GFXcanvas1 as expected.
#include "GFXcanvas.h"
