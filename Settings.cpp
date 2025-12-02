#include "Settings.h"


void sanitizeString(char* str, size_t size) {
  str[size - 1] = '\0';
  
  if ((unsigned char)str[0] == 0xFF) {
    str[0] = '\0';
  }
}

void saveAPSettings(const SettingsAP &settings){
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(0, settings);
  EEPROM.commit();
  EEPROM.end();
}

SettingsAP loadAPSettings() {
  SettingsAP settings;
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(0, settings);
  EEPROM.end();
  if ((unsigned char)settings.ssid[0] == 0xFF || settings.ssid[0] == 0) {
    strlcpy(settings.ssid, "Led_Setup", sizeof(settings.ssid));
    settings.password[0] = '\0';
    settings.requirePass = false;
    
    saveAPSettings(settings); 
  } else {
    settings.ssid[sizeof(settings.ssid) - 1] = '\0';
    settings.password[sizeof(settings.password) - 1] = '\0';
  }

  return settings;
}

void saveSTASettings(const SettingsSTA &settings) {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(sizeof(SettingsAP), settings);
  EEPROM.commit();
  EEPROM.end();
}

SettingsSTA loadSTASettings() {
  SettingsSTA settings;
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(sizeof(SettingsAP), settings);
  EEPROM.end();

  if ((unsigned char)settings.ssid[0] == 0xFF) {
    settings.ssid[0] = '\0';
    settings.password[0] = '\0';
    settings.useStaMode = false;

    saveSTASettings(settings);
  } else {
    settings.ssid[sizeof(settings.ssid) - 1] = '\0';
    settings.password[sizeof(settings.password) - 1] = '\0';
  }

  return settings;
}