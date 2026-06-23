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
  MeritLog,
  Inventory,
  Adventure,
  Battle,
  Status,
  Cultivation,
};

enum class ItemType : uint8_t {
  SpiritHerb,
  RecoveryPill,
  AttackTalisman,
  GuardTalisman,
  QingyunToken,
};

struct InventoryData {
  uint16_t items[5];
};

struct AiTaskRecord {
  uint8_t source;
  uint16_t durationSeconds;
  uint16_t experienceReward;
  uint16_t coinReward;
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
  uint32_t recentTaskHashes[16];
  uint8_t recentTaskIndex;
  InventoryData inventory;
  AiTaskRecord aiTaskRecords[10];
  uint8_t aiTaskRecordIndex;
  uint8_t aiTaskRecordCount;
  uint32_t crc32;
};
