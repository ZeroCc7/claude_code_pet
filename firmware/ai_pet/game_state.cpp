#include "game_state.h"

#include <cstring>

namespace {

constexpr uint32_t kSaveMagic = 0x50455431;
constexpr uint16_t kSaveVersion = 5;
constexpr uint8_t kNoActiveRegion = 0xFF;
constexpr uint16_t kMaxEnergy = 20;
constexpr uint16_t kPassiveRecoverySeconds = 300;
constexpr uint32_t kMeditationCycleSeconds = 86400;
constexpr uint8_t kMeditationsPerCycle = 3;

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
  data_.activeRegion = kNoActiveRegion;
}

void GameState::load(const PetSaveData& data) {
  data_ = data;
  data_.mood = clampPercent(data_.mood);
  data_.stamina = clampPercent(data_.stamina);
  data_.energy = min<uint16_t>(kMaxEnergy, data_.energy);
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
    if (data_.energy >= kMaxEnergy) {
      return false;
    }
    data_.energy = min<uint16_t>(kMaxEnergy, data_.energy + 3);
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

void GameState::interact() {
  data_.mood = clampPercent(data_.mood + 5);
  data_.tendencies[3]++;
}

bool GameState::feed() {
  if (data_.coins < 10 || data_.stamina >= 100) {
    return false;
  }
  data_.coins -= 10;
  data_.stamina = clampPercent(data_.stamina + 20);
  data_.tendencies[3]++;
  return true;
}

bool GameState::startExploration(uint8_t region) {
  if (region >= 3 || !regionUnlocked(region) || data_.energy < 3) {
    return false;
  }
  data_.energy -= 3;
  data_.activeRegion = region;
  return true;
}

bool GameState::regionUnlocked(uint8_t region) const {
  return region == 0 ||
         (region < 3 && (data_.bossDefeatedMask & (1U << (region - 1))));
}

bool GameState::tickExploration(uint32_t seed) {
  if (data_.activeRegion >= 3 || data_.energy == 0) {
    return false;
  }

  const uint8_t region = data_.activeRegion;
  data_.energy--;
  const uint8_t gain = 1 + seed % 3;
  data_.regionProgress[region] =
      min<uint8_t>(100, data_.regionProgress[region] + gain);
  data_.coins++;
  gainExperience(region + 1);
  data_.tendencies[region == 0 ? 0 : 2]++;

  if (data_.regionProgress[region] == 100 || data_.energy == 0) {
    data_.activeRegion = kNoActiveRegion;
  }
  return true;
}

bool GameState::startBoss(uint8_t region) {
  if (region >= 3 || data_.regionProgress[region] < 100) {
    return false;
  }
  data_.inBattle = 1;
  data_.battleRegion = region;
  data_.bossMaxHp = 25 + region * 20;
  data_.bossHp = data_.bossMaxHp;
  return true;
}

bool GameState::battleAction(uint8_t action) {
  if (!data_.inBattle) {
    return false;
  }

  uint8_t damage = 0;
  uint8_t incoming = 5 + data_.battleRegion * 2;
  if (action == 0) {
    damage = 6 + data_.level;
    data_.tendencies[0]++;
  } else if (action == 1 && data_.energy >= 2) {
    data_.energy -= 2;
    damage = 10 + data_.level * 2;
    data_.tendencies[1]++;
  } else if (action == 2 && data_.coins >= 5) {
    data_.coins -= 5;
    data_.stamina = clampPercent(data_.stamina + 15);
    incoming = 0;
    data_.tendencies[3]++;
  } else if (action == 3) {
    incoming = 3;
    data_.tendencies[2]++;
  } else {
    return false;
  }

  data_.bossHp = damage >= data_.bossHp ? 0 : data_.bossHp - damage;
  if (data_.bossHp == 0) {
    const uint8_t repeatCount = data_.bossWins[data_.battleRegion];
    const uint8_t shift = min<uint8_t>(repeatCount, 15);
    const uint16_t experienceReward =
        max<uint16_t>(1, (20 + data_.battleRegion * 10) >> shift);
    const uint16_t coinReward =
        max<uint16_t>(1, (20 + data_.battleRegion * 15) >> shift);
    data_.bossDefeatedMask |= 1U << data_.battleRegion;
    data_.bossWins[data_.battleRegion]++;
    data_.coins += coinReward;
    gainExperience(experienceReward);
    data_.inBattle = 0;
    return true;
  }

  data_.stamina =
      incoming >= data_.stamina ? 0 : data_.stamina - incoming;
  if (data_.stamina == 0) {
    data_.inBattle = 0;
    data_.stamina = 30;
  }
  return true;
}

void GameState::gainExperience(uint16_t amount) {
  data_.experience += amount;
  data_.level = min<uint8_t>(30, data_.experience / 20 + 1);
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
        min<uint16_t>(kMaxEnergy,
                      data_.energy + max<uint16_t>(1, minutes / 2));
  } else {
    gainExperience(max<uint16_t>(1, (minutes + 1) / 2));
  }
}

