#include "game_state.h"

#include <cstring>

#include "region_config.h"

namespace {

constexpr uint32_t kSaveMagic = 0x50455431;
constexpr uint16_t kSaveVersion = 10;
constexpr uint16_t kPassiveRecoverySeconds = 300;
constexpr uint8_t TECHNIQUE_MAX_LEVEL = 9;
constexpr uint16_t kTechniqueThresholds[TECHNIQUE_MAX_LEVEL] = {
    0, 10, 22, 38, 58, 82, 112, 148, 190};
constexpr uint16_t kTechniqueCosts[TECHNIQUE_MAX_LEVEL] = {
    50, 90, 150, 240, 360, 520, 720, 960, 1250};
constexpr uint8_t kTechniqueSword = 0;
constexpr uint8_t kTechniqueDan = 1;
constexpr uint8_t kTechniqueBody = 2;
constexpr uint8_t kTechniqueSpirit = 3;

}  // namespace

GameState::GameState() {
  reset();
}

void GameState::reset() {
  memset(&data_, 0, sizeof(data_));
  battleShield_ = 0;
  battleLogType_ = BattleLogType::None;
  battleLogValue_ = 0;
  data_.magic = kSaveMagic;
  data_.version = kSaveVersion;
  data_.size = sizeof(PetSaveData);
  data_.form = PetForm::Egg;
  data_.level = 1;
  data_.mood = 70;
  data_.stamina = 80;
  data_.coins = 30;
  data_.energy = 10;
  data_.regionsUnlocked = 0x01;
  for (uint8_t i = 0; i < kRegionCount; ++i) {
    data_.regionRound[i] = 1;
  }
  data_.adventureEventOrder = 0xE4;
  data_.adventurePhase = AdventurePhase::Idle;
  data_.currentEvent = AdventureEvent::None;
  data_.currentEventResult = EventResult::None;
  data_.lastBattleResult = BattleResult::Inactive;
}

void GameState::load(const PetSaveData& data) {
  const uint8_t savedBattleShield = battleShield_;
  const BattleLogType savedBattleLogType = battleLogType_;
  const int16_t savedBattleLogValue = battleLogValue_;
  data_ = data;
  battleShield_ = savedBattleShield;
  battleLogType_ = savedBattleLogType;
  battleLogValue_ = savedBattleLogValue;
  data_.mood = clampPercent(data_.mood);
  data_.stamina = clampPercent(data_.stamina);
  data_.energy = min<uint16_t>(maxEnergy(data_.form, data_.techniqueLevels),
                               data_.energy);
  for (uint8_t i = 0; i < kRegionCount; ++i) {
    data_.regionRound[i] = max<uint16_t>(1, data_.regionRound[i]);
  }
  if (data_.activeRegion >= kRegionCount) {
    data_.activeRegion = 0;
  }
}

const PetSaveData& GameState::data() const {
  return data_;
}

PetSaveData& GameState::mutableData() {
  return data_;
}

bool GameState::useItem(ItemType item) {
  const uint8_t index = static_cast<uint8_t>(item);
  uint16_t& quantity = data_.inventory.items[index];
  if (quantity == 0) {
    return false;
  }
  if (item == ItemType::SpiritHerb) {
    const uint16_t cap = maxEnergy(data_.form, data_.techniqueLevels);
    if (data_.energy >= cap) {
      return false;
    }
    const uint8_t restore =
        3 + (data_.techniqueLevels[kTechniqueDan] >= 1 ? 1 : 0) +
        (data_.techniqueLevels[kTechniqueDan] >= 6 ? 1 : 0);
    data_.energy = min<uint16_t>(cap, data_.energy + restore);
  } else if (item == ItemType::RecoveryPill) {
    if (data_.stamina >= 100) {
      return false;
    }
    const uint8_t restore =
        20 + (data_.techniqueLevels[kTechniqueDan] >= 2 ? 5 : 0) +
        (data_.techniqueLevels[kTechniqueDan] >= 7 ? 5 : 0);
    data_.stamina = clampPercent(data_.stamina + restore);
  } else {
    return false;
  }
  quantity--;
  return true;
}

uint8_t GameState::techniqueLevel(uint8_t index) const {
  return index < 4 ? data_.techniqueLevels[index] : 0;
}

