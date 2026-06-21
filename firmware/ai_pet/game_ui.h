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
  void restoreBackgroundRect(int16_t x, int16_t y, int16_t width,
                             int16_t height);
  void drawCare(const PetSaveData& data);
  void drawAdventure(const PetSaveData& data);
  void drawBattle(const PetSaveData& data);
  void drawStatus(const PetSaveData& data);
  void drawBar(int16_t x, int16_t y, uint8_t value, uint16_t color);
  void startFeedback(Feedback feedback);
  void drawFeedback(uint32_t now);
  Adafruit_GFX& target();
  ChineseText& text();

  DisplayDevice* display_ = nullptr;
  PetRenderer pet_;
  GFXcanvas16 petCanvas_{56, 50};
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
};
