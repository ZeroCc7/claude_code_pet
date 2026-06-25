#include "game_state.h"

#include <cstring>

namespace {

constexpr uint32_t kSaveMagic = 0x50455431;
constexpr uint16_t kSaveVersion = 7;
constexpr uint16_t kPassiveRecoverySeconds = 300;

}  // namespace

GameState::GameState() {
  reset();
}

void GameState::reset() {
  memset(&data_, 0, sizeof(data_));
  data_.magic = kSaveMagic;
  data_.version = kSaveVersion;
  data_.size = sizeof(PetSaveData);
  data_.form = PetForm::Egg;
  data_.level = 1;
  data_.mood = 70;
  data_.stamina = 80;
  data_.coins = 30;
  data_.energy = 10;
  data_.qingyunRound = 1;
  data_.adventurePhase = AdventurePhase::Idle;
  data_.currentEvent = QingyunEvent::None;
  data_.currentEventResult = EventResult::None;
  data_.lastBattleResult = BattleResult::Inactive;
}

void GameState::load(const PetSaveData& data) {
  data_ = data;
  data_.mood = clampPercent(data_.mood);
  data_.stamina = clampPercent(data_.stamina);
  data_.energy = min<uint16_t>(maxEnergy(data_.form), data_.energy);
  data_.qingyunRound = max<uint16_t>(1, data_.qingyunRound);
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
    if (data_.energy >= maxEnergy(data_.form)) {
      return false;
    }
    data_.energy = min<uint16_t>(maxEnergy(data_.form), data_.energy + 3);
  } else if (item == ItemType::RecoveryPill) {
    if (data_.stamina >= 100) {
      return false;
    }
    data_.stamina = clampPercent(data_.stamina + 20);
  } else {
    return false;
  }
  quantity--;
  return true;
}

bool GameState::startQingyunAdventure() {
  if (data_.adventurePhase == AdventurePhase::BossReady) {
    data_.qingyunProgress = 0;
    data_.qingyunEventMask = 0;
    data_.adventurePhase = AdventurePhase::Advancing;
    data_.currentEvent = QingyunEvent::None;
    data_.currentEventResult = EventResult::None;
    return true;
  }
  if (data_.adventurePhase != AdventurePhase::Idle || data_.energy < 3) {
    return false;
  }
  data_.energy -= 3;
  data_.adventurePhase = AdventurePhase::Advancing;
  data_.currentEvent = QingyunEvent::None;
  data_.currentEventResult = EventResult::None;
  return true;
}

void GameState::stopQingyunAdventure() {
  data_.adventurePhase = data_.qingyunBossUnlocked
                             ? AdventurePhase::BossReady
                             : AdventurePhase::Idle;
  data_.currentEvent = QingyunEvent::None;
  data_.currentEventResult = EventResult::None;
}

AdventureTick GameState::tickQingyunAdventure(uint32_t seed) {
  (void)seed;
  if (data_.adventurePhase == AdventurePhase::Choosing) {
    return AdventureTick::WaitingForChoice;
  }
  if (data_.adventurePhase != AdventurePhase::Advancing) {
    return AdventureTick::Inactive;
  }
  data_.energy--;
  data_.qingyunProgress =
      min<uint8_t>(100, data_.qingyunProgress + 2);

  constexpr uint8_t checkpoints[] = {25, 45, 65, 85};
  constexpr QingyunEvent events[] = {
      QingyunEvent::SpiritHerb, QingyunEvent::DemonBeast,
      QingyunEvent::WoundedCultivator, QingyunEvent::Shortcut};
  for (uint8_t index = 0; index < 4; ++index) {
    const uint8_t eventBit = 1U << index;
    if (data_.qingyunProgress >= checkpoints[index] &&
        !(data_.qingyunEventMask & eventBit)) {
      data_.qingyunEventMask |= eventBit;
      data_.currentEvent = events[index];
      data_.currentEventResult = EventResult::None;
      data_.adventurePhase = AdventurePhase::Choosing;
      return AdventureTick::EventTriggered;
    }
  }
  if (data_.qingyunProgress >= 100) {
    data_.qingyunBossUnlocked = 1;
    data_.adventurePhase = AdventurePhase::BossReady;
    return AdventureTick::BossUnlocked;
  }
  if (data_.energy == 0) {
    stopQingyunAdventure();
    return AdventureTick::EnergyDepleted;
  }
  return AdventureTick::Advanced;
}

