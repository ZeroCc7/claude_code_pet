#pragma once

#include <Arduino.h>

enum class PetForm : uint8_t {
  Egg,
  RookieA,
  RookieB,
  FinalA1,
  FinalA2,
  FinalB1,
  FinalB2,
};

enum class UiPage : uint8_t {
  Home,
  Care,
  Adventure,
  Battle,
  Status,
  Cultivation,
};

enum class MeditationResult : uint8_t {
  Restored,
  Full,
  Exhausted,
};

struct PetSaveData {
  uint32_t magic;
  uint16_t version;
  uint16_t size;
  uint32_t sequence;
  PetForm form;
  uint8_t level;
  uint16_t experience;
  uint8_t mood;
  uint8_t stamina;
  uint16_t coins;
  uint16_t energy;
  uint16_t tendencies[4];
  uint8_t regionProgress[3];
  uint8_t bossDefeatedMask;
  uint8_t bossWins[3];
  uint8_t activeRegion;
  uint8_t battleRegion;
  uint8_t bossHp;
  uint8_t bossMaxHp;
  uint8_t inBattle;
  uint32_t playSeconds;
  uint16_t energyRecoverySeconds;
  uint32_t meditationCycleSeconds;
  uint8_t meditationsUsed;
  uint32_t recentTaskHashes[16];
  uint8_t recentTaskIndex;
  uint32_t crc32;
};
