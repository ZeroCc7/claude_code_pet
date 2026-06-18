#pragma once

#include "button_scanner.h"
#include "display_device.h"
#include "game_state.h"
#include "game_ui.h"
#include "save_store.h"

class GameApp {
 public:
  void begin();
  void update(uint32_t now);

 private:
  void processInput(uint32_t now);
  void processSerial();
  void printStatus();
  void requestSave();

  DisplayDevice display_;
  ButtonScanner buttons_;
  GameState state_;
  SaveStore saves_;
  GameUi ui_;
  String serialCommand_;
  uint32_t lastTickAt_ = 0;
  uint32_t lastSaveAt_ = 0;
  bool savePending_ = false;
};

