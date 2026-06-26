#include "WString.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
namespace {
    std::string int_to_string(long value, unsigned char base) {
        if (base == 16) {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%lX", value);
            return buf;
        }
        if (base == 8) {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%lo", value);
            return buf;
        }
        if (base == 2) {
            if (value == 0) return "0";
            std::string result;
            unsigned long v = static_cast<unsigned long>(value);
            while (v) {
                result.insert(result.begin(), (v & 1) ? '1' : '0');
                v >>= 1;
            }
            return result;
        }
        return std::to_string(value);
    }

    std::string uint_to_string(unsigned long value, unsigned char base) {
        if (base == 16) {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%lX", value);
            return buf;
        }
        if (base == 8) {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%lo", value);
            return buf;
        }
        if (base == 2) {
            if (value == 0) return "0";
            std::string result;
            while (value) {
                result.insert(result.begin(), (value & 1) ? '1' : '0');
                value >>= 1;
            }
            return result;
        }
        return std::to_string(value);
    }
}  // namespace

// ---------------------------------------------------------------------------
// Constructors
// ---------------------------------------------------------------------------
String::String(const char* cstr) : buf_(cstr ? cstr : "") {}

String::String(char c) : buf_(1, c) {}

String::String(int value, unsigned char base)
    : buf_(int_to_string(static_cast<long>(value), base)) {}

String::String(unsigned int value, unsigned char base)
    : buf_(uint_to_string(static_cast<unsigned long>(value), base)) {}

String::String(long value, unsigned char base)
    : buf_(int_to_string(value, base)) {}

String::String(unsigned long value, unsigned char base)
    : buf_(uint_to_string(value, base)) {}

String::String(float value, unsigned char decimalPlaces) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.*f", decimalPlaces, static_cast<double>(value));
    buf_ = buf;
}

String::String(double value, unsigned char decimalPlaces) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.*f", decimalPlaces, value);
    buf_ = buf;
}

String::String(const String& other) : buf_(other.buf_) {}
String::String(String&& other) noexcept : buf_(std::move(other.buf_)) {}

// ---------------------------------------------------------------------------
// Assignment
// ---------------------------------------------------------------------------
String& String::operator=(const String& rhs)   { buf_ = rhs.buf_; return *this; }
String& String::operator=(String&& rhs) noexcept { buf_ = std::move(rhs.buf_); return *this; }
String& String::operator=(const char* cstr)     { buf_ = cstr ? cstr : ""; return *this; }

// ---------------------------------------------------------------------------
// Concatenation
// ---------------------------------------------------------------------------
String& String::operator+=(const String& rhs)  { buf_ += rhs.buf_; return *this; }
String& String::operator+=(const char* cstr)    { if (cstr) buf_ += cstr; return *this; }
String& String::operator+=(char c)              { buf_ += c; return *this; }
String& String::operator+=(int n)               { buf_ += std::to_string(n); return *this; }
String& String::operator+=(unsigned int n)      { buf_ += std::to_string(n); return *this; }
String& String::operator+=(long n)              { buf_ += std::to_string(n); return *this; }
String& String::operator+=(unsigned long n)     { buf_ += std::to_string(n); return *this; }

String operator+(const String& lhs, const String& rhs) {
    String result(lhs);
    result += rhs;
    return result;
}

String operator+(const String& lhs, const char* rhs) {
    String result(lhs);
    result += rhs;
    return result;
}

String operator+(const char* lhs, const String& rhs) {
    String result(lhs);
    result += rhs;
    return result;
}

// ---------------------------------------------------------------------------
// Comparison
// ---------------------------------------------------------------------------
bool String::operator==(const String& rhs) const { return buf_ == rhs.buf_; }
bool String::operator==(const char* rhs) const   { return buf_ == (rhs ? rhs : ""); }
bool String::operator!=(const String& rhs) const { return !(*this == rhs); }
bool String::operator!=(const char* rhs) const   { return !(*this == rhs); }
bool String::operator<(const String& rhs) const  { return buf_ < rhs.buf_; }
bool String::operator>(const String& rhs) const  { return buf_ > rhs.buf_; }

