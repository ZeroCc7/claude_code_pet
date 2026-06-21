#include "save_store.h"

#include <LittleFS.h>
#include <cstddef>
#include <cstring>

namespace {

constexpr char kSlotA[] = "/save_a.bin";
constexpr char kSlotB[] = "/save_b.bin";
constexpr uint32_t kSaveMagic = 0x50455431;
constexpr uint16_t kSaveVersion = 4;
constexpr uint16_t kLegacySaveVersionV3 = 3;
constexpr uint16_t kLegacySaveVersionV2 = 2;

struct PetSaveDataV3 {
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
  uint8_t activeRegion;
  uint8_t battleRegion;
  uint8_t bossHp;
  uint8_t bossMaxHp;
  uint8_t inBattle;
  uint32_t playSeconds;
  uint16_t energyRecoverySeconds;
  uint32_t meditationCycleSeconds;
  uint8_t meditationsUsed;
  uint32_t crc32;
};

struct PetSaveDataV2 {
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
  uint8_t activeRegion;
  uint8_t battleRegion;
  uint8_t bossHp;
  uint8_t bossMaxHp;
  uint8_t inBattle;
  uint32_t playSeconds;
  uint32_t crc32;
};

}  // namespace

bool SaveStore::begin() {
  return LittleFS.begin();
}

bool SaveStore::load(GameState& state) {
  PetSaveData a{};
  PetSaveData b{};
  bool migratedA = false;
  bool migratedB = false;
  const bool validA = readSlot(kSlotA, a, &migratedA) && valid(a);
  const bool validB = readSlot(kSlotB, b, &migratedB) && valid(b);

  if (!validA && !validB) {
    state.reset();
    currentSequence_ = 0;
    nextSlotA_ = true;
    return false;
  }

  const bool useA = validA && (!validB || a.sequence >= b.sequence);
  const PetSaveData& selected = useA ? a : b;
  state.load(selected);
  currentSequence_ = selected.sequence;
  nextSlotA_ = !useA;
  if ((useA && migratedA) || (!useA && migratedB)) {
    save(state);
  }
  return true;
}

bool SaveStore::save(GameState& state) {
  PetSaveData candidate = state.data();
  candidate.sequence = ++currentSequence_;
  candidate.crc32 = 0;
  candidate.crc32 =
      crc32(reinterpret_cast<const uint8_t*>(&candidate),
            offsetof(PetSaveData, crc32));

  const char* path = nextSlotA_ ? kSlotA : kSlotB;
  if (!writeSlot(path, candidate)) {
    currentSequence_--;
    return false;
  }

  PetSaveData verified{};
  if (!readSlot(path, verified) || !valid(verified) ||
      verified.sequence != candidate.sequence) {
    currentSequence_--;
    return false;
  }

  state.load(candidate);
  nextSlotA_ = !nextSlotA_;
  return true;
}

uint32_t SaveStore::crc32(const uint8_t* data, size_t length) {
  uint32_t value = 0xFFFFFFFF;
  for (size_t i = 0; i < length; ++i) {
    value ^= data[i];
    for (uint8_t bit = 0; bit < 8; ++bit) {
      const uint32_t mask = -(value & 1U);
      value = (value >> 1) ^ (0xEDB88320U & mask);
    }
  }
  return value ^ 0xFFFFFFFF;
}

