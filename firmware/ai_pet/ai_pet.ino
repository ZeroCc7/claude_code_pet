#include "diagnostics_app.h"

DiagnosticsApp app;

void setup() {
  app.begin();
}

void loop() {
  app.update(millis());
}

