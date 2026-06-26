#include "sdl_renderer.h"

#include <cstdio>

bool SdlRenderer::init(const char* title, int screenW, int screenH, int scale) {
  screenW_ = screenW;
  screenH_ = screenH;

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

  window_ = SDL_CreateWindow(
      title,
      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      screenW * scale, screenH * scale,
      SDL_WINDOW_SHOWN);
  if (!window_) {
    fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
    return false;
  }

  renderer_ = SDL_CreateRenderer(window_, -1,
                                 SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer_) {
    fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
    return false;
  }

  texture_ = SDL_CreateTexture(renderer_,
                               SDL_PIXELFORMAT_RGB565,
                               SDL_TEXTUREACCESS_STREAMING,
                               screenW, screenH);
  if (!texture_) {
    fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
    return false;
  }

  return true;
}

void SdlRenderer::present(const uint16_t* framebuffer, int w, int h) {
  if (!texture_ || !renderer_ || !framebuffer) return;
  SDL_UpdateTexture(texture_, nullptr, framebuffer, w * sizeof(uint16_t));
  SDL_RenderClear(renderer_);
  SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
  SDL_RenderPresent(renderer_);
}

void SdlRenderer::shutdown() {
  if (texture_)  { SDL_DestroyTexture(texture_);   texture_ = nullptr;  }
  if (renderer_) { SDL_DestroyRenderer(renderer_); renderer_ = nullptr; }
  if (window_)   { SDL_DestroyWindow(window_);     window_ = nullptr;   }
}
