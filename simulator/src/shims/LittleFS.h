#pragma once

#include "FS.h"

class LittleFSClass : public FS {
 public:
  bool begin();
};

extern LittleFSClass LittleFS;
