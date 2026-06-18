#include "flash_probe.h"

#include <LittleFS.h>

FlashProbeResult FlashProbe::run() {
  constexpr char kPath[] = "/diagnostic.tmp";
  constexpr uint8_t kPayload[] = {
      0x43, 0x4C, 0x41, 0x55, 0x44, 0x45, 0x50, 0x45, 0x54,
  };

  FlashProbeResult result{};
  result.mounted = LittleFS.begin();
  if (!result.mounted) {
    return result;
  }

  File output = LittleFS.open(kPath, "w");
  result.wrote =
      output && output.write(kPayload, sizeof(kPayload)) == sizeof(kPayload);
  output.close();

  if (!result.wrote) {
    LittleFS.remove(kPath);
    return result;
  }

  File input = LittleFS.open(kPath, "r");
  result.verified = input && input.size() == sizeof(kPayload);
  for (size_t i = 0; result.verified && i < sizeof(kPayload); ++i) {
    result.verified = input.read() == kPayload[i];
  }
  input.close();

  result.cleaned = LittleFS.remove(kPath);
  return result;
}

