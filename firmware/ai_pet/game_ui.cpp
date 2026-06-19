#include "game_ui.h"

#include "assets/immortal_cave_home.h"

namespace {

constexpr int16_t kPetRegionX = 36;
constexpr int16_t kPetRegionY = 28;
constexpr int16_t kPetRegionWidth = 56;
constexpr int16_t kPetRegionHeight = 50;

}  // namespace

void GameUi::begin(DisplayDevice& display) {
  display_ = &display;
  chinese_.begin(display);
  menuChinese_.begin(menuCanvas_);
  dirty_ = true;
}

void GameUi::handle(InputAction action, GameState& state) {
  if (action == InputAction::None) {
    return;
  }

  if (page_ == UiPage::Home) {
    if (action == InputAction::Confirm) {
      const uint8_t oldMood = state.data().mood;
      state.interact();
      startFeedback(state.data().mood > oldMood ? Feedback::MoodUp
                                                : Feedback::AlreadyFull);
    } else if (action == InputAction::Up) {
      page_ = UiPage::Care;
    } else if (action == InputAction::Down) {
      page_ = UiPage::Adventure;
    } else if (action == InputAction::Back) {
      page_ = UiPage::Status;
    }
  } else if (action == InputAction::Back && page_ != UiPage::Battle) {
    page_ = UiPage::Home;
  } else if (page_ == UiPage::Care) {
    if (action == InputAction::Up || action == InputAction::Down) {
      selection_ = 1 - selection_;
    } else if (action == InputAction::Confirm) {
      if (selection_ == 0) {
        const uint16_t oldCoins = state.data().coins;
        const uint8_t oldStamina = state.data().stamina;
        if (state.feed()) {
          startFeedback(Feedback::StaminaUp);
        } else if (oldCoins < 10) {
          startFeedback(Feedback::NoCoins);
        } else if (oldStamina >= 100) {
          startFeedback(Feedback::AlreadyFull);
        }
      } else {
        const uint8_t oldMood = state.data().mood;
        state.interact();
        startFeedback(state.data().mood > oldMood ? Feedback::MoodUp
                                                  : Feedback::AlreadyFull);
      }
    }
  } else if (page_ == UiPage::Adventure) {
    if (action == InputAction::Up) {
      selection_ = selection_ == 0 ? 2 : selection_ - 1;
    } else if (action == InputAction::Down) {
      selection_ = (selection_ + 1) % 3;
    } else if (action == InputAction::Confirm) {
      if (state.data().regionProgress[selection_] >= 100 &&
          !(state.data().bossDefeatedMask & (1U << selection_))) {
        if (state.startBoss(selection_)) {
          page_ = UiPage::Battle;
        }
      } else {
        state.startExploration(selection_);
      }
    }
  } else if (page_ == UiPage::Battle) {
    uint8_t battleAction = 0;
    if (action == InputAction::Confirm) {
      battleAction = 0;
    } else if (action == InputAction::Up) {
      battleAction = 1;
    } else if (action == InputAction::Down) {
      battleAction = 2;
    } else {
      battleAction = 3;
    }
    state.battleAction(battleAction);
    if (!state.data().inBattle) {
      page_ = UiPage::Adventure;
    }
  }
  dirty_ = true;
}

