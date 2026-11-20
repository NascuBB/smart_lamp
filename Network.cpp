#include "Network.h"
#include "config.h"
#include "Lamp.h"
#include "Settings.h"

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include <ArduinoJson.h>

namespace {
  enum class State : uint8_t {
    AccessPoint,
    Connected,
    Connecting,
    WaitingReconnect,
  };

  enum class UserResult : uint8_t { None, Pending, Connected, Failed };

  struct Candidate {
    char ssid[32];
    char password[32];
    bool save;
    bool fromUser;
  };

  struct ScanEntry {
    char ssid[33];
    int8_t rssi;
    bool secure;
  };

  const byte DNS_PORT = 53;
  const IPAddress AP_IP(192, 168, 0, 1);
  const uint8_t MAX_SCAN_ENTRIES = 20;

  DNSServer dns;
  State state = State::AccessPoint;
  bool apActive = false;
  unsigned long apOffAt = 0;
  unsigned long lastApRetryAt = 0;
  unsigned long lostAt = 0;

  Candidate queue[MAX_SAVED_WIFI_NETWORKS + 1];
  uint8_t queueLen = 0;
  uint8_t queuePos = 0;
  bool fallbackToSaved = false;
  unsigned long attemptDeadline = 0;

  bool userRequestPending = false;
  Candidate userRequest;
  UserResult userResult = UserResult::None;
  bool userWrongPassword = false;

  ScanEntry scanEntries[MAX_SCAN_ENTRIES];
  uint8_t scanCount = 0;
  unsigned long lastScanAt = 0;
  bool scanRequested = false;
  bool scanRunning = false;

  bool hasQueued(const char* ssid) {
    for (uint8_t i = 0; i < queueLen; i++) {
      if (strcmp(queue[i].ssid, ssid) == 0) return true;
    }
    return false;
  }

  void enqueue(const char* ssid, const char* password, bool save, bool fromUser) {
    if (queueLen >= sizeof(queue) / sizeof(queue[0]) || hasQueued(ssid)) {
      return;
    }
    Candidate& c = queue[queueLen++];
    strlcpy(c.ssid, ssid, sizeof(c.ssid));
    strlcpy(c.password, password, sizeof(c.password));
    c.save = save;
    c.fromUser = fromUser;
  }

  bool buildSavedQueue(bool includeLast, int found) {
    queueLen = 0;
    queuePos = 0;
    const SavedWiFiStorage& storage = savedWiFi();

    if (includeLast && storage.lastIndex >= 0) {
      const SavedWiFiNetwork& last = storage.networks[storage.lastIndex];
      enqueue(last.ssid, last.password, false, false);
    }

    if (found >= 0) {
      LOG("WiFi: scan found %d networks", found);
      for (uint8_t round = 0; round < MAX_SAVED_WIFI_NETWORKS; round++) {
        int best = -1;
        int bestRssi = -1000;
        for (int i = 0; i < found; i++) {
          String ssid = WiFi.SSID(i);
          if (hasQueued(ssid.c_str()) || !findSavedWiFiNetwork(ssid.c_str())) continue;
          if (WiFi.RSSI(i) > bestRssi) {
            bestRssi = WiFi.RSSI(i);
            best = i;
          }
        }
        if (best < 0) break;
        SavedWiFiNetwork network;
        findSavedWiFiNetwork(WiFi.SSID(best).c_str(), &network);
        enqueue(network.ssid, network.password, false, false);
      }
    }
    WiFi.scanDelete();

    return queueLen > 0;
  }