bool GameState::upgradeTechnique(uint8_t index) {
  if (index >= 4) {
    return false;
  }
  const uint8_t level = data_.techniqueLevels[index];
  if (level >= TECHNIQUE_MAX_LEVEL) {
    return false;
  }
  const uint16_t threshold = kTechniqueThresholds[level];
  const uint16_t cost = kTechniqueCosts[level];
  if (data_.tendencies[index] < threshold || data_.coins < cost) {
    return false;
  }
  data_.coins -= cost;
  data_.techniqueLevels[index] = level + 1;
  data_.energy = min<uint16_t>(data_.energy,
                               maxEnergy(data_.form, data_.techniqueLevels));
  return true;
}

bool GameState::isRegionUnlocked(uint8_t regionId) const {
  if (regionId >= kRegionCount) return false;
  return (data_.regionsUnlocked & (1U << regionId)) != 0;
}

bool GameState::tryUnlockRegion(uint8_t regionId) {
  if (regionId >= kRegionCount || regionId == 0) return false;
  if (isRegionUnlocked(regionId)) return false;
  const RegionConfig& cfg = kRegions[regionId];
  if (data_.level < cfg.unlock_level) return false;
  if (data_.coins < cfg.unlock_coins) return false;
  if (!isRegionUnlocked(cfg.prerequisite)) return false;
  if (data_.regionBossWins[cfg.prerequisite] == 0) return false;
  data_.coins -= cfg.unlock_coins;
  data_.regionsUnlocked |= (1U << regionId);
  return true;
}

void GameState::selectRegion(uint8_t regionId) {
  if (regionId < kRegionCount && isRegionUnlocked(regionId)) {
    data_.activeRegion = regionId;
  }
}

uint8_t GameState::activeRegion() const {
  return data_.activeRegion;
}

bool GameState::canUseRegionTokenForBoss() const {
  return data_.inventory.items[static_cast<uint8_t>(ItemType::RegionToken)] >
             0 &&
         data_.energy >= bossEnergyRequirement();
}

bool GameState::useRegionTokenForBoss() {
  if (!canUseRegionTokenForBoss()) {
    return false;
  }
  data_.inventory.items[static_cast<uint8_t>(ItemType::RegionToken)]--;
  data_.adventureProgress = 100;
  data_.bossUnlocked = 1;
  data_.adventurePhase = AdventurePhase::BossReady;
  data_.currentEvent = AdventureEvent::None;
  data_.currentEventResult = EventResult::None;
  return true;
}

bool GameState::startAdventure() {
  if (data_.adventurePhase == AdventurePhase::BossReady) {
    return false;
  }
  if (data_.adventurePhase != AdventurePhase::Idle || data_.energy < 3) {
    return false;
  }
  data_.energy -= 3;
  data_.adventurePhase = AdventurePhase::Advancing;
  data_.currentEvent = AdventureEvent::None;
  data_.currentEventResult = EventResult::None;

  uint8_t perm[4] = {0, 1, 2, 3};
  uint32_t s = millis();
  for (uint8_t i = 3; i > 0; --i) {
    s = s * 1103515245 + 12345;
    uint8_t j = (s >> 16) % (i + 1);
    uint8_t tmp = perm[i];
    perm[i] = perm[j];
    perm[j] = tmp;
  }
  data_.adventureEventOrder =
      (perm[0]) | (perm[1] << 2) | (perm[2] << 4) | (perm[3] << 6);

  return true;
}

void GameState::stopAdventure() {
  data_.adventurePhase = data_.adventureProgress >= 100
                             ? AdventurePhase::BossReady
                             : AdventurePhase::Idle;
  data_.currentEvent = AdventureEvent::None;
  data_.currentEventResult = EventResult::None;
}

