#include "game_ui.h"

#include "assets/cloud_terrace_home.h"
#include "assets/home_button_icons.h"
#include "assets/home_ui_icons.h"

#include <cstring>

namespace {

constexpr int16_t kPetRegionX = 28;
constexpr int16_t kPetRegionY = 14;
constexpr int16_t kPetRegionWidth = 72;
constexpr int16_t kPetRegionHeight = 100;
constexpr uint16_t kInkBlack = 0x0861;
constexpr uint16_t kInkBlue = 0x10C3;
constexpr uint16_t kPanelBlue = 0x1924;
constexpr uint16_t kPanelSelected = 0x2145;
constexpr uint16_t kDarkGold = 0x9BC6;
constexpr uint16_t kBrightGold = 0xFEC8;
constexpr uint16_t kWarmWhite = 0xEF5D;
constexpr uint16_t kMutedCyan = 0x7CEF;
constexpr uint16_t kCinnabar = 0xD9E4;

uint8_t utf8GlyphCount(const char* value) {
  uint8_t count = 0;
  while (*value) {
    if ((static_cast<uint8_t>(*value) & 0xC0) != 0x80) {
      ++count;
    }
    ++value;
  }
  return count;
}

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
    if (action == InputAction::Up) {
      selection_ = selection_ == 0 ? 2 : selection_ - 1;
    } else if (action == InputAction::Down) {
      selection_ = (selection_ + 1) % 3;
    } else if (action == InputAction::Confirm) {
      if (selection_ == 0) {
        const uint16_t oldCoins = state.data().coins;
        const uint8_t oldStamina = state.data().stamina;
        if (state.feed()) {
          startNotice("体力恢复");
        } else if (oldCoins < 10) {
          startNotice("灵石不足");
        } else if (oldStamina >= 100) {
          startNotice("当前已满");
        }
      } else if (selection_ == 1) {
        const uint8_t oldMood = state.data().mood;
        state.interact();
        startNotice(state.data().mood > oldMood ? "心境提升" : "当前已满");
      } else {
        switch (state.meditate()) {
          case MeditationResult::Restored:
            startNotice("灵力恢复");
            break;
          case MeditationResult::Full:
            startNotice("灵力已满");
            break;
          case MeditationResult::Exhausted:
            startNotice("今日已尽");
            break;
        }
      }
    }
  } else if (page_ == UiPage::Adventure) {
    if (action == InputAction::Up) {
      selection_ = selection_ == 0 ? 2 : selection_ - 1;
    } else if (action == InputAction::Down) {
      selection_ = (selection_ + 1) % 3;
    } else if (action == InputAction::Confirm) {
      if (state.data().regionProgress[selection_] >= 100) {
        if (state.startBoss(selection_)) {
          page_ = UiPage::Battle;
          startNotice("首领现身");
        }
      } else {
        if (state.startExploration(selection_)) {
          startNotice("历练已启");
        } else if (state.data().energy < 3) {
          startNotice("灵力不足");
        } else {
          startNotice("秘境未启");
        }
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
    const uint8_t oldBossHp = state.data().bossHp;
    const uint8_t oldStamina = state.data().stamina;
    const uint16_t oldEnergy = state.data().energy;
    const uint16_t oldCoins = state.data().coins;
    state.battleAction(battleAction);
    if (!state.data().inBattle) {
      page_ = UiPage::Adventure;
      startNotice(state.data().bossHp == 0 ? "首领已伏" : "败退洞府");
    } else if (state.data().stamina > oldStamina) {
      startNotice("气血恢复");
    } else if (battleAction == 3) {
      startNotice("防御架势");
    } else if (state.data().bossHp < oldBossHp) {
      startNotice("造成伤害");
    } else if (battleAction == 1 && oldEnergy < 2) {
      startNotice("灵力不足");
    } else if (battleAction == 2 && oldCoins < 5) {
      startNotice("灵石不足");
    }
  }
  dirty_ = true;
}

void GameUi::draw(const GameState& state, uint32_t now, bool force) {
  if (!display_) {
    return;
  }
  if (page_ == UiPage::Cultivation &&
      now - aiLastEventAt_ >= 600000) {
    page_ = UiPage::Home;
    aiResultActive_ = false;
    dirty_ = true;
  }
  if (page_ == UiPage::Cultivation && aiResultActive_ &&
      now - aiLastEventAt_ >= 2500) {
    page_ = UiPage::Home;
    aiResultActive_ = false;
    dirty_ = true;
  }

  const bool fullRedraw = (dirty_ || force) && page_ == UiPage::Home;
  const bool menuRedraw = (dirty_ || force) && page_ != UiPage::Home;
  const bool animate =
      ((page_ == UiPage::Home && now - lastAnimationAt_ >= 400) ||
       (page_ == UiPage::Cultivation &&
        now - lastAnimationAt_ >= 300));
  const bool feedbackActive =
      feedback_ != Feedback::None && now - feedbackStartedAt_ < 1200;
  const bool feedbackFrame =
      page_ == UiPage::Home && feedbackActive &&
      now - lastFeedbackFrameAt_ >= 80;
  const bool feedbackExpired =
      feedback_ != Feedback::None && !feedbackActive;
  const bool noticeExpired =
      notice_[0] != '\0' && now - noticeStartedAt_ >= 1200;
  if (noticeExpired) {
    notice_[0] = '\0';
  }
  const bool menuNoticeRefresh = noticeExpired && page_ != UiPage::Home;
  const bool redrawMenu =
      menuRedraw || menuNoticeRefresh ||
      (animate && page_ == UiPage::Cultivation);
  if (!fullRedraw && !redrawMenu && !animate && !feedbackFrame &&
      !feedbackExpired) {
    return;
  }
  if (animate) {
    lastAnimationAt_ = now;
  }
  if (feedbackFrame) {
    lastFeedbackFrameAt_ = now;
  }

  if (redrawMenu) {
    drawMenuFrame(state.data());
  } else if (fullRedraw) {
    drawInkBackground(128);
    drawHomeHeader(state.data());
    drawHome(state.data(), now);
  } else if (page_ == UiPage::Home) {
    if (feedbackExpired) {
      restoreBackgroundRect(14, 52, 100, 24);
    }
    drawHomePet(state.data(), now);
  }

  if (feedbackActive) {
    if (fullRedraw || redrawMenu || feedbackFrame) {
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

void GameUi::notify(const char* message) {
  startNotice(message);
}

void GameUi::showAiStatus(const char* source, AiWorkState state,
                          const char* taskId, uint32_t now) {
  if (strncmp(aiTaskId_, taskId, sizeof(aiTaskId_)) != 0) {
    strncpy(aiTaskId_, taskId, sizeof(aiTaskId_) - 1);
    aiTaskId_[sizeof(aiTaskId_) - 1] = '\0';
    aiTaskStartedAt_ = now;
  }
  strncpy(aiSource_, source, sizeof(aiSource_) - 1);
  aiSource_[sizeof(aiSource_) - 1] = '\0';
  aiState_ = state;
  aiLastEventAt_ = now;
  aiResultActive_ = false;
  page_ = state == AiWorkState::Idle ? UiPage::Home
                                     : UiPage::Cultivation;
  dirty_ = true;
}

void GameUi::showAiResult(const char* source, bool success,
                          uint16_t experienceGain, uint16_t coinGain,
                          bool evolved, uint32_t now) {
  strncpy(aiSource_, source, sizeof(aiSource_) - 1);
  aiSource_[sizeof(aiSource_) - 1] = '\0';
  aiResultActive_ = true;
  aiResultSuccess_ = success;
  aiExperienceGain_ = experienceGain;
  aiCoinGain_ = coinGain;
  aiEvolved_ = evolved;
  aiLastEventAt_ = now;
  page_ = UiPage::Cultivation;
  dirty_ = true;
}

void GameUi::showEvolution(PetForm form, uint32_t now) {
  (void)form;
  strncpy(aiSource_, "EVOLUTION", sizeof(aiSource_) - 1);
  aiSource_[sizeof(aiSource_) - 1] = '\0';
  aiResultActive_ = true;
  aiResultSuccess_ = true;
  aiExperienceGain_ = 0;
  aiCoinGain_ = 0;
  aiEvolved_ = true;
  aiLastEventAt_ = now;
  page_ = UiPage::Cultivation;
  dirty_ = true;
}

void GameUi::drawHeader(const PetSaveData& data) {
  Adafruit_GFX& tft = target();
  tft.fillRect(0, 0, 128, 18, 0x18E3);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(4, 5);
  tft.printf("LV%u  XP%u", data.level, data.experience);
}

void GameUi::drawHomeHeader(const PetSaveData& data) {
  Adafruit_GFX& tft = target();
  tft.fillRect(0, 0, 128, 14, kInkBlack);
  tft.drawRect(0, 0, 128, 14, kDarkGold);

  // Left column: realm + level (compact)
  text().color(kWarmWhite);
  text().draw(3, 11, "炼气");
  tft.setTextColor(kBrightGold);
  tft.setTextSize(1);
  tft.setCursor(28, 3);
  tft.printf("Lv%u", data.level);

  // Middle column: XP bar (compact diamond frame)
  const uint8_t currentExperience = data.experience % 20;
  tft.fillRect(52, 1, 42, 11, kInkBlue);
  tft.drawFastHLine(55, 0, 36, kBrightGold);
  tft.drawFastHLine(55, 12, 36, kDarkGold);
  tft.drawFastVLine(51, 4, 5, kDarkGold);
  tft.drawFastVLine(94, 4, 5, kDarkGold);
  tft.fillTriangle(51, 6, 55, 2, 55, 10, kDarkGold);
  tft.fillTriangle(95, 6, 91, 2, 91, 10, kDarkGold);
  const int16_t xpFill = currentExperience * 34 / 20;
  if (xpFill > 0) {
    tft.fillRect(56, 3, xpFill, 3, kBrightGold);
    tft.fillRect(56, 6, xpFill, 3, 0xDD45);
  }
  tft.setTextColor(kWarmWhite);
  tft.setCursor(66, 4);
  tft.printf("%u/20", currentExperience);

  // Right column: status icon (replace text with icon)
  if (aiState_ == AiWorkState::Idle) {
    // Offline/sleeping icon: small crescent moon
    tft.drawPixel(108, 2, kMutedCyan);
    tft.drawPixel(109, 3, kMutedCyan);
    tft.drawPixel(110, 4, kMutedCyan);
    tft.drawPixel(110, 5, kMutedCyan);
    tft.drawPixel(110, 6, kMutedCyan);
    tft.drawPixel(109, 7, kMutedCyan);
    tft.drawPixel(108, 8, kMutedCyan);
    tft.drawPixel(113, 2, kMutedCyan);
    tft.drawPixel(114, 3, kMutedCyan);
    tft.drawPixel(115, 4, kMutedCyan);
    tft.drawPixel(114, 5, kMutedCyan);
    tft.drawPixel(113, 6, kMutedCyan);
  } else {
    // Online/active icon: small lightning bolt
    tft.drawFastVLine(111, 1, 5, kBrightGold);
    tft.drawFastHLine(109, 6, 4, kBrightGold);
    tft.drawFastVLine(110, 7, 5, kBrightGold);
    tft.drawPixel(112, 5, kBrightGold);
    tft.drawPixel(109, 7, kBrightGold);
  }
}

void GameUi::drawInkBackground(int16_t fillStartY) {
  Adafruit_GFX& tft = target();
  tft.drawRGBBitmap(0, 0, kCloudTerraceHome, kCloudTerraceHomeWidth,
                    kCloudTerraceHomeHeight);
  tft.fillRect(0, fillStartY, 128, 160 - fillStartY, 0x0861);
  tft.drawFastHLine(0, fillStartY, 128, 0x6B8D);
}

void GameUi::drawMenuFrame(const PetSaveData& data) {
  uint16_t* frame = menuCanvas_.getBuffer();
  if (!frame) {
    return;
  }

  renderTarget_ = &menuCanvas_;
  renderText_ = &menuChinese_;
  drawInkBackground();
  target().fillRect(0, 16, 128, 144, kInkBlack);
  for (int16_t y = 18; y < 142; y += 12) {
    target().drawPixel(3, y, kInkBlue);
    target().drawPixel(124, y + 5, kInkBlue);
  }
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
    case UiPage::Cultivation:
      drawCultivation(data, millis());
      break;
    case UiPage::Home:
      break;
  }
  if (notice_[0] != '\0') {
    drawNotice();
  }
  renderTarget_ = nullptr;
  renderText_ = nullptr;

  Adafruit_ST7735& tft = display_->raw();
  tft.drawRGBBitmap(0, 0, menuCanvas_.getBuffer(), 128, 160);
}

void GameUi::drawCultivation(const PetSaveData& data, uint32_t now) {
  Adafruit_GFX& tft = target();
  drawTitlePlaque(aiResultActive_ ? "修炼结算" : "仙途修炼",
                  aiResultSuccess_ || !aiResultActive_ ? kBrightGold
                                                      : kCinnabar);
  drawPanel(9, 47, 110, 87, false);

  tft.setTextSize(1);
  tft.setTextColor(kMutedCyan);
  tft.setCursor(15, 53);
  tft.print(aiSource_);
  tft.setTextColor(kWarmWhite);
  tft.setCursor(87, 53);
  tft.printf("LV%u", data.level);

  if (aiResultActive_) {
    text().color(aiResultSuccess_ ? kBrightGold : kCinnabar);
    text().draw(34, 76,
                aiResultSuccess_ ? "修炼完成" : "修炼受阻");
    if (aiResultSuccess_) {
      if (aiEvolved_) {
        const char* forms[] = {"混沌灵卵", "凌霄麒麟", "镇岳麒麟",
                               "太虚剑仙", "九转丹仙", "不灭武仙",
                               "万灵仙尊"};
        text().color(0x7DFF);
        text().draw(34, 91, "灵光进化");
        text().color(kWarmWhite);
        const char* form = forms[static_cast<unsigned>(data.form)];
        const int16_t formWidth = utf8GlyphCount(form) * 12;
        text().draw(max<int16_t>(12, (128 - formWidth) / 2),
                    114, form);
      } else {
        tft.setTextColor(kWarmWhite);
        tft.setCursor(25, 88);
        tft.printf("EXP +%u", aiExperienceGain_);
        tft.setCursor(25, 101);
        tft.printf("STONE +%u", aiCoinGain_);
      }
    }
  } else {
    const char* label = "修炼暂歇";
    switch (aiState_) {
      case AiWorkState::Submitted: label = "接引任务"; break;
      case AiWorkState::Thinking: label = "推演中"; break;
      case AiWorkState::Tool: label = "施法中"; break;
      case AiWorkState::Editing: label = "炼器中"; break;
      case AiWorkState::Waiting: label = "等待指令"; break;
      case AiWorkState::Blocked: label = "修炼受阻"; break;
      case AiWorkState::Idle: label = "修炼暂歇"; break;
    }
    text().color(aiState_ == AiWorkState::Blocked ? kCinnabar
                                                  : kBrightGold);
    const int16_t labelWidth = utf8GlyphCount(label) * 12;
    text().draw(max<int16_t>(15, (128 - labelWidth) / 2), 76, label);

    const uint32_t elapsedSeconds =
        aiTaskStartedAt_ == 0 ? 0 : (now - aiTaskStartedAt_) / 1000;
    tft.setTextColor(kWarmWhite);
    tft.setCursor(41, 88);
    tft.printf("%02lu:%02lu",
               static_cast<unsigned long>(elapsedSeconds / 60),
               static_cast<unsigned long>(elapsedSeconds % 60));

    const int16_t centerX = 64;
    const int16_t centerY = 112;
    const uint8_t phase = (now / 300) % 8;
    for (uint8_t i = 0; i < 8; ++i) {
      const uint16_t color =
          i == phase ? kBrightGold : kDarkGold;
      const int16_t dx[] = {0, 9, 13, 9, 0, -9, -13, -9};
      const int16_t dy[] = {-13, -9, 0, 9, 13, 9, 0, -9};
      tft.fillCircle(centerX + dx[i], centerY + dy[i], 2, color);
    }
  }
  drawFooterHints("AI修炼联动", "K4返回");
}

void GameUi::drawTitlePlaque(const char* title, uint16_t accent) {
  Adafruit_GFX& tft = target();
  tft.fillRoundRect(14, 22, 100, 20, 4, kPanelBlue);
  tft.drawRoundRect(14, 22, 100, 20, 4, kDarkGold);
  tft.drawFastHLine(23, 25, 82, accent);
  tft.fillTriangle(8, 32, 14, 27, 14, 37, kDarkGold);
  tft.fillTriangle(120, 32, 114, 27, 114, 37, kDarkGold);
  text().color(accent);
  const int16_t width = utf8GlyphCount(title) * 12;
  text().draw(max<int16_t>(18, (128 - width) / 2), 37, title);
}

void GameUi::drawPanel(int16_t x, int16_t y, int16_t width, int16_t height,
                       bool selected) {
  Adafruit_GFX& tft = target();
  tft.fillRoundRect(x, y, width, height, 3,
                    selected ? kPanelSelected : kPanelBlue);
  tft.drawRoundRect(x, y, width, height, 3,
                    selected ? kBrightGold : kDarkGold);
  if (selected) {
    tft.drawRoundRect(x + 2, y + 2, width - 4, height - 4, 2, kDarkGold);
    tft.fillTriangle(x - 2, y + height / 2, x + 3, y + height / 2 - 4,
                     x + 3, y + height / 2 + 4, kBrightGold);
  }
}

void GameUi::drawFooterHints(const char* left, const char* right) {
  Adafruit_GFX& tft = target();
  tft.fillRect(0, 144, 128, 16, kInkBlue);
  tft.drawFastHLine(0, 144, 128, kDarkGold);
  text().color(kMutedCyan);
  text().draw(5, 157, left);
  const int16_t width = utf8GlyphCount(right) * 12;
  text().draw(max<int16_t>(68, 123 - width), 157, right);
}

void GameUi::drawProgressBar(int16_t x, int16_t y, int16_t width,
                             uint16_t value, uint16_t maximum,
                             uint16_t color) {
  Adafruit_GFX& tft = target();
  tft.drawRect(x, y, width, 7, kDarkGold);
  tft.fillRect(x + 1, y + 1, width - 2, 5, kInkBlack);
  if (maximum > 0) {
    const int16_t fill =
        min<int32_t>(width - 2, static_cast<int32_t>(value) * (width - 2) /
                                   maximum);
    tft.fillRect(x + 1, y + 1, fill, 5, color);
  }
}

void GameUi::startNotice(const char* message) {
  strncpy(notice_, message, sizeof(notice_) - 1);
  notice_[sizeof(notice_) - 1] = '\0';
  noticeStartedAt_ = millis();
  dirty_ = true;
}

void GameUi::drawNotice() {
  Adafruit_GFX& tft = target();
  const int16_t noticeY = (160 - 24) / 2;
  tft.fillRoundRect(20, noticeY, 88, 24, 4, kInkBlue);
  tft.drawRoundRect(20, noticeY, 88, 24, 4, kBrightGold);
  text().color(kBrightGold);
  const int16_t width = utf8GlyphCount(notice_) * 12;
  text().draw(max<int16_t>(24, (128 - width) / 2), noticeY + 17, notice_);
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
          pgm_read_word(kCloudTerraceHome +
                        (kPetRegionY + row) * kCloudTerraceHomeWidth +
                        kPetRegionX + column);
    }
  }

  int16_t petY = kPetRegionY + 33;
  if ((feedback_ == Feedback::MoodUp ||
       feedback_ == Feedback::StaminaUp) &&
      now - feedbackStartedAt_ < 700) {
    const uint32_t phase = (now - feedbackStartedAt_) % 350;
    petY -= phase < 175 ? phase / 50 : (350 - phase) / 50;
  }
  const int16_t petX = data.form >= PetForm::FinalA1 ? 16 : 5;
  pet_.draw(petCanvas_, data.form, petX, petY - kPetRegionY, now);
  tft.drawRGBBitmap(kPetRegionX, kPetRegionY, petCanvas_.getBuffer(),
                    kPetRegionWidth, kPetRegionHeight);
}

void GameUi::drawHomeStats(const PetSaveData& data) {
  drawHomeVitals(data);
}

void GameUi::drawHomeVitals(const PetSaveData& data) {
  Adafruit_GFX& tft = target();

  // ── 单行属性条 (y=114, 14px) - 4个14×14美术图标+数值 ──
  tft.fillRect(0, 114, 128, 14, kInkBlack);
  tft.drawFastHLine(0, 114, 128, kDarkGold);
  tft.drawFastHLine(0, 127, 128, kDarkGold);

  tft.setTextSize(1);

  // 心境 - 莲花图标 (列0, x=0)
  drawHomeIcon(1, 114, kHomeIconLotus);
  tft.setTextColor(data.mood < 25 ? kCinnabar : kWarmWhite);
  tft.setCursor(16, 118);
  tft.print(data.mood);

  // 体力 - 心形图标 (列1, x=32)
  drawHomeIcon(33, 114, kHomeIconHeart);
  tft.setTextColor(data.stamina < 25 ? kCinnabar : kWarmWhite);
  tft.setCursor(48, 118);
  tft.print(data.stamina);

  // 灵力 - 闪电图标 (列2, x=64)
  drawHomeIcon(65, 114, kHomeIconEnergy);
  tft.setTextColor(0x7DFF);
  tft.setCursor(80, 118);
  tft.print(data.energy);

  // 灵石 - 水晶图标 (列3, x=96)
  drawHomeIcon(97, 114, kHomeIconCrystal);
  tft.setTextColor(kBrightGold);
  tft.setCursor(112, 118);
  tft.print(data.coins);

  // ── 底部按钮区 (y=130, 30px, icons only) ──
  tft.fillRect(0, 130, 128, 30, 0x0862);
  tft.drawFastHLine(0, 130, 128, kDarkGold);

  tft.drawFastVLine(32, 133, 24, 0x4A85);
  tft.drawFastVLine(64, 133, 24, 0x4A85);
  tft.drawFastVLine(96, 133, 24, 0x4A85);

  drawButtonIcon(9, 138, kHomeButtonInteract);
  drawButtonIcon(41, 138, kHomeButtonCare);
  drawButtonIcon(73, 138, kHomeButtonAdventure);
  drawButtonIcon(105, 138, kHomeButtonStatus);
}

void GameUi::drawGoldPanel(int16_t x, int16_t y, int16_t width,
                           int16_t height) {
  Adafruit_GFX& tft = target();
  tft.fillRect(x + 1, y + 1, width - 2, height - 2, kInkBlue);
  tft.drawRect(x, y, width, height, kDarkGold);

  if (height >= 20) {
    tft.drawRect(x + 2, y + 2, width - 4, height - 4, 0x4A85);
  } else {
    tft.drawFastHLine(x + 2, y + 2, width - 4, 0x4A85);
  }

  tft.drawPixel(x + 1, y + 1, kBrightGold);
  tft.drawPixel(x + width - 2, y + 1, kBrightGold);
  tft.drawPixel(x + 1, y + height - 2, kBrightGold);
  tft.drawPixel(x + width - 2, y + height - 2, kBrightGold);
}

void GameUi::drawHomeIcon(int16_t x, int16_t y, const HomeUiIcon& icon) {
  target().drawRGBBitmap(x, y, icon.pixels, icon.mask, kHomeIconWidth,
                         kHomeIconHeight);
}

void GameUi::drawButtonIcon(int16_t x, int16_t y,
                            const HomeButtonIcon& icon) {
  Adafruit_GFX& tft = target();
  tft.drawRGBBitmap(x, y, icon.pixels, icon.mask,
                    kHomeButtonIconWidth, kHomeButtonIconHeight);
}

void GameUi::drawResourceBadge(int16_t x, int16_t y, uint16_t color,
                               const char* label, uint16_t value,
                               uint16_t maximum) {
  Adafruit_GFX& tft = target();
  tft.fillRoundRect(x, y, 56, 13, 3, kInkBlue);
  tft.drawRoundRect(x, y, 56, 13, 3, color);
  text().color(kWarmWhite);
  text().draw(x + 3, y + 11, label);
  tft.setTextColor(color);
  tft.setTextSize(1);
  tft.setCursor(x + 29, y + 3);
  if (maximum) {
    tft.printf("%u/%u", value, maximum);
  } else {
    tft.print(value);
  }
}

void GameUi::restoreBackgroundRect(int16_t x, int16_t y, int16_t width,
                                   int16_t height) {
  Adafruit_ST7735& tft = display_->raw();
  for (int16_t row = 0; row < height; ++row) {
    const uint16_t* source =
        kCloudTerraceHome + (y + row) * kCloudTerraceHomeWidth + x;
    tft.drawRGBBitmap(x, y + row, source, width, 1);
  }
}

void GameUi::drawCare(const PetSaveData& data) {
  Adafruit_GFX& tft = target();
  drawTitlePlaque("洞府培养", kBrightGold);

  drawPanel(8, 46, 112, 27, selection_ == 0);
  text().color(selection_ == 0 ? kBrightGold : kWarmWhite);
  text().draw(17, 59, "喂食");
  text().color(kMutedCyan);
  text().draw(67, 59, "十灵石");
  text().color(kWarmWhite);
  text().draw(17, 70, "体力 +20");

  drawPanel(8, 76, 112, 27, selection_ == 1);
  text().color(selection_ == 1 ? kBrightGold : kWarmWhite);
  text().draw(17, 89, "互动");
  text().color(kWarmWhite);
  text().draw(67, 89, "心境 +5");

  drawPanel(8, 106, 112, 32, selection_ == 2);
  text().color(selection_ == 2 ? kBrightGold : kWarmWhite);
  text().draw(17, 120, "打坐");
  text().color(kMutedCyan);
  text().draw(67, 120, "灵力 +3");
  text().color(kWarmWhite);
  text().draw(17, 134, "当前");
  tft.setTextColor(kWarmWhite);
  tft.setTextSize(1);
  tft.setCursor(48, 127);
  tft.print(data.energy);
  text().color(kMutedCyan);
  text().draw(67, 134, "余");
  tft.setCursor(87, 127);
  tft.print(data.meditationsUsed >= 3 ? 0 : 3 - data.meditationsUsed);
  text().draw(99, 134, "次");
  drawFooterHints("K1确认", "K4返回");
}

void GameUi::drawAdventure(const PetSaveData& data) {
  Adafruit_GFX& tft = target();
  drawTitlePlaque("秘境历练", kBrightGold);
  const char* zones[] = {"青竹灵境", "云海剑台", "玄岳古域"};
  for (uint8_t i = 0; i < 3; ++i) {
    const bool unlocked =
        i == 0 || (data.bossDefeatedMask & (1U << (i - 1)));
    const bool defeated = data.bossDefeatedMask & (1U << i);
    const bool bossReady = data.regionProgress[i] >= 100 && !defeated;
    const int16_t y = 46 + i * 31;
    drawPanel(7, y, 114, 28, i == selection_);
    text().color(!unlocked ? 0x632C
                           : i == selection_ ? kBrightGold : kWarmWhite);
    text().draw(13, y + 13, unlocked ? zones[i] : "尚未解锁");
    text().color(defeated ? 0x6E8D : bossReady ? kCinnabar : kMutedCyan);
    if (defeated) {
      text().draw(75, y + 13, "已镇守");
    } else if (bossReady) {
      text().draw(63, y + 13, "首领可战");
    }
    drawProgressBar(13, y + 18, 82, data.regionProgress[i], 100,
                    defeated ? 0x6E8D : kBrightGold);
    tft.setTextColor(unlocked ? kWarmWhite : 0x632C);
    tft.setTextSize(1);
    tft.setCursor(99, y + 18);
    tft.printf("%u%%", data.regionProgress[i]);
  }
  text().color(kMutedCyan);
  text().draw(8, 141, "灵力");
  tft.setTextColor(kWarmWhite);
  tft.setTextSize(1);
  tft.setCursor(38, 134);
  tft.print(data.energy);
  drawFooterHints("K1进入", "K4返回");
}

void GameUi::drawBattle(const PetSaveData& data) {
  Adafruit_GFX& tft = target();
  drawTitlePlaque("秘境首领", kCinnabar);

  drawPanel(7, 47, 114, 31, false);
  text().color(kCinnabar);
  text().draw(13, 61, "敌方气血");
  drawProgressBar(13, 67, 92, data.bossHp, data.bossMaxHp, kCinnabar);
  tft.setTextColor(kWarmWhite);
  tft.setTextSize(1);
  tft.setCursor(107, 66);
  tft.print(data.bossHp);

  drawPanel(7, 82, 114, 27, false);
  text().color(kMutedCyan);
  text().draw(13, 96, "己方体力");
  drawProgressBar(13, 101, 70, data.stamina, 100, 0x6E8D);
  text().draw(88, 96, "灵力");
  tft.setTextColor(kWarmWhite);
  tft.setCursor(91, 101);
  tft.print(data.energy);

  const char* actions[] = {"K1 攻击", "K2 法诀", "K3 丹药", "K4 防御"};
  const uint16_t colors[] = {kCinnabar, 0x7DFF, 0xD41F, 0x6E8D};
  for (uint8_t i = 0; i < 4; ++i) {
    const int16_t x = 7 + (i % 2) * 58;
    const int16_t y = 113 + (i / 2) * 15;
    tft.fillRoundRect(x, y, 55, 13, 2, kPanelBlue);
    tft.drawRoundRect(x, y, 55, 13, 2, colors[i]);
    text().color(colors[i]);
    text().draw(x + 4, y + 11, actions[i]);
  }
  drawFooterHints("四键出招", "回合制");
}

void GameUi::drawStatus(const PetSaveData& data) {
  Adafruit_GFX& tft = target();
  drawTitlePlaque("仙宠状态", kBrightGold);
  const char* forms[] = {"混沌灵卵", "凌霄麒麟", "镇岳麒麟", "太虚剑仙",
                         "九转丹仙", "不灭武仙", "万灵仙尊"};

  drawPanel(7, 47, 114, 38, false);
  text().color(kMutedCyan);
  text().draw(13, 62, "形态");
  text().color(kWarmWhite);
  text().draw(43, 62, forms[static_cast<unsigned>(data.form)]);
  text().color(kMutedCyan);
  text().draw(13, 79, "境界");
  text().color(kBrightGold);
  if (data.level < 3) {
    text().draw(43, 79, "三级初醒");
  } else if (data.level < 12) {
    text().draw(43, 79, "十二级化形");
  } else {
    text().draw(43, 79, "已臻化境");
  }
  tft.setTextColor(kWarmWhite);
  tft.setTextSize(1);
  tft.setCursor(91, 51);
  tft.printf("LV%u", data.level);

  drawPanel(7, 89, 114, 51, false);
  const char* labels[] = {"剑", "丹", "体", "灵"};
  const uint16_t colors[] = {0x7DFF, 0xD41F, kCinnabar, 0x6E8D};
  uint16_t maximum = 10;
  uint8_t strongest = 0;
  for (uint8_t i = 0; i < 4; ++i) {
    if (data.tendencies[i] > maximum) {
      maximum = data.tendencies[i];
    }
    if (data.tendencies[i] > data.tendencies[strongest]) {
      strongest = i;
    }
  }
  for (uint8_t i = 0; i < 4; ++i) {
    const int16_t y = 99 + i * 10;
    text().color(i == strongest ? kBrightGold : kWarmWhite);
    text().draw(13, y + 6, labels[i]);
    drawProgressBar(30, y, 70, data.tendencies[i], maximum,
                    i == strongest ? kBrightGold : colors[i]);
    tft.setTextColor(kWarmWhite);
    tft.setTextSize(1);
    tft.setCursor(104, y);
    tft.print(data.tendencies[i]);
  }
  drawFooterHints("修炼有时", "K4返回");
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
    const int16_t centerY = 40 - rise;
    for (uint8_t i = 0; i < 6; ++i) {
      const int16_t x = centerX - 24 + i * 9;
      const int16_t y = centerY + ((i * 7 + elapsed / 100) % 13);
      tft.drawPixel(x, y, 0xFFE0);
      tft.drawFastHLine(x - 1, y, 3, 0xFFE0);
      tft.drawFastVLine(x, y - 1, 3, 0xFFE0);
    }
  }

  tft.fillRoundRect(14, 52, 100, 24, 4, 0x18C3);
  tft.drawRoundRect(14, 52, 100, 24, 4, 0xD5EA);
  chinese_.color(feedback_ == Feedback::NoCoins ? 0xFB08 : 0xFFE0);
  switch (feedback_) {
    case Feedback::MoodUp:
      chinese_.draw(34, 69, "心境提升");
      break;
    case Feedback::StaminaUp:
      chinese_.draw(34, 69, "体力恢复");
      break;
    case Feedback::NoCoins:
      chinese_.draw(34, 69, "灵石不足");
      break;
    case Feedback::AlreadyFull:
      chinese_.draw(34, 69, "当前已满");
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
