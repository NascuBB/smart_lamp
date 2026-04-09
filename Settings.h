#ifndef Settings_H
#define Settings_H

#include <Arduino.h>
#include <EEPROM.h> 

#define EEPROM_SIZE 512
#define MAX_SAVED_WIFI_NETWORKS 5

struct SettingsSTA {
  char ssid[32];
  char password[32];
  bool useStaMode;
};

struct SettingsAP{
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

// struct Last { 
//   int lastEffect;
//   char lastIp[15];
// };

void saveSTASettings(const SettingsSTA &settings);
SettingsSTA loadSTASettings();

void saveAPSettings(const SettingsAP &settings);
SettingsAP loadAPSettings();

SavedWiFiStorage loadSavedWiFiStorage();
void saveSavedWiFiStorage(const SavedWiFiStorage& storage);
bool findSavedWiFiNetwork(const char* ssid, SavedWiFiNetwork* networkOut, int* indexOut = nullptr);
bool upsertSavedWiFiNetwork(const char* ssid, const char* password, bool setAsLast);
bool getLastSavedWiFiNetwork(SavedWiFiNetwork* networkOut);
void setLastSavedWiFiNetwork(const char* ssid);

// void saveLast(const Last &last);
// Last loadLast();

#endif