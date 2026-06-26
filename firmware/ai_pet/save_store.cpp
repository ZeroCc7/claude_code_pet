#include "save_store.h"

#include <LittleFS.h>
#include <cstddef>

namespace {

constexpr char kSlotA[] = "/save_a.bin";
constexpr char kSlotB[] = "/save_b.bin";
constexpr uint32_t kSaveMagic = 0x50455431;
constexpr uint16_t kSaveVersion = 8;

}  // namespace

bool SaveStore::begin() {
  return LittleFS.begin();
}

bool SaveStore::load(GameState& state) {
  PetSaveData a{};
  PetSaveData b{};
  const bool validA = readSlot(kSlotA, a) && valid(a);
  const bool validB = readSlot(kSlotB, b) && valid(b);

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

bool SaveStore::readSlot(const char* path, PetSaveData& data) {
  File input = LittleFS.open(path, "r");
  if (!input) {
    return false;
  }

  const size_t fileSize = input.size();
  if (fileSize != sizeof(PetSaveData)) {
    input.close();
    return false;
  }

  const size_t read =
      input.read(reinterpret_cast<uint8_t*>(&data), sizeof(PetSaveData));
  input.close();
  return read == sizeof(PetSaveData);
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
