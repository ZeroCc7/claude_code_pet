#include "game_state.h"

#include <cstring>

namespace {

constexpr uint32_t kSaveMagic = 0x50455431;
constexpr uint16_t kSaveVersion = 1;
constexpr uint8_t kNoActiveRegion = 0xFF;

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
}

const PetSaveData& GameState::data() const {
  return data_;
}

PetSaveData& GameState::mutableData() {
  return data_;
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
  if (region >= 3 || data_.energy < 3) {
    return false;
  }
  data_.energy -= 3;
  data_.activeRegion = region;
  return true;
}

void GameState::applyTask(uint32_t durationSeconds, bool success) {
  const uint16_t minutes =
      constrain(static_cast<uint16_t>(durationSeconds / 60), 1, 60);
  if (success) {
    data_.experience += minutes * 2;
    data_.coins += minutes;
    data_.energy =
        min<uint16_t>(999, data_.energy + max<uint16_t>(1, minutes / 2));
  } else {
    data_.experience += max<uint16_t>(1, (minutes + 1) / 2);
  }
}

uint8_t GameState::clampPercent(uint16_t value) {
  return static_cast<uint8_t>(min<uint16_t>(100, value));
}