  void startAp() {
    SettingsAP ap = loadAPSettings();
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255, 255, 255, 0));
    bool usePassword = ap.requirePass && strlen(ap.password) >= 8;
    WiFi.softAP(ap.ssid, usePassword ? ap.password : nullptr);
    dns.start(DNS_PORT, "*", AP_IP);
    apActive = true;
    apOffAt = 0;
    lastApRetryAt = millis();
    LOG("WiFi: AP '%s' started", ap.ssid);
  }

  void stopAp() {
    dns.stop();
    WiFi.softAPdisconnect(true);
    apActive = false;
    apOffAt = 0;
    LOG("WiFi: AP stopped");
  }

  void enterAccessPoint(bool signalFailure) {
    WiFi.disconnect(false);
    state = State::AccessPoint;
    if (!apActive) {
      startAp();
      if (signalFailure) {
        lamp::blink(RgbColor(255, 0, 0), 2, 600);
        lamp::restore();
      }
    }
  }

  void onConnected(const Candidate& c) {
    if (c.save) {
      upsertSavedWiFiNetwork(c.ssid, c.password, true);
    } else {
      setLastSavedWiFiNetwork(c.ssid);
    }
    setUseStaMode(true);

    if (c.fromUser) {
      userResult = UserResult::Connected;
    }

    MDNS.end();
    MDNS.begin(MDNS_NAME);
    MDNS.addService("http", "tcp", 80);

    state = State::Connected;
    if (apActive) {
      apOffAt = millis() + AP_GRACE_AFTER_CONNECT_MS;
    }
    LOG("WiFi: connected to '%s', IP %s", c.ssid, WiFi.localIP().toString().c_str());
  }

  void startAttempt() {
    const Candidate& c = queue[queuePos];
    LOG("WiFi: connecting to '%s' (%u/%u)", c.ssid, queuePos + 1, queueLen);

    if (scanRunning) {
      WiFi.scanDelete();
      scanRunning = false;
    }

    if (WiFi.status() == WL_CONNECTED) {
      WiFi.disconnect(false);
      unsigned long start = millis();
      while (WiFi.status() == WL_CONNECTED && millis() - start < 2000) {
        delay(10);
      }
    }

    WiFi.mode(apActive ? WIFI_AP_STA : WIFI_STA);
    WiFi.begin(c.ssid, c.password);
    attemptDeadline = millis() + CONNECT_TIMEOUT_MS;
    state = State::Connecting;
  }

  void onAttemptFailed(bool wrongPassword) {
    const Candidate& failed = queue[queuePos];
    LOG("WiFi: '%s' failed%s", failed.ssid, wrongPassword ? " (wrong password)" : "");
    WiFi.disconnect(false);

    if (failed.fromUser) {
      userResult = UserResult::Failed;
      userWrongPassword = wrongPassword;
    }

    queuePos++;
    if (queuePos < queueLen) {
      startAttempt();
      return;
    }

    if (fallbackToSaved) {
      fallbackToSaved = false;
      if (buildSavedQueue(true, WiFi.scanNetworks(false, true))) {
        startAttempt();
        return;
      }
    }

    enterAccessPoint(true);
  }

  void handleConnecting() {
    wl_status_t status = WiFi.status();
    if (status == WL_CONNECTED) {
      onConnected(queue[queuePos]);
    } else if (status == WL_WRONG_PASSWORD || status == WL_CONNECT_FAILED) {
      onAttemptFailed(status == WL_WRONG_PASSWORD);
    } else if ((long)(millis() - attemptDeadline) >= 0) {
      onAttemptFailed(false);
    }
  }

  int scanBlocking(uint8_t segment, uint8_t segments) {
    const unsigned long timeoutMs = 8000;
    WiFi.scanNetworks(true, true);
    unsigned long start = millis();
    bool on = true;
    while (WiFi.scanComplete() == WIFI_SCAN_RUNNING && millis() - start < timeoutMs) {
      lamp::showSegmentMarker(segment, segments, on);
      on = !on;
      delay(150);
    }
    lamp::showSegmentMarker(segment, segments, false);
    int found = WiFi.scanComplete();
    return found < 0 ? 0 : found;
  }

  bool pickBestSaved(int found, const char failed[][32], uint8_t failedCount, Candidate& out) {
    int best = -1;
    int bestRssi = -1000;
    for (int i = 0; i < found; i++) {
      String ssid = WiFi.SSID(i);
      if (!findSavedWiFiNetwork(ssid.c_str())) continue;
      bool alreadyFailed = false;
      for (uint8_t j = 0; j < failedCount; j++) {
        if (ssid == failed[j]) alreadyFailed = true;
      }
      if (!alreadyFailed && WiFi.RSSI(i) > bestRssi) {
        bestRssi = WiFi.RSSI(i);
        best = i;
      }
    }

    bool picked = false;
    if (best >= 0) {
      SavedWiFiNetwork network;
      findSavedWiFiNetwork(WiFi.SSID(best).c_str(), &network);
      out = Candidate{};
      strlcpy(out.ssid, network.ssid, sizeof(out.ssid));
      strlcpy(out.password, network.password, sizeof(out.password));
      picked = true;
    }
    WiFi.scanDelete();
    LOG("WiFi: scan found %d networks, %s", found, picked ? out.ssid : "no saved network in range");
    return picked;
  }

  bool connectBlocking(const Candidate& c, uint8_t segment, uint8_t segments) {
    LOG("WiFi: boot connect to '%s'", c.ssid);
    const uint16_t stepMs = 150;
    const uint16_t steps = BOOT_CONNECT_TIMEOUT_MS / stepMs;

    WiFi.begin(c.ssid, c.password);
    for (uint16_t step = 0; step <= steps; step++) {
      wl_status_t status = WiFi.status();
      if (status == WL_CONNECTED) {
        lamp::showSegmentProgress(segment, segments, steps, steps);
        return true;
      }
      if (status == WL_WRONG_PASSWORD) {
        break;
      }
      lamp::showSegmentProgress(segment, segments, step, steps);
      delay(stepMs);
    }

    lamp::showSegmentProgress(segment, segments, steps, steps);
    WiFi.disconnect(false);
    return false;
  }

  void startScanIfRequested() {
    if (!scanRequested || state == State::Connecting) {
      return;
    }
    scanRequested = false;
    if (WiFi.scanComplete() != WIFI_SCAN_RUNNING) {
      WiFi.scanDelete();
      WiFi.scanNetworks(true, true);
    }
    scanRunning = true;
  }

  void collectScanResults() {
    if (!scanRunning) {
      return;
    }
    int found = WiFi.scanComplete();
    if (found == WIFI_SCAN_RUNNING) {
      return;
    }

    scanRunning = false;
    if (found < 0) {
      return;
    }

    scanCount = 0;
    for (int i = 0; i < found; i++) {
      String ssid = WiFi.SSID(i);
      if (ssid.length() == 0) continue;

      ScanEntry* entry = nullptr;
      for (uint8_t j = 0; j < scanCount; j++) {
        if (ssid == scanEntries[j].ssid) {
          entry = &scanEntries[j];
          break;
        }
      }
      if (entry == nullptr) {
        if (scanCount >= MAX_SCAN_ENTRIES) continue;
        entry = &scanEntries[scanCount++];
        strlcpy(entry->ssid, ssid.c_str(), sizeof(entry->ssid));
        entry->rssi = -128;
      }
      if (WiFi.RSSI(i) > entry->rssi) {
        entry->rssi = WiFi.RSSI(i);
        entry->secure = WiFi.encryptionType(i) != ENC_TYPE_NONE;
      }
    }

    WiFi.scanDelete();
    lastScanAt = millis();
    LOG("WiFi: scan done, %u networks", scanCount);
  }

  const ScanEntry* findScanned(const char* ssid) {
    for (uint8_t i = 0; i < scanCount; i++) {
      if (strcmp(scanEntries[i].ssid, ssid) == 0) return &scanEntries[i];
    }
    return nullptr;
  }
}

