#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>

#include "Settings.h"
#include "HtmlResponses.h"
#include "SimpleOTA.h"

#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>

#define FW_VERSION "0.2.1"

//======================================LED======================================
//-------------------GLOBAL-------------------
#define LED_PIN     2
#define NUM_LEDS    30
#define INTERVAL    20
#define CONNECT_ATTEMPT_TIMEOUT_MS 5000
#define CONNECT_PROGRESS_STEP_MS 150

unsigned long lastUpdate = 0;
uint8_t brightness = 255;
uint8_t mode = 1;
uint8_t speed = 3;
bool power = true;
bool powerStateChanged = false;
char colorHex[8] = "#00ff00";

NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> strip(NUM_LEDS); //port 3 rx
//NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart1Ws2812xMethod> strip(NUM_LEDS, LED_PIN);
//-------------------GLOBAL-------------------

//-------------------Gradient-------------------
#define MAX_STOPS    10

GradientStop stops[10];
uint8_t stopsCount = 3;
//-------------------Gradient-------------------



//-------------------Rainbow/RGB-------------------
#define RAINBOW_SCALE 6   // больше = длиннее радуга, меньше = короче
#define RGB_SCALE     1   // больше = длиннее радуга, меньше = короче
uint16_t rainbowOffset = 0;
//-------------------Rainbow/RGB-------------------

//-------------------Fire-------------------
NeoPixelAnimator animations(NUM_LEDS); // один аниматор на пиксель

struct FirePixelState {
  RgbColor color;
};

FirePixelState states[NUM_LEDS];
//-------------------Fire-------------------
//======================================LED======================================


//======================================Server======================================
const byte DNS_PORT = 53;
DNSServer dnsServer;
AsyncWebServer server(80);

IPAddress apIP(192, 168, 0, 1);

unsigned long connectionStartTime = 0;
bool isConnecting = false;
bool isConnected = false;

bool isUpdateRequested = false;

String cachedNetworksJson = "[]";
unsigned long lastNetworksScanAt = 0;
bool isNetworksScanInProgress = false;
unsigned long scanPausedUntilMs = 0;
char pendingConnectSsid[32] = "";
char pendingConnectPassword[32] = "";
bool pendingConnectUsedSavedPassword = false;
bool pendingConnectSaveProvidedCredentials = false;
char rollbackConnectSsid[32] = "";
char rollbackConnectPassword[32] = "";
bool hasRollbackConnectCredentials = false;

bool hasQueuedConnectRequest = false;
//======================================Server======================================

void startAP();

void stopNetworksScan() {
  WiFi.scanDelete();
  isNetworksScanInProgress = false;
  scanPausedUntilMs = 0;
}

void cancelAndPauseNetworksScan(unsigned long pauseMs) {
  stopNetworksScan();

  unsigned long until = millis() + pauseMs;
  if ((long)(until - scanPausedUntilMs) > 0) {
    scanPausedUntilMs = until;
  }
}

String escapeJson(const String& input) {
  String output;
  output.reserve(input.length() + 8);

  for (size_t i = 0; i < input.length(); i++) {
    char c = input[i];
    if (c == '\\') {
      output += "\\\\";
    } else if (c == '"') {
      output += "\\\"";
    } else {
      output += c;
    }
  }

  return output;
}

String buildNetworksJsonFromScanResults(const int count) {
  String connectedSsid = "";
  if (WiFi.status() == WL_CONNECTED) {
    connectedSsid = WiFi.SSID();
  }

  String result = "[";
  result.reserve((count * 80) + 2);

  for (int i = 0; i < count; i++) {
    if (i > 0) {
      result += ',';
    }

    String ssidEscaped = escapeJson(WiFi.SSID(i));
    bool isConnected = (connectedSsid.length() > 0 && connectedSsid == WiFi.SSID(i));
    bool isSaved = findSavedWiFiNetwork(WiFi.SSID(i).c_str(), nullptr);
    bool secure = WiFi.encryptionType(i) != ENC_TYPE_NONE;
    result += "{\"ssid\":\"";
    result += ssidEscaped;
    result += "\",\"rssi\":";
    result += String(WiFi.RSSI(i));
    result += ",\"secure\":";
    result += secure ? "true" : "false";
    result += ",\"saved\":";
    result += isSaved ? "true" : "false";
    result += ",\"connected\":";
    result += isConnected ? "true" : "false";
    result += '}';
  }

  result += ']';
  return result;
}

void startNetworksScan(bool force) {
  if ((long)(millis() - scanPausedUntilMs) < 0) {
    return;
  }

  if (isConnecting || hasQueuedConnectRequest) {
    return;
  }

  if (WiFi.getMode() == WIFI_AP) {
    WiFi.mode(WIFI_AP_STA);
  }

  int scanState = WiFi.scanComplete();
  if (scanState == WIFI_SCAN_RUNNING) {
    isNetworksScanInProgress = true;
    return;
  }

  if (force) {
    WiFi.scanDelete();
  }

  WiFi.scanNetworks(true, true);
  isNetworksScanInProgress = true;
}

