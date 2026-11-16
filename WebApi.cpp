#include "WebApi.h"
#include "config.h"
#include "HtmlResponses.h"
#include "Lamp.h"
#include "Network.h"
#include "Settings.h"
#include "SimpleOTA.h"

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <Updater.h>

namespace {
  const size_t MAX_JSON_BODY = 2048;

  AsyncWebServer server(80);

  unsigned long rebootAt = 0;
  bool updateRequested = false;
  bool autoUpdateChecked = false;
  AsyncWebServerRequest* uploadOwner = nullptr;
  bool uploadInProgress = false;
  bool uploadFailed = false;
  String uploadError;
  bool restoreLampRequested = false;
  uint8_t uploadPercent = 0;
  unsigned long lastUploadDataAt = 0;
  const unsigned long UPLOAD_STALL_TIMEOUT_MS = 10000;

  using JsonHandler = void (*)(AsyncWebServerRequest*, JsonDocument&);

  void sendJson(AsyncWebServerRequest* request, int code, const __FlashStringHelper* body) {
    request->send(code, F("application/json"), body);
  }

  void sendOk(AsyncWebServerRequest* request) {
    sendJson(request, 200, F("{\"success\":true}"));
  }

  void sendError(AsyncWebServerRequest* request, int code, const char* message) {
    JsonDocument doc;
    doc["error"] = message;
    String body;
    serializeJson(doc, body);
    request->send(code, F("application/json"), body);
  }

  void onJsonPost(const char* uri, JsonHandler handler) {
    server.on(uri, HTTP_POST,
      [handler](AsyncWebServerRequest* request) {
        const char* body = static_cast<const char*>(request->_tempObject);
        if (body == nullptr) {
          sendError(request, 400, "Empty or too large body");
          return;
        }
        JsonDocument doc;
        if (deserializeJson(doc, body)) {
          sendError(request, 400, "Invalid JSON");
          return;
        }
        handler(request, doc);
      },
      nullptr,
      [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
        if (total > MAX_JSON_BODY) {
          return;
        }
        if (index == 0) {
          request->_tempObject = malloc(total + 1);
        }
        char* body = static_cast<char*>(request->_tempObject);
        if (body == nullptr) {
          return;
        }
        memcpy(body + index, data, len);
        if (index + len == total) {
          body[total] = '\0';
        }
      });
  }

  void handleSetPower(AsyncWebServerRequest* request, JsonDocument& doc) {
    bool power = doc["power"].is<bool>() ? doc["power"].as<bool>() : !lamp::state().power;
    lamp::setPower(power);
    sendOk(request);
  }

  void handleSetMode(AsyncWebServerRequest* request, JsonDocument& doc) {
    if (!doc["mode"].is<int>() || !lamp::setMode(doc["mode"].as<int>())) {
      sendError(request, 400, "Invalid mode");
      return;
    }
    sendOk(request);
  }

  void handleSetSpeed(AsyncWebServerRequest* request, JsonDocument& doc) {
    if (!doc["speed"].is<int>() || !lamp::setSpeed(doc["speed"].as<int>())) {
      sendError(request, 400, "Invalid speed");
      return;
    }
    sendOk(request);
  }

  void handleSetBrightness(AsyncWebServerRequest* request, JsonDocument& doc) {
    if (!doc["brightness"].is<int>()) {
      sendError(request, 400, "Invalid brightness");
      return;
    }
    lamp::setBrightness(constrain(doc["brightness"].as<int>(), 0, 255));
    sendOk(request);
  }

  void handleSetColor(AsyncWebServerRequest* request, JsonDocument& doc) {
    if (!lamp::setColor(doc["color"] | "")) {
      sendError(request, 400, "Invalid color");
      return;
    }
    sendOk(request);
  }

  void handleSetGradient(AsyncWebServerRequest* request, JsonDocument& doc) {
    JsonArrayConst colors = doc["colors"];
    JsonArrayConst stops = doc["stops"];
    size_t count = colors.size();
    if (count < 2 || count > MAX_GRADIENT_STOPS) {
      sendError(request, 400, "Need 2-10 colors");
      return;
    }

    const char* colorValues[MAX_GRADIENT_STOPS];
    float positions[MAX_GRADIENT_STOPS];
    bool hasPositions = stops.size() == count;
    for (size_t i = 0; i < count; i++) {
      colorValues[i] = colors[i] | "";
      positions[i] = hasPositions ? stops[i].as<float>() : 0;
    }

    if (!lamp::setGradient(colorValues, hasPositions ? positions : nullptr, count)) {
      sendError(request, 400, "Invalid gradient");
      return;
    }
    sendOk(request);
  }

