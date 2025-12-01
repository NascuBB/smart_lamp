#ifndef Settings_H
#define Settings_H

#include <Arduino.h>
#include <EEPROM.h> 

#define EEPROM_SIZE 512

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

struct Last { 
  int lastEffect;
  char lastIp[15];
};

void saveSTASettings(const SettingsSTA &settings);
SettingsSTA loadSTASettings();

void saveAPSettings(const SettingsAP &settings);
SettingsAP loadAPSettings();

void saveLast(const Last &last);
Last loadLast();

#endif