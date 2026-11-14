
#include "config.h"
#include "Lamp.h"
#include "Network.h"
#include "Settings.h"
#include "WebApi.h"

void setup() {
#if LOG_ENABLED
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  Serial.println();
#endif
  LOG("Smart lamp " FW_VERSION);

  storageBegin();
  lamp::begin();
  lamp::bootAnimation();
  net::begin();
  web::begin();
  lamp::restore();
}

void loop() {
  net::loop();
  web::loop();
  if (!web::isUploadingFirmware()) {
    lamp::loop();
  }
  storageLoop();
}