  void handleConnectNetwork(AsyncWebServerRequest* request, JsonDocument& doc) {
    switch (net::requestConnect(doc["ssid"] | "", doc["password"] | "", doc["passwordProvided"] | false)) {
      case net::ConnectError::None:
        sendJson(request, 200, F("{\"status\":\"connecting\"}"));
        break;
      case net::ConnectError::MissingSsid:
        sendError(request, 400, "Missing ssid");
        break;
      case net::ConnectError::Busy:
        sendError(request, 409, "Connection already in progress");
        break;
      case net::ConnectError::PasswordRequired:
        sendJson(request, 400, F("{\"error\":\"password_required\",\"requiresPassword\":true}"));
        break;
    }
  }

  void handleSaveStaMode(AsyncWebServerRequest* request, JsonDocument& doc) {
    if (!doc["useStaMode"].is<bool>()) {
      sendError(request, 400, "Missing useStaMode");
      return;
    }
    net::setStaModeEnabled(doc["useStaMode"].as<bool>());
    sendOk(request);
  }

  void handleSaveGeneralSettings(AsyncWebServerRequest* request, JsonDocument& doc) {
    GeneralPrefs prefs = getGeneralPrefs();
    prefs.rememberMode = doc["rememberMode"] | prefs.rememberMode;
    prefs.rememberLevels = doc["rememberLevels"] | prefs.rememberLevels;
    prefs.autoUpdate = doc["autoUpdate"] | prefs.autoUpdate;
    lamp::setPrefs(prefs);
    sendOk(request);
  }

  void handleSaveAp(AsyncWebServerRequest* request, JsonDocument& doc) {
    SettingsAP settings = {};
    strlcpy(settings.ssid, doc["ssid"] | "", sizeof(settings.ssid));
    strlcpy(settings.password, doc["password"] | "", sizeof(settings.password));
    settings.requirePass = doc["requirePass"] | false;

    if (settings.requirePass && strlen(settings.password) < 8) {
      sendError(request, 400, "Short password");
      return;
    }
    if (settings.ssid[0] == '\0') {
      strlcpy(settings.ssid, DEFAULT_AP_SSID, sizeof(settings.ssid));
    }
    saveAPSettings(settings);
    sendOk(request);
  }

  void handleUpdateSavedNetwork(AsyncWebServerRequest* request, JsonDocument& doc) {
    const char* oldSsid = doc["oldSsid"] | "";
    const char* newSsid = doc["newSsid"] | "";
    bool passwordProvided = doc["passwordProvided"] | false;
    const char* password = passwordProvided ? (doc["password"] | "") : nullptr;

    if (!updateSavedWiFiNetwork(oldSsid, newSsid, password)) {
      sendError(request, 400, "Update failed");
      return;
    }
    sendOk(request);
  }

  void handleDeleteSavedNetwork(AsyncWebServerRequest* request, JsonDocument& doc) {
    if (!deleteSavedWiFiNetwork(doc["ssid"] | "")) {
      sendError(request, 404, "Network not found");
      return;
    }
    sendOk(request);
  }

  void abortUpload(const char* reason) {
    LOG("Upload aborted: %s", reason);
    if (Update.isRunning()) {
      Update.end();
    }
    uploadOwner = nullptr;
    uploadInProgress = false;
    restoreLampRequested = true;
  }

  void failUpload(const String& error) {
    uploadFailed = true;
    uploadError = error;
    if (Update.isRunning()) {
      Update.end();
    }
    LOG("Upload failed: %s", error.c_str());
  }

  void handleFirmwareUpload(AsyncWebServerRequest* request, const String& filename, size_t index,
                            uint8_t* data, size_t len, bool final) {
    if (index == 0) {
      if (uploadOwner != nullptr) {
        return;
      }
      LOG("Upload: %s, %u bytes", filename.c_str(), request->contentLength());
      uploadOwner = request;
      uploadInProgress = true;
      uploadFailed = false;
      uploadError = String();
      uploadPercent = 0;
      request->onDisconnect([request]() {
        if (uploadOwner == request) {
          abortUpload("client disconnected");
        }
      });

      if (Update.isRunning()) {
        Update.end();
      }
      if (len == 0 || data[0] != 0xE9) {
        failUpload(F("Not an ESP8266 firmware file"));
      } else {
        Update.runAsync(true);
        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if (!Update.begin(maxSketchSpace, U_FLASH)) {
          failUpload(Update.getErrorString());
        }
      }
    }

    if (request != uploadOwner) {
      return;
    }
    lastUploadDataAt = millis();
    if (uploadFailed) {
      return;
    }

    if (Update.write(data, len) != len) {
      failUpload(Update.getErrorString());
      return;
    }
    if (request->contentLength() > 0) {
      uploadPercent = (uint64_t)(index + len) * 100 / request->contentLength();
    }

    if (final && !Update.end(true)) {
      failUpload(Update.getErrorString());
    }
  }

  void handleFirmwareUploaded(AsyncWebServerRequest* request) {
    if (request != uploadOwner) {
      sendError(request, 409, "No firmware file or another upload in progress");
      return;
    }

    uploadOwner = nullptr;
    uploadInProgress = false;
    if (uploadFailed) {
      sendError(request, 500, uploadError.c_str());
      restoreLampRequested = true;
      return;
    }

    LOG("Upload complete, rebooting");
    sendJson(request, 200, F("{\"status\":\"ok\"}"));
    rebootAt = millis() + 1000;
  }

