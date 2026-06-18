#include "button_scanner.h"

void ButtonScanner::begin() {
  for (size_t i = 0; i < board::kButtonCount; ++i) {
    pinMode(board::kButtonPins[i], INPUT);
    const bool level = digitalRead(board::kButtonPins[i]);
    states_[i] = {
        level,
        level == activeLevel_,
        false,
        millis(),
        0,
        ButtonEvent::None,
    };
  }
}

void ButtonScanner::update(uint32_t now) {
  for (size_t i = 0; i < board::kButtonCount; ++i) {
    ButtonState& state = states_[i];
    state.event = ButtonEvent::None;
    const bool raw = digitalRead(board::kButtonPins[i]);

    if (raw != state.rawLevel) {
      state.rawLevel = raw;
      state.changedAt = now;
    }

    const bool nextPressed = raw == activeLevel_;
    if (nextPressed != state.pressed &&
        now - state.changedAt >= kDebounceMs) {
      state.pressed = nextPressed;
      state.longSent = false;
      if (nextPressed) {
        state.pressedAt = now;
        state.event = ButtonEvent::Pressed;
      } else {
        state.event = ButtonEvent::Released;
      }
    }

    if (state.pressed && !state.longSent &&
        now - state.pressedAt >= kLongPressMs) {
      state.longSent = true;
      state.event = ButtonEvent::LongPressed;
    }
  }
}

const ButtonState& ButtonScanner::state(size_t index) const {
  return states_[index];
}

void ButtonScanner::setActiveLevel(bool activeLevel) {
  activeLevel_ = activeLevel;
}

bool ButtonScanner::activeLevel() const {
  return activeLevel_;
}

