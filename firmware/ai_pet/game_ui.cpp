#include "game_ui.h"

#include "assets/cloud_terrace_home.h"
#include "assets/home_button_icons.h"
#include "assets/home_ui_icons.h"
#include "region_config.h"
#include "assets/immortal_cave_home.h"
#include "assets/pet_sprites.h"
#include "assets/qingyun_scene.h"
#include "assets/qingyun_ui_icons.h"
#include "assets/qingyun_pets.h"
#include "assets/bamboo_realm_scene.h"
#include "assets/bamboo_guardian.h"

#include <cstring>

#include "region_config.h"

namespace {

constexpr int16_t kPetRegionX = 28;
constexpr int16_t kPetRegionY = 14;
constexpr int16_t kPetRegionWidth = 72;
constexpr int16_t kPetRegionHeight = 100;
constexpr uint8_t kQingyunAdventurePetSize = 48;
constexpr uint8_t kQingyunEventSubjectSize = 27;
constexpr uint16_t kInkBlack = 0x0861;
constexpr uint16_t kInkBlue = 0x10C3;
constexpr uint16_t kPanelBlue = 0x1924;
constexpr uint16_t kPanelSelected = 0x2145;
constexpr uint16_t kDarkGold = 0x9BC6;
constexpr uint16_t kBrightGold = 0xFEC8;
constexpr uint16_t kWarmWhite = 0xEF5D;
constexpr uint16_t kMutedCyan = 0x7CEF;
constexpr uint16_t kCinnabar = 0xD9E4;
constexpr uint32_t kPetEffectDurationMs = 500;
constexpr uint8_t kTechniqueMaxLevel = 9;
constexpr uint8_t kTechniqueThresholds[kTechniqueMaxLevel] = {
    0, 10, 22, 38, 58, 82, 112, 148, 190};
constexpr uint16_t kTechniqueCosts[kTechniqueMaxLevel] = {
    5, 8, 12, 18, 26, 36, 48, 62, 80};

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

uint16_t cumulativeXpBeforeLevel(uint8_t level) {
  uint16_t total = 0;
  for (uint8_t lv = 1; lv < level; ++lv) {
    total += GameState::experienceForLevel(lv);
  }
  return total;
}

const char* techniqueRealm(uint8_t level) {
  if (level == 0) {
    return "未修";
  }
  if (level <= 3) {
    return "入门";
  }
  if (level <= 6) {
    return "小成";
  }
  return "大成";
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
      page_ = UiPage::MeritLog;
    } else if (action == InputAction::Up) {
      page_ = UiPage::Inventory;
      selection_ = 0;
      inventoryTab_ = 0;
    } else if (action == InputAction::Down) {
      page_ = UiPage::RegionSelect;
      selection_ = 0;
    } else if (action == InputAction::Back) {
      page_ = UiPage::Status;
    }
  } else if (page_ == UiPage::Cultivation && action == InputAction::Back) {
    cultivationExitRequested_ = true;
    aiActive_ = false;
    aiResultActive_ = false;
    dirty_ = true;
  } else if (page_ == UiPage::Status) {
    if (action == InputAction::Up) {
      statusPage_ = (statusPage_ + 1) % 4;
      dirty_ = true;
    } else if (action == InputAction::Down) {
      statusPage_ = (statusPage_ + 3) % 4;
      dirty_ = true;
    } else if (action == InputAction::Confirm && statusPage_ == 3) {
      page_ = UiPage::TechniqueDetail;
      techniqueSelection_ = 0;
      dirty_ = true;
    } else if (action == InputAction::Back) {
      statusPage_ = 0;
      page_ = UiPage::Home;
    }
  } else if (page_ == UiPage::TechniqueDetail) {
    if (action == InputAction::Up) {
      techniqueSelection_ = (techniqueSelection_ + 3) % 4;
    } else if (action == InputAction::Down) {
      techniqueSelection_ = (techniqueSelection_ + 1) % 4;
    } else if (action == InputAction::Confirm) {
      startNotice(state.upgradeTechnique(techniqueSelection_)
                      ? "功法精进"
                      : "条件不足");
    } else if (action == InputAction::Back) {
      statusPage_ = 3;
      page_ = UiPage::Status;
    }
  } else if (action == InputAction::Back && page_ != UiPage::Adventure &&
             page_ != UiPage::Battle && page_ != UiPage::RegionSelect) {
    page_ = UiPage::Home;
  } else if (page_ == UiPage::MeritLog) {
    if (action == InputAction::Up) {
      meritPage_ = meritPage_ == 0 ? 4 : meritPage_ - 1;
    } else if (action == InputAction::Down) {
      meritPage_ = (meritPage_ + 1) % 5;
    }
  } else if (page_ == UiPage::Inventory) {
    if (inventoryTab_ == 1) {
      if (action == InputAction::Up || action == InputAction::Down) {
        inventoryTab_ = 0;
        selection_ = action == InputAction::Up ? 4 : 0;
      } else if (action == InputAction::Confirm) {
        startNotice("只能查看");
      }
    } else if (action == InputAction::Up) {
      if (selection_ == 0) {
        inventoryTab_ = 1;
      } else {
        selection_--;
      }
    } else if (action == InputAction::Down) {
      if (selection_ == 4) {
        inventoryTab_ = 1;
      } else {
        selection_++;
      }
    } else if (action == InputAction::Confirm) {
      const ItemType item = static_cast<ItemType>(selection_);
      if (state.useItem(item)) {
        startNotice(selection_ == 0 ? "灵力恢复" : "体力恢复");
      } else if (selection_ >= 2) {
        startNotice(selection_ == 4 ? "自动生效" : "战前使用");
      } else {
        startNotice("无法使用");
      }
    }
  } else if (page_ == UiPage::RegionSelect) {
    if (action == InputAction::Up) {
      selection_ = (selection_ + kRegionCount - 1) % kRegionCount;
    } else if (action == InputAction::Down) {
      selection_ = (selection_ + 1) % kRegionCount;
    } else if (action == InputAction::Confirm) {
      if (state.isRegionUnlocked(selection_)) {
        state.selectRegion(selection_);
        page_ = UiPage::Adventure;
        tokenPrompt_ = state.canUseRegionTokenForBoss();
      } else if (state.tryUnlockRegion(selection_)) {
        state.selectRegion(selection_);
        page_ = UiPage::Adventure;
        tokenPrompt_ = state.canUseRegionTokenForBoss();
        startNotice("区域解锁");
      } else {
        startNotice("条件不足");
      }
    } else if (action == InputAction::Back) {
      page_ = UiPage::Home;
    }
  } else if (page_ == UiPage::Adventure) {
    if (tokenPrompt_) {
      if (action == InputAction::Confirm) {
        if (state.useRegionTokenForBoss()) {
          tokenPrompt_ = false;
          page_ = UiPage::Battle;
          battlePrompt_ = true;
          useAttackTalisman_ = false;
          useGuardTalisman_ = false;
        }
      } else if (action == InputAction::Back) {
        tokenPrompt_ = false;
      }
      return;
    }
    const AdventurePhase phase = state.data().adventurePhase;
    if (phase == AdventurePhase::BossReady) {
      if (action == InputAction::Confirm) {
        page_ = UiPage::Battle;
        battlePrompt_ = true;
        useAttackTalisman_ = false;
        useGuardTalisman_ = false;
      } else if (action == InputAction::Back) {
        page_ = UiPage::Home;
      }
    } else if (phase == AdventurePhase::Result) {
      if (action == InputAction::Confirm) {
        state.acknowledgeAdventureResult();
      } else if (action == InputAction::Back) {
        state.stopAdventure();
        page_ = UiPage::Home;
      }
    } else if (phase == AdventurePhase::Advancing) {
      if (action == InputAction::Back) {
        state.stopAdventure();
        startNotice("结束历练");
      }
    } else if (action == InputAction::Confirm) {
      if (state.startAdventure()) {
        startNotice("踏上山道");
      } else {
        startNotice("灵力不足");
      }
    } else if (action == InputAction::Back) {
      page_ = UiPage::Home;
    }
  } else if (page_ == UiPage::Battle) {
    if (state.data().inBattle) {
      if (action == InputAction::Back) {
        state.retreatBoss();
        battlePrompt_ = false;
        startNotice("主动撤退");
      }
    } else if (battlePrompt_) {
      if (action == InputAction::Up &&
          state.data().inventory.items[
              static_cast<uint8_t>(ItemType::AttackTalisman)] > 0) {
        useAttackTalisman_ = !useAttackTalisman_;
      } else if (action == InputAction::Down &&
                 state.data().inventory.items[
                     static_cast<uint8_t>(ItemType::GuardTalisman)] > 0) {
        useGuardTalisman_ = !useGuardTalisman_;
      } else if (action == InputAction::Confirm) {
        if (state.startBossBattle(useAttackTalisman_,
                                         useGuardTalisman_)) {
          battlePrompt_ = false;
          startNotice("自动交锋");
        } else if (state.data().energy < 5) {
          startNotice("至少需五灵力");
        } else {
          startNotice("妖狼未现");
        }
      } else if (action == InputAction::Back) {
        page_ = UiPage::Adventure;
      }
    } else if (action == InputAction::Confirm ||
               action == InputAction::Back) {
      page_ = UiPage::Adventure;
      battlePrompt_ = true;
    }
  }
  dirty_ = true;
}

