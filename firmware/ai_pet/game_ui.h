#pragma once

#include "display_device.h"
#include "ai_event_protocol.h"
#include "chinese_text.h"
#include "game_state.h"
#include "input_actions.h"
#include "pet_renderer.h"
#include "assets/home_ui_icons.h"
#include "assets/home_button_icons.h"
#include "assets/qingyun_scene.h"
#include "assets/qingyun_ui_icons.h"
#include "assets/qingyun_pets.h"
#include "assets/qingyun_boss.h"

class GameUi {
 public:
  void begin(DisplayDevice& display);
  void handle(InputAction action, GameState& state);
  void draw(const GameState& state, uint32_t now, bool force = false);
  void notify(const char* message);
  void showAiActive(const char* source, uint32_t now);
  void showAiResult(const char* source, uint16_t experienceGain,
                    uint16_t coinGain,
                    bool evolved, uint32_t now);
  void showEvolution(PetForm form, uint32_t now);
  void setPreviewForm(PetForm form);
  void clearPreviewForm();
  bool previewEnabled() const;
  PetForm previewForm() const;
  UiPage page() const;
  bool aiResultActive() const { return aiResultActive_; }
  const char* aiSource() const { return aiSource_; }
  uint32_t aiLastEventAt() const { return aiLastEventAt_; }
  void clearAiCultivation(uint32_t now);
  bool consumeCultivationExit() {
    const bool v = cultivationExitRequested_;
    cultivationExitRequested_ = false;
    return v;
  }

 private:
  enum class Feedback : uint8_t {
    None,
    MoodUp,
    StaminaUp,
    NoCoins,
    AlreadyFull,
  };

  void drawHeader(const PetSaveData& data);
  void drawHomeHeader(const PetSaveData& data);
  void drawInkBackground(int16_t fillStartY = 112);
  void drawHomeFrame(const PetSaveData& data, uint32_t now);
  void drawMenuFrame(const GameState& state);
  void drawTitlePlaque(const char* title, uint16_t accent);
  void drawPanel(int16_t x, int16_t y, int16_t width, int16_t height,
                 bool selected);
  void drawFooterHints(const char* left, const char* right);
  void drawProgressBar(int16_t x, int16_t y, int16_t width, uint16_t value,
                       uint16_t maximum, uint16_t color);
  void startNotice(const char* message);
  void drawNotice();
  void drawHome(const PetSaveData& data, uint32_t now);
  void drawHomePet(const PetSaveData& data, uint32_t now);
  void drawHomeStats(const PetSaveData& data);
  void drawHomeVitals(const PetSaveData& data);
  void drawGoldPanel(int16_t x, int16_t y, int16_t width, int16_t height);
  void drawHomeIcon(int16_t x, int16_t y, const HomeUiIcon& icon);
  void drawButtonIcon(int16_t x, int16_t y,
                      const HomeButtonIcon& icon);
  void drawQingyunIcon(int16_t x, int16_t y,
                       const QingyunUiIcon& icon);
  void drawQingyunIconLarge(int16_t x, int16_t y,
                            const QingyunUiIcon& icon);
  void drawQingyunPet(PetForm form, int16_t x, int16_t y);
  void drawQingyunPetLarge(PetForm form, int16_t x, int16_t y);
  void drawResourceBadge(int16_t x, int16_t y, uint16_t color,
                         const char* label, uint16_t value,
                         uint16_t maximum = 0);
  void restoreBackgroundRect(int16_t x, int16_t y, int16_t width,
                             int16_t height);
  void drawMeritLog(const PetSaveData& data);
  void drawInventory(const PetSaveData& data);
  void drawTreasureInventory(const PetSaveData& data);
  void drawAdventure(const PetSaveData& data);
  void drawQingyunAdventure(const PetSaveData& data, uint32_t now);
  void drawQingyunEventResult(const PetSaveData& data);
  void drawQingyunBossPrompt(const GameState& state);
  void drawQingyunScene(const PetSaveData& data, uint32_t now);
  void drawQingyunEventSubject(QingyunEvent event);
  void drawBattle(const GameState& state);
  void drawStatus(const PetSaveData& data);
  void drawCultivation(const PetSaveData& data, uint32_t now);
  void drawBar(int16_t x, int16_t y, uint8_t value, uint16_t color);
  void startPetEffect(PetEffect effect, uint32_t now);
  void startFeedback(Feedback feedback);
  void drawFeedback(uint32_t now);
  Adafruit_GFX& target();
  ChineseText& text();
  PetForm displayForm(PetForm savedForm) const;

  DisplayDevice* display_ = nullptr;
  PetRenderer pet_;
  GFXcanvas16 petCanvas_{72, 100};
  GFXcanvas16 menuCanvas_{128, 160};
  ChineseText chinese_;
  ChineseText menuChinese_;
  Adafruit_GFX* renderTarget_ = nullptr;
  ChineseText* renderText_ = nullptr;
  UiPage page_ = UiPage::Home;
  uint8_t selection_ = 0;
  uint8_t inventoryTab_ = 0;
  uint8_t meritPage_ = 0;
  uint8_t statusPage_ = 0;
  bool battlePrompt_ = true;
  bool useAttackTalisman_ = false;
  bool useGuardTalisman_ = false;
  bool dirty_ = true;
  uint32_t lastAnimationAt_ = 0;
  Feedback feedback_ = Feedback::None;
  uint32_t feedbackStartedAt_ = 0;
  uint32_t lastFeedbackFrameAt_ = 0;
  PetEffect petEffect_ = PetEffect::None;
  uint32_t petEffectStartedAt_ = 0;
  char notice_[24] = {};
  uint32_t noticeStartedAt_ = 0;
  bool aiActive_ = false;
  char aiSource_[16] = {};
  uint32_t aiTaskStartedAt_ = 0;
  uint32_t aiLastEventAt_ = 0;
  bool aiResultActive_ = false;
  bool aiResultSuccess_ = false;
  uint16_t aiExperienceGain_ = 0;
  uint16_t aiCoinGain_ = 0;
  bool aiEvolved_ = false;
  bool cultivationExitRequested_ = false;
  bool previewEnabled_ = false;
  PetForm previewForm_ = PetForm::Egg;
};