AdventureTick GameState::tickAdventure(uint32_t seed) {
  if (data_.adventurePhase == AdventurePhase::Result) {
    return AdventureTick::Inactive;
  }
  if (data_.adventurePhase != AdventurePhase::Advancing) {
    return AdventureTick::Inactive;
  }
  const bool skipEnergyCost =
      data_.techniqueLevels[kTechniqueSpirit] >= 6 && seed % 100 < 10;
  if (!skipEnergyCost) {
    data_.energy--;
  }
  data_.adventureProgress =
      min<uint8_t>(100, data_.adventureProgress + 2);

  constexpr uint8_t checkpoints[] = {25, 45, 65, 85};
  constexpr AdventureEvent events[] = {
      AdventureEvent::Gather, AdventureEvent::Fight,
      AdventureEvent::Npc, AdventureEvent::Shortcut};
  for (uint8_t index = 0; index < 4; ++index) {
    const uint8_t eventBit = 1U << index;
    if (data_.adventureProgress >= checkpoints[index] &&
        !(data_.adventureEventMask & eventBit)) {
      data_.adventureEventMask |= eventBit;
      const uint8_t eventIdx =
          (data_.adventureEventOrder >> (index * 2)) & 0x03;
      data_.currentEvent = events[eventIdx];
      autoResolveEvent(seed);
      data_.adventurePhase = AdventurePhase::Result;
      return AdventureTick::EventTriggered;
    }
  }
  if (data_.adventureProgress >= 100) {
    data_.bossUnlocked = 1;
    data_.adventurePhase = AdventurePhase::BossReady;
    return AdventureTick::BossUnlocked;
  }
  if (data_.energy == 0) {
    stopAdventure();
    return AdventureTick::EnergyDepleted;
  }
  return AdventureTick::Advanced;
}

void GameState::autoResolveEvent(uint32_t seed) {
  EventResult result = EventResult::Continued;
  if (data_.currentEvent == AdventureEvent::Gather) {
    addItem(ItemType::SpiritHerb);
    addTendency(3, 1);
    result = EventResult::ItemGained;
  } else if (data_.currentEvent == AdventureEvent::Fight) {
    const uint16_t score =
        data_.level + data_.stamina / 10 + data_.tendencies[0] / 4 +
        seed % 10;
    if (score >= 15) {
      addTendency(0, 2);
      gainExperience(6);
      data_.coins += 5;
      result = EventResult::RewardGained;
    } else {
      const uint8_t damage = eventDamage(12);
      data_.stamina = data_.stamina > damage ? data_.stamina - damage : 0;
      result = EventResult::StaminaLost;
    }
  } else if (data_.currentEvent == AdventureEvent::Npc) {
    const uint16_t score =
        data_.level + data_.stamina / 10 + data_.tendencies[3] / 2 +
        seed % 10;
    if (score >= 22) {
      addItem(ItemType::RegionToken);
    } else {
      addItem(ItemType::RecoveryPill);
    }
    addTendency(3, 2);
    result = EventResult::ItemGained;
  } else if (data_.currentEvent == AdventureEvent::Shortcut) {
    const uint16_t score =
        data_.level + data_.energy + data_.tendencies[1] / 3 +
        seed % 10;
    if (score >= 17) {
      addTendency(1, 2);
      data_.adventureProgress =
          min<uint8_t>(100, data_.adventureProgress + 8 + seed % 8);
      result = EventResult::ProgressGained;
    } else {
      const uint8_t damage = eventDamage(10);
      data_.stamina = data_.stamina > damage ? data_.stamina - damage : 0;
      result = EventResult::StaminaLost;
    }
  }
  data_.currentEventResult = result;
}

void GameState::acknowledgeAdventureResult() {
  if (data_.adventurePhase != AdventurePhase::Result) {
    return;
  }
  data_.currentEvent = AdventureEvent::None;
  data_.currentEventResult = EventResult::None;
  if (data_.adventureProgress >= 100) {
    data_.bossUnlocked = 1;
  }
  data_.adventurePhase = data_.adventureProgress >= 100
                             ? AdventurePhase::BossReady
                             : AdventurePhase::Advancing;
}

void GameState::addItem(ItemType item) {
  uint16_t& quantity = data_.inventory.items[static_cast<uint8_t>(item)];
  if (quantity < 0xFFFFU) {
    quantity++;
  }
}

void GameState::addTendency(uint8_t slot, uint16_t amount) {
  if (slot < 4) {
    data_.tendencies[slot] =
        min<uint16_t>(100, data_.tendencies[slot] + amount);
  }
}

