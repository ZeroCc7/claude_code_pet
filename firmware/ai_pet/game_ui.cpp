#include "game_ui.h"

void GameUi::begin(DisplayDevice& display) {
  display_ = &display;
  dirty_ = true;
}

void GameUi::handle(InputAction action, GameState& state) {
  if (action == InputAction::None) {
    return;
  }

  if (page_ == UiPage::Home) {
    if (action == InputAction::Confirm) {
      state.interact();
    } else if (action == InputAction::Up) {
      page_ = UiPage::Care;
    } else if (action == InputAction::Down) {
      page_ = UiPage::Adventure;
    } else if (action == InputAction::Back) {
      page_ = UiPage::Status;
    }
  } else if (action == InputAction::Back) {
    page_ = UiPage::Home;
  } else if (page_ == UiPage::Care) {
    if (action == InputAction::Up || action == InputAction::Down) {
      selection_ = 1 - selection_;
    } else if (action == InputAction::Confirm) {
      if (selection_ == 0) {
        state.feed();
      } else {
        state.interact();
      }
    }
  } else if (page_ == UiPage::Adventure) {
    if (action == InputAction::Up) {
      selection_ = selection_ == 0 ? 2 : selection_ - 1;
    } else if (action == InputAction::Down) {
      selection_ = (selection_ + 1) % 3;
    } else if (action == InputAction::Confirm) {
      state.startExploration(selection_);
    }
  }
  dirty_ = true;
}

void GameUi::draw(const GameState& state, uint32_t now, bool force) {
  if (!display_) {
    return;
  }
  const bool animate = page_ == UiPage::Home && now - lastAnimationAt_ >= 600;
  if (!dirty_ && !force && !animate) {
    return;
  }
  if (animate) {
    lastAnimationAt_ = now;
  }

  Adafruit_ST7735& tft = display_->raw();
  if (dirty_ || force) {
    tft.fillScreen(ST77XX_BLACK);
    drawHeader(state.data());
  }

  switch (page_) {
    case UiPage::Home:
      drawHome(state.data(), now);
      break;
    case UiPage::Care:
      drawCare(state.data());
      break;
    case UiPage::Adventure:
      drawAdventure(state.data());
      break;
    case UiPage::Status:
      drawStatus(state.data());
      break;
    case UiPage::Battle:
      break;
  }
  dirty_ = false;
}

UiPage GameUi::page() const {
  return page_;
}

void GameUi::drawHeader(const PetSaveData& data) {
  Adafruit_ST7735& tft = display_->raw();
  tft.fillRect(0, 0, 128, 18, 0x18E3);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(4, 5);
  tft.printf("LV%u  XP%u", data.level, data.experience);
  tft.setCursor(104, 5);
  tft.print("USB");
}

void GameUi::drawHome(const PetSaveData& data, uint32_t now) {
  Adafruit_ST7735& tft = display_->raw();
  pet_.draw(tft, data.form, 44, 38, now);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(5, 91);
  tft.printf("Mood %u", data.mood);
  drawBar(5, 102, data.mood, ST77XX_MAGENTA);
  tft.setCursor(5, 116);
  tft.printf("Stamina %u", data.stamina);
  drawBar(5, 127, data.stamina, ST77XX_GREEN);
  tft.setCursor(5, 141);
  tft.printf("Energy %u  Coin %u", data.energy, data.coins);
}

void GameUi::drawCare(const PetSaveData& data) {
  Adafruit_ST7735& tft = display_->raw();
  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(2);
  tft.setCursor(8, 28);
  tft.print("CARE");
  const char* items[] = {"Feed -10", "Interact"};
  for (uint8_t i = 0; i < 2; ++i) {
    tft.setTextColor(i == selection_ ? ST77XX_YELLOW : ST77XX_WHITE);
    tft.setTextSize(1);
    tft.setCursor(12, 62 + i * 24);
    tft.printf("%c %s", i == selection_ ? '>' : ' ', items[i]);
  }
  tft.setCursor(12, 120);
  tft.setTextColor(ST77XX_WHITE);
  tft.printf("Coin %u  STA %u", data.coins, data.stamina);
  tft.setCursor(12, 145);
  tft.print("K4 Back");
}

void GameUi::drawAdventure(const PetSaveData& data) {
  Adafruit_ST7735& tft = display_->raw();
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(2);
  tft.setCursor(8, 26);
  tft.print("ADVENTURE");
  for (uint8_t i = 0; i < 3; ++i) {
    tft.setTextColor(i == selection_ ? ST77XX_YELLOW : ST77XX_WHITE);
    tft.setTextSize(1);
    tft.setCursor(10, 62 + i * 22);
    tft.printf("%c Zone %u  %u%%", i == selection_ ? '>' : ' ', i + 1,
               data.regionProgress[i]);
  }
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, 135);
  tft.printf("Energy %u", data.energy);
  tft.setCursor(10, 149);
  tft.print("K4 Back");
}

void GameUi::drawStatus(const PetSaveData& data) {
  Adafruit_ST7735& tft = display_->raw();
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(8, 26);
  tft.print("STATUS");
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(10, 60);
  tft.printf("Form %u", static_cast<unsigned>(data.form));
  tft.setCursor(10, 78);
  tft.printf("Play %lus", static_cast<unsigned long>(data.playSeconds));
  tft.setCursor(10, 100);
  tft.printf("T %u %u %u %u", data.tendencies[0], data.tendencies[1],
             data.tendencies[2], data.tendencies[3]);
  tft.setCursor(10, 149);
  tft.print("K4 Back");
}

void GameUi::drawBar(int16_t x, int16_t y, uint8_t value, uint16_t color) {
  Adafruit_ST7735& tft = display_->raw();
  tft.drawRect(x, y, 118, 7, ST77XX_WHITE);
  tft.fillRect(x + 1, y + 1, 116, 5, ST77XX_BLACK);
  tft.fillRect(x + 1, y + 1, value * 116 / 100, 5, color);
}

