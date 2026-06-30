#pragma once
#include <cstdint>

#define SPI_MODE0 0
#define MSBFIRST 1

class SPISettings {
public:
    SPISettings() = default;
    SPISettings(uint32_t, uint8_t, uint8_t) {}
};

class SPIClass {
public:
    void begin() {}
    void end() {}
    void setSCK(int) {}
    void setTX(int) {}
    void beginTransaction(SPISettings) {}
    void endTransaction() {}
    uint8_t transfer(uint8_t d) { return 0; }
    void setBitOrder(uint8_t) {}
    void setDataMode(uint8_t) {}
    void setClockDivider(uint8_t) {}
};

extern SPIClass SPI;
