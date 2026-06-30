#include <SDL2/SDL.h>
#include <cstdio>

#include "sim_config.h"
#include "backend/sdl_renderer.h"
#include "backend/input_manager.h"
#include "game_app.h"

// Declared in button_scanner_sim.cpp
bool sim_button_states[4] = {};

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return 1;
  }

  SdlRenderer renderer;
  if (!renderer.init("AI Pet Simulator", SIM_SCREEN_W, SIM_SCREEN_H, SIM_SCALE)) {
    fprintf(stderr, "Renderer init failed\n");
    SDL_Quit();
    return 1;
  }

  InputManager input;
  input.init();

  // Create and initialize the firmware app
  GameApp app;
  app.begin();

  bool running = true;
  while (running) {
    uint32_t frameStart = SDL_GetTicks();

    running = input.poll();
    if (input.quitRequested()) running = false;

    // Feed button states to the simulated button scanner
    const bool* states = input.getButtonStates();
    sim_button_states[0] = states[0];
    sim_button_states[1] = states[1];
    sim_button_states[2] = states[2];
    sim_button_states[3] = states[3];

    // Tick firmware
    app.update(SDL_GetTicks());

    // Render: get framebuffer from display device
    const uint16_t* fb = app.getDisplayFramebuffer();
    if (fb) {
      renderer.present(fb, SIM_SCREEN_W, SIM_SCREEN_H);
    }

    uint32_t frameTime = SDL_GetTicks() - frameStart;
    if (frameTime < SIM_FRAME_INTERVAL_MS) {
      SDL_Delay(SIM_FRAME_INTERVAL_MS - frameTime);
    }
  }

  input.shutdown();
  renderer.shutdown();
  SDL_Quit();
  return 0;
}