void GameUi::draw(const GameState& state, uint32_t now, bool force) {
  if (!display_) {
    return;
  }
  const bool fullRedraw = (dirty_ || force) && page_ == UiPage::Home;
  const bool menuRedraw = (dirty_ || force) && page_ != UiPage::Home;
  const bool animate = page_ == UiPage::Home && now - lastAnimationAt_ >= 600;
  const bool feedbackActive =
      feedback_ != Feedback::None && now - feedbackStartedAt_ < 1200;
  const bool feedbackFrame =
      page_ == UiPage::Home && feedbackActive &&
      now - lastFeedbackFrameAt_ >= 80;
  const bool feedbackExpired =
      feedback_ != Feedback::None && !feedbackActive;
  if (!fullRedraw && !menuRedraw && !animate && !feedbackFrame &&
      !feedbackExpired) {
    return;
  }
  if (animate) {
    lastAnimationAt_ = now;
  }
  if (feedbackFrame) {
    lastFeedbackFrameAt_ = now;
  }

  if (menuRedraw) {
    drawMenuFrame(state.data());
  } else if (fullRedraw) {
    drawInkBackground();
    drawHeader(state.data());
    drawHome(state.data(), now);
  } else if (page_ == UiPage::Home) {
    if (feedbackExpired) {
      restoreBackgroundRect(14, 84, 100, 24);
    }
    drawHomePet(state.data(), now);
  }

  if (feedbackActive) {
    if (fullRedraw || menuRedraw || feedbackFrame) {
      drawFeedback(now);
    }
  } else if (feedback_ != Feedback::None) {
    feedback_ = Feedback::None;
    if (page_ != UiPage::Home) {
      dirty_ = true;
      draw(state, now, false);
      return;
    }
  }
  dirty_ = false;
}

UiPage GameUi::page() const {
  return page_;
}

void GameUi::drawHeader(const PetSaveData& data) {
  Adafruit_GFX& tft = target();
  tft.fillRect(0, 0, 128, 18, 0x18E3);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(4, 5);
  tft.printf("LV%u  XP%u", data.level, data.experience);
  tft.setCursor(104, 5);
  tft.print("USB");
}

void GameUi::drawInkBackground() {
  Adafruit_GFX& tft = target();
  tft.drawRGBBitmap(0, 0, kImmortalCaveHome, kImmortalCaveHomeWidth,
                    kImmortalCaveHomeHeight);
  tft.fillRect(0, 112, 128, 48, 0x0861);
  tft.drawFastHLine(0, 112, 128, 0x6B8D);
}

void GameUi::drawMenuFrame(const PetSaveData& data) {
  uint16_t* frame = menuCanvas_.getBuffer();
  if (!frame) {
    return;
  }

  renderTarget_ = &menuCanvas_;
  renderText_ = &menuChinese_;
  drawInkBackground();
  drawHeader(data);
  switch (page_) {
    case UiPage::Care:
      drawCare(data);
      break;
    case UiPage::Adventure:
      drawAdventure(data);
      break;
    case UiPage::Status:
      drawStatus(data);
      break;
    case UiPage::Battle:
      drawBattle(data);
      break;
    case UiPage::Home:
      break;
  }
  renderTarget_ = nullptr;
  renderText_ = nullptr;

  Adafruit_ST7735& tft = display_->raw();
  tft.drawRGBBitmap(0, 0, menuCanvas_.getBuffer(), 128, 160);
}

void GameUi::drawHome(const PetSaveData& data, uint32_t now) {
  drawHomePet(data, now);
  drawHomeStats(data);
}

void GameUi::drawHomePet(const PetSaveData& data, uint32_t now) {
  Adafruit_ST7735& tft = display_->raw();
  uint16_t* frame = petCanvas_.getBuffer();
  if (!frame) {
    return;
  }
  for (int16_t row = 0; row < kPetRegionHeight; ++row) {
    for (int16_t column = 0; column < kPetRegionWidth; ++column) {
      frame[row * kPetRegionWidth + column] =
          pgm_read_word(kImmortalCaveHome +
                        (kPetRegionY + row) * kImmortalCaveHomeWidth +
                        kPetRegionX + column);
    }
  }

  int16_t petY = 38;
  if ((feedback_ == Feedback::MoodUp ||
       feedback_ == Feedback::StaminaUp) &&
      now - feedbackStartedAt_ < 700) {
    const uint32_t phase = (now - feedbackStartedAt_) % 350;
    petY -= phase < 175 ? phase / 25 : (350 - phase) / 25;
  }
  pet_.draw(petCanvas_, data.form, 44 - kPetRegionX, petY - kPetRegionY, now);
  tft.drawRGBBitmap(kPetRegionX, kPetRegionY, petCanvas_.getBuffer(),
                    kPetRegionWidth, kPetRegionHeight);
}

