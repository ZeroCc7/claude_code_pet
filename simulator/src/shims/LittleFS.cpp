#include "LittleFS.h"

#ifdef _WIN32
#include <direct.h>
#define SIM_MKDIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#define SIM_MKDIR(path) mkdir(path, 0755)
#endif

LittleFSClass LittleFS;

bool LittleFSClass::begin() {
  SIM_MKDIR("saves");
  return true;
}
