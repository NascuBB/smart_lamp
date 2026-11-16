#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

#ifndef FW_VERSION
#define FW_VERSION "0.4.1"
#endif

#define NUM_LEDS          30
#define FRAME_INTERVAL_MS 20
#define SWEEP_STEP_MS     30
#define STATE_SAVE_DELAY_MS 5000

#define MDNS_NAME                "lamp"
#define DEFAULT_AP_SSID          "Led_Setup"
#define BOOT_CONNECT_TIMEOUT_MS  7000
#define CONNECT_TIMEOUT_MS       15000
#define RECONNECT_DELAY_MS       10000
#define AP_RETRY_INTERVAL_MS     120000
#define AP_GRACE_AFTER_CONNECT_MS 30000
#define SCAN_CACHE_MS            15000
#define SCAN_REQUEST_WINDOW_MS   15000

#define OTA_LATEST_RELEASE_URL "https://github.com/NascuBB/smart_lamp/releases/latest"
#define OTA_FIRMWARE_URL       "https://github.com/NascuBB/smart_lamp/releases/latest/download/firmware.bin"

#define LOG_ENABLED 1

#if LOG_ENABLED
  #define LOG(fmt, ...) Serial.printf_P(PSTR(fmt "\n"), ##__VA_ARGS__)
#else
  #define LOG(fmt, ...) do {} while (0)
#endif

#endif