void updateNetworksScanCache() {
  int scanState = WiFi.scanComplete();

  if (scanState == WIFI_SCAN_RUNNING) {
    isNetworksScanInProgress = true;
    return;
  }

  if (scanState == WIFI_SCAN_FAILED) {
    isNetworksScanInProgress = false;
    return;
  }

  if (scanState >= 0) {
    cachedNetworksJson = buildNetworksJsonFromScanResults(scanState);
    lastNetworksScanAt = millis();
    isNetworksScanInProgress = false;
    WiFi.scanDelete();
  }
}

bool isScannedNetworkOpen(const char* ssid, bool* foundOut) {
  if (foundOut != nullptr) {
    *foundOut = false;
  }

  if (ssid == nullptr || ssid[0] == '\0') {
    return false;
  }

  int scanState = WiFi.scanComplete();
  if (scanState < 0) {
    return false;
  }

  for (int i = 0; i < scanState; i++) {
    if (strcmp(WiFi.SSID(i).c_str(), ssid) == 0) {
      if (foundOut != nullptr) {
        *foundOut = true;
      }

      return WiFi.encryptionType(i) == ENC_TYPE_NONE;
    }
  }

  return false;
}

bool beginConnectionAttempt(const char* ssid, const char* password, bool usedSavedPassword, bool saveProvidedCredentials) {
  if (ssid == nullptr || ssid[0] == '\0') {
    return false;
  }

  // Stop scan immediately so connection requests are not delayed or dropped.
  cancelAndPauseNetworksScan(8000);
  
  const char* safePassword = (password == nullptr) ? "" : password;

  SettingsSTA currentStaSettings = loadSTASettings();
  if (currentStaSettings.ssid[0] != '\0' && strcmp(currentStaSettings.ssid, ssid) != 0) {
    strlcpy(rollbackConnectSsid, currentStaSettings.ssid, sizeof(rollbackConnectSsid));
    strlcpy(rollbackConnectPassword, currentStaSettings.password, sizeof(rollbackConnectPassword));
    hasRollbackConnectCredentials = true;
  } else {
    hasRollbackConnectCredentials = false;
    rollbackConnectSsid[0] = '\0';
    rollbackConnectPassword[0] = '\0';
  }

  WiFi.softAPdisconnect(true);
  WiFi.disconnect(false);
  WiFi.mode(WIFI_STA);

  //Serial.println("Attempting to connect");
  connectionStartTime = millis();

  /*
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(safePassword);
  */

  WiFi.begin(ssid, safePassword);

  isConnecting = true;
  pendingConnectUsedSavedPassword = usedSavedPassword;
  pendingConnectSaveProvidedCredentials = saveProvidedCredentials;
  strlcpy(pendingConnectSsid, ssid, sizeof(pendingConnectSsid));
  strlcpy(pendingConnectPassword, safePassword, sizeof(pendingConnectPassword));

  return true;
}

void restorePreviousConnectionAfterFailure() {
  if (hasRollbackConnectCredentials && rollbackConnectSsid[0] != '\0') {
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);

    WiFi.begin(rollbackConnectSsid, rollbackConnectPassword);
    return;
  }

  startAP();
}

//======================================Server Init======================================
void startAP() {
  //Serial.println("AP MODE запуск");
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  SettingsAP s = loadAPSettings();
  const char* apSSID = (s.ssid[0] == '\0') ? "LED_Setup" : s.ssid;
  if(s.requirePass){
    WiFi.softAP(apSSID, s.password);
  }
  else{
    WiFi.softAP(apSSID);
  }
  dnsServer.start(DNS_PORT, "*", apIP);  // Перехватываем все домены
  server.onNotFound([](AsyncWebServerRequest *request){
    sendIndex(request, brightness, mode, speed, colorHex, stops, stopsCount, power);
  });

  //Serial.println("AP MODE запущен");
}

