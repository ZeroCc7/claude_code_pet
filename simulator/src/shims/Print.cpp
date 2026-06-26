#include "Print.h"
#include "WString.h"
#include <cstdio>
#include <cstdarg>
#include <cstring>

// ---------------------------------------------------------------------------
// write(buf, size) — default implementation loops over single-byte write()
// ---------------------------------------------------------------------------
size_t Print::write(const uint8_t *buf, size_t size) {
    size_t n = 0;
    for (size_t i = 0; i < size; ++i) {
        n += write(buf[i]);
    }
    return n;
}

// ---------------------------------------------------------------------------
// print() overloads
// ---------------------------------------------------------------------------
size_t Print::print(const char *s) {
    if (!s) return 0;
    size_t len = std::strlen(s);
    return write(reinterpret_cast<const uint8_t*>(s), len);
}

size_t Print::print(char c) {
    return write(static_cast<uint8_t>(c));
}

namespace {
    void int_to_buf(long value, int base, char *out, size_t out_size) {
        if (base == 16) {
            std::snprintf(out, out_size, "%lX", value);
        } else if (base == 8) {
            std::snprintf(out, out_size, "%lo", value);
        } else if (base == 2) {
            if (value == 0) { out[0] = '0'; out[1] = '\0'; return; }
            char tmp[65];
            int idx = 0;
            unsigned long v = static_cast<unsigned long>(value);
            while (v && idx < 64) {
                tmp[idx++] = (v & 1) ? '1' : '0';
                v >>= 1;
            }
            int out_idx = 0;
            while (idx > 0 && out_idx < static_cast<int>(out_size) - 1) {
                out[out_idx++] = tmp[--idx];
            }
            out[out_idx] = '\0';
        } else {
            std::snprintf(out, out_size, "%ld", value);
        }
    }

    void uint_to_buf(unsigned long value, int base, char *out, size_t out_size) {
        if (base == 16) {
            std::snprintf(out, out_size, "%lX", value);
        } else if (base == 8) {
            std::snprintf(out, out_size, "%lo", value);
        } else if (base == 2) {
            if (value == 0) { out[0] = '0'; out[1] = '\0'; return; }
            char tmp[65];
            int idx = 0;
            while (value && idx < 64) {
                tmp[idx++] = (value & 1) ? '1' : '0';
                value >>= 1;
            }
            int out_idx = 0;
            while (idx > 0 && out_idx < static_cast<int>(out_size) - 1) {
                out[out_idx++] = tmp[--idx];
            }
            out[out_idx] = '\0';
        } else {
            std::snprintf(out, out_size, "%lu", value);
        }
    }
}  // namespace

size_t Print::print(int n, int base) {
    char buf[65];
    int_to_buf(static_cast<long>(n), base, buf, sizeof(buf));
    return print(buf);
}

size_t Print::print(unsigned int n, int base) {
    char buf[65];
    uint_to_buf(static_cast<unsigned long>(n), base, buf, sizeof(buf));
    return print(buf);
}

size_t Print::print(long n, int base) {
    char buf[65];
    int_to_buf(n, base, buf, sizeof(buf));
    return print(buf);
}

size_t Print::print(unsigned long n, int base) {
    char buf[65];
    uint_to_buf(n, base, buf, sizeof(buf));
    return print(buf);
}

size_t Print::print(double n, int digits) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.*f", digits, n);
    return print(buf);
}

size_t Print::print(const String &s) {
    return print(s.c_str());
}

// ---------------------------------------------------------------------------
// println() overloads — print() + CRLF
// ---------------------------------------------------------------------------
size_t Print::println(const char *s) {
    size_t n = print(s);
    n += print('\r');
    n += print('\n');
    return n;
}

size_t Print::println(char c) {
    size_t n = print(c);
    n += print('\r');
    n += print('\n');
    return n;
}

size_t Print::println(int n, int base) {
    size_t w = print(n, base);
    w += print('\r');
    w += print('\n');
    return w;
}

size_t Print::println(unsigned int n, int base) {
    size_t w = print(n, base);
    w += print('\r');
    w += print('\n');
    return w;
}

size_t Print::println(long n, int base) {
    size_t w = print(n, base);
    w += print('\r');
    w += print('\n');
    return w;
}

size_t Print::println(unsigned long n, int base) {
    size_t w = print(n, base);
    w += print('\r');
    w += print('\n');
    return w;
}

size_t Print::println(double n, int digits) {
    size_t w = print(n, digits);
    w += print('\r');
    w += print('\n');
    return w;
}

size_t Print::println(const String &s) {
    return println(s.c_str());
}

// ---------------------------------------------------------------------------
// printf() — formatted output via vsnprintf into a stack buffer
// ---------------------------------------------------------------------------
int Print::printf(const char *fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    int len = std::vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (len <= 0) return 0;
    size_t to_write = static_cast<size_t>(len);
    if (to_write > sizeof(buf) - 1) to_write = sizeof(buf) - 1;
    return static_cast<int>(write(reinterpret_cast<const uint8_t*>(buf), to_write));
}
