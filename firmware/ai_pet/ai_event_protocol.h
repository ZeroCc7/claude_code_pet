#pragma once

#include <Arduino.h>

enum class AiEventKind : uint8_t {
  Status,
  Task,
};

enum class AiWorkState : uint8_t {
  Submitted,
  Thinking,
  Tool,
  Editing,
  Waiting,
  Blocked,
  Idle,
};

struct AiEvent {
  AiEventKind kind = AiEventKind::Status;
  AiWorkState state = AiWorkState::Idle;
  char source[16] = {};
  char taskId[64] = {};
  uint32_t durationSeconds = 0;
  bool success = false;
};

class AiEventProtocol {
 public:
  static constexpr size_t kMaximumMessageBytes = 384;

  static bool parse(const String& message, AiEvent& event,
                    const char*& error);
  static const char* stateName(AiWorkState state);

 private:
  static bool extractString(const String& message, const char* key,
                            char* destination, size_t capacity);
  static bool extractUnsigned(const String& message, const char* key,
                              uint32_t& value);
  static bool validSource(const char* source);
  static bool parseState(const char* value, AiWorkState& state);
};
