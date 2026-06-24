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

enum class AdventurePhase : uint8_t {
  Idle,
  Advancing,
  Choosing,
  Result,
  BossReady,
};

enum class AdventureTick : uint8_t {
  Inactive,
  Advanced,
  EventTriggered,
  WaitingForChoice,
  EnergyDepleted,
  BossUnlocked,
};

enum class QingyunEvent : uint8_t {
  None,
  SpiritHerb,
  DemonBeast,
  WoundedCultivator,
  Shortcut,
};

enum class EventResult : uint8_t {
  None,
  Continued,
  ItemGained,
  RewardGained,
  ProgressGained,
  StaminaLost,
};

enum class BattleResult : uint8_t {
  Inactive,
  Continue,
  Victory,
  Defeat,
  EnergyDepleted,
  Retreated,
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
  uint8_t qingyunProgress;
  uint8_t qingyunEventMask;
  uint8_t qingyunBossUnlocked;
  AdventurePhase adventurePhase;
  QingyunEvent currentEvent;
  EventResult currentEventResult;
  uint8_t qingyunBossWins;
  uint8_t qingyunBossDefeated;
  uint16_t qingyunRound;
  uint8_t qingyunMisses;
  uint8_t hasQingyunSword;
  uint8_t bossHp;
  uint8_t bossMaxHp;
  uint8_t inBattle;
  uint8_t battleRound;
  uint8_t battleAttackTalisman;
  uint8_t battleGuardTalisman;
  BattleResult lastBattleResult;
  uint32_t playSeconds;
  uint16_t energyRecoverySeconds;
  uint16_t staminaRecoverySeconds;
  uint16_t lastQingyunExperience;
  uint16_t lastQingyunCoins;
  uint8_t lastQingyunItems[4];
  uint8_t lastQingyunSword;
  InventoryData inventory;
  AiTaskRecord aiTaskRecords[10];
  uint8_t aiTaskRecordIndex;
  uint8_t aiTaskRecordCount;
  uint32_t crc32;
};
