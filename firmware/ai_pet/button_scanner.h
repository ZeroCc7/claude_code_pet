#pragma once

#include <Arduino.h>

#include "board_config.h"

enum class ButtonEvent : uint8_t {
  None,
  Pressed,
  Released,
  LongPressed,
};

struct ButtonState {
  bool rawLevel;
  bool pressed;
  bool longSent;
  uint32_t changedAt;
  uint32_t pressedAt;
  ButtonEvent event;
};

class ButtonScanner {
 public:
  void begin();
  void update(uint32_t now);
  const ButtonState& state(size_t index) const;
  void setActiveLevel(bool activeLevel);
  bool activeLevel() const;

 private:
  static constexpr uint32_t kDebounceMs = 25;
  static constexpr uint32_t kLongPressMs = 800;

  bool activeLevel_ = LOW;
  ButtonState states_[board::kButtonCount]{};
};