void beginServer(){
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    sendIndex(request, brightness, mode, speed, colorHex, stops, stopsCount, power);
  });

  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    SettingsSTA sSta = loadSTASettings();
    SettingsAP sAp = loadAPSettings();

    sendSettings(request, sSta, sAp);
  });

  server.on("/connecting", HTTP_GET, [](AsyncWebServerRequest *request){
    sendWrap(request, html_connecting);
  });

  server.on("/api/reboot", HTTP_POST, [](AsyncWebServerRequest *request){
    request->send(200, F("application/json"), F("{\"message\":\"Rebooting\"}"));
    if( mode == 4){
      animations.StopAll();
    }
    turnOff();
    //delay(1500);
    ESP.restart();
  });

  server.on("/api/save_sta_mode", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<128> doc;
      DeserializationError error = deserializeJson(doc, data);
      if (error) {
        request->send(400, F("application/json"), F("{\"error\":\"Invalid JSON\"}"));
        return;
      }

      if (!doc.containsKey("useStaMode")) {
        request->send(400, F("application/json"), F("{\"error\":\"Missing useStaMode\"}"));
        return;
      }

      SettingsSTA settings = loadSTASettings();
      settings.useStaMode = doc["useStaMode"];

      saveSTASettings(settings);

      if (!settings.useStaMode) {
        cachedNetworksJson = "[]";
        stopNetworksScan();
      }

      request->send(200, F("application/json"), F("{\"status\":\"saved\"}"));
  });

  server.on("/api/save_sta_credentials", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<256> doc;
      DeserializationError error = deserializeJson(doc, data);
      if (error) {
        request->send(400, F("application/json"), F("{\"error\":\"Invalid JSON\"}"));
        return;
      }

      SettingsSTA settings = loadSTASettings();
      strlcpy(settings.ssid, doc["ssid"] | "", sizeof(settings.ssid));
      strlcpy(settings.password, doc["password"] | "", sizeof(settings.password));

      saveSTASettings(settings);
      if (settings.ssid[0] != '\0') {
        upsertSavedWiFiNetwork(settings.ssid, settings.password, true);
      }

      request->send(200, F("application/json"), F("{\"status\":\"saved\"}"));
  });

  server.on("/api/save_ap", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<256> doc;
      DeserializationError error = deserializeJson(doc, data);
      if (error) {
        request->send(400, F("application/json"), F("{\"error\":\"Invalid JSON\"}"));
        return;
      }

      bool reqPass = doc["requirePass"];

      const char* password = doc["password"];
      if (reqPass && (password == nullptr || strlen(password) < 8)){
        request->send(400, F("application/json"), F("{\"error\":\"Short password\"}"));
        return;
      }

      SettingsAP settings;
      strlcpy(settings.ssid, doc["ssid"] | "", sizeof(settings.ssid));
      strlcpy(settings.password, doc["password"] | "", sizeof(settings.password));
      settings.requirePass = doc["requirePass"];

      saveAPSettings(settings);

      request->send(200, F("application/json"), F("{\"status\":\"saved\"}"));
  });

  server.on("/api/connect", HTTP_POST, [](AsyncWebServerRequest *request){
    cancelAndPauseNetworksScan(15000);

    SettingsSTA currentSTASettings = loadSTASettings();

    if (currentSTASettings.ssid[0] != '\0' && beginConnectionAttempt(currentSTASettings.ssid, currentSTASettings.password, false, false)) {
      request->send(200, F("application/json"), F("{\"status\":\"connecting\"}"));
      return;
    }

    SavedWiFiNetwork lastNetwork;
    if (getLastSavedWiFiNetwork(&lastNetwork) && beginConnectionAttempt(lastNetwork.ssid, lastNetwork.password, true, false)) {
      request->send(200, F("application/json"), F("{\"status\":\"connecting\"}"));
      return;
    }

    request->send(400, F("application/json"), F("{\"error\":\"No saved network\"}"));
  });

  server.on("/api/connect_network", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      //Serial.println("Received /api/connect_network request");

      if (index == 0) {
        cancelAndPauseNetworksScan(15000);
      }

      if (index == 0) {
        String* body = new String();
        body->reserve(total);
        request->_tempObject = body;
      }

      String* jsonBuffer = reinterpret_cast<String*>(request->_tempObject);
      if (jsonBuffer == nullptr) {
        request->send(500, F("application/json"), F("{\"error\":\"Body alloc failed\"}"));
        return;
      }

      for (size_t i = 0; i < len; i++) {
        (*jsonBuffer) += (char)data[i];
      }

      if (index + len != total) {
        return;
      }

      StaticJsonDocument<256> doc;
      DeserializationError error = deserializeJson(doc, *jsonBuffer);

      delete jsonBuffer;
      request->_tempObject = nullptr;

      if (error) {
        request->send(400, F("application/json"), F("{\"error\":\"Invalid JSON\"}"));
        return;
      }

      const char* ssid = doc["ssid"] | "";
      const char* providedPassword = doc["password"] | "";
      bool passwordProvided = doc["passwordProvided"] | false;

      ///Serial.print("Requested connection to SSID: ");
      //Serial.println(ssid);

      if (ssid == nullptr || ssid[0] == '\0') {
        request->send(400, F("application/json"), F("{\"error\":\"Missing ssid\"}"));
        return;
      }

      SavedWiFiNetwork savedNetwork;
      bool hasSavedNetwork = findSavedWiFiNetwork(ssid, &savedNetwork);
      bool hasProvidedPassword = passwordProvided;

      const char* passwordToUse = "";
      bool saveProvidedCredentials = false;
      bool usedSavedPassword = false;

      /*
      Serial.print("Has saved network: ");
      Serial.println(hasSavedNetwork);
      Serial.print("Has provided password: ");
      Serial.println(hasProvidedPassword);
      */

      if (hasProvidedPassword) {
        passwordToUse = providedPassword;
        saveProvidedCredentials = true;
      } else if (hasSavedNetwork) {
        passwordToUse = savedNetwork.password;
        usedSavedPassword = true;
      } else {
        bool foundInScan = false;
        bool networkIsOpen = isScannedNetworkOpen(ssid, &foundInScan);

        if (foundInScan && networkIsOpen) {
          passwordToUse = "";
          saveProvidedCredentials = true;
        } else {
          request->send(400, F("application/json"), F("{\"error\":\"password_required\",\"requiresPassword\":true}"));
          return;
        }
      }
      
      /*
      Serial.print("Password to use: ");
      Serial.println(passwordToUse);
      Serial.print("Save provided credentials: ");
      Serial.println(saveProvidedCredentials);
      Serial.print("Used saved password: ");
      Serial.println(usedSavedPassword);
      Serial.print("Ssid:"); Serial.println(ssid);
      */

      if (isConnecting || hasQueuedConnectRequest) {
        request->send(409, F("application/json"), F("{\"error\":\"Connection already in progress\"}"));
        return;
      }

      strlcpy(pendingConnectSsid, ssid, sizeof(pendingConnectSsid));
      strlcpy(pendingConnectPassword, passwordToUse, sizeof(pendingConnectPassword));
      pendingConnectUsedSavedPassword = usedSavedPassword;
      pendingConnectSaveProvidedCredentials = saveProvidedCredentials;
      hasQueuedConnectRequest = true;

      AsyncResponseStream *response = request->beginResponseStream("application/json");
      response->print(F("{\"status\":\"connecting\",\"ssid\":\""));
      response->print(escapeJson(ssid));
      response->print(F("\"}"));
      request->send(response);
      //Serial.println("Connection attempt queued");
  });

  server.on("/api/networks", HTTP_GET, [](AsyncWebServerRequest *request){
    SettingsSTA staSettings = loadSTASettings();
    if (!staSettings.useStaMode) {
      cachedNetworksJson = "[]";
      stopNetworksScan();

      AsyncResponseStream *disabledResponse = request->beginResponseStream("application/json");
      disabledResponse->print(F("{\"scanning\":false,\"disabled\":true,\"networks\":[]}"));
      request->send(disabledResponse);
      return;
    }

    if (isConnecting || hasQueuedConnectRequest) {
      AsyncResponseStream *busyResponse = request->beginResponseStream("application/json");
      busyResponse->print(F("{\"scanning\":false,\"busy\":true,\"networks\":"));
      busyResponse->print(cachedNetworksJson);
      busyResponse->print('}');
      request->send(busyResponse);
      return;
    }

    bool forceScan = request->hasParam("force") && request->getParam("force")->value() == "1";

    if (forceScan) {
      startNetworksScan(true);
    } else {
      if (!isNetworksScanInProgress && (lastNetworksScanAt == 0 || millis() - lastNetworksScanAt > 15000)) {
        startNetworksScan(false);
      }
    }

    updateNetworksScanCache();

    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print(F("{\"scanning\":"));
    response->print(isNetworksScanInProgress ? F("true") : F("false"));
    response->print(F(",\"networks\":"));
    response->print(cachedNetworksJson);
    response->print('}');
    request->send(response);
  });

  server.on("/api/setGradient", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, 
  [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {

    static String jsonBuffer; 

    if (index == 0) {
      jsonBuffer = "";
      jsonBuffer.reserve(total); 
    }

    for (size_t i = 0; i < len; i++) {
      jsonBuffer += (char)data[i];
    }

    if (index + len != total) {
      return; 
    }

    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, jsonBuffer);

    jsonBuffer = String(); //free bufer

    if (error) {
      char response[64]; 
      snprintf(response, sizeof(response), "{\"error\":\"%s\"}", error.c_str());
      request->send(400, "application/json", response);
      return;
    }

    JsonArray colorsJson = doc["colors"];
    if (colorsJson.isNull() || colorsJson.size() < 2) {
      request->send(400, F("application/json"), F("{\"error\":\"Need >1 colors\"}"));
      return;
    }

    stopsCount = 0;
    for (String hex : colorsJson) {
      if (stopsCount >= MAX_STOPS) break;
      strcpy(stops[stopsCount].colorHex, hex.c_str()); 
      stopsCount++;
    }

    JsonArray stopsJson = doc["stops"];
    if (!stopsJson.isNull() && stopsJson.size() == stopsCount) {
      for (int i = 0; i < stopsCount; i++) {
        stops[i].position = stopsJson[i];
      }
    } else {
      for (int i = 0; i < stopsCount; i++) {
        stops[i].position = (float)i / (stopsCount - 1);
      }
    }

    drawGradient(stops, stopsCount);
    request->send(200, F("application/json"), F("{\"success\":true}"));
});

  server.on("/api/setBrightness", HTTP_POST, [](AsyncWebServerRequest *request){},
    NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, data);
    if (error) {
      request->send(400, F("application/json"), F("{\"error\":\"Invalid JSON\"}"));
      return;
    }

    if (!doc.containsKey("brightness")) {
      request->send(400, F("application/json"), F("{\"error\":\"Missing brightness\"}"));
      return;
    }

    brightness = doc["brightness"];
    switch(mode){
      case 3: // static
      {
        RgbColor col = FromHex(colorHex).Dim(brightness);
        for (int i = 0; i < NUM_LEDS; i++) {
          strip.SetPixelColor(i, col);
        }
        strip.Show();
        break;
      }
      case 5: // gradient
        drawGradient(stops, stopsCount);
        break;
    }
    request->send(200, F("application/json"), F("{\"success\":true}"));
  });

  server.on("/api/setColor", HTTP_POST, [](AsyncWebServerRequest *request){},
    NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, data);
    if (error) {
      request->send(400, F("application/json"), F("{\"error\":\"Invalid JSON\"}"));
      return;
    }

    if (!doc.containsKey("color")) {
      request->send(400, F("application/json"), F("{\"error\":\"Missing color\"}"));
      return;
    }
    strcpy(colorHex, doc["color"]);
    RgbColor col = FromHex(colorHex).Dim(brightness);
    for (int i = 0; i < NUM_LEDS; i++) {
      strip.SetPixelColor(i, col);
    }
    strip.Show();

    request->send(200, F("application/json"), F("{\"success\":true}"));
  });

  server.on("/api/setMode", HTTP_POST, [](AsyncWebServerRequest *request){},
    NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, data);
    if (error) {
      request->send(400, F("application/json"), F("{\"error\":\"Invalid JSON\"}"));
      return;
    }

    if (!doc.containsKey("mode")) {
      request->send(400, F("application/json"), F("{\"error\":\"Missing mode\"}"));
      return;
    }

    mode = doc["mode"];

    if(mode == 3) {
      RgbColor col = FromHex(colorHex).Dim(brightness);
      for (int i = 0; i < NUM_LEDS; i++) {
        strip.SetPixelColor(i, col);
      }
      strip.Show();
    } else if( mode == 4){
      for (uint16_t i = 0; i < NUM_LEDS; i++) {
        StartFirePixel(i);
      }
    } else if (mode == 5) {
      drawGradient(stops, stopsCount);
    } else {
      animations.StopAll();
    }

    request->send(200, F("application/json"), F("{\"success\":true}"));
  });

  server.on("/api/setSpeed", HTTP_POST, [](AsyncWebServerRequest *request){},
    NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, data);
    if (error) {
      request->send(400, F("application/json"), F("{\"error\":\"Invalid JSON\"}"));
      return;
    }

    if (!doc.containsKey("speed")) {
      request->send(400, F("application/json"), F("{\"error\":\"Missing speed\"}"));
      return;
    }

    speed = doc["speed"];
    if(mode == 3) {
      animations.StopAll();
      for (uint16_t i = 0; i < NUM_LEDS; i++) {
        StartFirePixel(i);
      }
      strip.Show();
    }

    StaticJsonDocument<100> response;
    response["success"] = true;

    String responseString;
    serializeJson(response, responseString);
    request->send(200, "application/json", responseString);
  });

  server.on("/api/setPower", HTTP_POST, [](AsyncWebServerRequest *request){
    powerStateChanged = true;
    request->send(200, F("application/json"), F("{\"success\": true}"));
  });

  server.on("/api/update", HTTP_POST, [](AsyncWebServerRequest *request){
      request->send(200, F("text/plain"), F("Checking for updates..."));

      isUpdateRequested = true;
  });

  server.begin();
}