  void runGithubUpdate(bool interactive) {
    UpdateCheck check = checkForUpdate(OTA_LATEST_RELEASE_URL, FW_VERSION);
    if (check != UpdateCheck::Available && !interactive) {
      return;
    }
    if (check != UpdateCheck::Available) {
      RgbColor color = (check == UpdateCheck::UpToDate) ? RgbColor(0, 255, 0) : RgbColor(255, 100, 0);
      lamp::blink(color, 3, 400);
      lamp::restore();
      return;
    }

    lamp::fill(RgbColor(0));
    server.end();
    LOG("OTA: web server stopped, free heap %u", ESP.getFreeHeap());
    bool ok = performOTA(OTA_FIRMWARE_URL, [](int percent) {
      lamp::showProgress(percent, RgbColor(0, 255, 255));
    });

    if (ok) {
      lamp::saveNow();
      storageFlush();
      lamp::fill(RgbColor(0, 255, 0));
      delay(500);
      lamp::fill(RgbColor(0));
      ESP.restart();
    }

    server.begin();
    lamp::blink(RgbColor(255, 0, 0), 3, 600);
    lamp::restore();
  }
}

namespace web {

void begin() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    sendIndex(request, lamp::state());
  });
  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest* request) {
    sendSettings(request, loadAPSettings(), getUseStaMode());
  });
  server.on("/saved_networks", HTTP_GET, [](AsyncWebServerRequest* request) {
    sendSavedNetworks(request, savedWiFi());
  });

  onJsonPost("/api/setPower", handleSetPower);
  onJsonPost("/api/setMode", handleSetMode);
  onJsonPost("/api/setSpeed", handleSetSpeed);
  onJsonPost("/api/setBrightness", handleSetBrightness);
  onJsonPost("/api/setColor", handleSetColor);
  onJsonPost("/api/setGradient", handleSetGradient);

  onJsonPost("/api/connect_network", handleConnectNetwork);
  onJsonPost("/api/save_sta_mode", handleSaveStaMode);
  onJsonPost("/api/save_ap", handleSaveAp);
  onJsonPost("/api/save_general_settings", handleSaveGeneralSettings);
  onJsonPost("/api/saved_networks/update", handleUpdateSavedNetwork);
  onJsonPost("/api/saved_networks/delete", handleDeleteSavedNetwork);

  server.on("/api/connection_status", HTTP_GET, [](AsyncWebServerRequest* request) {
    AsyncResponseStream* response = request->beginResponseStream("application/json");
    net::printStatusJson(*response);
    request->send(response);
  });

  server.on("/api/networks", HTTP_GET, [](AsyncWebServerRequest* request) {
    bool force = request->hasParam("force") && request->getParam("force")->value() == "1";
    AsyncResponseStream* response = request->beginResponseStream("application/json");
    net::printNetworksJson(*response, force);
    request->send(response);
  });

  server.on("/api/reboot", HTTP_POST, [](AsyncWebServerRequest* request) {
    sendJson(request, 200, F("{\"message\":\"Rebooting\"}"));
    rebootAt = millis() + 500;
  });

  server.on("/api/update", HTTP_POST, [](AsyncWebServerRequest* request) {
    request->send(200, F("text/plain"), F("Checking for updates..."));
    updateRequested = true;
  });

  server.on("/api/firmware", HTTP_POST, handleFirmwareUploaded, handleFirmwareUpload);

  server.onNotFound([](AsyncWebServerRequest* request) {
    if (request->url().startsWith("/api/")) {
      sendError(request, 404, "Not found");
      return;
    }
    sendIndex(request, lamp::state());
  });

  server.begin();
}

void loop() {
  if (uploadInProgress) {
    if (millis() - lastUploadDataAt > UPLOAD_STALL_TIMEOUT_MS) {
      abortUpload("stalled");
    } else {
      lamp::showProgress(uploadPercent, RgbColor(0, 255, 255));
    }
  }

  if (restoreLampRequested) {
    restoreLampRequested = false;
    lamp::restore();
  }

  if (rebootAt != 0 && (long)(millis() - rebootAt) >= 0) {
    LOG("Rebooting");
    lamp::saveNow();
    storageFlush();
    lamp::turnOffNow();
    ESP.restart();
  }

  if (!autoUpdateChecked && net::isStaConnected()) {
    autoUpdateChecked = true;
    if (getGeneralPrefs().autoUpdate) {
      LOG("OTA: automatic update check");
      runGithubUpdate(false);
    }
  }

  if (updateRequested) {
    updateRequested = false;
    runGithubUpdate(true);
  }
}

bool isUploadingFirmware() {
  return uploadInProgress;
}

}