void GameUi::draw(const GameState& state, uint32_t now, bool force) {
  if (!display_) {
    return;
  }
  if (page_ == UiPage::Cultivation && aiResultActive_ &&
      now - aiLastEventAt_ >= 2500) {
    page_ = UiPage::Home;
    aiResultActive_ = false;
    dirty_ = true;
  }
  if (petEffect_ != PetEffect::None &&
      now - petEffectStartedAt_ >= kPetEffectDurationMs) {
    if (page_ == UiPage::Cultivation && !aiResultActive_) {
      // 修炼中状态：特效持续循环播放，不清除
    } else {
      petEffect_ = PetEffect::None;
      if (page_ == UiPage::Home) {
        dirty_ = true;
      }
    }
  }

  const bool fullRedraw = (dirty_ || force) && page_ == UiPage::Home;
  const bool menuRedraw = (dirty_ || force) && page_ != UiPage::Home;
  const uint32_t homeAnimationInterval =
      petEffect_ == PetEffect::None ? 400 : 100;
  const uint32_t cultivationInterval =
      petEffect_ == PetEffect::None ? 300 : 100;
  const bool animate =
      ((page_ == UiPage::Home &&
        now - lastAnimationAt_ >= homeAnimationInterval) ||
       (page_ == UiPage::Cultivation &&
        now - lastAnimationAt_ >= cultivationInterval) ||
       ((page_ == UiPage::Adventure || page_ == UiPage::Battle) &&
        now - lastAnimationAt_ >= 250));
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
      (animate && page_ != UiPage::Home);
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
    drawMenuFrame(state);
  } else if (fullRedraw) {
    drawHomeFrame(state.data(), now);
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

void GameUi::clearAiCultivation(uint32_t now) {
  (void)now;
  aiActive_ = false;
  aiTaskStartedAt_ = 0;
  aiLastEventAt_ = 0;
  aiResultActive_ = false;
  page_ = UiPage::Home;
  dirty_ = true;
}

void GameUi::notify(const char* message) {
  startNotice(message);
}

void GameUi::showAiActive(const char* source, uint32_t now) {
  strncpy(aiSource_, source, sizeof(aiSource_) - 1);
  aiSource_[sizeof(aiSource_) - 1] = '\0';
  aiActive_ = true;
  aiTaskStartedAt_ = now;
  aiLastEventAt_ = now;
  aiResultActive_ = false;
  cultivationExitRequested_ = false;
  page_ = UiPage::Cultivation;
  startPetEffect(PetEffect::AiComplete, now);
  dirty_ = true;
}

void GameUi::showAiResult(const char* source, uint16_t experienceGain,
                          uint16_t coinGain,
                          bool evolved, uint32_t now) {
  strncpy(aiSource_, source, sizeof(aiSource_) - 1);
  aiSource_[sizeof(aiSource_) - 1] = '\0';
  aiActive_ = false;
  aiResultActive_ = true;
  aiResultSuccess_ = true;
  aiExperienceGain_ = experienceGain;
  aiCoinGain_ = coinGain;
  aiEvolved_ = evolved;
  aiLastEventAt_ = now;
  page_ = UiPage::Cultivation;
  startPetEffect(evolved ? PetEffect::Evolution : PetEffect::AiComplete, now);
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
  startPetEffect(PetEffect::Evolution, now);
  dirty_ = true;
}

void GameUi::setPreviewForm(PetForm form) {
  previewEnabled_ = true;
  previewForm_ = form;
  page_ = UiPage::Home;
  dirty_ = true;
}

void GameUi::clearPreviewForm() {
  previewEnabled_ = false;
  page_ = UiPage::Home;
  dirty_ = true;
}

bool GameUi::previewEnabled() const {
  return previewEnabled_;
}

PetForm GameUi::previewForm() const {
  return previewForm_;
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

  // Left column: level
  tft.setTextColor(kBrightGold);
  tft.setTextSize(1);
  tft.setCursor(4, 3);
  tft.printf("Lv%u", data.level);

  // Middle column: XP bar (compact diamond frame)
  const uint16_t xpThreshold = GameState::experienceForLevel(data.level);
  const uint16_t levelXp = data.experience - cumulativeXpBeforeLevel(data.level);
  tft.fillRect(52, 1, 72, 11, kInkBlue);
  tft.drawFastHLine(55, 0, 66, kBrightGold);
  tft.drawFastHLine(55, 12, 66, kDarkGold);
  tft.drawFastVLine(51, 4, 5, kDarkGold);
  tft.drawFastVLine(124, 4, 5, kDarkGold);
  tft.fillTriangle(51, 6, 55, 2, 55, 10, kDarkGold);
  tft.fillTriangle(125, 6, 121, 2, 121, 10, kDarkGold);
  const int16_t xpFill = xpThreshold > 0 ? levelXp * 64 / xpThreshold : 0;
  if (xpFill > 0) {
    tft.fillRect(56, 3, xpFill, 3, kBrightGold);
    tft.fillRect(56, 6, xpFill, 3, 0xDD45);
  }
}

void GameUi::drawInkBackground(int16_t fillStartY) {
  Adafruit_GFX& tft = target();
  tft.drawRGBBitmap(0, 0, kCloudTerraceHome, kCloudTerraceHomeWidth,
                    kCloudTerraceHomeHeight);
  tft.fillRect(0, fillStartY, 128, 160 - fillStartY, 0x0861);
  tft.drawFastHLine(0, fillStartY, 128, 0x6B8D);
}

void GameUi::drawHomeFrame(const PetSaveData& data, uint32_t now) {
  uint16_t* frame = menuCanvas_.getBuffer();
  if (!frame) {
    return;
  }

  renderTarget_ = &menuCanvas_;
  renderText_ = &menuChinese_;
  drawInkBackground(128);
  drawHomeHeader(data);
  drawHome(data, now);
  renderTarget_ = nullptr;
  renderText_ = nullptr;

  Adafruit_ST7735& tft = display_->raw();
  tft.drawRGBBitmap(0, 0, menuCanvas_.getBuffer(), 128, 160);
}

void GameUi::drawMenuFrame(const GameState& state) {
  const PetSaveData& data = state.data();
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
    case UiPage::RegionSelect:
      drawRegionSelect(state);
      break;
    case UiPage::MeritLog:
      drawMeritLog(data);
      break;
    case UiPage::Inventory:
      drawInventory(data);
      break;
    case UiPage::Adventure:
      if (tokenPrompt_) {
        drawRegionTokenPrompt(state);
      } else {
        drawAdventure(data);
      }
      break;
    case UiPage::Status:
      drawStatus(data);
      break;
    case UiPage::TechniqueDetail:
      drawTechniqueDetail(state);
      break;
    case UiPage::Battle:
      drawBattle(state);
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
  tft.drawRGBBitmap(0, 0, kImmortalCaveHome, kImmortalCaveHomeWidth,
                    kImmortalCaveHomeHeight);
  tft.fillRect(0, 0, 128, 18, kInkBlack);
  tft.drawFastHLine(0, 18, 128, kDarkGold);
  tft.setTextColor(kMutedCyan);
  tft.setTextSize(1);
  tft.setCursor(4, 5);
  tft.print(aiSource_);
  tft.setTextColor(kWarmWhite);
  tft.setCursor(100, 5);
  tft.printf("LV%u", data.level);

  const char* label;
  uint16_t labelColor;
  if (aiResultActive_) {
    label = "修炼完成";
    labelColor = kBrightGold;
  } else {
    label = "修炼中";
    labelColor = kBrightGold;
  }
  text().color(labelColor);
  const int16_t labelWidth = utf8GlyphCount(label) * 12;
  text().draw(max<int16_t>(0, (128 - labelWidth) / 2), 24, label);

  const PetForm form = displayForm(data.form);
  const bool finalForm = form >= PetForm::FinalA1;
  const uint8_t spriteW = finalForm ? kFinalPetSpriteWidth : kPetSpriteWidth;
  const int16_t petX = (128 - spriteW) / 2;
  const int16_t petY = finalForm ? 32 : 42;
  const uint32_t effectElapsed =
      petEffect_ == PetEffect::None ? 0 : now - petEffectStartedAt_;
  pet_.draw(tft, form, petX, petY, now, petEffect_, effectElapsed, finalForm);

  tft.fillRect(0, 120, 128, 40, kInkBlack);
  tft.drawFastHLine(0, 120, 128, kDarkGold);
  tft.setTextColor(kWarmWhite);
  tft.setTextSize(1);
  if (aiResultActive_) {
    if (aiResultSuccess_) {
      if (aiEvolved_) {
        const char* forms[] = {"混沌灵卵", "凌霄麒麟", "镇岳麒麟",
                               "太虚剑仙", "九转丹仙", "不灭武仙",
                               "万灵仙尊"};
        text().color(0x7DFF);
        text().draw(34, 124, "灵光进化");
        text().color(kWarmWhite);
        const char* formName = forms[static_cast<unsigned>(data.form)];
        const int16_t formWidth = utf8GlyphCount(formName) * 12;
        text().draw(max<int16_t>(0, (128 - formWidth) / 2), 136, formName);
      } else {
        tft.setCursor(16, 128);
        tft.printf("EXP +%u", aiExperienceGain_);
        tft.setCursor(72, 128);
        tft.printf("STN +%u", aiCoinGain_);
      }
    }
  } else {
    const uint32_t elapsedSeconds =
        aiTaskStartedAt_ == 0 ? 0 : (now - aiTaskStartedAt_) / 1000;
    tft.setCursor(49, 128);
    tft.printf("%02lu:%02lu",
               static_cast<unsigned long>(elapsedSeconds / 60),
               static_cast<unsigned long>(elapsedSeconds % 60));
  }
  text().color(kWarmWhite);
  text().draw(4, 146, "AI修炼联动");
  const char* rightHint = "返回";
  const int16_t rhWidth = utf8GlyphCount(rightHint) * 12;
  const int16_t rhX = max<int16_t>(68, 123 - rhWidth);
  text().draw(rhX, 146, rightHint);
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
  }
}

