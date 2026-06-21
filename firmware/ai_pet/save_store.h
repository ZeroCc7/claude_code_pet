#pragma once

#include "game_state.h"

class SaveStore {
 public:
  bool begin();
  bool load(GameState& state);
  bool save(GameState& state);
  static uint32_t crc32(const uint8_t* data, size_t length);

 private:
  bool readSlot(const char* path, PetSaveData& data,
                bool* migrated = nullptr);
  bool writeSlot(const char* path, const PetSaveData& data);
  bool valid(const PetSaveData& data) const;
  uint32_t currentSequence_ = 0;
  bool nextSlotA_ = true;
};
