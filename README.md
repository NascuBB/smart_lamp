# Smart lamp

Smart lamp with an ESP8266 and WS2812b LED strip, controlled via browser over Wi-Fi.

## Features:
- 5 light modes (rainbow, solid RGB, static color, fire, gradient), settings survive reboots
- AP and STA operating modes, up to 5 saved networks, automatic reconnect
- Captive portal in AP mode
- OTA updates from GitHub Releases or by uploading a `.bin` in the settings page

## Building
Arduino IDE, board **Generic ESP8266 Module**, Flash Size **4MB (FS:1MB OTA:~1019KB)**, CPU 160 MHz.
Libraries: ESPAsyncWebServer + ESPAsyncTCP, ArduinoJson 7, NeoPixelBus.
The strip data line is on **RX (GPIO3)** (DMA method). Serial log is TX-only at 115200.


## Releasing
1. Add a `## X.Y.Z` section to `CHANGELOG.md`.
2. Set `FW_VERSION` in `config.h` to `X.Y.Z` and push to `main`.

GitHub Actions builds the firmware and publishes release `vX.Y.Z` with `firmware.bin` and the changelog section as its description.
Then it updates `version.txt`, which 0.3.x lamps use to detect updates. Do not edit `version.txt` by hand.
Every push and pull request is also built, and the binary is available in the workflow run artifacts.

Lamps on 0.4.0+ read the version from the latest release tag and only install a newer one.

## Important notes:
This is a beta firmware version, it contains bugs, and the page is currently only available in Russian.
