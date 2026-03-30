#include "Settings.h"

#include <cstddef>
#include <cstring>

namespace {
  const uint32_t SAVED_WIFI_MAGIC = 0x57534631UL; // WSF1
  const int AP_SETTINGS_ADDR = 0;
  const int STA_SETTINGS_ADDR = AP_SETTINGS_ADDR + sizeof(SettingsAP);
  const int SAVED_WIFI_ADDR = STA_SETTINGS_ADDR + sizeof(SettingsSTA);

  static_assert(SAVED_WIFI_ADDR + (int)sizeof(SavedWiFiStorage) <= EEPROM_SIZE, "EEPROM_SIZE is too small for SavedWiFiStorage");

  void sanitizeString(char* str, size_t size) {
    str[size - 1] = '\0';
    if ((unsigned char)str[0] == 0xFF) {
      str[0] = '\0';
    }
  }

  void sanitizeSavedNetwork(SavedWiFiNetwork& network) {
    sanitizeString(network.ssid, sizeof(network.ssid));
    sanitizeString(network.password, sizeof(network.password));

    if (network.ssid[0] == '\0') {
      network.used = false;
      network.password[0] = '\0';
    }
  }

  int findStorageIndexBySsid(const SavedWiFiStorage& storage, const char* ssid) {
    if (ssid == nullptr || ssid[0] == '\0') {
      return -1;
    }

    for (int i = 0; i < MAX_SAVED_WIFI_NETWORKS; i++) {
      if (storage.networks[i].used && strcmp(storage.networks[i].ssid, ssid) == 0) {
        return i;
      }
    }

    return -1;
  }

  int findFreeStorageIndex(const SavedWiFiStorage& storage) {
    for (int i = 0; i < MAX_SAVED_WIFI_NETWORKS; i++) {
      if (!storage.networks[i].used) {
        return i;
      }
    }

    return -1;
  }

  void migrateLegacyStaToSavedWiFi(SavedWiFiStorage& storage) {
    SettingsSTA sta = loadSTASettings();
    if (sta.ssid[0] == '\0') {
      return;
    }

    strlcpy(storage.networks[0].ssid, sta.ssid, sizeof(storage.networks[0].ssid));
    strlcpy(storage.networks[0].password, sta.password, sizeof(storage.networks[0].password));
    storage.networks[0].used = true;
    storage.lastIndex = 0;
  }
}

void saveAPSettings(const SettingsAP &settings){
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(AP_SETTINGS_ADDR, settings);
  EEPROM.commit();
  EEPROM.end();
}

SettingsAP loadAPSettings() {
  SettingsAP settings;
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(AP_SETTINGS_ADDR, settings);
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
  EEPROM.put(STA_SETTINGS_ADDR, settings);
  EEPROM.commit();
  EEPROM.end();
}

SettingsSTA loadSTASettings() {
  SettingsSTA settings;
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(STA_SETTINGS_ADDR, settings);
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

SavedWiFiStorage loadSavedWiFiStorage() {
  SavedWiFiStorage storage;

  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(SAVED_WIFI_ADDR, storage);
  EEPROM.end();

  if (storage.magic != SAVED_WIFI_MAGIC) {
    memset(&storage, 0, sizeof(storage));
    storage.magic = SAVED_WIFI_MAGIC;
    storage.lastIndex = -1;
    migrateLegacyStaToSavedWiFi(storage);
    saveSavedWiFiStorage(storage);
    return storage;
  }

  for (int i = 0; i < MAX_SAVED_WIFI_NETWORKS; i++) {
    sanitizeSavedNetwork(storage.networks[i]);
  }

  if (storage.lastIndex < 0 || storage.lastIndex >= MAX_SAVED_WIFI_NETWORKS || !storage.networks[storage.lastIndex].used) {
    storage.lastIndex = -1;
  }

  return storage;
}

void saveSavedWiFiStorage(const SavedWiFiStorage& storage) {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(SAVED_WIFI_ADDR, storage);
  EEPROM.commit();
  EEPROM.end();
}

bool findSavedWiFiNetwork(const char* ssid, SavedWiFiNetwork* networkOut, int* indexOut) {
  SavedWiFiStorage storage = loadSavedWiFiStorage();
  int index = findStorageIndexBySsid(storage, ssid);
  if (index < 0) {
    return false;
  }

  if (networkOut != nullptr) {
    *networkOut = storage.networks[index];
  }

  if (indexOut != nullptr) {
    *indexOut = index;
  }

  return true;
}

bool upsertSavedWiFiNetwork(const char* ssid, const char* password, bool setAsLast) {
  if (ssid == nullptr || ssid[0] == '\0') {
    return false;
  }

  SavedWiFiStorage storage = loadSavedWiFiStorage();

  int index = findStorageIndexBySsid(storage, ssid);
  if (index < 0) {
    index = findFreeStorageIndex(storage);
  }

  if (index < 0) {
    index = storage.lastIndex >= 0 ? storage.lastIndex : 0;
  }

  bool wasUsed = storage.networks[index].used;
  storage.networks[index].used = true;
  strlcpy(storage.networks[index].ssid, ssid, sizeof(storage.networks[index].ssid));

  if (password != nullptr && password[0] != '\0') {
    strlcpy(storage.networks[index].password, password, sizeof(storage.networks[index].password));
  } else if (!wasUsed) {
    storage.networks[index].password[0] = '\0';
  }

  if (setAsLast) {
    storage.lastIndex = index;
  }

  saveSavedWiFiStorage(storage);
  return true;
}

bool getLastSavedWiFiNetwork(SavedWiFiNetwork* networkOut) {
  if (networkOut == nullptr) {
    return false;
  }

  SavedWiFiStorage storage = loadSavedWiFiStorage();
  if (storage.lastIndex < 0 || storage.lastIndex >= MAX_SAVED_WIFI_NETWORKS) {
    return false;
  }

  if (!storage.networks[storage.lastIndex].used) {
    return false;
  }

  *networkOut = storage.networks[storage.lastIndex];
  return true;
}

void setLastSavedWiFiNetwork(const char* ssid) {
  if (ssid == nullptr || ssid[0] == '\0') {
    return;
  }

  SavedWiFiStorage storage = loadSavedWiFiStorage();
  int index = findStorageIndexBySsid(storage, ssid);
  if (index < 0) {
    return;
  }

  storage.lastIndex = index;
  saveSavedWiFiStorage(storage);
}