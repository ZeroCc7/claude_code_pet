#pragma once

#include <Arduino.h>

enum class AiEventKind : uint8_t {
  Start,
  End,
};

struct AiEvent {
  AiEventKind kind = AiEventKind::Start;
  char source[16] = {};
};

class AiEventProtocol {
 public:
  static constexpr size_t kMaximumMessageBytes = 384;

  static bool parse(const String& message, AiEvent& event,
                    const char*& error);

 private:
  static bool extractString(const String& message, const char* key,
                            char* destination, size_t capacity);
  static bool validSource(const char* source);
};
