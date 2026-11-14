#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>

namespace net {
  enum class ConnectError : uint8_t {
    None,
    MissingSsid,
    Busy,
    PasswordRequired,
  };

  void begin();
  void loop();

  bool isStaConnected();

  ConnectError requestConnect(const char* ssid, const char* password, bool passwordProvided);
  void setStaModeEnabled(bool enabled);
  void printStatusJson(Print& out);
  void printNetworksJson(Print& out, bool forceScan);
}

#endif
