#include "SimpleOTA.h"
#include "config.h"

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Updater.h>

namespace {
  const int MAX_REDIRECTS = 5;

  void parseVersion(const char* text, int parts[3]) {
    parts[0] = parts[1] = parts[2] = 0;
    if (text == nullptr) return;
    if (*text == 'v' || *text == 'V') text++;
    sscanf(text, "%d.%d.%d", &parts[0], &parts[1], &parts[2]);
  }

  int requestWithoutRedirect(WiFiClientSecure& client, const String& url, String& location) {
    HTTPClient http;
    http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
    if (!http.begin(client, url)) {
      return -1;
    }
    const char* headerKeys[] = {"Location"};
    http.collectHeaders(headerKeys, 1);
    int code = http.GET();
    location = http.header("Location");
    http.end();
    return code;
  }

  bool isRedirect(int code) {
    return code == HTTP_CODE_MOVED_PERMANENTLY || code == HTTP_CODE_FOUND ||
           code == HTTP_CODE_TEMPORARY_REDIRECT || code == HTTP_CODE_PERMANENT_REDIRECT;
  }
}

int compareVersions(const char* a, const char* b) {
  int pa[3], pb[3];
  parseVersion(a, pa);
  parseVersion(b, pb);
  for (int i = 0; i < 3; i++) {
    if (pa[i] != pb[i]) return pa[i] < pb[i] ? -1 : 1;
  }
  return 0;
}

UpdateCheck checkForUpdate(const char* latestReleaseUrl, const char* currentVersion) {
  if (WiFi.status() != WL_CONNECTED) return UpdateCheck::Error;

  WiFiClientSecure client;
  client.setInsecure();
  client.setBufferSizes(1024, 1024);
  client.setTimeout(6000);

  String location;
  int code = requestWithoutRedirect(client, latestReleaseUrl, location);
  int tagPos = location.lastIndexOf("/tag/");
  if (!isRedirect(code) || tagPos < 0) {
    LOG("OTA: version check failed, code %d", code);
    return UpdateCheck::Error;
  }

  String latest = location.substring(tagPos + 5);
  LOG("OTA: current %s, latest %s", currentVersion, latest.c_str());
  return compareVersions(latest.c_str(), currentVersion) > 0 ? UpdateCheck::Available : UpdateCheck::UpToDate;
}

bool performOTA(const char* firmwareURL, const ProgressCallback& onProgress) {
  if (WiFi.status() != WL_CONNECTED) return false;

  String url = firmwareURL;
  {
    WiFiClientSecure client;
    client.setInsecure();
    client.setBufferSizes(1024, 1024);
    client.setTimeout(6000);

    bool resolved = false;
    for (int i = 0; i < MAX_REDIRECTS && !resolved; i++) {
      String location;
      int code = requestWithoutRedirect(client, url, location);
      if (isRedirect(code) && location.length() > 0) {
        url = location;
      } else if (code == HTTP_CODE_OK) {
        resolved = true;
      } else {
        LOG("OTA: resolve failed, code %d", code);
        return false;
      }
    }
    if (!resolved) {
      LOG("OTA: too many redirects");
      return false;
    }
  }

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(15000);
  client.setBufferSizes(ESP.getFreeHeap() > 20000 ? 16384 : 5120, 512);

  HTTPClient http;
  http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
  if (!http.begin(client, url)) {
    LOG("OTA: connect failed");
    return false;
  }

  int code = http.GET();
  int contentLength = http.getSize();
  if (code != HTTP_CODE_OK || contentLength <= 0) {
    LOG("OTA: download failed, code %d, length %d", code, contentLength);
    http.end();
    return false;
  }

  if (!Update.begin(contentLength)) {
    LOG("OTA: Update.begin failed: %s", Update.getErrorString().c_str());
    http.end();
    return false;
  }

  WiFiClient* stream = http.getStreamPtr();
  uint8_t buffer[256];
  int totalRead = 0;
  int lastPercent = -1;
  unsigned long lastDataAt = millis();

  while (totalRead < contentLength) {
    size_t available = stream->available();
    if (available == 0) {
      if (!http.connected() || millis() - lastDataAt > 5000) {
        LOG("OTA: connection lost at %d/%d", totalRead, contentLength);
        break;
      }
      delay(1);
      continue;
    }

    lastDataAt = millis();
    int read = stream->readBytes(buffer, min(available, sizeof(buffer)));
    if (Update.write(buffer, read) != (size_t)read) {
      LOG("OTA: write failed: %s", Update.getErrorString().c_str());
      break;
    }
    totalRead += read;

    int percent = (int)((int64_t)totalRead * 100 / contentLength);
    if (onProgress && percent != lastPercent) {
      lastPercent = percent;
      onProgress(percent);
    }
    yield();
  }

  http.end();

  if (totalRead != contentLength || !Update.end()) {
    LOG("OTA: failed: %s", Update.getErrorString().c_str());
    Update.end();
    return false;
  }

  LOG("OTA: success, %d bytes", totalRead);
  return true;
}
