#include "board_config.h"

void setup() {
  Serial.begin(board::kUsbBaud);
  pinMode(board::kBacklightPin, OUTPUT);
  digitalWrite(board::kBacklightPin, LOW);
}

void loop() {
}

