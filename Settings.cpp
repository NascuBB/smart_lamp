#include "Settings.h"
#include "config.h"

#include <EEPROM.h>
#include <cstring>

namespace {
  struct SettingsSTA {
    char ssid[32];
    char password[32];
    bool useStaMode;
  };

  struct StoredLampState {
    uint32_t magic;
    LampState state;
  };

  const uint32_t SAVED_WIFI_MAGIC = 0x57534631UL;
  const uint32_t LAMP_STATE_MAGIC = 0x4C4D5031UL;

  const int AP_SETTINGS_ADDR  = 0;
  const int STA_SETTINGS_ADDR = AP_SETTINGS_ADDR + sizeof(SettingsAP);
  const int SAVED_WIFI_ADDR   = STA_SETTINGS_ADDR + sizeof(SettingsSTA);
  const int LAMP_STATE_ADDR   = 512;

  static_assert(SAVED_WIFI_ADDR + sizeof(SavedWiFiStorage) <= LAMP_STATE_ADDR, "Wi-Fi settings overlap lamp state");
  static_assert(LAMP_STATE_ADDR + sizeof(StoredLampState) <= EEPROM_SIZE, "EEPROM_SIZE is too small");

  SavedWiFiStorage wifiStorage;
  bool commitPending = false;
  unsigned long lastWriteAt = 0;

  template <typename T>
  void writeBlock(int address, const T& value) {
    EEPROM.put(address, value);
    commitPending = true;
    lastWriteAt = millis();
  }

  void sanitizeString(char* str, size_t size) {
    str[size - 1] = '\0';
    if ((uint8_t)str[0] == 0xFF) {
      str[0] = '\0';
    }
  }

  SettingsSTA loadSta() {
    SettingsSTA sta;
    EEPROM.get(STA_SETTINGS_ADDR, sta);
    sanitizeString(sta.ssid, sizeof(sta.ssid));
    sanitizeString(sta.password, sizeof(sta.password));
    if ((uint8_t)sta.useStaMode == 0xFF) {
      sta.useStaMode = false;
    }
    return sta;
  }

  int findIndexBySsid(const char* ssid) {
    if (ssid == nullptr || ssid[0] == '\0') {
      return -1;
    }
    for (int i = 0; i < MAX_SAVED_WIFI_NETWORKS; i++) {
      if (wifiStorage.networks[i].used && strcmp(wifiStorage.networks[i].ssid, ssid) == 0) {
        return i;
      }
    }
    return -1;
  }

  int findFreeIndex() {
    for (int i = 0; i < MAX_SAVED_WIFI_NETWORKS; i++) {
      if (!wifiStorage.networks[i].used) {
        return i;
      }
    }
    return -1;
  }

  void clearNetwork(SavedWiFiNetwork& network) {
    memset(&network, 0, sizeof(network));
  }

  void saveWifiStorage() {
    writeBlock(SAVED_WIFI_ADDR, wifiStorage);
  }

  void loadWifiStorage() {
    EEPROM.get(SAVED_WIFI_ADDR, wifiStorage);

    if (wifiStorage.magic != SAVED_WIFI_MAGIC) {
      memset(&wifiStorage, 0, sizeof(wifiStorage));
      wifiStorage.magic = SAVED_WIFI_MAGIC;
      wifiStorage.lastIndex = -1;
      saveWifiStorage();
    }

    for (auto& network : wifiStorage.networks) {
      sanitizeString(network.ssid, sizeof(network.ssid));
      sanitizeString(network.password, sizeof(network.password));
      if (network.ssid[0] == '\0') {
        clearNetwork(network);
      }
    }

    if (wifiStorage.lastIndex < 0 || wifiStorage.lastIndex >= MAX_SAVED_WIFI_NETWORKS ||
        !wifiStorage.networks[wifiStorage.lastIndex].used) {
      wifiStorage.lastIndex = -1;
    }
  }

  void migrateLegacySta() {
    SettingsSTA sta = loadSta();
    if (sta.ssid[0] == '\0') {
      return;
    }

    LOG("Settings: migrating legacy STA network '%s'", sta.ssid);
    upsertSavedWiFiNetwork(sta.ssid, sta.password, true);
    memset(sta.ssid, 0, sizeof(sta.ssid));
    memset(sta.password, 0, sizeof(sta.password));
    writeBlock(STA_SETTINGS_ADDR, sta);
  }
}

void storageBegin() {
  EEPROM.begin(EEPROM_SIZE);
  loadWifiStorage();
  migrateLegacySta();
}

void storageLoop() {
  if (commitPending && millis() - lastWriteAt > 500) {
    commitPending = false;
    if (!EEPROM.commit()) {
      LOG("Settings: EEPROM commit failed");
    }
  }
}

void storageFlush() {
  commitPending = false;
  EEPROM.commit();
}

