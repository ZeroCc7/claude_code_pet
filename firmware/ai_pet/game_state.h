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
  bool startQingyunAdventure();
  void stopQingyunAdventure();
  AdventureTick tickQingyunAdventure(uint32_t seed);
  void acknowledgeAdventureResult();
  bool startQingyunWolfBattle(bool useAttackTalisman,
                              bool useGuardTalisman);
  BattleResult tickQingyunWolfBattle(uint32_t seed);
  void retreatQingyunWolf();
  static uint16_t maxEnergy(PetForm form);
  static uint16_t experienceForLevel(uint8_t level);
  uint8_t qingyunBossMaxHp() const;
  uint16_t qingyunDamagePercent() const;
  void gainExperience(uint16_t amount);
  void applyTask(const char* source, uint32_t durationSeconds, bool success,
                 bool halved = false);
  void completeAiTask(const char* source, uint32_t durationSeconds,
                      bool halved = false);
  bool tickRuntime(uint32_t seconds);

 private:
  void addItem(ItemType item);
  void addTendency(uint8_t slot, uint16_t amount);
  void autoResolveEvent(uint32_t seed);
  uint8_t qingyunAttackDamage(uint32_t seed) const;
  uint8_t qingyunIncomingDamage(uint32_t seed) const;
  uint16_t qingyunHealthPercent() const;
  uint16_t qingyunCompletionExperience() const;
  uint16_t qingyunCompletionCoins() const;
  uint8_t qingyunEventDamage(uint8_t baseDamage) const;
  void resetQingyunRun();
  void grantQingyunItems(uint32_t seed);
  void rollQingyunSword(uint32_t seed);
  void finishQingyunBattle(BattleResult result, bool resetHp = true);
  void updateEvolution();
  static uint8_t clampPercent(uint16_t value);
  PetSaveData data_{};
};