bool GameState::startBossBattle(bool useAttackTalisman,
                                bool useGuardTalisman) {
  if (data_.inBattle || !data_.bossUnlocked ||
      data_.adventureProgress < 100 || data_.energy < bossEnergyRequirement()) {
    return false;
  }
  data_.inBattle = 1;
  data_.bossMaxHp = bossMaxHp();
  data_.bossHp = data_.bossMaxHp;
  data_.battleRound = 0;
  data_.lastBattleResult = BattleResult::Continue;
  battleLogType_ = BattleLogType::None;
  battleLogValue_ = 0;
  uint16_t& attackQuantity =
      data_.inventory.items[static_cast<uint8_t>(ItemType::AttackTalisman)];
  uint16_t& guardQuantity =
      data_.inventory.items[static_cast<uint8_t>(ItemType::GuardTalisman)];
  data_.battleAttackTalisman = useAttackTalisman && attackQuantity > 0;
  data_.battleGuardTalisman = useGuardTalisman && guardQuantity > 0;
  if (data_.battleAttackTalisman) {
    attackQuantity--;
  }
  if (data_.battleGuardTalisman) {
    guardQuantity--;
  }
  const uint8_t bodyLevel = data_.techniqueLevels[kTechniqueBody];
  battleShield_ = (bodyLevel >= 2 ? 3 : 0) + (bodyLevel >= 5 ? 4 : 0) +
                  (bodyLevel >= 8 ? 5 : 0);
  return true;
}

uint8_t GameState::attackDamage(uint32_t seed) const {
  uint16_t damage =
      6 + data_.level + min<uint16_t>(10, data_.tendencies[0] * 3 / 8) +
      min<uint16_t>(8, data_.tendencies[1] / 3);
  if (data_.form == PetForm::RookieA || data_.form == PetForm::FinalA1) {
    damage++;
  } else if (data_.form == PetForm::FinalA2) {
    damage += 2;
  }
  const uint8_t swordLevel = data_.techniqueLevels[kTechniqueSword];
  const uint8_t swordBonus = (swordLevel >= 1 ? 3 : 0) +
                             (swordLevel >= 3 ? 3 : 0) +
                             (swordLevel >= 5 ? 4 : 0) +
                             (swordLevel >= 7 ? 5 : 0);
  damage = damage * (100 + swordBonus) / 100;
  uint8_t criticalRate =
      5 + min<uint16_t>(20, data_.tendencies[0] / 3) +
      (swordLevel >= 2 ? 2 : 0) + (swordLevel >= 6 ? 3 : 0);
  if (data_.regionTreasure[2]) {
    criticalRate = min<uint8_t>(100, criticalRate + 15);
  }
  if (seed % 100 < criticalRate) {
    damage = damage * (swordLevel >= 9 ? 220 : 200) / 100;
  }
  if (data_.battleAttackTalisman) {
    damage = damage * 120 / 100;
  }
  if (data_.regionTreasure[0]) {
    damage = damage * 110 / 100;
  }
  const uint8_t pierce =
      (swordLevel >= 4 ? 1 : 0) + (swordLevel >= 8 ? 1 : 0);
  damage += pierce * 2;
  return min<uint16_t>(255, max<uint16_t>(1, damage));
}

uint8_t GameState::incomingDamage(uint32_t seed) const {
  const uint8_t bodyLevel = data_.techniqueLevels[kTechniqueBody];
  const uint8_t spiritLevel = data_.techniqueLevels[kTechniqueSpirit];
  uint8_t dodgeRate =
      5 + min<uint16_t>(20, data_.tendencies[2] / 3) +
      (bodyLevel >= 6 ? 3 : 0) + (spiritLevel >= 2 ? 2 : 0) +
      (spiritLevel >= 7 ? 3 : 0);
  if (data_.regionTreasure[1]) {
    dodgeRate = min<uint8_t>(100, dodgeRate + 10);
  }
  if ((seed / 100) % 100 < dodgeRate) {
    return 0;
  }
  uint16_t damage =
      max<int16_t>(
          1, kRegions[data_.activeRegion].base_boss_damage -
                 min<uint16_t>(5, data_.tendencies[2] / 6));
  if (data_.form == PetForm::RookieB || data_.form == PetForm::FinalB1) {
    damage = max<uint8_t>(1, damage - 1);
  } else if (data_.form == PetForm::FinalB2) {
    damage = max<uint8_t>(1, damage - 2);
  }
  if (data_.battleGuardTalisman) {
    damage = max<uint8_t>(1, damage * 80 / 100);
  }
  damage = max<uint16_t>(1, damage * damagePercent() / 100);
  if (data_.regionTreasure[0]) {
    damage = max<uint16_t>(1, damage * 90 / 100);
  }
  uint8_t bodyReduction = (bodyLevel >= 1 ? 3 : 0) +
                          (bodyLevel >= 4 ? 4 : 0) +
                          (bodyLevel >= 7 ? 5 : 0);
  if (data_.stamina < 20 && bodyLevel >= 9) {
    bodyReduction += 10;
  }
  if (bodyReduction > 0) {
    damage = max<uint16_t>(1, damage * (100 - bodyReduction) / 100);
  }
  return min<uint16_t>(255, damage);
}

