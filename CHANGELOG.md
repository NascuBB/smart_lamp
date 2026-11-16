# Changelog

Each `## X.Y.Z` section becomes the description of the GitHub release `vX.Y.Z`.
Write the section before bumping `FW_VERSION` in `config.h`.

## 0.4.1

### Added
- Two settings toggles, both disabled by default:
  - "Remember mode": the mode, static color and gradient are restored after a reboot.
  - "Remember brightness and speed": brightness and speed are restored after a reboot.

### Changed
- Lamp settings are no longer restored after a reboot unless one of the toggles above is enabled. With both disabled, nothing is written to flash when the lamp is adjusted.

## 0.4.0

### Added
- Lamp settings (mode, brightness, speed, color, gradient) are saved and restored after a reboot.
- Firmware can be uploaded as a `.bin` file from the settings page.
- In access point mode the lamp retries saved networks every 2 minutes while nobody is connected to it, so it recovers when the router boots later than the lamp.
- Connecting to a network from the access point keeps the AP up for 30 seconds, so the settings page can show the lamp's new address.
- Manual network entry now connects right away, including hidden networks.
- Serial log (TX only, 115200).

### Changed
- Update check reads the version from the latest release tag and only installs a newer version. `version.txt` is kept for 0.3.x lamps.
- Gradient points are dragged with pointer events, so dragging works on phones. The touch area around each point is larger.
- Code split into modules: `Lamp`, `Network`, `WebApi`, `Settings`, `SimpleOTA`, `HtmlResponses`.
- Wi-Fi credentials are stored in one place (saved networks list). Settings from 0.3.x are migrated automatically.
- Power button sends the desired state instead of toggling.

### Fixed
- Connection status endpoint was missing, so the settings page never showed the connection result.
- Speed buttons restarted the fire animation in static color mode instead of fire mode.
- Long or non-string colors in API requests could overflow buffers and crash the lamp.
- JSON request bodies could be read past their end.
- Reboot from the web page could crash instead of restarting cleanly.
- Gradient points outside the 0–100% range produced wrong colors.
- Wrong password for the currently connected network could be saved as valid.
- Network names with HTML in them could break the settings page.
- "Delete point" button in the gradient editor was unstyled.

## 0.3.0
- Redesigned Wi-Fi management: network scan, up to 5 saved networks, saved networks page.
- Automatic reconnect after the connection drops, falling back to the access point.
- Connection progress shown on the LED strip.
- mDNS fix (`lamp.local`).

## 0.2.1
- Gradient editor with up to 10 draggable points.

## 0.1.3
- Fixed brightness handling.

## 0.1.2
- Working OTA updates from GitHub releases.

## 0.1.1
- Cleanup and first OTA test.

## 0.1
- Initial version: rainbow, solid, static and fire modes, AP and STA modes, captive portal.
