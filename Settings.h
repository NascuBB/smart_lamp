#ifndef Settings_H
#define Settings_H

#include <Arduino.h>

#define EEPROM_SIZE 1024
#define MAX_SAVED_WIFI_NETWORKS 5
#define MAX_GRADIENT_STOPS 10

struct SettingsAP {
  char ssid[32];
  char password[32];
  bool requirePass;
};

struct SavedWiFiNetwork {
  char ssid[32];
  char password[32];
  bool used;
};

struct SavedWiFiStorage {
  uint32_t magic;
  int8_t lastIndex;
  SavedWiFiNetwork networks[MAX_SAVED_WIFI_NETWORKS];
};

struct GradientStop {
  float position;
  char colorHex[8];
};

struct LampState {
  uint8_t mode;
  uint8_t brightness;
  uint8_t speed;
  bool power;
  char colorHex[8];
  uint8_t stopsCount;
  GradientStop stops[MAX_GRADIENT_STOPS];
};

void storageBegin();
void storageLoop();
void storageFlush();

SettingsAP loadAPSettings();
void saveAPSettings(const SettingsAP& settings);

bool getUseStaMode();
void setUseStaMode(bool value);

const SavedWiFiStorage& savedWiFi();
bool findSavedWiFiNetwork(const char* ssid, SavedWiFiNetwork* networkOut = nullptr);
bool getLastSavedWiFiNetwork(SavedWiFiNetwork* networkOut);
bool upsertSavedWiFiNetwork(const char* ssid, const char* password, bool setAsLast);
void setLastSavedWiFiNetwork(const char* ssid);
bool deleteSavedWiFiNetwork(const char* ssid);
bool updateSavedWiFiNetwork(const char* oldSsid, const char* newSsid, const char* newPassword);

bool loadLampState(LampState& state);
void saveLampState(const LampState& state);

#endif
