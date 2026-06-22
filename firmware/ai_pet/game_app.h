#pragma once

#include "button_scanner.h"
#include "ai_event_protocol.h"
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
  void processSerial(uint32_t now);
  void printStatus();
  void processPreviewCommand(const String& command, uint32_t now);
  void processAiEvent(const AiEvent& event, uint32_t now);
  void printAck(const AiEvent& event, const char* status,
                uint16_t experience = 0, uint16_t coins = 0);
  void requestSave();

  DisplayDevice display_;
  ButtonScanner buttons_;
  GameState state_;
  SaveStore saves_;
  GameUi ui_;
  String serialCommand_;
  bool serialOverflow_ = false;
  uint32_t lastTickAt_ = 0;
  uint32_t lastSaveAt_ = 0;
  uint32_t lastExplorationAt_ = 0;
  bool savePending_ = false;
  bool fsReady_ = false;
};