BattleResult GameState::tickBossBattle(uint32_t seed) {
  if (!data_.inBattle) {
    return BattleResult::Inactive;
  }
  data_.energy--;
  data_.battleRound++;
  if (data_.energy == 0) {
    battleLogType_ = BattleLogType::EnergyDepleted;
    battleLogValue_ = 0;
    resetRun();
    finishBattle(BattleResult::EnergyDepleted);
    return BattleResult::EnergyDepleted;
  }
  const uint8_t swordLevel = data_.techniqueLevels[kTechniqueSword];
  uint8_t criticalRate =
      5 + min<uint16_t>(20, data_.tendencies[0] / 3) +
      (swordLevel >= 2 ? 2 : 0) + (swordLevel >= 6 ? 3 : 0);
  if (data_.regionTreasure[2]) {
    criticalRate = min<uint8_t>(100, criticalRate + 15);
  }
  const bool critical = seed % 100 < criticalRate;
  const uint8_t damage = attackDamage(seed);
  data_.bossHp = damage >= data_.bossHp ? 0 : data_.bossHp - damage;
  battleLogType_ = critical ? BattleLogType::PlayerCritical
                            : BattleLogType::PlayerHit;
  battleLogValue_ = damage;
  if (data_.bossHp == 0) {
    const uint8_t r = data_.activeRegion;
    data_.lastBossExperience = completionExperience();
    data_.lastBossCoins = completionCoins();
    grantItems(seed);
    rollTreasure(seed);
    if (data_.regionBossWins[r] < 0xFFU) {
      data_.regionBossWins[r]++;
    }
    data_.bossDefeated = 1;
    gainExperience(data_.lastBossExperience);
    data_.coins = min<uint32_t>(
        0xFFFFU, static_cast<uint32_t>(data_.coins) +
                     data_.lastBossCoins);
    if (data_.techniqueLevels[kTechniqueSpirit] >= 3) {
      data_.energy = min<uint16_t>(
          maxEnergy(data_.form, data_.techniqueLevels), data_.energy + 1);
    }
    if (data_.regionRound[r] < 0xFFFFU) {
      data_.regionRound[r]++;
    }
    const RegionConfig& cfg = kRegions[r];
    const uint16_t bossBonus =
        cfg.tendency_base + min<uint16_t>(3, data_.regionRound[r] / 5);
    for (uint8_t i = 0; i < 4; ++i) {
      addTendency(i, bossBonus + cfg.reward_bias[i]);
    }
    if (!data_.regionTreasure[4] && r == 4) {
    }
    battleLogType_ = BattleLogType::Victory;
    battleLogValue_ = 0;
    resetRun();
    finishBattle(BattleResult::Victory, false);
    return BattleResult::Victory;
  }
  uint8_t incoming = incomingDamage(seed);
  if (incoming == 0) {
    battleLogType_ = BattleLogType::BossMiss;
    battleLogValue_ = 0;
  }
  if (battleShield_ > 0) {
    const uint8_t blocked = min<uint8_t>(battleShield_, incoming);
    battleShield_ -= blocked;
    incoming -= blocked;
    if (blocked > 0) {
      battleLogType_ = BattleLogType::Shield;
      battleLogValue_ = blocked;
    }
  }
  if (incoming > 0) {
    battleLogType_ = BattleLogType::BossHit;
    battleLogValue_ = incoming;
  }
  data_.stamina =
      incoming >= data_.stamina ? 0 : data_.stamina - incoming;
  const uint8_t affinityRate =
      min<uint16_t>(20, data_.tendencies[3] / 3) +
      (data_.techniqueLevels[kTechniqueSpirit] >= 1 ? 3 : 0) +
      (data_.techniqueLevels[kTechniqueSpirit] >= 5 ? 4 : 0) +
      (data_.techniqueLevels[kTechniqueSpirit] >= 8 ? 3 : 0);
  if (data_.stamina > 0 && (seed / 10000) % 100 < affinityRate) {
    const uint8_t heal =
        2 + (data_.techniqueLevels[kTechniqueSpirit] >= 4 ? 1 : 0);
    data_.stamina = clampPercent(data_.stamina + heal);
    battleLogType_ = BattleLogType::Heal;
    battleLogValue_ = heal;
  }
  if (data_.stamina == 0) {
    data_.stamina = 30;
    battleLogType_ = BattleLogType::Defeat;
    battleLogValue_ = 0;
    resetRun();
    finishBattle(BattleResult::Defeat);
    return BattleResult::Defeat;
  }
  data_.lastBattleResult = BattleResult::Continue;
  return BattleResult::Continue;
}

