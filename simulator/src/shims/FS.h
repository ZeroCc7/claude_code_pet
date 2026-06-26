#pragma once

#include "Print.h"
#include <cstdio>
#include <cstring>

class File : public Print {
 public:
  File() = default;

  File(FILE* f, const char* name) : fp_(f) {
    if (name) {
      strncpy(name_, name, sizeof(name_) - 1);
      name_[sizeof(name_) - 1] = '\0';
    }
  }

  ~File() {
    close();
  }

  size_t write(uint8_t c) override {
    if (!fp_) return 0;
    return fwrite(&c, 1, 1, fp_);
  }

  size_t write(const uint8_t* buf, size_t size) override {
    if (!fp_) return 0;
    return fwrite(buf, 1, size, fp_);
  }

  int available() {
    if (!fp_) return 0;
    long pos = ftell(fp_);
    fseek(fp_, 0, SEEK_END);
    long end = ftell(fp_);
    fseek(fp_, pos, SEEK_SET);
    return static_cast<int>(end - pos);
  }

  int read() {
    if (!fp_) return -1;
    uint8_t c;
    if (fread(&c, 1, 1, fp_) != 1) return -1;
    return c;
  }

  size_t read(uint8_t* buf, size_t len) {
    if (!fp_) return 0;
    return fread(buf, 1, len, fp_);
  }

  int peek() {
    if (!fp_) return -1;
    long pos = ftell(fp_);
    int c = read();
    fseek(fp_, pos, SEEK_SET);
    return c;
  }

  void flush() {
    if (fp_) fflush(fp_);
  }

  void close() {
    if (fp_) {
      fclose(fp_);
      fp_ = nullptr;
    }
  }

  uint32_t position() {
    if (!fp_) return 0;
    return static_cast<uint32_t>(ftell(fp_));
  }

  uint32_t size() {
    if (!fp_) return 0;
    long pos = ftell(fp_);
    fseek(fp_, 0, SEEK_END);
    long end = ftell(fp_);
    fseek(fp_, pos, SEEK_SET);
    return static_cast<uint32_t>(end);
  }

  const char* name() const {
    return name_;
  }

  operator bool() const {
    return fp_ != nullptr;
  }

 private:
  FILE* fp_ = nullptr;
  char name_[64]{};
};

class FS {
 public:
  File open(const char* path, const char* mode = "r") {
    if (!path) return File();
    char fullPath[256];
    snprintf(fullPath, sizeof(fullPath), "saves%s",
             path[0] == '/' ? path : path);
    const char* fmode = "r";
    if (mode && mode[0] == 'w') fmode = "wb";
    else if (mode && mode[0] == 'r') fmode = "rb";
    FILE* f = fopen(fullPath, fmode);
    if (!f) return File();
    return File(f, path);
  }

  bool remove(const char* path) {
    if (!path) return false;
    char fullPath[256];
    snprintf(fullPath, sizeof(fullPath), "saves%s",
             path[0] == '/' ? path : path);
    return ::remove(fullPath) == 0;
  }
};
