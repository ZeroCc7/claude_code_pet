// Simulator replacement for save_store.cpp
// Uses standard file I/O instead of LittleFS.

#include "save_store.h"

#include <cstdio>
#include <cstddef>
#include <cstring>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define SIM_MKDIR(path) _mkdir(path)
#else
#define SIM_MKDIR(path) mkdir(path, 0755)
#endif

namespace {

constexpr char kSlotA[] = "saves/save_a.bin";
constexpr char kSlotB[] = "saves/save_b.bin";
constexpr uint32_t kSaveMagic = 0x50455431;
constexpr uint16_t kSaveVersion = 7;

}  // namespace

bool SaveStore::begin() {
  SIM_MKDIR("saves");
  return true;
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
  FILE* fp = fopen(path, "rb");
  if (!fp) {
    return false;
  }

  fseek(fp, 0, SEEK_END);
  const long fileSize = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  if (fileSize != static_cast<long>(sizeof(PetSaveData))) {
    fclose(fp);
    return false;
  }

  const size_t bytesRead = fread(&data, 1, sizeof(PetSaveData), fp);
  fclose(fp);
  return bytesRead == sizeof(PetSaveData);
}

bool SaveStore::writeSlot(const char* path, const PetSaveData& data) {
  FILE* fp = fopen(path, "wb");
  if (!fp) {
    return false;
  }
  const size_t written = fwrite(&data, 1, sizeof(PetSaveData), fp);
  fflush(fp);
  fclose(fp);
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