bool connectToWiFi(const char* ssid, const char* password, uint8_t segmentIndex) {
  if (ssid == nullptr || ssid[0] == '\0') {
    return false;
  }

  const uint16_t stepsPerAttempt = CONNECT_ATTEMPT_TIMEOUT_MS / CONNECT_PROGRESS_STEP_MS;
  uint16_t currentStep = 0;

  showConnectionProgress(0, stepsPerAttempt, segmentIndex);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false);
  delay(120);
  WiFi.begin(ssid, password == nullptr ? "" : password);

  unsigned long startAttemptTime = millis();
  while (millis() - startAttemptTime < CONNECT_ATTEMPT_TIMEOUT_MS) {
    if (WiFi.status() == WL_CONNECTED) {
      showConnectionProgress(stepsPerAttempt, stepsPerAttempt, segmentIndex);
      return true;
    }

    if (currentStep < stepsPerAttempt) {
      currentStep++;
    }
    showConnectionProgress(currentStep, stepsPerAttempt, segmentIndex);
    delay(CONNECT_PROGRESS_STEP_MS);
  }

  WiFi.disconnect(false);
  delay(100);
  showConnectionProgress(stepsPerAttempt, stepsPerAttempt, segmentIndex);
  return false;
}

void persistActiveStaNetwork(const char* ssid, const char* password) {
  if (ssid == nullptr || ssid[0] == '\0') {
    return;
  }

  SettingsSTA staSettings = loadSTASettings();
  strlcpy(staSettings.ssid, ssid, sizeof(staSettings.ssid));
  strlcpy(staSettings.password, password == nullptr ? "" : password, sizeof(staSettings.password));
  staSettings.useStaMode = true;
  saveSTASettings(staSettings);

  upsertSavedWiFiNetwork(staSettings.ssid, staSettings.password, true);
  setLastSavedWiFiNetwork(staSettings.ssid);
}