uint32_t GameState::taskHash(const char* source, const char* taskId) {
  uint32_t value = 0x811C9DC5U;
  const char* parts[] = {source, ":", taskId};
  for (const char* part : parts) {
    while (*part) {
      value ^= static_cast<uint8_t>(*part++);
      value *= 0x01000193U;
    }
  }
  return value == 0 ? 1 : value;
}

bool GameState::hasProcessedTask(const char* source,
                                 const char* taskId) const {
  const uint32_t digest = taskHash(source, taskId);
  for (uint32_t stored : data_.recentTaskHashes) {
    if (stored == digest) {
      return true;
    }
  }
  return false;
}

bool GameState::applyAiTask(const char* source, const char* taskId,
                            uint32_t durationSeconds, bool success,
                            bool halved) {
  if (!success || hasProcessedTask(source, taskId)) {
    return false;
  }
  const uint16_t oldExperience = data_.experience;
  const uint16_t oldCoins = data_.coins;
  applyTask(durationSeconds, true, halved);
  data_.recentTaskHashes[data_.recentTaskIndex] = taskHash(source, taskId);
  data_.recentTaskIndex =
      (data_.recentTaskIndex + 1) %
      (sizeof(data_.recentTaskHashes) / sizeof(data_.recentTaskHashes[0]));
  AiTaskRecord& record = data_.aiTaskRecords[data_.aiTaskRecordIndex];
  record.source = strcmp(source, "codex") == 0 ? 0 :
                  strcmp(source, "claude_code") == 0 ? 1 :
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
  return true;
}

bool GameState::tickRuntime(uint32_t seconds) {
  bool changed = false;
  data_.meditationCycleSeconds += seconds;
  while (data_.meditationCycleSeconds >= kMeditationCycleSeconds) {
    data_.meditationCycleSeconds -= kMeditationCycleSeconds;
    if (data_.meditationsUsed != 0) {
      data_.meditationsUsed = 0;
      changed = true;
    }
  }

  if (data_.energy >= kMaxEnergy) {
    data_.energyRecoverySeconds = 0;
    return changed;
  }

  data_.energyRecoverySeconds += seconds;
  while (data_.energyRecoverySeconds >= kPassiveRecoverySeconds &&
         data_.energy < kMaxEnergy) {
    data_.energyRecoverySeconds -= kPassiveRecoverySeconds;
    data_.energy++;
    changed = true;
  }
  if (data_.energy >= kMaxEnergy) {
    data_.energyRecoverySeconds = 0;
  }
  return changed;
}

MeditationResult GameState::meditate() {
  if (data_.energy >= kMaxEnergy) {
    return MeditationResult::Full;
  }
  if (data_.meditationsUsed >= kMeditationsPerCycle) {
    return MeditationResult::Exhausted;
  }
  data_.energy = min<uint16_t>(kMaxEnergy, data_.energy + 3);
  data_.meditationsUsed++;
  data_.tendencies[1]++;
  return MeditationResult::Restored;
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
