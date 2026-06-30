#pragma once

#include <cstdint>

constexpr int SIM_SCALE = 4;
constexpr int SIM_SCREEN_W = 128;
constexpr int SIM_SCREEN_H = 160;
constexpr int SIM_WINDOW_W = SIM_SCREEN_W * SIM_SCALE;
constexpr int SIM_WINDOW_H = SIM_SCREEN_H * SIM_SCALE;
constexpr int SIM_NUM_BUTTONS = 4;
constexpr uint32_t SIM_FRAME_INTERVAL_MS = 33; // ~30 FPS
