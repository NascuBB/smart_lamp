#ifndef HtmlResponses_H
#define HtmlResponses_H

#include <ESPAsyncWebServer.h>
#include <Arduino.h>
#include "Settings.h"

void sendWrap(AsyncWebServerRequest *request, const char* progmem_content);
void sendIndex(AsyncWebServerRequest *request, const uint8_t brightness, const uint8_t mode, const uint8_t speed, const char* colorHex, const bool power);
//String index_P(const uint8_t brightness, const uint8_t mode, const bool power, const char* color);
// String wrap_P(const char* progmem_content, const Settings& settings, const Last& last, int brightness);

extern const char html_template_p1[];
extern const char html_template_p2[];
//extern const char html_template_short[];
extern const char html_index_p1[];
extern const char html_index_p2[];
extern const char html_settings_p1[];
extern const char html_settings_p2[];
extern const char html_connecting[];
// extern const char html_connected[];

#endif