bool connectUsingSavedNetworks() {
  SavedWiFiStorage storage = loadSavedWiFiStorage();
  uint8_t attemptIndex = 0;
  const uint8_t MAX_ATTEMPTS = 3;

  if (attemptIndex < MAX_ATTEMPTS) {
    if (storage.lastIndex >= 0 && storage.lastIndex < MAX_SAVED_WIFI_NETWORKS && storage.networks[storage.lastIndex].used) {
      if (connectToWiFi(storage.networks[storage.lastIndex].ssid, storage.networks[storage.lastIndex].password, attemptIndex)) {
        persistActiveStaNetwork(storage.networks[storage.lastIndex].ssid, storage.networks[storage.lastIndex].password);
        return true;
      }
    }
    attemptIndex++;
  }

  WiFi.mode(WIFI_STA);
  int foundNetworks = WiFi.scanNetworks(false, true);
  if (foundNetworks <= 0) {
    WiFi.scanDelete();
    return false;
  }

  for (int i = 0; i < foundNetworks && attemptIndex < MAX_ATTEMPTS; i++) {
    String scannedSsid = WiFi.SSID(i);

    for (int j = 0; j < MAX_SAVED_WIFI_NETWORKS; j++) {
      if (!storage.networks[j].used) {
        continue;
      }

      if (strcmp(storage.networks[j].ssid, scannedSsid.c_str()) == 0) {
        if (connectToWiFi(storage.networks[j].ssid, storage.networks[j].password, attemptIndex)) {
          persistActiveStaNetwork(storage.networks[j].ssid, storage.networks[j].password);
          WiFi.scanDelete();
          return true;
        }
        attemptIndex++;
        break;
      }
    }
  }

  WiFi.scanDelete();
  return false;
}
//======================================Server Init======================================

