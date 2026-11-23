# Architecture

## Runtime overview

`XR-CORE.ino` constructs the firmware modules and performs startup. It initializes serial diagnostics and status LEDs, halts if the OLED cannot initialize, mounts LittleFS and starts the web server through `WebConfig`, starts Wi-Fi, then creates two pinned FreeRTOS tasks.

| Task | Core | Main work |
| --- | --- | --- |
| `SensorTask` | 1 | Updates demo telemetry every pass, rotates OLED pages every 20 seconds, and refreshes the display every 250 ms. |
| `NetworkTask` | 0 | Initializes and services LoRa, Wi-Fi, Arduino IoT Cloud, and MQTT, and updates the data LED every 250 ms. |

The Arduino `loop()` intentionally idles because application work runs in the explicit tasks. Task creation return values are currently not checked.

## Module responsibilities

- **Wi-Fi**: `WiFiManager` loads/saves credentials in Preferences, attempts station mode, falls back to AP mode after a connection timeout, handles the GPIO 0 button, and provides scan/status helpers. It also synchronizes time for the OLED clock.
- **Web**: `WebConfig` mounts LittleFS and serves the dashboard, login page, and authenticated endpoints using ESPAsyncWebServer. Session state is maintained by the firmware.
- **Display**: `OLEDDisplay` wraps Adafruit SSD1306 drawing operations. Its bitmap helper reads packed monochrome bytes from LittleFS.
- **Telemetry**: `DummyTelemetry` produces a demonstration value. It uses a critical section to protect access across tasks.
- **MQTT**: `MQTTManager` wraps PubSubClient and supports plain or TLS clients, optional CA material from LittleFS, Preferences-backed settings, subscriptions, and callbacks.
- **LoRa**: `LoRaManager` uses RadioLib, persists radio parameters, transmits a JSON telemetry packet every 10 seconds, and reads received packets from the network task. RadioLib's transmit call blocks that task for the packet airtime.
- **Cloud**: `ArduinoCloudManager` owns Arduino IoT Cloud initialization and loop processing.
- **Device diagnostics**: `ESPDevice` exposes chip, memory, flash, and sketch information.

## Data and storage

Preferences stores Wi-Fi credentials and settings for network features. LittleFS stores the static web app, OLED bitmap, and optional MQTT CA certificate (`/mqtt-ca.crt`). The `partitions.csv` table labels the filesystem partition `spiffs`, while application code accesses it through the Arduino `LittleFS` API; retain this compatibility when changing the partition table. `WebConfig::begin()` mounts it with format-on-failure enabled (`LittleFS.begin(true)`), so an unrecoverable mount failure may format and erase filesystem contents.

The custom table is for a 4 MB flash device. It reserves NVS, OTA metadata, two 1.5 MB OTA application slots, a 896 KB filesystem area, and a 64 KB coredump area. Do not select a different partition layout without checking firmware size, filesystem capacity, and OTA requirements.

## Build-time and runtime configuration

`Config.h` declares configuration values and default LoRa pin macros; `Config.cpp` defines defaults. Wi-Fi credentials are entered through the web flow and stored in Preferences. MQTT and LoRa settings can be changed through protected APIs and persisted. Arduino IoT Cloud requires credentials from the corresponding Thing.

Some feature settings are latched during module initialization. After changing settings that report `restartRequired`, restart the board for them to take effect.

## Known scope

The current OLED dashboard uses placeholder values, and `DummyTelemetry` is demonstration data. Physical sensor integration and production transport security are outside the current implementation. LoRa currently sends the node ID, sequence number, and demo telemetry value as JSON; received payloads are logged to Serial.
