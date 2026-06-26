#pragma once
#include <cstdint>
#include <cstddef>

class String; // forward declare

class Print {
public:
    virtual ~Print() = default;
    virtual size_t write(uint8_t c) = 0;
    virtual size_t write(const uint8_t *buf, size_t size);

    size_t print(const char *s);
    size_t print(char c);
    size_t print(int n, int base = 10);
    size_t print(unsigned int n, int base = 10);
    size_t print(long n, int base = 10);
    size_t print(unsigned long n, int base = 10);
    size_t print(double n, int digits = 2);
    size_t print(const String &s);

    size_t println(const char *s = "");
    size_t println(char c);
    size_t println(int n, int base = 10);
    size_t println(unsigned int n, int base = 10);
    size_t println(long n, int base = 10);
    size_t println(unsigned long n, int base = 10);
    size_t println(double n, int digits = 2);
    size_t println(const String &s);

    int printf(const char *fmt, ...);

    size_t getWriteError() { return write_error_; }
    void clearWriteError() { write_error_ = 0; }

protected:
    void setWriteError(int err = 1) { write_error_ = err; }
    size_t write_error_ = 0;
};
