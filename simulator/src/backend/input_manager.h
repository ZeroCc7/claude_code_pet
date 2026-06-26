#pragma once

#include <SDL2/SDL.h>

class InputManager {
 public:
  bool init();
  bool poll();
  const bool* getButtonStates() const;
  bool quitRequested() const;
  void shutdown();

 private:
  bool states_[4] = {};
  bool quit_ = false;
};
