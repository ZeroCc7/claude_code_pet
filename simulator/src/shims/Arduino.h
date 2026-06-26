#pragma once

// Include ALL standard C++ headers BEFORE defining min/max macros.
// These macros conflict with std::min/std::max and standard library
// template member functions (chrono::duration::min(), string_view::compare, etc.).
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <functional>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------
// Arduino type aliases
// ---------------------------------------------------------------------------
using byte = uint8_t;
using word = uint16_t;

// ---------------------------------------------------------------------------
// Pin modes and digital values
// ---------------------------------------------------------------------------
#define INPUT        0
#define OUTPUT       1
#define INPUT_PULLUP 2

#define HIGH 1
#define LOW  0

// ---------------------------------------------------------------------------
// PROGMEM — no-op on x86 desktop
// ---------------------------------------------------------------------------
#define PROGMEM
#define pgm_read_byte(addr)       (*(const uint8_t*)(addr))
#define pgm_read_word(addr)       (*(const uint16_t*)(addr))
#define pgm_read_dword(addr)      (*(const uint32_t*)(addr))
#define pgm_read_byte_near(addr)  pgm_read_byte(addr)
#define pgm_read_word_near(addr)  pgm_read_word(addr)
#define pgm_read_float(addr)      (*(const float*)(addr))
#define pgm_read_ptr(addr)        (*(const void**)(addr))

// ---------------------------------------------------------------------------
// Math macros
// ---------------------------------------------------------------------------
#ifndef constrain
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#endif

// min/max — the real Arduino core (Earle Philhower) exposes std::min/std::max
// in the global namespace so firmware code can write min<T>(a, b).  We must NOT
// define them as C macros because that breaks template call syntax and clashes
// with standard library headers.
using std::min;
using std::max;

#define _BV(bit) (1 << (bit))

#define sq(x)       ((x)*(x))
#define radians(deg) ((deg)*M_PI/180.0)
#define degrees(rad) ((rad)*180.0/M_PI)

// F() — flash-string helper; on desktop just passes the string literal through
#define F(string_literal) (string_literal)

// ---------------------------------------------------------------------------
// Arduino core function declarations
// ---------------------------------------------------------------------------
unsigned long millis();
unsigned long micros();
void          delay(unsigned long ms);
void          delayMicroseconds(unsigned int us);

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int  digitalRead(uint8_t pin);
int  analogRead(uint8_t pin);
void analogWrite(uint8_t pin, int val);

long random(long max_val);
long random(long min_val, long max_val);
void randomSeed(unsigned long seed);

long map(long value, long fromLow, long fromHigh, long toLow, long toHigh);

// ---------------------------------------------------------------------------
// Dependent shim headers (order matters: WString before Print before Serial)
// ---------------------------------------------------------------------------
#include "WString.h"
#include "Print.h"
#include "HardwareSerial.h"
