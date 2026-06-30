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

  char type[8] = {};
  if (!extractString(message, "type", type, sizeof(type)) ||
      !extractString(message, "source", event.source,
                     sizeof(event.source))) {
    error = "missing_field";
    return false;
  }
  if (!validSource(event.source)) {
    error = "unknown_source";
    return false;
  }
  if (strcmp(type, "start") == 0) {
    event.kind = AiEventKind::Start;
    return true;
  }
  if (strcmp(type, "end") == 0) {
    event.kind = AiEventKind::End;
    return true;
  }
  error = "unknown_type";
  return false;
}

bool AiEventProtocol::extractString(const String& message, const char* key,
                                    char* destination, size_t capacity) {
  const String marker = String("\"") + key + "\"";
  int start = message.indexOf(marker);
  if (start < 0) {
    return false;
  }
  start = message.indexOf(':', start + marker.length());
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

bool AiEventProtocol::validSource(const char* source) {
  return strcmp(source, "codex") == 0 ||
         strcmp(source, "claude-code") == 0 ||
         strcmp(source, "opencode") == 0 ||
         strcmp(source, "codefree-o") == 0;
}
