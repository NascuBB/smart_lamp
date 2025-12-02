#include "SimpleOTA.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Updater.h>

void setupSecureClient(WiFiClientSecure &client) {
  client.setInsecure();
  client.setBufferSizes(1024, 1024); 
}

bool performOTA(const char* firmwareURL, ProgressCallback onProgress) {
  if (WiFi.status() != WL_CONNECTED) return false;

  WiFiClientSecure client;
  setupSecureClient(client);
  
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  
  if (!http.begin(client, firmwareURL)) return false;

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    http.end();
    return false;
  }

  int contentLength = http.getSize();
  if (contentLength <= 0) {
    http.end();
    return false;
  }

  if (!Update.begin(contentLength)) {
    http.end();
    return false;
  }

  WiFiClient * stream = http.getStreamPtr();
  uint8_t buff[128];
  int totalRead = 0;
  
  // ============================
  while (http.connected() && (totalRead < contentLength)) {
    size_t size = stream->available();
    if (size) {
      int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
      
      Update.write(buff, c);
      
      totalRead += c;

      if (onProgress) {
        int percent = (totalRead * 100) / contentLength;
        onProgress(percent);
      }
      
      // Watchdog
      yield(); 
    }
  }
  // ============================

  if (Update.end()) {
    http.end();
    return Update.isFinished();
  } else {
    http.end();
    return false;
  }
}

bool isUpdateAvailable(const char* versionURL, const char* currentVersion) {
  if (WiFi.status() != WL_CONNECTED) return false;

  WiFiClientSecure client;
  setupSecureClient(client);

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  if (!http.begin(client, versionURL)) {
    return false;
  }

  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String remoteVersion = http.getString();
    
    http.end();
    
    remoteVersion.trim();
    
    //Serial.print(F("Current: ")); Serial.println(currentVersion);
    //Serial.print(F("Remote: "));  Serial.println(remoteVersion);

    if (!remoteVersion.equals(currentVersion)) {
      return true;
    }
  } else {
    http.end();
  }
  
  return false;
}