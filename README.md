# XR-CORE

ESP32 firmware for the XR-CORE device. The project combines a Wi-Fi configuration flow, an OLED status display, a browser dashboard stored in LittleFS, and optional MQTT, LoRa, and Arduino IoT Cloud integrations.

## Contents

- [Features](#features)
- [Repository layout](#repository-layout)
- [Hardware and board setup](#hardware-and-board-setup)
- [Build and upload firmware](#build-and-upload-firmware)
- [Build and upload the web filesystem](#build-and-upload-the-web-filesystem)
- [Configuration and security](#configuration-and-security)
- [Web interface](#web-interface)
- [Documentation](#documentation)

## Features

- FreeRTOS sensor/application and network tasks pinned to ESP32 cores.
- Persistent Wi-Fi credentials, station/access-point modes, Wi-Fi scanning, and an AP-mode button.
- OLED dashboard with device, Wi-Fi, and logo pages.
- LittleFS-hosted authenticated web dashboard and configuration APIs.
- Optional MQTT with TLS CA certificate upload, optional RadioLib LoRa telemetry transmit and receive, and optional Arduino IoT Cloud.
- Local web UI source and a PNG-to-raw monochrome OLED image converter.

Feature defaults in `Config.cpp` disable MQTT, LoRa, and Arduino IoT Cloud. Device-specific values and saved settings control runtime behavior.

## Repository layout

| Path | Purpose |
| --- | --- |
| `XR-CORE.ino` | Firmware composition, startup, status LEDs, and FreeRTOS tasks. |
| `Config.h`, `Config.cpp` | Device identity, default credentials/settings, and LoRa pin profile. |
| `WiFi_Manager.*` | Wi-Fi lifecycle, saved credentials, scanning, time, and AP-mode button behavior. |
| `Web_Config.*` | Async web server, dashboard file serving, session authentication, and device configuration APIs. |
| `OLED_Display.*` | SSD1306 drawing helpers and LittleFS bitmap display. |
| `DummyTelemetry.*` | Demo telemetry value used by the dashboard. |
| `MQTT_Manager.*` | MQTT settings, reconnect, TLS, publish/subscribe, and callbacks. |
| `LoRa_Manager.*` | LoRa radio initialization and persisted radio settings. |
| `ArduinoCloud_Manager.*` | Arduino IoT Cloud connection lifecycle. |
| `ESP_Device.*` | ESP32 chip, memory, flash, and sketch diagnostics. |
| `Sensor_Filter.*` | Sensor filtering utility. |
| `partitions.csv` | Custom 4 MB flash layout with OTA slots and a SPIFFS-compatible filesystem partition used by LittleFS. |
| `data/` | LittleFS image contents: production UI, assets, logo, and optional MQTT CA certificate at runtime. |
| `local web/` | Vite-based dashboard source and widget modules. |
| `tools/convert_logo.py` | Converts `data/logo.png` to the OLED's 1-bit `data/logo.raw` format. |
| `docs/` | Setup, architecture, API, hardware, and maintenance guides. |

Do not edit generated hashed assets under `data/assets/` by hand. Make UI changes in `local web/` and rebuild the production UI.

## Hardware and board setup

The provided Arduino IDE profile is based on the attached configuration: **ESP32-WROOM-DA Module**, ESP32 Arduino core **3.3.12**, 4 MB flash, 80 MHz flash, DIO flash mode, 240 MHz CPU, and custom partition scheme. The compile output supplied with the project reports 1,332,391 bytes of sketch storage and 54,868 bytes of global RAM; these are reference results and may change with source or toolchain updates.

The default LoRa profile uses SX1276, NSS 5, reset 14, DIO0 2, and DIO1 disabled. Confirm the actual board and radio wiring before enabling LoRa. See [hardware notes](docs/HARDWARE.md).

## Build and upload firmware

1. Install Arduino IDE 2.x and ESP32 Arduino core 3.3.12 (or a compatible version).
2. Install the third-party libraries listed in [build setup](docs/BUILD_AND_UPLOAD.md).
3. Open `XR-CORE.ino` from this sketch folder.
4. Select the ESP32-WROOM-DA board and options described in [build setup](docs/BUILD_AND_UPLOAD.md), including the custom `partitions.csv` scheme.
5. Set project-specific configuration and credentials before connecting the device to a network.
6. Compile and upload the sketch over the selected serial port.

## Build and upload the web filesystem

The browser UI must be built into `data/` and uploaded as a filesystem image separately from the firmware. Install the ESP32 filesystem uploader for Arduino IDE following [Random Nerd Tutorials' installation guide](https://randomnerdtutorials.com/install-esp32-filesystem-uploader-arduino-ide/). Then:

1. From `local web/`, run `npm install` once and `npm run build` after UI changes.
2. Confirm `data/index.html`, `data/login.html`, and `data/assets/` contain the production build. Keep `data/logo.raw` in place.
3. In Arduino IDE, select the same board and custom partition scheme, then choose **Tools > ESP32 Sketch Data Upload** (the uploader menu label may vary by plugin version).
4. Upload the firmware sketch separately if needed. A filesystem upload replaces filesystem contents, so preserve any runtime files that must remain, including the MQTT CA certificate.

See [build and upload](docs/BUILD_AND_UPLOAD.md) for partition sizing and full dependency versions.

## Configuration and security

`Config.cpp` contains firmware defaults for identity, access point name, web login, MQTT, LoRa, and cloud integration. The checked-in copy currently has a web password value: replace it with a unique secret before use and do not commit real device, Wi-Fi, MQTT, or cloud credentials. Follow [configuration and security](docs/CONFIGURATION.md).

Wi-Fi credentials and user-adjustable MQTT/LoRa settings are persisted with ESP32 Preferences. The web server uses a browser session cookie and serves protected APIs only to authenticated sessions. The login page and public device status are available without a session. The HTTP interface does not provide transport encryption; use it only on a trusted local network and avoid exposing it directly to the internet.

## Web interface

The frontend source lives in `local web/`, with widget modules under `local web/src/widgets/`. Use `npm run dev` for local frontend development and `npm run build` to regenerate the production files in `data/`. The dummy telemetry endpoint demonstrates live dashboard polling; it is not connected to a physical sensor yet.

## Documentation

- [Architecture](docs/ARCHITECTURE.md)
- [Build and upload](docs/BUILD_AND_UPLOAD.md)
- [Configuration and security](docs/CONFIGURATION.md)
- [Hardware notes](docs/HARDWARE.md)
- [Web UI and API](docs/WEB_UI_AND_API.md)
- [Maintenance](docs/MAINTENANCE.md)
- [Changelog](CHANGELOG.md)
- [Contributing](CONTRIBUTING.md)
- [Security policy](SECURITY.md)