// ---------------------------------------------------------------------------
// Element access
// ---------------------------------------------------------------------------
char  String::operator[](unsigned int index) const { return buf_[index]; }
char& String::operator[](unsigned int index)       { return buf_[index]; }

// ---------------------------------------------------------------------------
// Capacity / conversion
// ---------------------------------------------------------------------------
unsigned int String::length() const  { return static_cast<unsigned int>(buf_.size()); }
const char*  String::c_str() const   { return buf_.c_str(); }

// ---------------------------------------------------------------------------
// Arduino String API
// ---------------------------------------------------------------------------
char String::charAt(unsigned int index) const {
    if (index >= buf_.size()) return 0;
    return buf_[index];
}

void String::setCharAt(unsigned int index, char c) {
    if (index < buf_.size()) buf_[index] = c;
}

String String::substring(unsigned int from) const {
    if (from >= buf_.size()) return String("");
    return String(buf_.substr(from).c_str());
}

String String::substring(unsigned int from, unsigned int to) const {
    if (from >= buf_.size()) return String("");
    if (to > buf_.size()) to = static_cast<unsigned int>(buf_.size());
    if (from >= to) return String("");
    return String(buf_.substr(from, to - from).c_str());
}

int String::indexOf(char ch) const {
    auto pos = buf_.find(ch);
    return pos == std::string::npos ? -1 : static_cast<int>(pos);
}

int String::indexOf(const String& str) const {
    auto pos = buf_.find(str.buf_);
    return pos == std::string::npos ? -1 : static_cast<int>(pos);
}

int String::indexOf(char ch, unsigned int fromIndex) const {
    auto pos = buf_.find(ch, fromIndex);
    return pos == std::string::npos ? -1 : static_cast<int>(pos);
}

int String::indexOf(const String& str, unsigned int fromIndex) const {
    auto pos = buf_.find(str.buf_, fromIndex);
    return pos == std::string::npos ? -1 : static_cast<int>(pos);
}

int String::lastIndexOf(char ch) const {
    auto pos = buf_.rfind(ch);
    return pos == std::string::npos ? -1 : static_cast<int>(pos);
}

int String::lastIndexOf(const String& str) const {
    auto pos = buf_.rfind(str.buf_);
    return pos == std::string::npos ? -1 : static_cast<int>(pos);
}

bool String::startsWith(const String& prefix) const {
    if (prefix.buf_.size() > buf_.size()) return false;
    return buf_.compare(0, prefix.buf_.size(), prefix.buf_) == 0;
}

bool String::endsWith(const String& suffix) const {
    if (suffix.buf_.size() > buf_.size()) return false;
    return buf_.compare(buf_.size() - suffix.buf_.size(), suffix.buf_.size(), suffix.buf_) == 0;
}

long String::toInt() const {
    if (buf_.empty()) return 0;
    return std::atol(buf_.c_str());
}

float String::toFloat() const {
    if (buf_.empty()) return 0.0f;
    return static_cast<float>(std::atof(buf_.c_str()));
}

void String::trim() {
    auto start = buf_.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) { buf_.clear(); return; }
    auto end = buf_.find_last_not_of(" \t\r\n");
    buf_ = buf_.substr(start, end - start + 1);
}

void String::replace(const String& find, const String& replacement) {
    if (find.buf_.empty()) return;
    size_t pos = 0;
    while ((pos = buf_.find(find.buf_, pos)) != std::string::npos) {
        buf_.replace(pos, find.buf_.size(), replacement.buf_);
        pos += replacement.buf_.size();
    }
}

void String::toCharArray(char* buf, unsigned int bufsize) const {
    if (bufsize == 0) return;
    size_t copy_len = std::min(static_cast<size_t>(bufsize - 1), buf_.size());
    std::memcpy(buf, buf_.data(), copy_len);
    buf[copy_len] = '\0';
}

void String::reserve(unsigned int size) {
    buf_.reserve(size);
}

void String::remove(unsigned int index, unsigned int count) {
    if (index >= buf_.size()) return;
    buf_.erase(index, count);
}

void String::toUpperCase() {
    std::transform(buf_.begin(), buf_.end(), buf_.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
}

void String::toLowerCase() {
    std::transform(buf_.begin(), buf_.end(), buf_.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
}