namespace net {

void begin() {
  WiFi.persistent(false);
  WiFi.setAutoConnect(false);
  WiFi.setAutoReconnect(false);

  uint8_t savedCount = 0;
  for (const auto& network : savedWiFi().networks) {
    if (network.used) savedCount++;
  }

  if (!getUseStaMode() || savedCount == 0) {
    startAp();
    state = State::AccessPoint;
    return;
  }

  WiFi.mode(WIFI_STA);
  const uint8_t segments = BOOT_CONNECT_SEGMENTS;

  Candidate candidate = {};
  SavedWiFiNetwork last;
  if (getLastSavedWiFiNetwork(&last)) {
    strlcpy(candidate.ssid, last.ssid, sizeof(candidate.ssid));
    strlcpy(candidate.password, last.password, sizeof(candidate.password));
    if (connectBlocking(candidate, 0, segments)) {
      onConnected(candidate);
      return;
    }
  }
  lamp::showSegmentProgress(0, segments, 1, 1);

  char failed[BOOT_CONNECT_SEGMENTS][32] = {};
  uint8_t failedCount = 0;
  for (uint8_t segment = 1; segment < segments; segment++) {
    int found = scanBlocking(segment, segments);
    if (pickBestSaved(found, failed, failedCount, candidate)) {
      if (connectBlocking(candidate, segment, segments)) {
        onConnected(candidate);
        return;
      }
      strlcpy(failed[failedCount++], candidate.ssid, sizeof(failed[0]));
    }
    lamp::showSegmentProgress(segment, segments, 1, 1);
  }

  enterAccessPoint(true);
}

void loop() {
  if (apActive) {
    dns.processNextRequest();
  }

  if (userRequestPending && state != State::Connecting) {
    userRequestPending = false;
    fallbackToSaved = (state == State::Connected || state == State::WaitingReconnect);
    queue[0] = userRequest;
    queueLen = 1;
    queuePos = 0;
    startAttempt();
  }

  switch (state) {
    case State::Connected:
      if (WiFi.status() != WL_CONNECTED) {
        LOG("WiFi: connection lost");
        MDNS.end();
        state = State::WaitingReconnect;
        lostAt = millis();
        break;
      }
      MDNS.update();
      if (apActive && apOffAt != 0 && (long)(millis() - apOffAt) >= 0) {
        stopAp();
      }
      break;

    case State::WaitingReconnect:
      if (millis() - lostAt >= RECONNECT_DELAY_MS) {
        fallbackToSaved = false;
        if (buildSavedQueue(true, WiFi.scanNetworks(false, true))) {
          startAttempt();
        } else {
          enterAccessPoint(true);
        }
      }
      break;

    case State::AccessPoint:
      if (getUseStaMode() && savedWiFi().lastIndex >= 0 && WiFi.softAPgetStationNum() == 0 &&
          millis() - lastApRetryAt >= AP_RETRY_INTERVAL_MS) {
        lastApRetryAt = millis();
        fallbackToSaved = false;
        if (buildSavedQueue(false, WiFi.scanNetworks(false, true))) {
          startAttempt();
        }
      }
      break;

    case State::Connecting:
      handleConnecting();
      break;
  }

  startScanIfRequested();
  collectScanResults();
}

bool isStaConnected() {
  return state == State::Connected;
}

ConnectError requestConnect(const char* ssid, const char* password, bool passwordProvided) {
  if (ssid == nullptr || ssid[0] == '\0') {
    return ConnectError::MissingSsid;
  }
  if (state == State::Connecting || userRequestPending) {
    return ConnectError::Busy;
  }

  Candidate c = {};
  strlcpy(c.ssid, ssid, sizeof(c.ssid));
  c.fromUser = true;

  SavedWiFiNetwork saved;
  const ScanEntry* scanned = findScanned(ssid);
  if (passwordProvided) {
    strlcpy(c.password, password == nullptr ? "" : password, sizeof(c.password));
    c.save = true;
  } else if (findSavedWiFiNetwork(ssid, &saved)) {
    strlcpy(c.password, saved.password, sizeof(c.password));
  } else if (scanned != nullptr && !scanned->secure) {
    c.save = true;
  } else {
    return ConnectError::PasswordRequired;
  }

  userRequest = c;
  userRequestPending = true;
  userResult = UserResult::Pending;
  userWrongPassword = false;
  return ConnectError::None;
}

void setStaModeEnabled(bool enabled) {
  setUseStaMode(enabled);
  if (!enabled) {
    scanCount = 0;
  }
}

void printStatusJson(Print& out) {
  JsonDocument doc;
  uint8_t connected;
  switch (userResult) {
    case UserResult::Pending:   connected = 0; break;
    case UserResult::Connected: connected = 1; break;
    case UserResult::Failed:    connected = 2; break;
    default:                    connected = (state == State::Connected) ? 1 : 2; break;
  }
  if (userResult == UserResult::Connected && state != State::Connected) {
    connected = 2;
  }

  doc["connected"] = connected;
  if (state == State::Connected) {
    doc["ip"] = WiFi.localIP().toString();
    doc["ssid"] = WiFi.SSID();
  }
  doc["needsPassword"] = userResult == UserResult::Failed && userWrongPassword;
  serializeJson(doc, out);
}

void printNetworksJson(Print& out, bool forceScan) {
  JsonDocument doc;

  if (!getUseStaMode()) {
    doc["disabled"] = true;
    doc["scanning"] = false;
    doc["networks"].to<JsonArray>();
    serializeJson(doc, out);
    return;
  }

  bool busy = state == State::Connecting || userRequestPending;
  if (!busy && (forceScan || lastScanAt == 0 || millis() - lastScanAt > SCAN_CACHE_MS)) {
    scanRequested = true;
  }

  doc["busy"] = busy;
  doc["scanning"] = !busy && (scanRequested || scanRunning);

  String connectedSsid = (state == State::Connected) ? WiFi.SSID() : String();
  JsonArray networks = doc["networks"].to<JsonArray>();
  for (uint8_t i = 0; i < scanCount; i++) {
    const ScanEntry& entry = scanEntries[i];
    JsonObject network = networks.add<JsonObject>();
    network["ssid"] = entry.ssid;
    network["rssi"] = entry.rssi;
    network["secure"] = entry.secure;
    network["saved"] = findSavedWiFiNetwork(entry.ssid);
    network["connected"] = connectedSsid == entry.ssid;
  }
  serializeJson(doc, out);
}

}
