#pragma once

#include "button_scanner.h"
#include "display_device.h"
#include "flash_probe.h"

class DiagnosticsApp {
 public:
  void begin();
  void update(uint32_t now);

 private:
  enum class Page : uint8_t {
    Red,
    Green,
    Blue,
    White,
    Black,
    Bars,
    Grid,
    Keys,
  };

  void showPage(Page page);
  void printButtonEvents();
  void printReady();

  DisplayDevice display_;
  ButtonScanner buttons_;
  FlashProbe flashProbe_;
  Page page_ = Page::Red;
  uint32_t pageStartedAt_ = 0;
  uint32_t lastKeyDrawAt_ = 0;
};