EventResult GameState::resolveQingyunEvent(uint8_t choice, uint32_t seed) {
  if (data_.adventurePhase != AdventurePhase::Choosing) {
    return EventResult::None;
  }
  EventResult result = EventResult::Continued;
  if (data_.currentEvent == QingyunEvent::SpiritHerb && choice == 0) {
    addItem(ItemType::SpiritHerb);
    result = EventResult::ItemGained;
  } else if (data_.currentEvent == QingyunEvent::DemonBeast &&
             choice == 0) {
    const uint16_t score =
        data_.level + data_.stamina / 10 + data_.tendencies[0] / 4 +
        seed % 10;
    if (score >= 18) {
      gainExperience(6);
      data_.coins += 5;
      result = EventResult::RewardGained;
    } else {
      const uint8_t damage = qingyunEventDamage(12);
      data_.stamina = data_.stamina > damage ? data_.stamina - damage : 0;
      result = EventResult::StaminaLost;
    }
  } else if (data_.currentEvent == QingyunEvent::WoundedCultivator &&
             choice == 0) {
    const uint16_t score =
        data_.level + data_.stamina / 10 + data_.tendencies[3] / 2 +
        seed % 10;
    if (score >= 25) {
      addItem(ItemType::QingyunToken);
      data_.qingyunBossUnlocked = 1;
    } else {
      addItem(ItemType::RecoveryPill);
    }
    result = EventResult::ItemGained;
  } else if (data_.currentEvent == QingyunEvent::Shortcut) {
    if (choice == 0) {
      const uint16_t score =
          data_.level + data_.energy + data_.tendencies[1] / 3 +
          seed % 10;
      if (score >= 20) {
        data_.qingyunProgress =
            min<uint8_t>(100, data_.qingyunProgress + 8 + seed % 8);
        result = EventResult::ProgressGained;
      } else {
        const uint8_t damage = qingyunEventDamage(10);
        data_.stamina = data_.stamina > damage ? data_.stamina - damage : 0;
        result = EventResult::StaminaLost;
      }
    } else {
      data_.qingyunProgress =
          min<uint8_t>(100, data_.qingyunProgress + 3);
      result = EventResult::ProgressGained;
    }
  }
  data_.currentEventResult = result;
  data_.adventurePhase = AdventurePhase::Result;
  return result;
}

void GameState::acknowledgeAdventureResult() {
  if (data_.adventurePhase != AdventurePhase::Result) {
    return;
  }
  data_.currentEvent = QingyunEvent::None;
  data_.currentEventResult = EventResult::None;
  if (data_.qingyunProgress >= 100) {
    data_.qingyunBossUnlocked = 1;
  }
  data_.adventurePhase = data_.qingyunBossUnlocked
                             ? AdventurePhase::BossReady
                             : AdventurePhase::Advancing;
}

void GameState::abandonQingyunEvent() {
  if (data_.adventurePhase == AdventurePhase::Choosing) {
    stopQingyunAdventure();
  }
}

void GameState::addItem(ItemType item) {
  uint16_t& quantity = data_.inventory.items[static_cast<uint8_t>(item)];
  if (quantity < 0xFFFFU) {
    quantity++;
  }
}

bool GameState::startQingyunWolfBattle(bool useAttackTalisman,
                                       bool useGuardTalisman) {
  if (data_.inBattle || !data_.qingyunBossUnlocked || data_.energy < 5) {
    return false;
  }
  data_.inBattle = 1;
  data_.bossMaxHp = qingyunBossMaxHp();
  data_.bossHp = data_.bossMaxHp;
  data_.battleRound = 0;
  data_.lastBattleResult = BattleResult::Continue;
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
  return true;
}

uint8_t GameState::qingyunAttackDamage(uint32_t seed) const {
  uint16_t damage =
      4 + data_.level + min<uint16_t>(8, data_.tendencies[0] / 5) +
      min<uint16_t>(10, data_.tendencies[1] / 4);
  if (data_.form == PetForm::RookieA || data_.form == PetForm::FinalA1) {
    damage++;
  } else if (data_.form == PetForm::FinalA2) {
    damage += 2;
  }
  const uint8_t criticalRate =
      5 + min<uint16_t>(15, data_.tendencies[0] / 4);
  if (seed % 100 < criticalRate) {
    damage *= 2;
  }
  if (data_.battleAttackTalisman) {
    damage = damage * 120 / 100;
  }
  if (data_.hasQingyunSword) {
    damage = damage * 110 / 100;
  }
  return min<uint16_t>(255, max<uint16_t>(1, damage));
}