void GameUi::drawFooterHints(const char* left, const char* right) {
  Adafruit_GFX& tft = target();
  tft.fillRect(0, 144, 128, 16, kInkBlue);
  tft.drawFastHLine(0, 144, 128, kDarkGold);
  text().color(kWarmWhite);
  text().draw(4, 157, left);
  const int16_t width = utf8GlyphCount(right) * 12;
  const int16_t rightX = max<int16_t>(68, 124 - width);
  text().draw(rightX, 157, right);
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
  Adafruit_GFX& tft = target();
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
  const PetForm form = displayForm(data.form);
  const bool finalForm = form >= PetForm::FinalA1;
  const int16_t petX = finalForm ? 4 : 5;
  if (finalForm) {
    petY = kPetRegionY + 18;
  }
  const uint32_t effectElapsed =
      petEffect_ == PetEffect::None ? 0 : now - petEffectStartedAt_;
  pet_.draw(petCanvas_, form, petX, petY - kPetRegionY, now, petEffect_,
            effectElapsed);
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

  drawButtonIcon(9, 138, kHomeButtonMeritLog);
  drawButtonIcon(41, 138, kHomeButtonInventory);
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

void GameUi::drawQingyunIcon(int16_t x, int16_t y,
                             const QingyunUiIcon& icon) {
  target().drawRGBBitmap(x, y, icon.pixels, icon.mask,
                         kQingyunIconWidth, kQingyunIconHeight);
}

void GameUi::drawRegionTreasureIcon(int16_t x, int16_t y,
                                    const RegionTreasureIcon& icon) {
  target().drawRGBBitmap(x, y, icon.pixels, icon.mask,
                         kRegionTreasureIconWidth,
                         kRegionTreasureIconHeight);
}

void GameUi::drawQingyunIconLarge(int16_t x, int16_t y,
                                  const QingyunUiIcon& icon) {
  Adafruit_GFX& tft = target();
  for (uint8_t row = 0; row < kQingyunIconHeight; ++row) {
    for (uint8_t column = 0; column < kQingyunIconWidth; ++column) {
      const uint8_t mask =
          pgm_read_byte(icon.mask + row * 3 + column / 8);
      if ((mask & (0x80 >> (column % 8))) == 0) {
        continue;
      }
      const uint16_t color = pgm_read_word(
          icon.pixels + row * kQingyunIconWidth + column);
      const int16_t x0 =
          x + column * kQingyunEventSubjectSize / kQingyunIconWidth;
      const int16_t x1 =
          x + ((column + 1) * kQingyunEventSubjectSize +
               kQingyunIconWidth - 1) /
                  kQingyunIconWidth;
      const int16_t y0 =
          y + row * kQingyunEventSubjectSize / kQingyunIconHeight;
      const int16_t y1 =
          y + ((row + 1) * kQingyunEventSubjectSize +
               kQingyunIconHeight - 1) /
                  kQingyunIconHeight;
      tft.fillRect(x0, y0, x1 - x0, y1 - y0, color);
    }
  }
}

void GameUi::drawQingyunPet(PetForm form, int16_t x, int16_t y) {
  const QingyunPetSprite* sprites[] = {
      &kQingyunPetEgg, &kQingyunPetRookieA, &kQingyunPetRookieB,
      &kQingyunPetFinalA1, &kQingyunPetFinalA2, &kQingyunPetFinalB1,
      &kQingyunPetFinalB2};
  const QingyunPetSprite& sprite =
      *sprites[min<uint8_t>(static_cast<uint8_t>(form), 6)];
  target().drawRGBBitmap(x, y, sprite.pixels, sprite.mask,
                         kQingyunPetWidth, kQingyunPetHeight);
}

void GameUi::drawQingyunPetLarge(PetForm form, int16_t x, int16_t y) {
  const QingyunPetSprite* sprites[] = {
      &kQingyunPetEgg, &kQingyunPetRookieA, &kQingyunPetRookieB,
      &kQingyunPetFinalA1, &kQingyunPetFinalA2, &kQingyunPetFinalB1,
      &kQingyunPetFinalB2};
  const QingyunPetSprite& sprite =
      *sprites[min<uint8_t>(static_cast<uint8_t>(form), 6)];
  Adafruit_GFX& tft = target();
  for (uint8_t row = 0; row < kQingyunPetHeight; ++row) {
    for (uint8_t column = 0; column < kQingyunPetWidth; ++column) {
      const uint8_t mask =
          pgm_read_byte(sprite.mask + row * 4 + column / 8);
      if ((mask & (0x80 >> (column % 8))) == 0) {
        continue;
      }
      const uint16_t color = pgm_read_word(
          sprite.pixels + row * kQingyunPetWidth + column);
      const int16_t x0 =
          x + column * kQingyunAdventurePetSize / kQingyunPetWidth;
      const int16_t x1 =
          x + ((column + 1) * kQingyunAdventurePetSize +
               kQingyunPetWidth - 1) /
                  kQingyunPetWidth;
      const int16_t y0 =
          y + row * kQingyunAdventurePetSize / kQingyunPetHeight;
      const int16_t y1 =
          y + ((row + 1) * kQingyunAdventurePetSize +
               kQingyunPetHeight - 1) /
                  kQingyunPetHeight;
      tft.fillRect(x0, y0, x1 - x0, y1 - y0, color);
    }
  }
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

void GameUi::drawMeritLog(const PetSaveData& data) {
  Adafruit_GFX& tft = target();
  drawTitlePlaque("功德簿", kBrightGold);
  const char* sources[] = {"Codex", "Claude", "OpenCode", "CodeFree"};
  const uint8_t firstOffset = meritPage_ * 2;
  for (uint8_t row = 0; row < 2; ++row) {
    const uint8_t offset = firstOffset + row;
    const int16_t y = 48 + row * 43;
    drawPanel(7, y, 114, 38, false);
    if (offset >= data.aiTaskRecordCount) {
      text().color(0x632C);
      text().draw(14, y + 22, "尚无记载");
      continue;
    }
    const uint8_t index =
        (data.aiTaskRecordIndex + 10 - 1 - offset) % 10;
    const AiTaskRecord& record = data.aiTaskRecords[index];
    tft.setTextColor(kBrightGold);
    tft.setTextSize(1);
    tft.setCursor(13, y + 7);
    tft.print(sources[min<uint8_t>(record.source, 3)]);
    tft.setTextColor(kWarmWhite);
    tft.setCursor(13, y + 20);
    tft.printf("%um%us", record.durationSeconds / 60,
               record.durationSeconds % 60);
    tft.setCursor(69, y + 20);
    tft.printf("+%uXP", record.experienceReward);
    tft.setCursor(69, y + 30);
    tft.printf("+%u", record.coinReward);
  }
  text().color(kMutedCyan);
  text().draw(8, 140, "每页二则");
  tft.setTextColor(kWarmWhite);
  tft.setCursor(96, 133);
  tft.printf("%u/5", meritPage_ + 1);
  drawFooterHints("翻页", "返回");
}

void GameUi::drawInventory(const PetSaveData& data) {
  if (inventoryTab_ == 1) {
    drawTreasureInventory(data);
    return;
  }
  Adafruit_GFX& tft = target();
  drawTitlePlaque("乾坤袋", kBrightGold);
  const char* names[] = {"灵草", "回春丹", "攻击符", "护身符",
                         "青云信物"};
  const QingyunUiIcon* icons[] = {
      &kQingyunIconSpiritHerb, &kQingyunIconRecoveryPill,
      &kQingyunIconAttackTalisman, &kQingyunIconGuardTalisman,
      &kQingyunIconToken};
  for (uint8_t i = 0; i < 5; ++i) {
    const int16_t y = 45 + i * 19;
    drawPanel(8, y, 112, 17, selection_ == i);
    drawQingyunIcon(11, y, *icons[i]);
    text().color(selection_ == i ? kBrightGold : kWarmWhite);
    text().draw(32, y + 13, names[i]);
    tft.setTextColor(kMutedCyan);
    tft.setTextSize(1);
    tft.setCursor(91, y + 5);
    const uint16_t quantity = data.inventory.items[i];
    if (quantity > 999) {
      tft.print("999+");
    } else {
      tft.print(quantity);
    }
  }
  drawFooterHints("使用", "返回");
}

void GameUi::drawTreasureInventory(const PetSaveData& data) {
  drawTitlePlaque("宝物", kBrightGold);
  drawPanel(10, 45, 108, 88, false);
  drawPanel(16, 53, 44, 70, false);
  drawRegionTreasureIcon(26, 59, kRegionTreasureQingyunSword);
  text().color(data.regionTreasure[0] ? kBrightGold : 0x632C);
  text().draw(22, 93, "青云剑");
  text().color(data.regionTreasure[0] ? kWarmWhite : 0x632C);
  text().draw(20, 110, data.regionTreasure[0] ? "攻防" : "未得");

  drawPanel(68, 53, 44, 70, false);
  drawRegionTreasureIcon(78, 59, kRegionTreasureSpiritBambooJade);
  text().color(data.regionTreasure[1] ? kBrightGold : 0x632C);
  text().draw(72, 93, "灵竹玉佩");
  text().color(data.regionTreasure[1] ? kWarmWhite : 0x632C);
  text().draw(73, 110, data.regionTreasure[1] ? "闪避" : "未得");
  drawFooterHints("切换", "返回");
}

void GameUi::drawRegionSelect(const GameState& state) {
  const PetSaveData& data = state.data();
  Adafruit_GFX& tft = target();
  drawTitlePlaque("秘境", kBrightGold);
  for (uint8_t i = 0; i < kRegionCount; ++i) {
    const int16_t y = 43 + i * 19;
    const bool unlocked = state.isRegionUnlocked(i);
    drawPanel(8, y, 112, 17, selection_ == i);
    text().color(unlocked ? (selection_ == i ? kBrightGold : kWarmWhite)
                          : kMutedCyan);
    text().draw(14, y + 13, kRegions[i].name);
    tft.setTextColor(unlocked ? kMutedCyan : 0x632C);
    tft.setTextSize(1);
    tft.setCursor(88, y + 5);
    if (unlocked) {
      tft.printf("R%u", data.regionRound[i]);
    } else {
      tft.printf("L%u", kRegions[i].unlock_level);
    }
  }
  drawFooterHints("进入", "返回");
}

void GameUi::drawRegionTokenPrompt(const GameState& state) {
  const PetSaveData& data = state.data();
  Adafruit_GFX& tft = target();
  drawTitlePlaque("秘境令", kBrightGold);
  drawPanel(10, 47, 108, 76, false);
  text().color(kWarmWhite);
  text().draw(25, 68, "消耗一枚令牌");
  text().draw(21, 87, "直接挑战首领");
  text().color(kMutedCyan);
  text().draw(28, 107, "当前持有");
  tft.setTextColor(kBrightGold);
  tft.setTextSize(1);
  tft.setCursor(87, 98);
  tft.print(data.inventory.items[static_cast<uint8_t>(ItemType::RegionToken)]);
  drawFooterHints("使用", "不用");
}

void GameUi::drawAdventure(const PetSaveData& data) {
  if (data.adventurePhase == AdventurePhase::Result) {
    drawQingyunEventResult(data);
  } else {
    drawQingyunAdventure(data, millis());
  }
}

void GameUi::drawQingyunScene(const PetSaveData& data, uint32_t now) {
  Adafruit_GFX& tft = target();
  if (data.activeRegion == 1) {
    tft.drawRGBBitmap(0, 36, kBambooRealmScene, kBambooRealmSceneWidth,
                      kBambooRealmSceneHeight);
  } else {
    tft.drawRGBBitmap(0, 36, kQingyunScene, kQingyunSceneWidth,
                      kQingyunSceneHeight);
  }
  const int16_t bob =
      data.adventurePhase == AdventurePhase::Advancing
          ? static_cast<int16_t>((now / 250) % 2)
          : 0;
  drawQingyunPetLarge(data.form, 40, 42 + bob);
}

void GameUi::drawQingyunEventSubject(const PetSaveData& data,
                                     AdventureEvent event) {
  drawQingyunScene(data, millis());
  if (event == AdventureEvent::Gather) {
    drawQingyunIconLarge(50, 50, kQingyunIconSpiritHerb);
  } else if (event == AdventureEvent::Npc) {
    drawQingyunIconLarge(50, 50, kQingyunIconWoundedCultivator);
  } else if (event == AdventureEvent::Fight) {
    drawQingyunIconLarge(50, 50, kQingyunIconDemonBeast);
  } else if (event == AdventureEvent::Shortcut) {
    drawQingyunIconLarge(50, 50, kQingyunIconShortcut);
  }
}

void GameUi::drawQingyunAdventure(const PetSaveData& data, uint32_t now) {
  Adafruit_GFX& tft = target();
  text().color(kBrightGold);
  text().draw(data.activeRegion == 1 ? 38 : 38, 31,
              kRegions[data.activeRegion].name);
  tft.setTextColor(kMutedCyan);
  tft.setTextSize(1);
  tft.setCursor(101, 24);
  tft.printf("R%u", data.regionRound[data.activeRegion]);
  tft.drawFastHLine(8, 34, 112, kDarkGold);
  drawQingyunScene(data, now);
  drawPanel(7, 93, 114, 46, false);
  drawProgressBar(13, 99, 82, data.adventureProgress, 100, kBrightGold);
  tft.setTextColor(kWarmWhite);
  tft.setTextSize(1);
  tft.setCursor(99, 95);
  tft.printf("%u%%", data.adventureProgress);
  drawHomeIcon(14, 109, kHomeIconEnergy);
  drawHomeIcon(68, 109, kHomeIconHeart);
  tft.setTextColor(kMutedCyan);
  tft.setCursor(34, 113);
  tft.print(data.energy);
  tft.setCursor(88, 113);
  tft.print(data.stamina);
  if (data.adventurePhase == AdventurePhase::Advancing) {
    text().color(kBrightGold);
    text().draw(35, 136, "自动前行");
    drawFooterHints("自动前行", "结束");
  } else if (data.adventurePhase == AdventurePhase::BossReady) {
    text().color(kCinnabar);
    text().draw(35, 136, "首领现身");
    drawFooterHints("挑战", "返回");
  } else {
    text().color(kWarmWhite);
    text().draw(35, 136, "整装待发");
    drawFooterHints("开始", "返回");
  }
}

void GameUi::drawQingyunEventResult(const PetSaveData& data) {
  text().color(kBrightGold);
  text().draw(38, 31, "秘境际遇");
  target().drawFastHLine(8, 34, 112, kDarkGold);
  drawQingyunEventSubject(data, data.currentEvent);
  const char* titles[] = {"", "灵草", "妖兽", "受伤修士", "山道捷径"};
  const uint8_t event = static_cast<uint8_t>(data.currentEvent);
  text().color(kBrightGold);
  text().draw(9, 100, titles[event]);
  drawPanel(10, 108, 108, 28, false);
  const char* results[] = {"风过无痕", "继续前行", "获得物品",
                           "有所收获", "进度提升", "体力受损"};
  text().color(kWarmWhite);
  text().draw(29, 126,
              results[static_cast<uint8_t>(data.currentEventResult)]);
  drawFooterHints("继续", "结束");
}

void GameUi::drawQingyunBossPrompt(const GameState& state) {
  const PetSaveData& data = state.data();
  Adafruit_GFX& tft = target();

  tft.fillRoundRect(14, 8, 100, 30, 4, kPanelBlue);
  tft.drawRoundRect(14, 8, 100, 30, 4, kDarkGold);
  tft.drawFastHLine(23, 10, 82, kCinnabar);
  text().color(kCinnabar);
  text().draw(36, 23, kRegions[data.activeRegion].boss_name);
  text().color(kMutedCyan);
  tft.setCursor(18, 28);
  tft.printf("LV%u", data.level);
  tft.setCursor(97, 28);
  tft.printf("R%u", data.regionRound[data.activeRegion]);

  tft.fillRect(0, 38, 128, 68, 0x0841);
  if (data.activeRegion == 1) {
    tft.drawRGBBitmap(28, 34, kBambooGuardianLargePixels,
                      kBambooGuardianLargeMask,
                      kBambooGuardianLargeWidth,
                      kBambooGuardianLargeHeight);
  } else {
    tft.drawRGBBitmap(28, 34, kQingyunBossLargePixels,
                      kQingyunBossLargeMask,
                      kQingyunBossLargeWidth, kQingyunBossLargeHeight);
  }

  tft.fillRect(0, 106, 128, 11, 0x0841);
  text().color(kMutedCyan);
  tft.setCursor(20, 109);
  tft.printf("HP:%u", state.bossMaxHp());
  tft.setCursor(76, 109);
  tft.printf("ATK:%u",
             kRegions[data.activeRegion].base_boss_damage *
                 state.damagePercent() / 100);

  drawPanel(8, 118, 52, 22, useAttackTalisman_);
  drawQingyunIcon(11, 120, kQingyunIconAttackTalisman);
  text().color(useAttackTalisman_ ? kBrightGold : kWarmWhite);
  tft.setCursor(34, 124);
  tft.printf("x%u", data.inventory.items[
      static_cast<uint8_t>(ItemType::AttackTalisman)]);

  drawPanel(68, 118, 52, 22, useGuardTalisman_);
  drawQingyunIcon(71, 120, kQingyunIconGuardTalisman);
  text().color(useGuardTalisman_ ? kBrightGold : kWarmWhite);
  tft.setCursor(94, 124);
  tft.printf("x%u", data.inventory.items[
      static_cast<uint8_t>(ItemType::GuardTalisman)]);

  drawQingyunIcon(46, 143, kQingyunIconEnergy);
  text().color(kMutedCyan);
  tft.setCursor(68, 147);
  tft.printf("%u/5", data.energy);

  drawFooterHints("开战", "返回");
}

void GameUi::drawBattle(const GameState& state) {
  const PetSaveData& data = state.data();
  Adafruit_GFX& tft = target();
  if (!data.inBattle && battlePrompt_) {
    drawQingyunBossPrompt(state);
    return;
  }

  const uint16_t displayRound =
      !data.inBattle && data.lastBattleResult == BattleResult::Victory &&
              data.regionRound[data.activeRegion] > 1
          ? data.regionRound[data.activeRegion] - 1
          : data.regionRound[data.activeRegion];

  if (!data.inBattle && data.lastBattleResult == BattleResult::Victory) {
    tft.fillRoundRect(14, 8, 100, 30, 4, kPanelBlue);
    tft.drawRoundRect(14, 8, 100, 30, 4, kDarkGold);
    tft.drawFastHLine(23, 10, 82, kCinnabar);
    text().color(kCinnabar);
    text().draw(36, 23, kRegions[data.activeRegion].boss_name);
    text().color(kMutedCyan);
    tft.setTextSize(1);
    tft.setCursor(18, 28);
    tft.printf("LV%u", data.level);
    tft.setCursor(97, 28);
    tft.printf("R%u", displayRound);
    drawPanel(8, 43, 112, 91, false);
    text().color(kBrightGold);
    text().draw(43, 61, "通关奖励");
    tft.setTextColor(kWarmWhite);
    tft.setCursor(17, 69);
    tft.printf("XP +%u", data.lastBossExperience);
    text().color(kWarmWhite);
    text().draw(67, 77, "灵石");
    tft.setCursor(94, 69);
    tft.printf("+%u", data.lastBossCoins);
    const QingyunUiIcon* rewardIcons[] = {
        &kQingyunIconSpiritHerb, &kQingyunIconRecoveryPill,
        &kQingyunIconAttackTalisman, &kQingyunIconGuardTalisman};
    uint8_t rewardX = 18;
    for (uint8_t index = 0; index < 4; ++index) {
      if (data.lastBossItems[index] == 0) {
        continue;
      }
      drawQingyunIcon(rewardX, 86, *rewardIcons[index]);
      tft.setCursor(rewardX + 19, 91);
      tft.printf("+%u", data.lastBossItems[index]);
      rewardX += 48;
    }
    text().color(data.lastBossTreasure ? kBrightGold : kMutedCyan);
    text().draw(30, 116,
                data.lastBossTreasure
                    ? kRegions[data.activeRegion].treasure_name
                    : "宝物尚无踪");
    drawFooterHints("返回山道", "战斗结束");
    return;
  }

  tft.fillRect(0, 18, 128, 22, kInkBlack);
  tft.drawFastHLine(0, 39, 128, kDarkGold);
  text().color(kCinnabar);
  text().draw(6, 31, kRegions[data.activeRegion].boss_name);
  tft.setTextColor(kMutedCyan);
  tft.setTextSize(1);
  tft.setCursor(84, 20);
  tft.printf("Lv%u", data.level);
  tft.setCursor(108, 20);
  tft.printf("R%u", displayRound);
  text().color(kCinnabar);
  text().draw(8, 39, "敌");
  drawProgressBar(25, 31, 62, data.bossHp, data.bossMaxHp, kCinnabar);
  tft.setTextColor(kWarmWhite);
  tft.setCursor(92, 31);
  tft.print(data.bossHp);

  tft.fillRect(0, 40, 128, 54, 0x11E5);
  const uint8_t bossFrame =
      data.inBattle
          ? static_cast<uint8_t>((millis() / 200) %
                                 (data.activeRegion == 1
                                      ? kBambooGuardianFrameCount
                                      : kQingyunBossFrameCount))
          : 0;
  if (data.activeRegion == 1) {
    tft.drawRGBBitmap(80, 44, kBambooGuardianPixels[bossFrame],
                      kBambooGuardianMasks[bossFrame],
                      kBambooGuardianWidth, kBambooGuardianHeight);
  } else {
    tft.drawRGBBitmap(80, 44, kQingyunBossPixels[bossFrame],
                      kQingyunBossMasks[bossFrame],
                      kQingyunBossWidth, kQingyunBossHeight);
  }
  drawQingyunPetLarge(data.form, 8, 44);
  tft.drawFastHLine(0, 94, 128, kDarkGold);

  drawPanel(7, 98, 114, 39, false);
  text().color(kMutedCyan);
  text().draw(13, 110, "我");
  drawProgressBar(29, 102, 58, data.stamina, 100, 0x6E8D);
  tft.setTextColor(kWarmWhite);
  tft.setCursor(93, 101);
  tft.print(data.stamina);
  text().color(kMutedCyan);
  text().draw(13, 125, "灵力");
  tft.setTextColor(kWarmWhite);
  tft.setCursor(47, 118);
  tft.print(data.energy);
  text().color(kMutedCyan);
  text().draw(72, 125, "回合");
  tft.setCursor(104, 118);
  tft.print(data.battleRound);
  if (data.inBattle) {
    drawBattleLog(state);
    drawFooterHints("自动中", "撤退");
  } else {
    const char* results[] = {"", "", "大胜", "战败", "灵力耗尽",
                             "已撤退"};
    text().color(data.lastBattleResult == BattleResult::Victory
                     ? kBrightGold
                     : kWarmWhite);
    text().draw(42, 90,
                results[static_cast<uint8_t>(data.lastBattleResult)]);
    drawFooterHints("返回山道", "战斗结束");
  }
}

void GameUi::drawBattleLog(const GameState& state) {
  Adafruit_GFX& tft = target();
  tft.fillRect(0, 131, 128, 13, kInkBlack);
  text().color(kBrightGold);
  const int16_t y = 143;
  switch (state.battleLogType()) {
    case BattleLogType::PlayerHit:
      text().draw(23, y, "剑气");
      tft.setTextColor(kWarmWhite);
      tft.setTextSize(1);
      tft.setCursor(60, 136);
      tft.printf("-%d", state.battleLogValue());
      break;
    case BattleLogType::PlayerCritical:
      text().draw(23, y, "暴击");
      tft.setTextColor(kBrightGold);
      tft.setTextSize(1);
      tft.setCursor(60, 136);
      tft.printf("-%d", state.battleLogValue());
      break;
    case BattleLogType::BossHit:
      text().draw(23, y, "敌袭");
      tft.setTextColor(kWarmWhite);
      tft.setTextSize(1);
      tft.setCursor(60, 136);
      tft.printf("-%d", state.battleLogValue());
      break;
    case BattleLogType::BossMiss:
      text().draw(41, y, "闪避");
      break;
    case BattleLogType::Shield:
      text().draw(29, y, "护体");
      tft.setTextColor(kWarmWhite);
      tft.setTextSize(1);
      tft.setCursor(66, 136);
      tft.printf("-%d", state.battleLogValue());
      break;
    case BattleLogType::Heal:
      text().draw(29, y, "回元");
      tft.setTextColor(kWarmWhite);
      tft.setTextSize(1);
      tft.setCursor(66, 136);
      tft.printf("+%d", state.battleLogValue());
      break;
    case BattleLogType::Victory:
      text().draw(35, y, "首领伏诛");
      break;
    case BattleLogType::Defeat:
      text().draw(41, y, "战败");
      break;
    case BattleLogType::EnergyDepleted:
      text().draw(29, y, "灵力枯竭");
      break;
    case BattleLogType::None:
      text().draw(35, y, "自动交锋");
      break;
  }
}

void GameUi::drawStatus(const PetSaveData& data) {
  Adafruit_GFX& tft = target();
  const char* titles[] = {"仙宠状态", "战斗属性", "修为资源", "功法概览"};
  drawTitlePlaque(titles[statusPage_], kBrightGold);

  // Page indicator dots at y=146
  for (uint8_t i = 0; i < 4; ++i) {
    tft.fillRect(51 + i * 8, 146, 5, 3,
                 i == statusPage_ ? kBrightGold : 0x4208);
  }

  if (statusPage_ == 0) {
    // ── Page 1: Base (form, realm, tendencies) ──
    const char* forms[] = {"混沌灵卵", "凌霄麒麟", "镇岳麒麟", "太虚剑仙",
                           "九转丹仙", "不灭武仙", "万灵仙尊"};
    drawPanel(7, 47, 114, 38, false);
    text().color(kMutedCyan);
    text().draw(13, 62, "形态");
    text().color(kWarmWhite);
    text().draw(43, 62,
                forms[static_cast<unsigned>(displayForm(data.form))]);
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
    drawFooterHints("翻页", "返回");

  } else if (statusPage_ == 1) {
    // ── Page 2: Battle Stats ──
    const uint16_t atkBase = 4 + data.level +
        min<uint16_t>(8, data.tendencies[0] / 5) +
        min<uint16_t>(10, data.tendencies[1] / 4);
    uint16_t atk = atkBase;
    if (data.form == PetForm::RookieA || data.form == PetForm::FinalA1) {
      atk++;
    } else if (data.form == PetForm::FinalA2) {
      atk += 2;
    }
    const uint8_t critRate =
        5 + min<uint16_t>(15, data.tendencies[0] / 4);
    const uint8_t def =
        max<int16_t>(1, 8 - min<uint16_t>(5, data.tendencies[2] / 8));
    const uint8_t dodgeRate =
        5 + min<uint16_t>(15, data.tendencies[2] / 4);

    drawPanel(7, 47, 114, 38, false);
    text().color(kMutedCyan);
    text().draw(13, 62, "攻击");
    text().color(kBrightGold);
    tft.setCursor(55, 55);
    tft.printf("%u", atk);
    text().color(kMutedCyan);
    text().draw(13, 79, "暴击率");
    text().color(kCinnabar);
    tft.setCursor(70, 72);
    tft.printf("%u%%", critRate);

    drawPanel(7, 89, 114, 38, false);
    text().color(kMutedCyan);
    text().draw(13, 104, "防御");
    text().color(kBrightGold);
    tft.setCursor(55, 97);
    tft.printf("%u", def);
    text().color(kMutedCyan);
    text().draw(13, 121, "闪避率");
    text().color(kCinnabar);
    tft.setCursor(70, 114);
    tft.printf("%u%%", dodgeRate);

    if (data.regionTreasure[0]) {
      drawPanel(7, 131, 114, 12, false);
      text().color(kBrightGold);
      text().draw(13, 140, "青云剑 攻+10% 防+10%");
    }

    drawFooterHints("翻页", "上页");

  } else if (statusPage_ == 2) {
    // ── Page 3: Vitals & Resources ──
    drawPanel(7, 47, 114, 66, false);

    text().color(kMutedCyan);
    text().draw(13, 62, "灵力");
    text().color(0x7DFF);
    tft.setCursor(55, 55);
    tft.printf("%u/%u", data.energy,
               GameState::maxEnergy(data.form, data.techniqueLevels));

    text().color(kMutedCyan);
    text().draw(13, 78, "体力");
    text().color(kWarmWhite);
    tft.setCursor(55, 71);
    tft.printf("%u", data.stamina);

    text().color(kMutedCyan);
    text().draw(13, 94, "心境");
    text().color(data.mood < 25 ? kCinnabar : kWarmWhite);
    tft.setCursor(55, 87);
    tft.printf("%u", data.mood);

    text().color(kMutedCyan);
    text().draw(13, 110, "灵石");
    text().color(kBrightGold);
    tft.setCursor(55, 103);
    tft.printf("%u", data.coins);

    drawFooterHints("翻页", "上页");
  } else {
    drawTechniqueOverview(data);
  }
}

void GameUi::drawTechniqueOverview(const PetSaveData& data) {
  Adafruit_GFX& tft = target();
  const char* names[] = {"太虚剑诀", "九转丹法", "不灭玄功", "万灵息法"};
  const uint16_t colors[] = {0x7DFF, 0xD41F, kCinnabar, 0x6E8D};

  drawPanel(7, 47, 114, 84, false);
  for (uint8_t i = 0; i < 4; ++i) {
    const int16_t y = 56 + i * 19;
    text().color(colors[i]);
    text().draw(13, y + 7, names[i]);
    text().color(kWarmWhite);
    tft.setTextSize(1);
    tft.setTextColor(kWarmWhite);
    tft.setCursor(76, y);
    tft.printf("Lv%u", data.techniqueLevels[i]);
    text().color(kBrightGold);
    text().draw(100, y + 7, techniqueRealm(data.techniqueLevels[i]));
  }

  drawFooterHints("详情", "上页");
}

void GameUi::drawTechniqueDetail(const GameState& state) {
  Adafruit_GFX& tft = target();
  const PetSaveData& data = state.data();
  const char* names[] = {"太虚剑诀", "九转丹法", "不灭玄功", "万灵息法"};
  const char* labels[] = {"剑", "丹", "体", "灵"};
  const char* effects[] = {"伤害暴击", "丹药回复", "减伤护体", "灵力上限"};
  const uint16_t colors[] = {0x7DFF, 0xD41F, kCinnabar, 0x6E8D};
  const uint8_t index = techniqueSelection_;
  const uint8_t level = data.techniqueLevels[index];

  drawTitlePlaque("功法修炼", kBrightGold);
  drawPanel(7, 43, 114, 88, false);

  for (uint8_t i = 0; i < 4; ++i) {
    const int16_t y = 50 + i * 12;
    tft.fillRect(11, y - 1, 106, 11,
                 i == index ? kPanelSelected : kInkBlue);
    text().color(i == index ? kBrightGold : colors[i]);
    text().draw(16, y + 8, names[i]);
    tft.setTextSize(1);
    tft.setTextColor(kWarmWhite);
    tft.setCursor(86, y + 1);
    tft.printf("Lv%u", data.techniqueLevels[i]);
  }

  text().color(kMutedCyan);
  text().draw(15, 108, effects[index]);
  text().color(kBrightGold);
  text().draw(72, 108, techniqueRealm(level));

  if (level >= kTechniqueMaxLevel) {
    text().color(kBrightGold);
    text().draw(15, 124, "已至大成");
  } else {
    const uint8_t next = level;
    text().color(kMutedCyan);
    text().draw(15, 124, "所需");
    text().color(kWarmWhite);
    text().draw(48, 124, labels[index]);
    text().draw(82, 124, "石");
    tft.setTextSize(1);
    tft.setTextColor(kWarmWhite);
    tft.setCursor(62, 117);
    tft.printf("%u", kTechniqueThresholds[next]);
    tft.setCursor(98, 117);
    tft.printf("%u", kTechniqueCosts[next]);
  }

  drawFooterHints("修炼", "返回");
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
  if (feedback == Feedback::MoodUp || feedback == Feedback::StaminaUp) {
    startPetEffect(PetEffect::Interaction, millis());
  }
  dirty_ = true;
}

void GameUi::startPetEffect(PetEffect effect, uint32_t now) {
  petEffect_ = effect;
  petEffectStartedAt_ = now;
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

PetForm GameUi::displayForm(PetForm savedForm) const {
  return previewEnabled_ ? previewForm_ : savedForm;
}
