#include "game_app.h"

GameApp app;

void setup() {
  app.begin();
}

void loop() {
  app.update(millis());
}