//======================================Main======================================
void setup() {
  stops[0] = GradientStop{ 0.0, "#00b7ff" }; 
  stops[1] = GradientStop{ 0.4, "#00ff33" };
  stops[2] = GradientStop{ 1.0, "#ffe500" };
  //pinMode(LED_PIN, OUTPUT);

  strip.Begin();
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.SetPixelColor(i, RgbColor(0, 0, 0));
  }
  strip.Show();

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    strip.SetPixelColor(i, Wheel((i * RAINBOW_SCALE + rainbowOffset) & 0xFF));
    strip.Show();
    delay(30);
  }
  //Serial.begin(74880);
  //Serial.println("Debug mode");
  WiFi.persistent(false);
  WiFi.setAutoConnect(false);
  WiFi.setAutoReconnect(false);

  SettingsSTA s = loadSTASettings();

  if (s.useStaMode) {
    if (!connectUsingSavedNetworks()) {
      startAP();
        for (uint8_t i = 0; i < 2; i++) {
        for (uint16_t j = 0; j < NUM_LEDS; j++) {
          strip.SetPixelColor(j, RgbColor(255, 0, 0));
        }
        strip.Show();
        delay(300);

        for (uint16_t j = 0; j < NUM_LEDS; j++) {
          strip.SetPixelColor(j, RgbColor(0, 0, 0));
        }
        strip.Show();
        delay(300);
      }
    } else {
      if (MDNS.begin("lamp")) {
        //Serial.println("mDNS responder started");
      }
      isConnected = true;
      server.begin();
    } 
  } else {
    startAP();
  }
  beginServer();
}

