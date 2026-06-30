#include "GFXcanvas.h"
#include <cstring>

GFXcanvas16::GFXcanvas16(uint16_t w, uint16_t h)
    : Adafruit_GFX(w, h) {
    delete[] framebuffer_;
    framebuffer_ = new uint16_t[w * h]{};
    owns_buffer_ = true;
}

GFXcanvas16::~GFXcanvas16() {
}

void GFXcanvas16::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
    framebuffer_[y * width_ + x] = color;
}

void GFXcanvas16::fillScreen(uint16_t color) {
    std::fill(framebuffer_, framebuffer_ + (width_ * height_), color);
}

uint16_t* GFXcanvas16::getBuffer() {
    return framebuffer_;
}

GFXcanvas1::GFXcanvas1(uint16_t w, uint16_t h)
    : Adafruit_GFX(w, h) {
    delete[] framebuffer_;
    framebuffer_ = nullptr;
    owns_buffer_ = false;

    uint32_t bytes = ((uint32_t)w * h + 7) / 8;
    buffer_1bit_ = new uint8_t[bytes]{};
}

GFXcanvas1::~GFXcanvas1() {
    delete[] buffer_1bit_;
    buffer_1bit_ = nullptr;
}

void GFXcanvas1::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return;

    int16_t t;
    switch (getRotation()) {
    case 1:
        t = x;
        x = WIDTH_ - 1 - y;
        y = t;
        break;
    case 2:
        x = WIDTH_ - 1 - x;
        y = HEIGHT_ - 1 - y;
        break;
    case 3:
        t = x;
        x = y;
        y = HEIGHT_ - 1 - t;
        break;
    }

    uint32_t bit_offset = (uint32_t)y * WIDTH_ + x;
    uint32_t byte_offset = bit_offset / 8;
    uint8_t bit_mask = 0x80 >> (bit_offset & 7);

    if (color)
        buffer_1bit_[byte_offset] |= bit_mask;
    else
        buffer_1bit_[byte_offset] &= ~bit_mask;
}

void GFXcanvas1::fillScreen(uint16_t color) {
    uint32_t bytes = ((uint32_t)WIDTH_ * HEIGHT_ + 7) / 8;
    std::memset(buffer_1bit_, color ? 0xFF : 0x00, bytes);
}

uint8_t* GFXcanvas1::getBuffer() {
    return buffer_1bit_;
}