void GameUi::drawHomeStats(const PetSaveData& data) {
  Adafruit_GFX& tft = target();
  text().color(0xF7BE);
  text().draw(5, 124, "心境");
  text().draw(67, 124, "体力");
  drawBar(5, 128, data.mood, 0xF9B2);
  drawBar(67, 128, data.stamina, 0x5EAA);
  text().draw(5, 148, "灵力");
  text().draw(67, 148, "灵石");
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(35, 141);
  tft.print(data.energy);
  tft.setCursor(97, 141);
  tft.print(data.coins);
}

void GameUi::restoreBackgroundRect(int16_t x, int16_t y, int16_t width,
                                   int16_t height) {
  Adafruit_ST7735& tft = display_->raw();
  for (int16_t row = 0; row < height; ++row) {
    const uint16_t* source =
        kImmortalCaveHome + (y + row) * kImmortalCaveHomeWidth + x;
    tft.drawRGBBitmap(x, y + row, source, width, 1);
  }
}

void GameUi::drawCare(const PetSaveData& data) {
  Adafruit_GFX& tft = target();
  text().color(0x7FFF);
  text().draw(8, 38, "培养");
  const char* items[] = {"喂食  十灵石", "互动"};
  for (uint8_t i = 0; i < 2; ++i) {
    text().color(i == selection_ ? 0xFFE0 : ST77XX_WHITE);
    text().draw(20, 70 + i * 24, items[i]);
    if (i == selection_) {
      tft.fillTriangle(8, 62 + i * 24, 14, 66 + i * 24,
                       8, 70 + i * 24, 0xFFE0);
    }
  }
  text().color(ST77XX_WHITE);
  text().draw(12, 126, "灵石");
  text().draw(66, 126, "体力");
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(42, 119);
  tft.print(data.coins);
  tft.setCursor(96, 119);
  tft.print(data.stamina);
  text().draw(12, 151, "返回");
}

void GameUi::drawAdventure(const PetSaveData& data) {
  Adafruit_GFX& tft = target();
  text().color(0x7FEF);
  text().draw(8, 38, "历练");
  const char* zones[] = {"青竹灵境", "云海剑台", "玄岳古域"};
  for (uint8_t i = 0; i < 3; ++i) {
    const bool unlocked =
        i == 0 || (data.bossDefeatedMask & (1U << (i - 1)));
    text().color(!unlocked ? 0x7BEF
                           : i == selection_ ? ST77XX_YELLOW
                                             : ST77XX_WHITE);
    text().draw(18, 66 + i * 22, unlocked ? zones[i] : "尚未解锁");
    tft.setTextColor(i == selection_ ? ST77XX_YELLOW : ST77XX_WHITE);
    tft.setTextSize(1);
    tft.setCursor(92, 59 + i * 22);
    tft.printf("%u%%", data.regionProgress[i]);
    if (i == selection_) {
      tft.fillTriangle(7, 58 + i * 22, 13, 62 + i * 22,
                       7, 66 + i * 22, ST77XX_YELLOW);
    }
  }
  text().color(ST77XX_WHITE);
  text().draw(10, 137, "灵力");
  text().draw(76, 151, "返回");
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(42, 130);
  tft.print(data.energy);
}

