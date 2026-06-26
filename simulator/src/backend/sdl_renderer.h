#pragma once

#include <SDL2/SDL.h>
#include <cstdint>

class SdlRenderer {
 public:
  bool init(const char* title, int screenW, int screenH, int scale);
  void present(const uint16_t* framebuffer, int w, int h);
  void shutdown();

 private:
  SDL_Window* window_ = nullptr;
  SDL_Renderer* renderer_ = nullptr;
  SDL_Texture* texture_ = nullptr;
  int screenW_ = 0;
  int screenH_ = 0;
};
