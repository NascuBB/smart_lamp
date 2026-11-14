#include "Lamp.h"
#include "config.h"

#include <NeoPixelAnimator.h>

namespace {
  NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> strip(NUM_LEDS);
  NeoPixelAnimator animations(NUM_LEDS);

  const uint8_t RAINBOW_SCALE = 6;
  const uint8_t SOLID_SCALE = 1;

  LampState st;
  RgbColor fireColors[NUM_LEDS];

  uint8_t renderedMode = 0;
  bool powerChangeRequested = false;
  bool requestedPower = true;
  bool stateDirty = false;
  unsigned long stateChangedAt = 0;
  unsigned long lastFrameAt = 0;
  uint16_t rainbowOffset = 0;

  void setDefaults() {
    st.mode = MODE_RAINBOW;
    st.brightness = 255;
    st.speed = 3;
    st.power = true;
    strlcpy(st.colorHex, "#00ff00", sizeof(st.colorHex));
    st.stopsCount = 3;
    st.stops[0] = GradientStop{0.0f, "#00b7ff"};
    st.stops[1] = GradientStop{0.4f, "#00ff33"};
    st.stops[2] = GradientStop{1.0f, "#ffe500"};
  }

  bool isValidState(const LampState& s) {
    if (s.mode < MODE_RAINBOW || s.mode > MODE_GRADIENT) return false;
    if (s.speed < 1 || s.speed > 5) return false;
    if (s.stopsCount < 2 || s.stopsCount > MAX_GRADIENT_STOPS) return false;
    if (!lamp::isValidHexColor(s.colorHex)) return false;
    for (uint8_t i = 0; i < s.stopsCount; i++) {
      if (!lamp::isValidHexColor(s.stops[i].colorHex)) return false;
    }
    return true;
  }

  void markChanged() {
    stateDirty = true;
    stateChangedAt = millis();
  }

  RgbColor fromHex(const char* hex) {
    if (!lamp::isValidHexColor(hex)) {
      return RgbColor(0);
    }
    uint32_t value = strtoul(hex + 1, nullptr, 16);
    return RgbColor((value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF);
  }

  RgbColor wheel(uint8_t pos) {
    if (pos < 85) {
      return RgbColor(pos * 3, 255 - pos * 3, 0);
    }
    if (pos < 170) {
      pos -= 85;
      return RgbColor(255 - pos * 3, 0, pos * 3);
    }
    pos -= 170;
    return RgbColor(0, pos * 3, 255 - pos * 3);
  }

  RgbColor gradientAt(float pos) {
    const GradientStop* stops = st.stops;
    uint8_t count = st.stopsCount;

    if (pos <= stops[0].position) return fromHex(stops[0].colorHex);
    if (pos >= stops[count - 1].position) return fromHex(stops[count - 1].colorHex);

    uint8_t k = 0;
    while (k < count - 2 && pos > stops[k + 1].position) {
      k++;
    }

    float span = stops[k + 1].position - stops[k].position;
    float progress = (span > 0) ? (pos - stops[k].position) / span : 0;
    return RgbColor::LinearBlend(fromHex(stops[k].colorHex), fromHex(stops[k + 1].colorHex), progress);
  }

  RgbColor pixelColor(uint16_t i) {
    switch (st.mode) {
      case MODE_RAINBOW:  return wheel((i * RAINBOW_SCALE + rainbowOffset) & 0xFF);
      case MODE_SOLID:    return wheel((i * SOLID_SCALE + rainbowOffset) & 0xFF);
      case MODE_STATIC:   return fromHex(st.colorHex);
      case MODE_GRADIENT: return gradientAt((float)i / (NUM_LEDS - 1));
      case MODE_FIRE:     return fireColors[i];
      default:            return RgbColor(0);
    }
  }

  void drawFrame() {
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      strip.SetPixelColor(i, pixelColor(i).Dim(st.brightness));
    }
    strip.Show();
  }

  void startFirePixel(uint16_t index) {
    RgbColor from = fireColors[index];
    RgbColor to(180 + random(75), random(100), 0);
    uint16_t duration = random(150 * (6 - st.speed), 200 * (6 - st.speed));

    animations.StartAnimation(index, duration, [=](const AnimationParam& param) {
      RgbColor color = RgbColor::LinearBlend(from, to, param.progress);
      fireColors[index] = color;
      strip.SetPixelColor(index, color.Dim(st.brightness));
      if (param.state == AnimationState_Completed) {
        startFirePixel(index);
      }
    });
  }

  void enterMode() {
    if (st.mode == MODE_FIRE) {
      for (uint16_t i = 0; i < NUM_LEDS; i++) {
        if (!animations.IsAnimationActive(i)) {
          startFirePixel(i);
        }
      }
    } else if (animations.IsAnimating()) {
      animations.StopAll();
    }
    renderedMode = st.mode;
    drawFrame();
  }

  void sweepOn() {
    if (st.mode == MODE_FIRE) {
      enterMode();
      return;
    }
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      strip.SetPixelColor(i, pixelColor(i).Dim(st.brightness));
      strip.Show();
      delay(SWEEP_STEP_MS);
    }
    renderedMode = st.mode;
  }

  void applyPowerChange() {
    powerChangeRequested = false;
    if (requestedPower == st.power) {
      return;
    }

    st.power = requestedPower;
    if (st.power) {
      sweepOn();
    } else {
      lamp::turnOffNow();
      for (auto& color : fireColors) {
        color = RgbColor(0);
      }
      renderedMode = 0;
    }
  }
}

