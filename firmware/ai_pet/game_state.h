#pragma once

#include "game_types.h"

class GameState {
 public:
  GameState();

  void reset();
  void load(const PetSaveData& data);
  const PetSaveData& data() const;
  PetSaveData& mutableData();

  void interact();
  bool feed();
  bool startExploration(uint8_t region);
  void applyTask(uint32_t durationSeconds, bool success);

 private:
  static uint8_t clampPercent(uint16_t value);
  PetSaveData data_{};
};

