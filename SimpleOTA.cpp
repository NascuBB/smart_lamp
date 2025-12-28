#include "SimpleOTA.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Updater.h>

void setupSecureClient(WiFiClientSecure &client) {
  client.setInsecure();
  client.setBufferSizes(1024, 1024); 
}

bool performOTA(const char* firmwareURL, const ProgressCallback& onProgress) {
  if (WiFi.status() != WL_CONNECTED) return false;

  String currentURL = firmwareURL;
  
  WiFiClientSecure client;
  setupSecureClient(client);
  client.setTimeout(6000);

  HTTPClient http;
  http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);

  //Serial.println(F("OTA: Resolving URL..."));

  for (int i = 0; i < 5; i++) {
    if (!http.begin(client, currentURL)) {
      Serial.println(F("OTA: Connect failed during resolve"));
      return false;
    }

    const char* headerKeys[] = {"Location"};
    http.collectHeaders(headerKeys, 1);
    
    int code = http.GET();
    
    if (code == HTTP_CODE_FOUND || code == HTTP_CODE_MOVED_PERMANENTLY) {
      String newURL = http.header("Location");
      Serial.print(F("OTA: Redirect to: "));
      // Serial.println(newURL);
      
      if (newURL.length() > 5) {
        currentURL = newURL;
      } else {
        //Serial.println(F("OTA: Empty redirect header!"));
        http.end();
        return false;
      }
      
      http.end();
      
      delay(10);
      continue;
      
    } else if (code == HTTP_CODE_OK) {
      //Serial.println(F("OTA: Final URL found!"));
      http.end();
      break;
    } else {
      //Serial.printf("OTA: Error resolving link, code: %d\n", code);
      http.end();
      return false;
    }
  }
  
  client.stop(); // cleanup after resolving redirect
  yield();

  
  WiFiClientSecure client2; 
  client2.setInsecure();
  

  client2.setTimeout(15000); 
  uint32_t freeHeap = ESP.getFreeHeap();
  bool bufferSuccess = false;
  if (freeHeap > 20000) {
     //Serial.println(F("OTA: Allocating BIG buffer (16KB)..."));
     client2.setBufferSizes(16384, 512); 
     bufferSuccess = true; 
  } else {
     //Serial.println(F("OTA: Low RAM! Allocating SMALL buffer (5KB)..."));
     client2.setBufferSizes(5120, 512);
     bufferSuccess = true;
  }

  HTTPClient http2;
  http2.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
  
  //Serial.println(F("OTA: Connecting to file server..."));
  
  if (!http2.begin(client2, currentURL)) {
    //Serial.println(F("OTA: Connect to file server failed"));
    return false;
  }

  int httpCode = http2.GET();
  if (httpCode != HTTP_CODE_OK) {
    //Serial.printf("OTA: Download failed, code: %d\n", httpCode);
    http2.end();
    return false;
  }

  int contentLength = http2.getSize();
  if (contentLength <= 0) {
    //Serial.println(F("OTA: Content-Length error"));
    http2.end();
    return false;
  }

  if (!Update.begin(contentLength)) {
    //Serial.println(F("OTA: No space"));
    http2.end();
    return false;
  }

  WiFiClient * stream = http2.getStreamPtr();
  
  if (stream == nullptr) {
    //Serial.println(F("OTA: Stream is NULL. 6KB buffer too small?"));
    http2.end();
    return false;
  }

  uint8_t buff[128]; 
  int totalRead = 0;
  unsigned long lastDataTime = millis();

  //Serial.printf("Downloading %d bytes...\n", contentLength);

  while (totalRead < contentLength) {
    size_t size = stream->available();
    
    if (size > 0) {
      lastDataTime = millis(); 
      int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
      
      if (Update.write(buff, c) != c) {
         //Serial.println(F("OTA: Write error"));
         break;
      }
      
      totalRead += c;

      if (onProgress) {
        onProgress((totalRead * 100) / contentLength);
      }
      yield(); 
    } else {
      if (!http2.connected()) {
        if (totalRead < contentLength) {
             //Serial.println(F("OTA: Connection cut"));
             break;
        }
      }
      delay(1);
      if (millis() - lastDataTime > 5000) {
        //Serial.println(F("OTA: Timeout"));
        break;
      }
    }
  }
  
  if (totalRead == contentLength) {
    if (Update.end()) {
      //Serial.println(F("OTA Success!"));
      http2.end();
      return true;
    }
  } 
  
  http2.end();
  return false;
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