#pragma once

#include "display_device.h"
#include "ai_event_protocol.h"
#include "chinese_text.h"
#include "game_state.h"
#include "input_actions.h"
#include "pet_renderer.h"
#include "assets/home_ui_icons.h"

class GameUi {
 public:
  void begin(DisplayDevice& display);
  void handle(InputAction action, GameState& state);
  void draw(const GameState& state, uint32_t now, bool force = false);
  void notify(const char* message);
  void showAiStatus(const char* source, AiWorkState state,
                    const char* taskId, uint32_t now);
  void showAiResult(const char* source, bool success,
                    uint16_t experienceGain, uint16_t coinGain,
                    bool evolved, uint32_t now);
  void showEvolution(PetForm form, uint32_t now);
  UiPage page() const;

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
  void drawMenuFrame(const PetSaveData& data);
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
  void drawResourceBadge(int16_t x, int16_t y, uint16_t color,
                         const char* label, uint16_t value,
                         uint16_t maximum = 0);
  void restoreBackgroundRect(int16_t x, int16_t y, int16_t width,
                             int16_t height);
  void drawCare(const PetSaveData& data);
  void drawAdventure(const PetSaveData& data);
  void drawBattle(const PetSaveData& data);
  void drawStatus(const PetSaveData& data);
  void drawCultivation(const PetSaveData& data, uint32_t now);
  void drawBar(int16_t x, int16_t y, uint8_t value, uint16_t color);
  void startFeedback(Feedback feedback);
  void drawFeedback(uint32_t now);
  Adafruit_GFX& target();
  ChineseText& text();

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
  bool dirty_ = true;
  uint32_t lastAnimationAt_ = 0;
  Feedback feedback_ = Feedback::None;
  uint32_t feedbackStartedAt_ = 0;
  uint32_t lastFeedbackFrameAt_ = 0;
  char notice_[24] = {};
  uint32_t noticeStartedAt_ = 0;
  AiWorkState aiState_ = AiWorkState::Idle;
  char aiSource_[16] = {};
  char aiTaskId_[64] = {};
  uint32_t aiTaskStartedAt_ = 0;
  uint32_t aiLastEventAt_ = 0;
  bool aiResultActive_ = false;
  bool aiResultSuccess_ = false;
  uint16_t aiExperienceGain_ = 0;
  uint16_t aiCoinGain_ = 0;
  bool aiEvolved_ = false;
};