void loop() {
  updateNetworksScanCache();

  if (hasQueuedConnectRequest && !isConnecting) {
    bool started = beginConnectionAttempt(
      pendingConnectSsid,
      pendingConnectPassword,
      pendingConnectUsedSavedPassword,
      pendingConnectSaveProvidedCredentials
    );

    if (!started) {
      //Serial.println("Failed to queue-start Wi-Fi connect attempt");
      pendingConnectUsedSavedPassword = false;
      pendingConnectSaveProvidedCredentials = false;
      pendingConnectSsid[0] = '\0';
      pendingConnectPassword[0] = '\0';
    }

    hasQueuedConnectRequest = false;
  }

  if(isConnecting){
    
    //Serial.println(WiFi.status());
    if (WiFi.status() == WL_CONNECTED) {
      
      isConnecting = false;

      if (pendingConnectSsid[0] != '\0') {
        SettingsSTA staSettings;
        strlcpy(staSettings.ssid, pendingConnectSsid, sizeof(staSettings.ssid));
        strlcpy(staSettings.password, pendingConnectPassword, sizeof(staSettings.password));
        staSettings.useStaMode = true;
        saveSTASettings(staSettings);

        if (pendingConnectSaveProvidedCredentials) {
          upsertSavedWiFiNetwork(pendingConnectSsid, pendingConnectPassword, true);
        }

        setLastSavedWiFiNetwork(pendingConnectSsid);
      }

      hasRollbackConnectCredentials = false;
      rollbackConnectSsid[0] = '\0';
      rollbackConnectPassword[0] = '\0';
      pendingConnectUsedSavedPassword = false;
      pendingConnectSaveProvidedCredentials = false;
      pendingConnectSsid[0] = '\0';
      pendingConnectPassword[0] = '\0';
    } else if (millis() - connectionStartTime > 15000){
      //Serial.println("Failed to connect to Wi-Fi");

      //Serial.println("restoring privious connection");
      //Serial.print("SSID: ");      Serial.println(rollbackConnectSsid);

      restorePreviousConnectionAfterFailure();

      isConnecting = false;
      pendingConnectUsedSavedPassword = false;
      pendingConnectSaveProvidedCredentials = false;
      pendingConnectSsid[0] = '\0';
      pendingConnectPassword[0] = '\0';
      hasRollbackConnectCredentials = false;
      rollbackConnectSsid[0] = '\0';
      rollbackConnectPassword[0] = '\0';
    }

  }

  if (WiFi.status() != WL_CONNECTED) {
    dnsServer.processNextRequest();
  } else if (!isConnected && millis() - connectionStartTime > 20000) {
    WiFi.softAPdisconnect(true);
    //Serial.println("AP отключён через 20 сек");
    isConnected = true;
  }
  else{
    if(!isConnecting) {
      MDNS.update();
    }
  }


  if(isUpdateRequested){
    isUpdateRequested = false;
    const char* ver_url = "https://raw.githubusercontent.com/NascuBB/smart_lamp/main/version.txt";
    const char* bin_url = "https://github.com/NascuBB/smart_lamp/releases/latest/download/firmware.bin";

    bool needUpdate = isUpdateAvailable(ver_url, FW_VERSION);
    
    if (!needUpdate) {
        for(int k=0; k<3; k++) {
          for(int i = 0; i < NUM_LEDS; i++){
            strip.SetPixelColor(i, RgbColor(0, 0, 0));
          }
          strip.Show();
          delay(200);
          for(int i = 0; i < NUM_LEDS; i++){
            strip.SetPixelColor(i, RgbColor(0, 255, 0));
          }
          strip.Show();
          delay(200);
          if(mode == 3) {
            RgbColor col = FromHex(colorHex).Dim(brightness);
            for (int i = 0; i < NUM_LEDS; i++) {
              strip.SetPixelColor(i, col);
            }
            strip.Show();
          } else if (mode == 5) {
            drawGradient(stops, stopsCount);
          }
        }
        return;
    }

    if(mode == 4) animations.StopAll(); 
    
    bool success = performOTA(bin_url, [=](int percent) {

      int ledsToLight = map(percent, 0, 100, 0, NUM_LEDS);
      for(int i = 0; i < NUM_LEDS; i++) {
        if (i < ledsToLight) strip.SetPixelColor(i, RgbColor(0, 255, 255));
        else strip.SetPixelColor(i, RgbColor(0, 0, 0));
      }
      strip.Show();
    });
    
    if (success) {
      for(int i = 0; i < NUM_LEDS; i++) strip.SetPixelColor(i, RgbColor(0, 255, 0));
      strip.Show();
      delay(500);
      for(int i = 0; i < NUM_LEDS; i++){
        strip.SetPixelColor(i, RgbColor(0, 0, 0));
      }
      strip.Show();
      delay(100);
      ESP.restart();
    } else {
      for(int k=0; k<3; k++) {
        for(int i=0; i < NUM_LEDS; i++) strip.SetPixelColor(i, RgbColor(255, 0, 0));
        strip.Show();
        delay(300);
        for(int i = 0; i < NUM_LEDS; i++){
          strip.SetPixelColor(i, RgbColor(0, 0, 0));
        }
        strip.Show();
        delay(300);
      }
    }
  }

  if(powerStateChanged){
    if(power) {
      if(mode == 4){
        animations.StopAll();
      }
      turnOff();
      power = false;
    } else {
      RgbColor col;
      switch(mode){
        case 1:
          for (uint16_t i = 0; i < NUM_LEDS; i++) {
            strip.SetPixelColor(i, Wheel((i * RAINBOW_SCALE + rainbowOffset) & 0xFF).Dim(brightness));
            strip.Show();
            delay(30);
          }
        break;
        case 2:
          for (uint16_t i = 0; i < NUM_LEDS; i++) {
            strip.SetPixelColor(i, Wheel((i * RGB_SCALE + rainbowOffset) & 0xFF).Dim(brightness));
            strip.Show();
            delay(30);
          }
        break;
        case 3:
          col = FromHex(colorHex).Dim(brightness);
          for (int i = 0; i < NUM_LEDS; i++) {
            strip.SetPixelColor(i, col);
            strip.Show();
            delay(30);
          }
        break;
        case 4:
          for (uint16_t i = 0; i < NUM_LEDS; i++) {
            StartFirePixel(i);
          }
        break;
        case 5:
        for (uint16_t i = 0; i < NUM_LEDS; i++) {
          float pos = (float)i / (NUM_LEDS - 1);

          int startIndex = 0;
          for (int k = 0; k < stopsCount - 1; k++) {
            if (pos >= stops[k].position && pos <= stops[k+1].position) {
              startIndex = k;
              break;
            }
          }

          GradientStop start = stops[startIndex];
          GradientStop end = stops[startIndex + 1];

          float localProgress = (pos - start.position) / (end.position - start.position);
          
          if (end.position == start.position) localProgress = 0;

          RgbColor blended = RgbColor::LinearBlend(FromHex(start.colorHex), FromHex(end.colorHex), localProgress);
          
          strip.SetPixelColor(i, blended.Dim(brightness));
          strip.Show();
          delay(30);
        }
        break;
      }
      power = true;
    }
    powerStateChanged = false;
  }

  if(power){
    unsigned long currentMillis = millis();

    if (currentMillis - lastUpdate >= INTERVAL) {
      lastUpdate = currentMillis;

      switch(mode){
        case 1:
          for (uint16_t i = 0; i < NUM_LEDS; i++) {
            strip.SetPixelColor(i, Wheel((i * RAINBOW_SCALE + rainbowOffset) & 0xFF).Dim(brightness));
          }
          strip.Show();
          rainbowOffset += speed; 
        break;
        case 2:
          for (uint16_t i = 0; i < NUM_LEDS; i++) {
            strip.SetPixelColor(i, Wheel((i * RGB_SCALE + rainbowOffset) & 0xFF).Dim(brightness));
          }
          strip.Show();
          rainbowOffset += speed; 
        break;
        case 4:
          animations.UpdateAnimations();
          strip.Show();
        break;
      }
    }
  }
}
//======================================Main======================================

