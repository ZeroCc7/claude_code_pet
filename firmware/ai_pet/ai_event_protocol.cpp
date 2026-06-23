#include "ai_event_protocol.h"

#include <cstring>

bool AiEventProtocol::parse(const String& message, AiEvent& event,
                            const char*& error) {
  error = "invalid";
  if (message.length() == 0 ||
      message.length() > kMaximumMessageBytes) {
    error = "too_long";
    return false;
  }
  if (message[0] != '{' || message[message.length() - 1] != '}') {
    error = "invalid_json";
    return false;
  }

  char type[12] = {};
  char state[16] = {};
  if (!extractString(message, "type", type, sizeof(type)) ||
      !extractString(message, "source", event.source,
                     sizeof(event.source)) ||
      !extractString(message, "task_id", event.taskId,
                     sizeof(event.taskId))) {
    error = "missing_field";
    return false;
  }
  if (!validSource(event.source)) {
    error = "unknown_source";
    return false;
  }

  if (strcmp(type, "status") == 0) {
    if (!extractString(message, "state", state, sizeof(state)) ||
        !parseState(state, event.state)) {
      error = "unknown_state";
      return false;
    }
    event.kind = AiEventKind::Status;
    return true;
  }

  if (strcmp(type, "task") == 0) {
    char result[12] = {};
    if (!extractString(message, "state", state, sizeof(state)) ||
        strcmp(state, "completed") != 0 ||
        !extractString(message, "result", result, sizeof(result)) ||
        !extractUnsigned(message, "duration", event.durationSeconds)) {
      error = "invalid_task";
      return false;
    }
    if (strcmp(result, "success") != 0 &&
        strcmp(result, "failure") != 0 &&
        strcmp(result, "cancelled") != 0) {
      error = "invalid_result";
      return false;
    }
    event.kind = AiEventKind::Task;
    event.success = strcmp(result, "success") == 0;
    event.durationSeconds = min<uint32_t>(event.durationSeconds, 3600U);
    return true;
  }

  error = "unknown_type";
  return false;
}

const char* AiEventProtocol::stateName(AiWorkState state) {
  switch (state) {
    case AiWorkState::Submitted: return "submitted";
    case AiWorkState::Thinking: return "thinking";
    case AiWorkState::Tool: return "tool";
    case AiWorkState::Editing: return "editing";
    case AiWorkState::Waiting: return "waiting";
    case AiWorkState::Blocked: return "blocked";
    case AiWorkState::Idle: return "idle";
  }
  return "idle";
}

bool AiEventProtocol::extractString(const String& message, const char* key,
                                    char* destination, size_t capacity) {
  const String marker = String("\"") + key + "\"";
  int start = message.indexOf(marker);
  if (start < 0) {
    return false;
  }
  start = message.indexOf(':', start + marker.length());
  if (start < 0) {
    return false;
  }
  start = message.indexOf('"', start + 1);
  if (start < 0) {
    return false;
  }
  const int end = message.indexOf('"', start + 1);
  if (end < 0 || end == start + 1 ||
      static_cast<size_t>(end - start) >= capacity) {
    return false;
  }
  const String value = message.substring(start + 1, end);
  value.toCharArray(destination, capacity);
  return true;
}

bool AiEventProtocol::extractUnsigned(const String& message, const char* key,
                                      uint32_t& value) {
  const String marker = String("\"") + key + "\"";
  int start = message.indexOf(marker);
  if (start < 0) {
    return false;
  }
  start = message.indexOf(':', start + marker.length());
  if (start < 0) {
    return false;
  }
  ++start;
  while (start < static_cast<int>(message.length()) &&
         message[start] == ' ') {
    ++start;
  }
  if (start >= static_cast<int>(message.length()) ||
      !isDigit(message[start])) {
    return false;
  }
  uint32_t parsed = 0;
  while (start < static_cast<int>(message.length()) &&
         isDigit(message[start])) {
    parsed = parsed * 10 + (message[start] - '0');
    ++start;
  }
  value = parsed;
  return true;
}

bool AiEventProtocol::validSource(const char* source) {
  return strcmp(source, "codex") == 0 ||
         strcmp(source, "claude_code") == 0 ||
         strcmp(source, "opencode") == 0 ||
         strcmp(source, "codefree_o") == 0;
}

bool AiEventProtocol::parseState(const char* value, AiWorkState& state) {
  const char* names[] = {"submitted", "thinking", "tool", "editing",
                         "waiting", "blocked", "idle"};
  for (uint8_t index = 0; index < 7; ++index) {
    if (strcmp(value, names[index]) == 0) {
      state = static_cast<AiWorkState>(index);
      return true;
    }
  }
  return false;
}