void GameState::retreatBoss() {
  if (data_.inBattle) {
    resetRun();
    finishBattle(BattleResult::Retreated);
  }
}

BattleLogType GameState::battleLogType() const {
  return battleLogType_;
}

int16_t GameState::battleLogValue() const {
  return battleLogValue_;
}

uint16_t GameState::bossMaxHp() const {
  const uint8_t r = data_.activeRegion;
  return static_cast<uint32_t>(kRegions[r].base_boss_hp) * healthPercent() /
         100;
}

uint16_t GameState::recoveryIntervalSeconds() const {
  return data_.techniqueLevels[kTechniqueDan] >= 9 ? 180
                                                  : kPassiveRecoverySeconds;
}

uint8_t GameState::bossEnergyRequirement() const {
  return data_.techniqueLevels[kTechniqueDan] >= 4 ? 4 : 5;
}

uint16_t GameState::completionExperience() const {
  const uint8_t r = data_.activeRegion;
  const uint16_t round =
      min<uint16_t>(50, max<uint16_t>(1, data_.regionRound[r]));
  const uint16_t laterRounds = round > 10 ? round - 10 : 0;
  return kRegions[r].base_xp + min<uint16_t>(round, 10) + laterRounds / 5;
}

uint16_t GameState::completionCoins() const {
  const uint8_t r = data_.activeRegion;
  const uint16_t round =
      min<uint16_t>(50, max<uint16_t>(1, data_.regionRound[r]));
  const uint16_t laterRounds = round > 10 ? round - 10 : 0;
  return kRegions[r].base_coins + 2 * min<uint16_t>(round, 10) +
         laterRounds;
}

uint16_t GameState::healthPercent() const {
  const uint8_t r = data_.activeRegion;
  const RegionConfig& cfg = kRegions[r];
  const uint16_t round =
      min<uint16_t>(50, max<uint16_t>(1, data_.regionRound[r]));
  const uint16_t laterRounds = round > 10 ? round - 10 : 0;
  return 100 + min<uint16_t>(round - 1, 9) * cfg.hp_scale_first10 +
         laterRounds * cfg.hp_scale_later;
}

uint16_t GameState::damagePercent() const {
  const uint8_t r = data_.activeRegion;
  const RegionConfig& cfg = kRegions[r];
  const uint16_t round =
      min<uint16_t>(50, max<uint16_t>(1, data_.regionRound[r]));
  const uint16_t laterRounds = round > 10 ? round - 10 : 0;
  return 100 + min<uint16_t>(round - 1, 9) * cfg.dmg_scale_first10 +
         laterRounds * cfg.dmg_scale_later;
}

uint8_t GameState::eventDamage(uint8_t baseDamage) const {
  return min<uint16_t>(
      255, max<uint16_t>(1, baseDamage * damagePercent() / 100));
}

