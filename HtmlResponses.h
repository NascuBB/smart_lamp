#ifndef HtmlResponses_H
#define HtmlResponses_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include "Settings.h"

void sendIndex(AsyncWebServerRequest* request, const LampState& state);
void sendSettings(AsyncWebServerRequest* request, const SettingsAP& ap, bool useStaMode);
void sendSavedNetworks(AsyncWebServerRequest* request, const SavedWiFiStorage& storage);

extern const char html_template_p1[];
extern const char html_template_p2[];
extern const char html_index_p1[];
extern const char html_index_p2[];
extern const char html_settings_p1[];
extern const char html_settings_p2[];
extern const char html_saved_networks_p1[];
extern const char html_saved_networks_p2[];

#endif
