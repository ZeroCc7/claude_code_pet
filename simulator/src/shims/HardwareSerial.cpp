#include "HardwareSerial.h"
#include <cstdio>

// ---------------------------------------------------------------------------
// Global instance — mirrors the Arduino `Serial` object
// ---------------------------------------------------------------------------
HardwareSerial Serial;

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------
void HardwareSerial::begin(unsigned long /*baud*/) {}

// ---------------------------------------------------------------------------
// RX ring buffer
// ---------------------------------------------------------------------------
int HardwareSerial::available() {
    return (rx_head_ - rx_tail_ + RX_BUF_SIZE) % RX_BUF_SIZE;
}

int HardwareSerial::read() {
    if (rx_head_ == rx_tail_) return -1;  // empty
    uint8_t byte = rx_buffer_[rx_tail_];
    rx_tail_ = (rx_tail_ + 1) % RX_BUF_SIZE;
    return byte;
}

int HardwareSerial::peek() {
    if (rx_head_ == rx_tail_) return -1;  // empty
    return rx_buffer_[rx_tail_];
}

void HardwareSerial::flush() {
    std::fflush(stdout);
}

// ---------------------------------------------------------------------------
// TX — write to stdout so simulator output is visible in the terminal
// ---------------------------------------------------------------------------
size_t HardwareSerial::write(uint8_t c) {
    std::putchar(static_cast<int>(c));
    std::fflush(stdout);
    return 1;
}

size_t HardwareSerial::write(const uint8_t *buf, size_t size) {
    if (size == 0) return 0;
    std::fwrite(buf, 1, size, stdout);
    std::fflush(stdout);
    return size;
}

// ---------------------------------------------------------------------------
// Simulator helpers — push bytes into the RX ring buffer
// ---------------------------------------------------------------------------
void HardwareSerial::sim_inject_rx(uint8_t byte) {
    int next_head = (rx_head_ + 1) % RX_BUF_SIZE;
    if (next_head == rx_tail_) return;  // buffer full, drop byte
    rx_buffer_[rx_head_] = byte;
    rx_head_ = next_head;
}

void HardwareSerial::sim_inject_rx(const uint8_t *buf, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        sim_inject_rx(buf[i]);
    }
}
