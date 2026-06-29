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
  uint8_t techniqueLevel(uint8_t index) const;
  bool upgradeTechnique(uint8_t index);
  bool isRegionUnlocked(uint8_t regionId) const;
  bool tryUnlockRegion(uint8_t regionId);
  void selectRegion(uint8_t regionId);
  uint8_t activeRegion() const;
  bool canUseRegionTokenForBoss() const;
  bool useRegionTokenForBoss();
  bool startAdventure();
  void stopAdventure();
  AdventureTick tickAdventure(uint32_t seed);
  void acknowledgeAdventureResult();
  bool startBossBattle(bool useAttackTalisman, bool useGuardTalisman);
  BattleResult tickBossBattle(uint32_t seed);
  void retreatBoss();
  static uint16_t maxEnergy(PetForm form);
  static uint16_t maxEnergy(PetForm form, const uint8_t techniqueLevels[4]);
  static uint16_t experienceForLevel(uint8_t level);
  uint16_t bossMaxHp() const;
  uint16_t recoveryIntervalSeconds() const;
  uint8_t bossEnergyRequirement() const;
  uint16_t damagePercent() const;
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
  uint8_t attackDamage(uint32_t seed) const;
  uint8_t incomingDamage(uint32_t seed) const;
  uint16_t healthPercent() const;
  uint16_t completionExperience() const;
  uint16_t completionCoins() const;
  uint8_t eventDamage(uint8_t baseDamage) const;
  void resetRun();
  void grantItems(uint32_t seed);
  void rollTreasure(uint32_t seed);
  void finishBattle(BattleResult result, bool resetHp = true);
  void applyTreasureBonus(uint8_t regionId);
  void updateEvolution();
  static uint8_t clampPercent(uint16_t value);
  PetSaveData data_{};
  uint8_t battleShield_ = 0;
};
