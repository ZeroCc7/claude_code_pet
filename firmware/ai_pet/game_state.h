#pragma once

#include "game_types.h"

class GameState {
 public:
  GameState();

  void reset();
  void load(const PetSaveData& data);
  const PetSaveData& data() const;
  PetSaveData& mutableData();

  bool useItem(ItemType item);
  void interact();
  bool feed();
  bool startExploration(uint8_t region);
  bool regionUnlocked(uint8_t region) const;
  bool tickExploration(uint32_t seed);
  bool startBoss(uint8_t region);
  bool battleAction(uint8_t action);
  void gainExperience(uint16_t amount);
  void applyTask(uint32_t durationSeconds, bool success, bool halved = false);
  bool hasProcessedTask(const char* source, const char* taskId) const;
  bool applyAiTask(const char* source, const char* taskId,
                   uint32_t durationSeconds, bool success, bool halved = false);
  bool tickRuntime(uint32_t seconds);
  MeditationResult meditate();

 private:
  void updateEvolution();
  static uint32_t taskHash(const char* source, const char* taskId);
  static uint8_t clampPercent(uint16_t value);
  PetSaveData data_{};
};
