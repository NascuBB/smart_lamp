#ifndef LAMP_H
#define LAMP_H

#include <Arduino.h>
#include <NeoPixelBus.h>
#include "Settings.h"

enum LampMode : uint8_t {
  MODE_RAINBOW  = 1,
  MODE_SOLID    = 2,
  MODE_STATIC   = 3,
  MODE_FIRE     = 4,
  MODE_GRADIENT = 5,
};

namespace lamp {
  void begin();
  void loop();
  void saveNow();
  void setPrefs(const GeneralPrefs& prefs);

  const LampState& state();

  bool setMode(uint8_t mode);
  void setBrightness(uint8_t brightness);
  bool setSpeed(uint8_t speed);
  bool setColor(const char* hex);
  bool setGradient(const char* const* colors, const float* positions, uint8_t count);
  void setPower(bool on);

  void fill(RgbColor color);
  void blink(RgbColor color, uint8_t times, uint16_t periodMs);
  void showProgress(uint8_t percent, RgbColor color);
  void showSegmentProgress(uint8_t segment, uint8_t segments, uint16_t step, uint16_t steps);
  void bootAnimation();
  void turnOffNow();
  void restore();

  bool isValidHexColor(const char* hex);
}

#endif
