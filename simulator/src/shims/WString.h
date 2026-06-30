#pragma once

#include <string>
#include <cstdint>
#include <cstring>

class String {
public:
    // -----------------------------------------------------------------------
    // Constructors
    // -----------------------------------------------------------------------
    String() = default;
    String(const char* cstr);
    String(char c);
    String(int value, unsigned char base = 10);
    String(unsigned int value, unsigned char base = 10);
    String(long value, unsigned char base = 10);
    String(unsigned long value, unsigned char base = 10);
    String(float value, unsigned char decimalPlaces = 2);
    String(double value, unsigned char decimalPlaces = 2);
    String(const String& other);
    String(String&& other) noexcept;

    // -----------------------------------------------------------------------
    // Assignment
    // -----------------------------------------------------------------------
    String& operator=(const String& rhs);
    String& operator=(String&& rhs) noexcept;
    String& operator=(const char* cstr);

    // -----------------------------------------------------------------------
    // Concatenation
    // -----------------------------------------------------------------------
    String& operator+=(const String& rhs);
    String& operator+=(const char* cstr);
    String& operator+=(char c);
    String& operator+=(int n);
    String& operator+=(unsigned int n);
    String& operator+=(long n);
    String& operator+=(unsigned long n);

    friend String operator+(const String& lhs, const String& rhs);
    friend String operator+(const String& lhs, const char* rhs);
    friend String operator+(const char* lhs, const String& rhs);

    // -----------------------------------------------------------------------
    // Comparison
    // -----------------------------------------------------------------------
    bool operator==(const String& rhs) const;
    bool operator==(const char* rhs) const;
    bool operator!=(const String& rhs) const;
    bool operator!=(const char* rhs) const;
    bool operator<(const String& rhs) const;
    bool operator>(const String& rhs) const;

    // -----------------------------------------------------------------------
    // Element access
    // -----------------------------------------------------------------------
    char  operator[](unsigned int index) const;
    char& operator[](unsigned int index);

    // -----------------------------------------------------------------------
    // Capacity / conversion
    // -----------------------------------------------------------------------
    unsigned int length() const;
    const char*  c_str() const;

    // -----------------------------------------------------------------------
    // Arduino String API
    // -----------------------------------------------------------------------
    char charAt(unsigned int index) const;
    void setCharAt(unsigned int index, char c);

    String substring(unsigned int from) const;
    String substring(unsigned int from, unsigned int to) const;

    int indexOf(char ch) const;
    int indexOf(const String& str) const;
    int indexOf(char ch, unsigned int fromIndex) const;
    int indexOf(const String& str, unsigned int fromIndex) const;

    int lastIndexOf(char ch) const;
    int lastIndexOf(const String& str) const;

    bool startsWith(const String& prefix) const;
    bool endsWith(const String& suffix) const;

    long   toInt() const;
    float  toFloat() const;

    void trim();
    void replace(const String& find, const String& replacement);
    void toCharArray(char* buf, unsigned int bufsize) const;
    void reserve(unsigned int size);
    void remove(unsigned int index, unsigned int count = 1);

    void toUpperCase();
    void toLowerCase();

    // -----------------------------------------------------------------------
    // Implicit conversion to const char* (Arduino code relies on this)
    // -----------------------------------------------------------------------
    operator const char*() const { return c_str(); }

private:
    std::string buf_;
};