SettingsAP loadAPSettings() {
  SettingsAP settings;
  EEPROM.get(AP_SETTINGS_ADDR, settings);
  sanitizeString(settings.ssid, sizeof(settings.ssid));
  sanitizeString(settings.password, sizeof(settings.password));

  if (settings.ssid[0] == '\0') {
    strlcpy(settings.ssid, DEFAULT_AP_SSID, sizeof(settings.ssid));
    settings.password[0] = '\0';
    settings.requirePass = false;
  }
  if ((uint8_t)settings.requirePass == 0xFF) {
    settings.requirePass = false;
  }
  return settings;
}

void saveAPSettings(const SettingsAP& settings) {
  writeBlock(AP_SETTINGS_ADDR, settings);
}

bool getUseStaMode() {
  return loadSta().useStaMode;
}

void setUseStaMode(bool value) {
  SettingsSTA sta = loadSta();
  if (sta.useStaMode == value) {
    return;
  }
  sta.useStaMode = value;
  writeBlock(STA_SETTINGS_ADDR, sta);
}

const SavedWiFiStorage& savedWiFi() {
  return wifiStorage;
}

bool findSavedWiFiNetwork(const char* ssid, SavedWiFiNetwork* networkOut) {
  int index = findIndexBySsid(ssid);
  if (index < 0) {
    return false;
  }
  if (networkOut != nullptr) {
    *networkOut = wifiStorage.networks[index];
  }
  return true;
}

bool getLastSavedWiFiNetwork(SavedWiFiNetwork* networkOut) {
  if (wifiStorage.lastIndex < 0) {
    return false;
  }
  if (networkOut != nullptr) {
    *networkOut = wifiStorage.networks[wifiStorage.lastIndex];
  }
  return true;
}

bool upsertSavedWiFiNetwork(const char* ssid, const char* password, bool setAsLast) {
  if (ssid == nullptr || ssid[0] == '\0') {
    return false;
  }

  int index = findIndexBySsid(ssid);
  if (index < 0) {
    index = findFreeIndex();
  }
  if (index < 0) {
    index = (wifiStorage.lastIndex + 1) % MAX_SAVED_WIFI_NETWORKS;
  }

  SavedWiFiNetwork& network = wifiStorage.networks[index];
  bool changed = !network.used || strcmp(network.ssid, ssid) != 0 ||
                 strcmp(network.password, password == nullptr ? "" : password) != 0;

  network.used = true;
  strlcpy(network.ssid, ssid, sizeof(network.ssid));
  strlcpy(network.password, password == nullptr ? "" : password, sizeof(network.password));

  if (setAsLast && wifiStorage.lastIndex != index) {
    wifiStorage.lastIndex = index;
    changed = true;
  }

  if (changed) {
    saveWifiStorage();
  }
  return true;
}

void setLastSavedWiFiNetwork(const char* ssid) {
  int index = findIndexBySsid(ssid);
  if (index < 0 || wifiStorage.lastIndex == index) {
    return;
  }
  wifiStorage.lastIndex = index;
  saveWifiStorage();
}

bool deleteSavedWiFiNetwork(const char* ssid) {
  int index = findIndexBySsid(ssid);
  if (index < 0) {
    return false;
  }

  clearNetwork(wifiStorage.networks[index]);

  if (wifiStorage.lastIndex == index) {
    wifiStorage.lastIndex = -1;
    for (int i = 0; i < MAX_SAVED_WIFI_NETWORKS; i++) {
      if (wifiStorage.networks[i].used) {
        wifiStorage.lastIndex = i;
        break;
      }
    }
  }

  saveWifiStorage();
  return true;
}

bool updateSavedWiFiNetwork(const char* oldSsid, const char* newSsid, const char* newPassword) {
  if (newSsid == nullptr || newSsid[0] == '\0') {
    return false;
  }

  int sourceIndex = findIndexBySsid(oldSsid);
  if (sourceIndex < 0) {
    return false;
  }

  int targetIndex = findIndexBySsid(newSsid);
  int writeIndex = (targetIndex >= 0) ? targetIndex : sourceIndex;

  SavedWiFiNetwork& target = wifiStorage.networks[writeIndex];
  const char* password = (newPassword != nullptr) ? newPassword : wifiStorage.networks[sourceIndex].password;
  char passwordCopy[sizeof(target.password)];
  strlcpy(passwordCopy, password, sizeof(passwordCopy));

  target.used = true;
  strlcpy(target.ssid, newSsid, sizeof(target.ssid));
  strlcpy(target.password, passwordCopy, sizeof(target.password));

  if (writeIndex != sourceIndex) {
    clearNetwork(wifiStorage.networks[sourceIndex]);
    if (wifiStorage.lastIndex == sourceIndex) {
      wifiStorage.lastIndex = writeIndex;
    }
  }

  saveWifiStorage();
  return true;
}

bool loadLampState(LampState& state) {
  StoredLampState stored;
  EEPROM.get(LAMP_STATE_ADDR, stored);
  if (stored.magic != LAMP_STATE_MAGIC) {
    return false;
  }
  state = stored.state;
  return true;
}

void saveLampState(const LampState& state) {
  StoredLampState stored;
  stored.magic = LAMP_STATE_MAGIC;
  stored.state = state;
  writeBlock(LAMP_STATE_ADDR, stored);
}
