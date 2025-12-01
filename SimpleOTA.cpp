#include "SimpleOTA.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Updater.h>

bool performOTA(const char* firmwareURL, ProgressCallback onProgress) {
  if (WiFi.status() != WL_CONNECTED) return false;

  WiFiClientSecure client;
  client.setInsecure();
  
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

  // Начинаем обновление
  if (!Update.begin(contentLength)) {
    http.end();
    return false;
  }

  // === РУЧНОЙ ЦИКЛ ЗАГРУЗКИ ===
  WiFiClient * stream = http.getStreamPtr();
  uint8_t buff[128]; // Буфер чтения (маленький, чтобы беречь RAM)
  int totalRead = 0;
  
  while (http.connected() && (totalRead < contentLength)) {
    // Есть ли данные для чтения?
    size_t size = stream->available();
    if (size) {
      // Читаем не больше, чем размер нашего буфера
      int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
      
      // Пишем во флеш
      Update.write(buff, c);
      
      totalRead += c;

      // Вызываем функцию отрисовки ленты (если она передана)
      if (onProgress) {
        // Считаем процент (0-100)
        int percent = (totalRead * 100) / contentLength;
        onProgress(percent);
      }
      
      // Даем процессору "подышать", чтобы не сработал Watchdog
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
  client.setInsecure(); // GitHub требует SSL
  
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  
  // Пробуем скачать файл версии
  if (!http.begin(client, versionURL)) {
    return false;
  }

  int httpCode = http.GET();
  
  // Если сервер ответил 200 OK
  if (httpCode == HTTP_CODE_OK) {
    // Получаем строку с сервера (например "1.1")
    String remoteVersion = http.getString();
    
    http.end(); // Закрываем соединение сразу, чтобы освободить RAM
    
    // Убираем пробелы и переносы строк (очень важно для GitHub!)
    remoteVersion.trim();
    
    Serial.print(F("Current: ")); Serial.println(currentVersion);
    Serial.print(F("Remote: "));  Serial.println(remoteVersion);

    // Сравниваем строки. Если НЕ равны - значит есть обнова.
    if (!remoteVersion.equals(currentVersion)) {
      return true; // Нужно обновлять!
    }
  } else {
    http.end();
  }
  
  return false; // Обновление не требуется или ошибка сети
}