uint8_t GameState::qingyunIncomingDamage(uint32_t seed) const {
  const uint8_t dodgeRate =
      5 + min<uint16_t>(15, data_.tendencies[2] / 4);
  if ((seed / 100) % 100 < dodgeRate) {
    return 0;
  }
  uint16_t damage =
      max<int16_t>(1, 8 - min<uint16_t>(5, data_.tendencies[2] / 8));
  if (data_.form == PetForm::RookieB || data_.form == PetForm::FinalB1) {
    damage = max<uint8_t>(1, damage - 1);
  } else if (data_.form == PetForm::FinalB2) {
    damage = max<uint8_t>(1, damage - 2);
  }
  if (data_.battleGuardTalisman) {
    damage = max<uint8_t>(1, damage * 80 / 100);
  }
  damage = max<uint16_t>(1, damage * qingyunDamagePercent() / 100);
  if (data_.hasQingyunSword) {
    damage = max<uint16_t>(1, damage * 90 / 100);
  }
  return min<uint16_t>(255, damage);
}

BattleResult GameState::tickQingyunWolfBattle(uint32_t seed) {
  if (!data_.inBattle) {
    return BattleResult::Inactive;
  }
  data_.energy--;
  data_.battleRound++;
  if (data_.energy == 0) {
    data_.qingyunProgress = 0;
    data_.adventurePhase = AdventurePhase::Idle;
    finishQingyunBattle(BattleResult::EnergyDepleted);
    return BattleResult::EnergyDepleted;
  }
  const uint8_t damage = qingyunAttackDamage(seed);
  data_.bossHp = damage >= data_.bossHp ? 0 : data_.bossHp - damage;
  if (data_.bossHp == 0) {
    data_.lastQingyunExperience = qingyunCompletionExperience();
    data_.lastQingyunCoins = qingyunCompletionCoins();
    grantQingyunItems(seed);
    rollQingyunSword(seed);
    if (data_.qingyunBossWins < 0xFFU) {
      data_.qingyunBossWins++;
    }
    data_.qingyunBossDefeated = 1;
    gainExperience(data_.lastQingyunExperience);
    data_.coins = min<uint32_t>(
        0xFFFFU, static_cast<uint32_t>(data_.coins) +
                     data_.lastQingyunCoins);
    if (data_.qingyunRound < 0xFFFFU) {
      data_.qingyunRound++;
    }
    data_.qingyunProgress = 0;
    data_.adventurePhase = AdventurePhase::Idle;
    finishQingyunBattle(BattleResult::Victory, false);
    return BattleResult::Victory;
  }
  const uint8_t incoming = qingyunIncomingDamage(seed);
  data_.stamina =
      incoming >= data_.stamina ? 0 : data_.stamina - incoming;
  const uint8_t affinityRate =
      min<uint16_t>(20, data_.tendencies[3] / 3);
  if (data_.stamina > 0 && (seed / 10000) % 100 < affinityRate) {
    data_.stamina = clampPercent(data_.stamina + 2);
  }
  if (data_.stamina == 0) {
    data_.stamina = 30;
    data_.qingyunProgress = 0;
    data_.adventurePhase = AdventurePhase::Idle;
    finishQingyunBattle(BattleResult::Defeat);
    return BattleResult::Defeat;
  }
  data_.lastBattleResult = BattleResult::Continue;
  return BattleResult::Continue;
}

void GameState::retreatQingyunWolf() {
  if (data_.inBattle) {
    data_.qingyunProgress = 0;
    data_.adventurePhase = AdventurePhase::Idle;
    finishQingyunBattle(BattleResult::Retreated);
  }
}

uint8_t GameState::qingyunBossMaxHp() const {
  return min<uint16_t>(255, 48 * qingyunHealthPercent() / 100);
}

uint16_t GameState::qingyunCompletionExperience() const {
  const uint16_t round = min<uint16_t>(50, max<uint16_t>(1, data_.qingyunRound));
  const uint16_t laterRounds = round > 10 ? round - 10 : 0;
  return 6 + min<uint16_t>(round, 10) + laterRounds / 5;
}

uint16_t GameState::qingyunCompletionCoins() const {
  const uint16_t round = min<uint16_t>(50, max<uint16_t>(1, data_.qingyunRound));
  const uint16_t laterRounds = round > 10 ? round - 10 : 0;
  return 15 + 2 * min<uint16_t>(round, 10) + laterRounds;
}

uint16_t GameState::qingyunHealthPercent() const {
  const uint16_t round = min<uint16_t>(50, max<uint16_t>(1, data_.qingyunRound));
  const uint16_t laterRounds = round > 10 ? round - 10 : 0;
  return 100 + min<uint16_t>(round - 1, 9) * 15 +
         laterRounds * 3;
}

