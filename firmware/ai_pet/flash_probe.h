#pragma once

#include <Arduino.h>

struct FlashProbeResult {
  bool mounted;
  bool wrote;
  bool verified;
  bool cleaned;
};

class FlashProbe {
 public:
  FlashProbeResult run();
};

