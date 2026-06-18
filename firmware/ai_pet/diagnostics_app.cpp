#include "diagnostics_app.h"

namespace {

constexpr uint32_t kPageMs = 1200;

}  // namespace

void DiagnosticsApp::begin() {
  Serial.begin(board::kUsbBaud);
  const uint32_t waitStarted = millis();
  while (!Serial && millis() - waitStarted < 2000) {
    delay(10);
  }

  Serial.println("DIAG boot");
  display_.begin();
  buttons_.begin();

  flashResult_ = flashProbe_.run();
  Serial.printf(
      "FLASH mounted=%d wrote=%d verified=%d cleaned=%d\n",
      flashResult_.mounted,
      flashResult_.wrote,
      flashResult_.verified,
      flashResult_.cleaned);

  showPage(page_);
  pageStartedAt_ = millis();
}

void DiagnosticsApp::update(uint32_t now) {
  processSerial();
  buttons_.update(now);
  printButtonEvents();

  if (page_ != Page::Keys && now - pageStartedAt_ >= kPageMs) {
    page_ = static_cast<Page>(static_cast<uint8_t>(page_) + 1);
    pageStartedAt_ = now;
    showPage(page_);
    if (page_ == Page::Keys) {
      printReady();
    }
  }

  if (page_ == Page::Keys && now - lastKeyDrawAt_ >= 50) {
    lastKeyDrawAt_ = now;
    Adafruit_ST7735& tft = display_.raw();
    tft.fillRect(0, 24, board::kScreenWidth, 100, ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);
    for (size_t i = 0; i < board::kButtonCount; ++i) {
      tft.setCursor(8, 28 + static_cast<int>(i) * 22);
      tft.printf(
          "K%d: %s",
          static_cast<int>(i + 1),
          buttons_.state(i).pressed ? "DOWN" : "UP");
    }
  }
}

void DiagnosticsApp::processSerial() {
  while (Serial.available()) {
    const char next = static_cast<char>(Serial.read());
    if (next == '\r') {
      continue;
    }
    if (next == '\n') {
      serialCommand_.trim();
      if (serialCommand_ == "REPORT") {
        printReport();
      }
      serialCommand_ = "";
      continue;
    }
    if (serialCommand_.length() < 32) {
      serialCommand_ += next;
    }
  }
}

void DiagnosticsApp::printReport() {
  Serial.println("DIAG boot");
  Serial.printf(
      "FLASH mounted=%d wrote=%d verified=%d cleaned=%d\n",
      flashResult_.mounted,
      flashResult_.wrote,
      flashResult_.verified,
      flashResult_.cleaned);
  Serial.println("DIAG ready");
}

void DiagnosticsApp::showPage(Page page) {
  switch (page) {
    case Page::Red:
      display_.drawSolid(ST77XX_RED, "RED");
      break;
    case Page::Green:
      display_.drawSolid(ST77XX_GREEN, "GREEN");
      break;
    case Page::Blue:
      display_.drawSolid(ST77XX_BLUE, "BLUE");
      break;
    case Page::White:
      display_.drawSolid(ST77XX_WHITE, "WHITE");
      break;
    case Page::Black:
      display_.drawSolid(ST77XX_BLACK, "BLACK");
      break;
    case Page::Bars:
      display_.drawColorBars();
      break;
    case Page::Grid:
      display_.drawGrid();
      break;
    case Page::Keys:
      display_.drawSolid(ST77XX_BLACK, "KEY TEST");
      break;
  }
}

void DiagnosticsApp::printButtonEvents() {
  for (size_t i = 0; i < board::kButtonCount; ++i) {
    const ButtonEvent event = buttons_.state(i).event;
    if (event == ButtonEvent::None) {
      continue;
    }

    const char* name = event == ButtonEvent::Pressed
                           ? "pressed"
                           : event == ButtonEvent::Released ? "released"
                                                            : "long";
    Serial.printf(
        "BUTTON k=%d event=%s raw=%d\n",
        static_cast<int>(i + 1),
        name,
        buttons_.state(i).rawLevel);
  }
}

void DiagnosticsApp::printReady() {
  Serial.println("DIAG ready");
}
