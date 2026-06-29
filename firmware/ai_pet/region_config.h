#pragma once

#include <Arduino.h>

constexpr uint8_t kRegionCount = 5;

struct RegionConfig {
  const char* name;
  const char* boss_name;
  const char* event_names[4];
  uint8_t base_boss_hp;
  uint8_t hp_scale_first10;
  uint8_t hp_scale_later;
  uint8_t base_boss_damage;
  uint8_t dmg_scale_first10;
  uint8_t dmg_scale_later;
  uint8_t base_xp;
  uint8_t base_coins;
  uint8_t tendency_base;
  uint8_t unlock_level;
  uint16_t unlock_coins;
  uint8_t prerequisite;
  uint8_t reward_bias[4];
  const char* treasure_name;
};

extern const RegionConfig kRegions[kRegionCount];
