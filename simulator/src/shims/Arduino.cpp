#include "Arduino.h"
#include <chrono>
#include <cstdio>

// ---------------------------------------------------------------------------
// Time base — captured once at process start
// ---------------------------------------------------------------------------
namespace {
    const auto kEpoch = std::chrono::steady_clock::now();
}

unsigned long millis() {
    auto now = std::chrono::steady_clock::now();
    return static_cast<unsigned long>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now - kEpoch).count());
}

unsigned long micros() {
    auto now = std::chrono::steady_clock::now();
    return static_cast<unsigned long>(
        std::chrono::duration_cast<std::chrono::microseconds>(now - kEpoch).count());
}

// ---------------------------------------------------------------------------
// Delays — no-ops so we never block the host game loop
// ---------------------------------------------------------------------------
void delay(unsigned long /*ms*/) {}
void delayMicroseconds(unsigned int /*us*/) {}

// ---------------------------------------------------------------------------
// GPIO stubs — all no-ops; digitalRead always returns HIGH (pull-up idle)
// ---------------------------------------------------------------------------
void pinMode(uint8_t /*pin*/, uint8_t /*mode*/) {}
void digitalWrite(uint8_t /*pin*/, uint8_t /*val*/) {}
int  digitalRead(uint8_t /*pin*/)  { return HIGH; }
int  analogRead(uint8_t /*pin*/)   { return 0; }
void analogWrite(uint8_t /*pin*/, int /*val*/) {}

// ---------------------------------------------------------------------------
// Random — thin wrapper over std::rand()
// ---------------------------------------------------------------------------
long random(long max_val) {
    if (max_val <= 0) return 0;
    return std::rand() % max_val;
}

long random(long min_val, long max_val) {
    if (max_val <= min_val) return min_val;
    return min_val + std::rand() % (max_val - min_val);
}

void randomSeed(unsigned long seed) {
    std::srand(static_cast<unsigned int>(seed));
}

// ---------------------------------------------------------------------------
// map() — standard Arduino linear interpolation
// ---------------------------------------------------------------------------
long map(long value, long fromLow, long fromHigh, long toLow, long toHigh) {
    if (fromHigh == fromLow) return toLow;  // avoid divide-by-zero
    return (value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow) + toLow;
}
