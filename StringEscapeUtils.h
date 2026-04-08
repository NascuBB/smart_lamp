#ifndef STRING_ESCAPE_UTILS_H
#define STRING_ESCAPE_UTILS_H

#include <Arduino.h>

String escapeJson(const String& input);
String escapeJson(const char* input);
String escapeJsSingleQuoted(const String& input);
String escapeJsSingleQuoted(const char* input);

#endif