bool SaveStore::readSlot(const char* path, PetSaveData& data,
                         bool* migrated) {
  if (migrated) {
    *migrated = false;
  }
  File input = LittleFS.open(path, "r");
  if (!input) {
    return false;
  }

  const size_t fileSize = input.size();
  if (fileSize == sizeof(PetSaveData)) {
    const size_t read =
        input.read(reinterpret_cast<uint8_t*>(&data), sizeof(PetSaveData));
    input.close();
    return read == sizeof(PetSaveData);
  }

  if (fileSize == sizeof(PetSaveDataV3)) {
    PetSaveDataV3 legacy{};
    const size_t read =
        input.read(reinterpret_cast<uint8_t*>(&legacy), sizeof(legacy));
    input.close();
    if (read != sizeof(legacy) || legacy.magic != kSaveMagic ||
        legacy.version != kLegacySaveVersionV3 ||
        legacy.size != sizeof(PetSaveDataV3)) {
      return false;
    }
    const uint32_t expected =
        crc32(reinterpret_cast<const uint8_t*>(&legacy),
              offsetof(PetSaveDataV3, crc32));
    if (expected != legacy.crc32) {
      return false;
    }
    data = {};
    data.magic = legacy.magic;
    data.version = kSaveVersion;
    data.size = sizeof(PetSaveData);
    data.sequence = legacy.sequence;
    data.form = legacy.form;
    data.level = legacy.level;
    data.experience = legacy.experience;
    data.mood = legacy.mood;
    data.stamina = legacy.stamina;
    data.coins = legacy.coins;
    data.energy = min<uint16_t>(20, legacy.energy);
    memcpy(data.tendencies, legacy.tendencies, sizeof(data.tendencies));
    memcpy(data.regionProgress, legacy.regionProgress,
           sizeof(data.regionProgress));
    data.bossDefeatedMask = legacy.bossDefeatedMask;
    for (uint8_t region = 0; region < 3; ++region) {
      data.bossWins[region] =
          (legacy.bossDefeatedMask & (1U << region)) ? 1 : 0;
    }
    data.activeRegion = legacy.activeRegion;
    data.battleRegion = legacy.battleRegion;
    data.bossHp = legacy.bossHp;
    data.bossMaxHp = legacy.bossMaxHp;
    data.inBattle = legacy.inBattle;
    data.playSeconds = legacy.playSeconds;
    data.energyRecoverySeconds = legacy.energyRecoverySeconds;
    data.meditationCycleSeconds = legacy.meditationCycleSeconds;
    data.meditationsUsed = legacy.meditationsUsed;
    data.crc32 =
        crc32(reinterpret_cast<const uint8_t*>(&data),
              offsetof(PetSaveData, crc32));
    if (migrated) {
      *migrated = true;
    }
    return true;
  }

  if (fileSize != sizeof(PetSaveDataV2)) {
    input.close();
    return false;
  }

  PetSaveDataV2 legacy{};
  const size_t read =
      input.read(reinterpret_cast<uint8_t*>(&legacy), sizeof(legacy));
  input.close();
  if (read != sizeof(legacy) || legacy.magic != kSaveMagic ||
      legacy.version != kLegacySaveVersionV2 ||
      legacy.size != sizeof(PetSaveDataV2)) {
    return false;
  }
  const uint32_t expected =
      crc32(reinterpret_cast<const uint8_t*>(&legacy),
            offsetof(PetSaveDataV2, crc32));
  if (expected != legacy.crc32) {
    return false;
  }

  data = {};
  data.magic = legacy.magic;
  data.version = kSaveVersion;
  data.size = sizeof(PetSaveData);
  data.sequence = legacy.sequence;
  data.form = legacy.form;
  data.level = legacy.level;
  data.experience = legacy.experience;
  data.mood = legacy.mood;
  data.stamina = legacy.stamina;
  data.coins = legacy.coins;
  data.energy = min<uint16_t>(20, legacy.energy);
  memcpy(data.tendencies, legacy.tendencies, sizeof(data.tendencies));
  memcpy(data.regionProgress, legacy.regionProgress,
         sizeof(data.regionProgress));
  data.bossDefeatedMask = legacy.bossDefeatedMask;
  for (uint8_t region = 0; region < 3; ++region) {
    data.bossWins[region] =
        (legacy.bossDefeatedMask & (1U << region)) ? 1 : 0;
  }
  data.activeRegion = legacy.activeRegion;
  data.battleRegion = legacy.battleRegion;
  data.bossHp = legacy.bossHp;
  data.bossMaxHp = legacy.bossMaxHp;
  data.inBattle = legacy.inBattle;
  data.playSeconds = legacy.playSeconds;
  data.crc32 =
      crc32(reinterpret_cast<const uint8_t*>(&data),
            offsetof(PetSaveData, crc32));
  if (migrated) {
    *migrated = true;
  }
  return true;
}

bool SaveStore::writeSlot(const char* path, const PetSaveData& data) {
  File output = LittleFS.open(path, "w");
  if (!output) {
    return false;
  }
  const size_t written =
      output.write(reinterpret_cast<const uint8_t*>(&data),
                   sizeof(PetSaveData));
  output.flush();
  output.close();
  return written == sizeof(PetSaveData);
}

bool SaveStore::valid(const PetSaveData& data) const {
  if (data.magic != kSaveMagic || data.version != kSaveVersion ||
      data.size != sizeof(PetSaveData)) {
    return false;
  }
  const uint32_t expected =
      crc32(reinterpret_cast<const uint8_t*>(&data),
            offsetof(PetSaveData, crc32));
  return expected == data.crc32;
}