uint16_t GameState::qingyunDamagePercent() const {
  const uint16_t round = min<uint16_t>(50, max<uint16_t>(1, data_.qingyunRound));
  const uint16_t laterRounds = round > 10 ? round - 10 : 0;
  return 100 + min<uint16_t>(round - 1, 9) * 8 +
         laterRounds * 2;
}

uint8_t GameState::qingyunEventDamage(uint8_t baseDamage) const {
  return min<uint16_t>(
      255, max<uint16_t>(1, baseDamage * qingyunDamagePercent() / 100));
}

void GameState::resetQingyunRun() {
  data_.qingyunProgress = 0;
  data_.qingyunEventMask = 0;
  data_.adventurePhase = AdventurePhase::Idle;
  data_.currentEvent = QingyunEvent::None;
  data_.currentEventResult = EventResult::None;
}

void GameState::grantQingyunItems(uint32_t seed) {
  memset(data_.lastQingyunItems, 0, sizeof(data_.lastQingyunItems));
  const uint8_t rewardCount = 1 + seed % 2;
  const uint8_t quantity =
      1 + min<uint16_t>(4, (max<uint16_t>(1, data_.qingyunRound) - 1) / 10);
  const uint8_t first = (seed / 2) % 4;
  const uint8_t second = (first + 1 + (seed / 8) % 3) % 4;
  const uint8_t rewards[] = {first, second};
  for (uint8_t index = 0; index < rewardCount; ++index) {
    const uint8_t item = rewards[index];
    data_.lastQingyunItems[item] = quantity;
    uint16_t& stored = data_.inventory.items[item];
    stored = min<uint32_t>(
        0xFFFFU, static_cast<uint32_t>(stored) + quantity);
  }
}

void GameState::rollQingyunSword(uint32_t seed) {
  data_.lastQingyunSword = 0;
  if (data_.hasQingyunSword) {
    data_.qingyunMisses = 0;
    return;
  }
  const uint8_t chance =
      min<uint16_t>(10, max<uint16_t>(1, data_.qingyunRound));
  if (data_.qingyunMisses >= 19 || seed % 100 < chance) {
    data_.hasQingyunSword = 1;
    data_.lastQingyunSword = 1;
    data_.qingyunMisses = 0;
  } else if (data_.qingyunMisses < 0xFFU) {
    data_.qingyunMisses++;
  }
}

void GameState::finishQingyunBattle(BattleResult result, bool resetHp) {
  data_.inBattle = 0;
  data_.lastBattleResult = result;
  if (resetHp) {
    data_.bossHp = data_.bossMaxHp;
  }
  data_.battleAttackTalisman = 0;
  data_.battleGuardTalisman = 0;
}

uint16_t GameState::maxEnergy(PetForm form) {
  if (form >= PetForm::FinalA1) {
    return 80;
  }
  if (form >= PetForm::RookieA) {
    return 40;
  }
  return 20;
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

void GameState::applyTask(uint32_t durationSeconds, bool success,
                         bool halved) {
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
        min<uint16_t>(maxEnergy(data_.form),
                      data_.energy + max<uint16_t>(1, minutes / 2));
  } else {
    gainExperience(max<uint16_t>(1, (minutes + 1) / 2));
  }
}

void GameState::completeAiTask(const char* source, uint32_t durationSeconds,
                               bool halved) {
  const uint16_t oldExperience = data_.experience;
  const uint16_t oldCoins = data_.coins;
  applyTask(durationSeconds, true, halved);
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
  if (data_.energy >= maxEnergy(data_.form)) {
    data_.energyRecoverySeconds = 0;
  } else {
    data_.energyRecoverySeconds += seconds;
    while (data_.energyRecoverySeconds >= kPassiveRecoverySeconds &&
           data_.energy < maxEnergy(data_.form)) {
      data_.energyRecoverySeconds -= kPassiveRecoverySeconds;
      data_.energy++;
      changed = true;
    }
    if (data_.energy >= maxEnergy(data_.form)) {
      data_.energyRecoverySeconds = 0;
    }
  }

  if (data_.stamina >= 100) {
    data_.staminaRecoverySeconds = 0;
  } else {
    data_.staminaRecoverySeconds += seconds;
    while (data_.staminaRecoverySeconds >= kPassiveRecoverySeconds &&
           data_.stamina < 100) {
      data_.staminaRecoverySeconds -= kPassiveRecoverySeconds;
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
