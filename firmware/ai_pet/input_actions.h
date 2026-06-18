#pragma once

#include "button_scanner.h"

enum class InputAction : uint8_t {
  None,
  Confirm,
  Up,
  Down,
  Back,
};

inline InputAction actionForButton(size_t index, ButtonEvent event) {
  if (event != ButtonEvent::Pressed) {
    return InputAction::None;
  }
  switch (index) {
    case 0:
      return InputAction::Confirm;
    case 1:
      return InputAction::Up;
    case 2:
      return InputAction::Down;
    case 3:
      return InputAction::Back;
    default:
      return InputAction::None;
  }
}