//======================================Additional======================================
void FireUpdate(const AnimationParam& param, FirePixelState* state, RgbColor startColor, RgbColor endColor, uint16_t index)
{
  RgbColor updated = RgbColor::LinearBlend(startColor, endColor, param.progress);
  strip.SetPixelColor(index, updated.Dim(brightness));
  
  if (param.state == AnimationState_Completed) {
    StartFirePixel(index);
  }
}

RgbColor GetRandomFireColor()
{
  uint8_t r = 180 + random(75);
  uint8_t g = random(100);
  uint8_t b = 0;
  return RgbColor(r, g, b);
}

void StartFirePixel(uint16_t index)
{
  RgbColor start = strip.GetPixelColor(index);
  RgbColor end = GetRandomFireColor();
  uint16_t time = random(150 * (6 - speed), 200 * (6 - speed));

  animations.StartAnimation(index, time, [=](const AnimationParam& param) {
    FireUpdate(param, &states[index], start, end, index);
  });
}

void drawGradient(GradientStop* stops, int count) {
  if (animations.IsAnimating()) animations.StopAll();

  if (count < 2) return; 

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    float pos = (float)i / (NUM_LEDS - 1);

    int startIndex = 0;
    for (int k = 0; k < count - 1; k++) {
      if (pos >= stops[k].position && pos <= stops[k+1].position) {
        startIndex = k;
        break;
      }
    }

    GradientStop start = stops[startIndex];
    GradientStop end = stops[startIndex + 1];

    float localProgress = (pos - start.position) / (end.position - start.position);
    
    if (end.position == start.position) localProgress = 0;

    RgbColor blended = RgbColor::LinearBlend(FromHex(start.colorHex), FromHex(end.colorHex), localProgress);
    
    strip.SetPixelColor(i, blended.Dim(brightness));
  }
  strip.Show();
}

void turnOff() {
  for (int i = NUM_LEDS - 1; i >= 0; i--) {
    strip.SetPixelColor(i, RgbColor(0,0,0));
    strip.Show();
    delay(30);
  }
}


void showConnectionProgress(uint16_t currentStep, uint16_t totalSteps, uint8_t segmentIndex) {
  if (totalSteps == 0) {
    totalSteps = 1;
  }

  uint16_t clampedStep = currentStep;
  if (clampedStep > totalSteps) {
    clampedStep = totalSteps;
  }

  uint16_t ledsPerSegment = NUM_LEDS / 3;
  uint16_t segmentStart = segmentIndex * ledsPerSegment;
  uint16_t segmentEnd = (segmentIndex == 2) ? NUM_LEDS : segmentStart + ledsPerSegment;

  uint16_t ledsToLight = segmentStart + (uint32_t)clampedStep * (segmentEnd - segmentStart) / totalSteps;
  if (ledsToLight > segmentEnd) {
    ledsToLight = segmentEnd;
  }

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    if (i < segmentStart) {
      strip.SetPixelColor(i, RgbColor(0, 180, 255));
    } else if (i < ledsToLight) {
      strip.SetPixelColor(i, RgbColor(0, 180, 255));
    }
  }
  strip.Show();
}

RgbColor FromHex(const char* hex) {
  uint8_t r = 0, g = 0, b = 0;
  if (hex[0] == '#' && strlen(hex) == 7) {
    sscanf(hex + 1, "%02hhx%02hhx%02hhx", &r, &g, &b);
  }
  return RgbColor(r, g, b);
}

RgbColor Wheel(byte pos) {
  if (pos < 85) {
    return RgbColor(pos * 3, 255 - pos * 3, 0);
  } else if (pos < 170) {
    pos -= 85;
    return RgbColor(255 - pos * 3, 0, pos * 3);
  } else {
    pos -= 170;
    return RgbColor(0, pos * 3, 255 - pos * 3);
  }
}
//======================================Additional======================================