namespace lamp {

void begin() {
  setDefaults();
  LampState saved;
  if (loadLampState(saved) && isValidState(saved)) {
    st = saved;
    LOG("Lamp: restored mode=%u brightness=%u speed=%u", st.mode, st.brightness, st.speed);
  }
  st.power = true;

  strip.Begin();
  fill(RgbColor(0));
}

void loop() {
  if (powerChangeRequested) {
    applyPowerChange();
  }

  if (stateDirty && millis() - stateChangedAt > STATE_SAVE_DELAY_MS) {
    stateDirty = false;
    saveLampState(st);
  }

  if (!st.power) {
    return;
  }

  if (renderedMode != st.mode) {
    enterMode();
  }

  unsigned long now = millis();
  if (now - lastFrameAt < FRAME_INTERVAL_MS) {
    return;
  }
  lastFrameAt = now;

  switch (st.mode) {
    case MODE_RAINBOW:
    case MODE_SOLID:
      rainbowOffset += st.speed;
      drawFrame();
      break;
    case MODE_FIRE:
      animations.UpdateAnimations();
      strip.Show();
      break;
    default:
      break;
  }
}

void saveNow() {
  if (stateDirty) {
    stateDirty = false;
    saveLampState(st);
  }
}

const LampState& state() {
  return st;
}

bool setMode(uint8_t mode) {
  if (mode < MODE_RAINBOW || mode > MODE_GRADIENT) {
    return false;
  }
  if (st.mode != mode) {
    st.mode = mode;
    renderedMode = 0;
    markChanged();
  }
  return true;
}

void setBrightness(uint8_t brightness) {
  st.brightness = brightness;
  renderedMode = 0;
  markChanged();
}

bool setSpeed(uint8_t speed) {
  if (speed < 1 || speed > 5) {
    return false;
  }
  st.speed = speed;
  markChanged();
  return true;
}

bool setColor(const char* hex) {
  if (!isValidHexColor(hex)) {
    return false;
  }
  strlcpy(st.colorHex, hex, sizeof(st.colorHex));
  renderedMode = 0;
  markChanged();
  return true;
}

bool setGradient(const char* const* colors, const float* positions, uint8_t count) {
  if (count < 2 || count > MAX_GRADIENT_STOPS) {
    return false;
  }
  for (uint8_t i = 0; i < count; i++) {
    if (!isValidHexColor(colors[i])) {
      return false;
    }
  }

  float previous = 0;
  for (uint8_t i = 0; i < count; i++) {
    float pos = positions != nullptr ? positions[i] : (float)i / (count - 1);
    pos = constrain(pos, previous, 1.0f);
    previous = pos;
    st.stops[i].position = pos;
    strlcpy(st.stops[i].colorHex, colors[i], sizeof(st.stops[i].colorHex));
  }
  st.stopsCount = count;
  renderedMode = 0;
  markChanged();
  return true;
}

void setPower(bool on) {
  requestedPower = on;
  powerChangeRequested = true;
}

void fill(RgbColor color) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    strip.SetPixelColor(i, color);
  }
  strip.Show();
}

void blink(RgbColor color, uint8_t times, uint16_t periodMs) {
  if (animations.IsAnimating()) {
    animations.StopAll();
  }
  for (uint8_t i = 0; i < times; i++) {
    fill(color);
    delay(periodMs / 2);
    fill(RgbColor(0));
    delay(periodMs / 2);
  }
}

void showProgress(uint8_t percent, RgbColor color) {
  uint16_t lit = (uint32_t)min<uint8_t>(percent, 100) * NUM_LEDS / 100;
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    strip.SetPixelColor(i, i < lit ? color : RgbColor(0));
  }
  strip.Show();
}

void showSegmentProgress(uint8_t segment, uint8_t segments, uint16_t step, uint16_t steps) {
  if (segments == 0) segments = 1;
  if (steps == 0) steps = 1;
  step = min(step, steps);

  uint16_t start = (uint32_t)segment * NUM_LEDS / segments;
  uint16_t end = (uint32_t)(segment + 1) * NUM_LEDS / segments;
  uint16_t lit = start + (uint32_t)step * (end - start) / steps;

  const RgbColor color(0, 180, 255);
  for (uint16_t i = 0; i < lit; i++) {
    strip.SetPixelColor(i, color);
  }
  strip.Show();
}

void bootAnimation() {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    strip.SetPixelColor(i, wheel((i * RAINBOW_SCALE) & 0xFF));
    strip.Show();
    delay(SWEEP_STEP_MS);
  }
}

void turnOffNow() {
  if (animations.IsAnimating()) {
    animations.StopAll();
  }
  for (int i = NUM_LEDS - 1; i >= 0; i--) {
    strip.SetPixelColor(i, RgbColor(0));
    strip.Show();
    delay(SWEEP_STEP_MS);
  }
}

void restore() {
  if (st.power) {
    enterMode();
  } else {
    fill(RgbColor(0));
  }
}

bool isValidHexColor(const char* hex) {
  if (hex == nullptr || hex[0] != '#' || strlen(hex) != 7) {
    return false;
  }
  for (uint8_t i = 1; i < 7; i++) {
    if (!isxdigit((unsigned char)hex[i])) {
      return false;
    }
  }
  return true;
}

}
