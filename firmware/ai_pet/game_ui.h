#pragma once

#include "display_device.h"
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
  void drawHeader(const PetSaveData& data);
  void drawHome(const PetSaveData& data, uint32_t now);
  void drawCare(const PetSaveData& data);
  void drawAdventure(const PetSaveData& data);
  void drawStatus(const PetSaveData& data);
  void drawBar(int16_t x, int16_t y, uint8_t value, uint16_t color);

  DisplayDevice* display_ = nullptr;
  PetRenderer pet_;
  UiPage page_ = UiPage::Home;
  uint8_t selection_ = 0;
  bool dirty_ = true;
  uint32_t lastAnimationAt_ = 0;
};