void GameState::resetRun() {
  data_.adventureProgress = 0;
  data_.adventureEventMask = 0;
  data_.adventureEventOrder = 0;
  data_.bossUnlocked = 0;
  data_.adventurePhase = AdventurePhase::Idle;
  data_.currentEvent = AdventureEvent::None;
  data_.currentEventResult = EventResult::None;
}

void GameState::grantItems(uint32_t seed) {
  memset(data_.lastBossItems, 0, sizeof(data_.lastBossItems));
  const uint8_t r = data_.activeRegion;
  const uint8_t rewardCount = 1 + seed % 2;
  const uint8_t quantity =
      1 + min<uint16_t>(4,
                        (max<uint16_t>(1, data_.regionRound[r]) - 1) / 10);
  const uint8_t first = (seed / 2) % 4;
  const uint8_t second = (first + 1 + (seed / 8) % 3) % 4;
  const uint8_t rewards[] = {first, second};
  for (uint8_t index = 0; index < rewardCount; ++index) {
    const uint8_t item = rewards[index];
    data_.lastBossItems[item] = quantity;
    uint16_t& stored = data_.inventory.items[item];
    stored = min<uint32_t>(
        0xFFFFU, static_cast<uint32_t>(stored) + quantity);
  }
}

void GameState::rollTreasure(uint32_t seed) {
  const uint8_t r = data_.activeRegion;
  data_.lastBossTreasure = 0;
  if (data_.regionTreasure[r]) {
    data_.regionMisses[r] = 0;
    return;
  }
  const uint8_t chance =
      min<uint16_t>(10, max<uint16_t>(1, data_.regionRound[r]));
  if (data_.regionMisses[r] >= 19 || seed % 100 < chance) {
    data_.regionTreasure[r] = 1;
    data_.lastBossTreasure = 1;
    data_.regionMisses[r] = 0;
    applyTreasureBonus(r);
  } else if (data_.regionMisses[r] < 0xFFU) {
    data_.regionMisses[r]++;
  }
}

void GameState::applyTreasureBonus(uint8_t regionId) {
  switch (regionId) {
    case 3:
      data_.stamina =
          min<uint8_t>(100, data_.stamina + 20);
      break;
    case 4:
      for (uint8_t i = 0; i < 4; ++i) {
        addTendency(i, 10);
      }
      break;
    default:
      break;
  }
}

void GameState::finishBattle(BattleResult result, bool resetHp) {
  data_.inBattle = 0;
  data_.lastBattleResult = result;
  if (resetHp) {
    data_.bossHp = data_.bossMaxHp;
  }
  data_.battleAttackTalisman = 0;
  data_.battleGuardTalisman = 0;
  battleShield_ = 0;
}

uint16_t GameState::maxEnergy(PetForm form) {
  const uint8_t emptyTechniques[4] = {0, 0, 0, 0};
  return maxEnergy(form, emptyTechniques);
}

uint16_t GameState::maxEnergy(PetForm form,
                              const uint8_t techniqueLevels[4]) {
  uint16_t base = 20;
  if (form >= PetForm::FinalA1) {
    base = 80;
  } else if (form >= PetForm::RookieA) {
    base = 40;
  }
  if (techniqueLevels[kTechniqueSpirit] >= 9) {
    base += max<uint16_t>(10, base * 30 / 100);
  }
  return base;
}

uint16_t GameState::experienceForLevel(uint8_t level) {
  if (level <= 2) {
    return 20;
  }
  if (level <= 11) {
    return 50;
  }
  return 100;
}

void GameState::gainExperience(uint16_t amount) {
  data_.experience += amount;
  uint16_t xp = data_.experience;
  uint8_t level = 1;
  while (level < 30) {
    const uint16_t need = experienceForLevel(level);
    if (xp < need) {
      break;
    }
    xp -= need;
    ++level;
  }
  data_.level = level;
  updateEvolution();
}

