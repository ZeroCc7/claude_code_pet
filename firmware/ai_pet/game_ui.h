#pragma once

#include "display_device.h"
#include "chinese_text.h"
#include "game_state.h"
#include "input_actions.h"
#include "pet_renderer.h"

class GameUi {
 public:
  void begin(DisplayDevice& display);
  void handle(InputAction action, GameState& state);
  void draw(const GameState& state, uint32_t now, bool force = false);
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
  void drawInkBackground();
  void drawHome(const PetSaveData& data, uint32_t now);
  void drawHomePet(const PetSaveData& data, uint32_t now);
  void drawHomeStats(const PetSaveData& data);
  void restoreBackgroundRect(int16_t x, int16_t y, int16_t width,
                             int16_t height);
  void restoreHomeDynamicRegions();
  void drawCare(const PetSaveData& data);
  void drawAdventure(const PetSaveData& data);
  void drawBattle(const PetSaveData& data);
  void drawStatus(const PetSaveData& data);
  void drawBar(int16_t x, int16_t y, uint8_t value, uint16_t color);
  void startFeedback(Feedback feedback);
  void drawFeedback(uint32_t now);

  DisplayDevice* display_ = nullptr;
  PetRenderer pet_;
  ChineseText chinese_;
  UiPage page_ = UiPage::Home;
  uint8_t selection_ = 0;
  bool dirty_ = true;
  uint32_t lastAnimationAt_ = 0;
  Feedback feedback_ = Feedback::None;
  uint32_t feedbackStartedAt_ = 0;
  uint32_t lastFeedbackFrameAt_ = 0;
};
