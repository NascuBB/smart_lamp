#include "StringEscapeUtils.h"

String escapeJson(const String& input) {
  String output;
  output.reserve(input.length() + 8);

  for (size_t i = 0; i < input.length(); i++) {
    const char c = input[i];

    switch (c) {
      case '\\': output += F("\\\\"); break;
      case '"': output += F("\\\""); break;
      case '\b': output += F("\\b"); break;
      case '\f': output += F("\\f"); break;
      case '\n': output += F("\\n"); break;
      case '\r': output += F("\\r"); break;
      case '\t': output += F("\\t"); break;
      default:
        if (static_cast<uint8_t>(c) < 0x20) {
          char buffer[7];
          snprintf(buffer, sizeof(buffer), "\\u%04x", static_cast<uint8_t>(c));
          output += buffer;
        } else {
          output += c;
        }
        break;
    }
  }

  return output;
}

String escapeJson(const char* input) {
  if (input == nullptr) {
    return String();
  }

  return escapeJson(String(input));
}

String escapeJsSingleQuoted(const String& input) {
  String output;
  output.reserve(input.length() + 8);

  for (size_t i = 0; i < input.length(); i++) {
    const char c = input[i];

    switch (c) {
      case '\\': output += F("\\\\"); break;
      case '\'': output += F("\\'"); break;
      case '\n': output += F("\\n"); break;
      case '\r': output += F("\\r"); break;
      case '\t': output += F("\\t"); break;
      default:
        output += c;
        break;
    }
  }

  return output;
}

String escapeJsSingleQuoted(const char* input) {
  if (input == nullptr) {
    return String();
  }

  return escapeJsSingleQuoted(String(input));
}