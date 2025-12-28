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
//======================================Server======================================

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

    AsyncResponseStream *response = request->beginResponseStream("text/html");

    response->print(FPSTR(html_template_p1));
    response->print(FPSTR(html_settings_p1));
    
    response->printf("ssidSta: '%s',", sSta.ssid);
    response->printf("passwordSta: '%s',", sSta.password);
    response->printf("useStaMode: %s,", sSta.useStaMode ? "true" : "false");  
    response->printf("ssidAp: '%s',", sAp.ssid);
    response->printf("passwordAp: '%s',", sAp.password);
    response->printf("requiresPassAp: %s };\n", sAp.requirePass ? "true" : "false");

    response->print(FPSTR(html_settings_p2));
    response->print(FPSTR(html_template_p2));

    request->send(response);
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

  server.on("/api/save_sta", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<256> doc;
      DeserializationError error = deserializeJson(doc, data);
      if (error) {
        request->send(400, F("application/json"), F("{\"error\":\"Invalid JSON\"}"));
        return;
      }

      SettingsSTA settings;
      strlcpy(settings.ssid, doc["ssid"] | "", sizeof(settings.ssid));
      strlcpy(settings.password, doc["password"] | "", sizeof(settings.password));
      settings.useStaMode = doc["useStaMode"];

      saveSTASettings(settings);

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
    SettingsSTA currentSTASettings = loadSTASettings();

    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(currentSTASettings.ssid, currentSTASettings.password);

    connectionStartTime = millis();
    isConnecting = true;

    request->send(200);
  });

  server.on("/api/connection_status", HTTP_GET, [](AsyncWebServerRequest *request){
    if (WiFi.status() == WL_CONNECTED) {
      isConnecting = false;
      connectionStartTime = millis();
      IPAddress ip = WiFi.localIP();
      char buffer[64];
      snprintf(buffer, sizeof(buffer), "{\"connected\":1,\"ip\":\"%s\"}", WiFi.localIP().toString().c_str());
      request->send(200, F("application/json"), buffer);
    } else if (isConnecting && millis() - connectionStartTime < 20000) {
      request->send(200, F("application/json"), F("{\"connected\":0}"));
    } else {
      isConnecting = false;
      request->send(200, F("application/json"), F("{\"connected\":2}"));
    }
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

bool connectToWiFi(SettingsSTA s) {

  WiFi.begin(s.ssid, s.password);
  uint8_t counter = 1;
  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {

    for (uint16_t i = 0; i < counter; i++) {
      strip.SetPixelColor(i, RgbColor(0,255,255));
    }
    strip.Show();
    counter++;
    delay(500);
    Serial.print(".");
  }
  return WiFi.status() == WL_CONNECTED;
}
//======================================Server Init======================================

//======================================Main======================================
void setup() {
  stops[0] = GradientStop{ 0.0, "#00b7ff" }; 
  stops[1] = GradientStop{ 0.4, "#00ff33" };
  stops[2] = GradientStop{ 1.0, "#ffe500" };
  pinMode(LED_PIN, OUTPUT);

  strip.Begin();
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.SetPixelColor(i, RgbColor(0, 0, 0));
  }
  strip.Show();

  for (uint16_t i = 0; i <= NUM_LEDS; i++) {
    strip.SetPixelColor(i, Wheel((i * RAINBOW_SCALE + rainbowOffset) & 0xFF));
    strip.Show();
    delay(30);
  }
  //Serial.begin(74880);
  SettingsSTA s = loadSTASettings();

  if (s.useStaMode) {
    if (!connectToWiFi(s)) {
      startAP();
      for(uint8_t i = 0; i < 2; i++){
        for (int j = 0; j < NUM_LEDS; j++) {
          strip.SetPixelColor(j, RgbColor(255, 0, 0));
        }
        strip.Show();
        delay(250);
        for (int i = 0; i < NUM_LEDS; i++) {
          strip.SetPixelColor(i, RgbColor(0, 0, 0));
        }
        strip.Show();
        delay(250);
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
  if (WiFi.status() != WL_CONNECTED) {
    dnsServer.processNextRequest();
  } else if (!isConnected && millis() - connectionStartTime > 20000) {
    WiFi.softAPdisconnect(true);
    //Serial.println("AP отключён через 20 сек");
    isConnected = true;
  }
  else{
    MDNS.update();
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
  for (int i = NUM_LEDS; i >= 0; i--) {
    strip.SetPixelColor(i, RgbColor(0,0,0));
    strip.Show();
    delay(30);
  }
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