void GameState::applyTask(const char* source, uint32_t durationSeconds,
                          bool success, bool halved) {
  const uint16_t minutes =
      constrain(static_cast<uint16_t>(durationSeconds / 60), 1, 60);
  if (success) {
    uint16_t expGain = minutes * 2;
    uint16_t coinGain = minutes;
    if (halved) {
      expGain = max<uint16_t>(1, expGain / 2);
      coinGain = max<uint16_t>(1, coinGain / 2);
    }
    gainExperience(expGain);
    data_.coins += coinGain;
    data_.energy =
        min<uint16_t>(maxEnergy(data_.form, data_.techniqueLevels),
                      data_.energy + max<uint16_t>(1, minutes / 2));
    const uint16_t tendencyGain =
        max<uint16_t>(1, min<uint16_t>(4, minutes / 5));
    const uint8_t slot =
        strcmp(source, "codex") == 0 ? 0 :
        strcmp(source, "claude-code") == 0 ? 1 :
        strcmp(source, "opencode") == 0 ? 2 : 3;
    addTendency(slot, tendencyGain);
  } else {
    gainExperience(max<uint16_t>(1, (minutes + 1) / 2));
  }
}

void GameState::completeAiTask(const char* source, uint32_t durationSeconds,
                               bool halved) {
  const uint16_t oldExperience = data_.experience;
  const uint16_t oldCoins = data_.coins;
  applyTask(source, durationSeconds, true, halved);
  AiTaskRecord& record = data_.aiTaskRecords[data_.aiTaskRecordIndex];
  record.source = strcmp(source, "codex") == 0 ? 0 :
                  strcmp(source, "claude-code") == 0 ? 1 :
                  strcmp(source, "opencode") == 0 ? 2 : 3;
  record.durationSeconds =
      min<uint32_t>(durationSeconds, 0xFFFFU);
  record.experienceReward = data_.experience - oldExperience;
  record.coinReward = data_.coins - oldCoins;
  data_.aiTaskRecordIndex =
      (data_.aiTaskRecordIndex + 1) %
      (sizeof(data_.aiTaskRecords) / sizeof(data_.aiTaskRecords[0]));
  data_.aiTaskRecordCount =
      min<uint8_t>(sizeof(data_.aiTaskRecords) / sizeof(data_.aiTaskRecords[0]),
                   data_.aiTaskRecordCount + 1);
}

bool GameState::tickRuntime(uint32_t seconds) {
  bool changed = false;
  const uint16_t energyCap = maxEnergy(data_.form, data_.techniqueLevels);
  const uint16_t recoveryInterval = recoveryIntervalSeconds();
  if (data_.energy >= energyCap) {
    data_.energyRecoverySeconds = 0;
  } else {
    data_.energyRecoverySeconds += seconds;
    while (data_.energyRecoverySeconds >= recoveryInterval &&
           data_.energy < energyCap) {
      data_.energyRecoverySeconds -= recoveryInterval;
      data_.energy++;
      changed = true;
    }
    if (data_.energy >= energyCap) {
      data_.energyRecoverySeconds = 0;
    }
  }

  if (data_.stamina >= 100) {
    data_.staminaRecoverySeconds = 0;
  } else {
    data_.staminaRecoverySeconds += seconds;
    while (data_.staminaRecoverySeconds >= recoveryInterval &&
           data_.stamina < 100) {
      data_.staminaRecoverySeconds -= recoveryInterval;
      data_.stamina = clampPercent(data_.stamina + 5);
      changed = true;
    }
    if (data_.stamina >= 100) {
      data_.staminaRecoverySeconds = 0;
    }
  }
  return changed;
}

void GameState::updateEvolution() {
  if (data_.level >= 12 &&
      (data_.form == PetForm::RookieA || data_.form == PetForm::RookieB)) {
    if (data_.form == PetForm::RookieA) {
      data_.form = data_.tendencies[0] >= data_.tendencies[1]
                       ? PetForm::FinalA1
                       : PetForm::FinalA2;
    } else {
      data_.form = data_.tendencies[2] >= data_.tendencies[3]
                       ? PetForm::FinalB1
                       : PetForm::FinalB2;
    }
  } else if (data_.level >= 3 && data_.form == PetForm::Egg) {
    const uint16_t agile = data_.tendencies[0] + data_.tendencies[1];
    const uint16_t steady = data_.tendencies[2] + data_.tendencies[3];
    data_.form = agile >= steady ? PetForm::RookieA : PetForm::RookieB;
  }
}

uint8_t GameState::clampPercent(uint16_t value) {
  return static_cast<uint8_t>(min<uint16_t>(100, value));
}
