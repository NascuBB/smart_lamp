#include "Settings.h"

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
  return settings;
}

void saveLast(const Last &last){
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(sizeof(SettingsSTA) + sizeof(SettingsAP), last);
  EEPROM.commit();
  EEPROM.end();
}

Last loadLast(){
  Last last;
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(sizeof(SettingsSTA) + sizeof(SettingsAP), last);
  EEPROM.end();
  return last;
}
