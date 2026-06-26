#pragma once
#include "Print.h"
#include <cstdint>

class HardwareSerial : public Print {
public:
    void begin(unsigned long baud);
    int available();
    int read();
    int peek();
    void flush();
    size_t write(uint8_t c) override;
    size_t write(const uint8_t *buf, size_t size) override;

    // Arduino: operator bool() returns true when Serial is connected/ready.
    // Simulator: always ready.
    operator bool() const { return true; }

    // Simulator: inject bytes into RX buffer
    void sim_inject_rx(uint8_t byte);
    void sim_inject_rx(const uint8_t *buf, size_t len);

private:
    static constexpr int RX_BUF_SIZE = 512;
    uint8_t rx_buffer_[RX_BUF_SIZE]{};
    int rx_head_ = 0;
    int rx_tail_ = 0;
};

extern HardwareSerial Serial;