void GameUi::drawBattle(const PetSaveData& data) {
  Adafruit_GFX& tft = target();
  text().color(0xFFE0);
  text().draw(8, 36, "秘境首领");
  text().color(ST77XX_WHITE);
  text().draw(8, 58, "敌方气血");
  tft.drawRect(70, 50, 50, 7, ST77XX_WHITE);
  tft.fillRect(71, 51, 48, 5, ST77XX_BLACK);
  if (data.bossMaxHp) {
    tft.fillRect(71, 51, data.bossHp * 48 / data.bossMaxHp, 5, ST77XX_RED);
  }
  text().draw(8, 82, "一键 攻击");
  text().draw(8, 102, "二键 法诀");
  text().draw(8, 122, "三键 丹药");
  text().draw(8, 142, "四键 防御");
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(87, 72);
  tft.printf("HP %u", data.stamina);
}

void GameUi::drawStatus(const PetSaveData& data) {
  Adafruit_GFX& tft = target();
  text().color(ST77XX_YELLOW);
  text().draw(8, 38, "状态");
  text().color(ST77XX_WHITE);
  const char* forms[] = {"混沌灵卵", "凌霄麒麟", "镇岳麒麟", "太虚剑仙",
                         "九转丹仙", "不灭武仙", "万灵仙尊"};
  text().draw(10, 65, "形态");
  text().draw(48, 65, forms[static_cast<unsigned>(data.form)]);
  text().draw(10, 88, "修炼");
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(48, 81);
  tft.printf("%lus", static_cast<unsigned long>(data.playSeconds));
  text().draw(10, 112, "剑  丹  体  灵");
  tft.setCursor(15, 120);
  tft.printf("%u  %u  %u  %u", data.tendencies[0], data.tendencies[1],
             data.tendencies[2], data.tendencies[3]);
  text().draw(76, 151, "返回");
}

void GameUi::drawBar(int16_t x, int16_t y, uint8_t value, uint16_t color) {
  Adafruit_GFX& tft = target();
  const int16_t width = 56;
  tft.drawRect(x, y, width, 7, ST77XX_WHITE);
  tft.fillRect(x + 1, y + 1, width - 2, 5, ST77XX_BLACK);
  tft.fillRect(x + 1, y + 1, value * (width - 2) / 100, 5, color);
}

void GameUi::startFeedback(Feedback feedback) {
  feedback_ = feedback;
  feedbackStartedAt_ = millis();
  dirty_ = true;
}

void GameUi::drawFeedback(uint32_t now) {
  Adafruit_ST7735& tft = display_->raw();
  const uint32_t elapsed = now - feedbackStartedAt_;
  const int16_t rise = min<uint32_t>(10, elapsed / 80);

  if (feedback_ == Feedback::MoodUp || feedback_ == Feedback::StaminaUp) {
    const int16_t centerX = 64;
    const int16_t centerY = 43 - rise;
    for (uint8_t i = 0; i < 6; ++i) {
      const int16_t x = centerX - 24 + i * 9;
      const int16_t y = centerY + ((i * 7 + elapsed / 100) % 13);
      tft.drawPixel(x, y, 0xFFE0);
      tft.drawFastHLine(x - 1, y, 3, 0xFFE0);
      tft.drawFastVLine(x, y - 1, 3, 0xFFE0);
    }
  }

  tft.fillRoundRect(14, 84, 100, 24, 4, 0x18C3);
  tft.drawRoundRect(14, 84, 100, 24, 4, 0xD5EA);
  chinese_.color(feedback_ == Feedback::NoCoins ? 0xFB08 : 0xFFE0);
  switch (feedback_) {
    case Feedback::MoodUp:
      chinese_.draw(34, 101, "心境提升");
      break;
    case Feedback::StaminaUp:
      chinese_.draw(34, 101, "体力恢复");
      break;
    case Feedback::NoCoins:
      chinese_.draw(34, 101, "灵石不足");
      break;
    case Feedback::AlreadyFull:
      chinese_.draw(34, 101, "当前已满");
      break;
    case Feedback::None:
      break;
  }
}

Adafruit_GFX& GameUi::target() {
  return renderTarget_ ? *renderTarget_ : display_->raw();
}

ChineseText& GameUi::text() {
  return renderText_ ? *renderText_ : chinese_;
}
