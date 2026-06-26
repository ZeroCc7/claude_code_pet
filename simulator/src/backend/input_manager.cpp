#include "input_manager.h"

bool InputManager::init() {
  return true;
}

bool InputManager::poll() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      quit_ = true;
    }
  }

  const uint8_t* keys = SDL_GetKeyboardState(nullptr);

  // Button 0 (Confirm): Z, Return, or Space
  states_[0] = keys[SDL_SCANCODE_Z] ||
               keys[SDL_SCANCODE_RETURN] ||
               keys[SDL_SCANCODE_SPACE];

  // Button 1 (Up): Up arrow or W
  states_[1] = keys[SDL_SCANCODE_UP] ||
               keys[SDL_SCANCODE_W];

  // Button 2 (Down): Down arrow or S
  states_[2] = keys[SDL_SCANCODE_DOWN] ||
               keys[SDL_SCANCODE_S];

  // Button 3 (Back): X or Backspace
  states_[3] = keys[SDL_SCANCODE_X] ||
               keys[SDL_SCANCODE_BACKSPACE];

  if (keys[SDL_SCANCODE_ESCAPE]) {
    quit_ = true;
  }

  return !quit_;
}

const bool* InputManager::getButtonStates() const {
  return states_;
}

bool InputManager::quitRequested() const {
  return quit_;
}

void InputManager::shutdown() {
}
