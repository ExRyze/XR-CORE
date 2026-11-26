# Architecture

## Runtime overview

`XR-CORE.ino` constructs the firmware modules and performs startup. It initializes serial diagnostics and status LEDs, halts if the OLED cannot initialize, mounts LittleFS and starts the web server through `WebConfig`, starts Wi-Fi, then creates three pinned FreeRTOS tasks.

| Task | Core | Main work |
| --- | --- | --- |
| `SensorTask` | 1 | Updates demo telemetry every pass, rotates OLED pages every 20 seconds, and refreshes the display every 250 ms. |
| `NetworkTask` | 0 | Services Wi-Fi, Arduino IoT Cloud, LoRa, and the data LED every 250 ms. |
| `MqttTask` | 1 | Runs at priority 0 and initializes/services MQTT while Wi-Fi is connected, keeping broker and TLS connection work off `NetworkTask`. |

The Arduino `loop()` intentionally idles because application work runs in the explicit tasks. Task creation return values are currently not checked.

## Module responsibilities

- **Wi-Fi**: `WiFiManager` gives saved NVS credentials priority. When the NVS namespace or SSID is absent, it copies configured defaults into NVS before attempting station mode; without either source, it starts AP mode. It falls back to AP mode after a connection timeout, handles the configured AP button when its pin is non-negative, and provides scan/status helpers. With `WEB_AP_AUTO_OPEN` enabled, it also runs captive DNS in AP mode. It synchronizes time for the OLED clock.
- **Web**: `WebConfig` mounts LittleFS and serves the dashboard, login page, and authenticated endpoints using ESPAsyncWebServer. Session state is maintained by the firmware.
- **Display**: `OLEDDisplay` wraps Adafruit SSD1306 drawing operations. Its bitmap helper reads packed monochrome bytes from LittleFS.
- **Telemetry**: `DummyTelemetry` produces a demonstration value. It uses a critical section to protect access across tasks.
- **MQTT**: `MQTTManager` wraps PubSubClient and supports plain or TLS clients, CA material from LittleFS, Preferences-backed settings, subscriptions, callbacks, and the telemetry/status/info topic payloads. The configured enabled state is separate from client readiness, so missing TLS CA material does not mark the feature as disabled in the web settings.
- **LoRa**: `LoRaManager` uses RadioLib, persists radio parameters, transmits a JSON telemetry packet every 10 seconds, and reads received packets from the network task. RadioLib's transmit call blocks that task for the packet airtime.
- **Cloud**: `ArduinoCloudManager` owns Arduino IoT Cloud initialization and loop processing.
- **Device diagnostics**: `ESPDevice` exposes chip, memory, flash, and sketch information.

## Data and storage

Preferences stores Wi-Fi credentials and settings for network features. LittleFS stores the static web app, OLED bitmap, and MQTT CA certificate (`/mqtt-ca.crt`). The `partitions.csv` table labels the filesystem partition `spiffs`, while application code accesses it through the Arduino `LittleFS` API; retain this compatibility when changing the partition table. `WebConfig::begin()` mounts it with format-on-failure enabled (`LittleFS.begin(true)`), so an unrecoverable mount failure may format and erase filesystem contents.

The custom table is for a 4 MB flash device. It reserves NVS, OTA metadata, two 1.5 MB OTA application slots, a 896 KB filesystem area, and a 64 KB coredump area. Do not select a different partition layout without checking firmware size, filesystem capacity, and OTA requirements.

## Build-time and runtime configuration

`Config.h` declares configuration values and default LoRa pin macros; `Config.cpp` defines defaults. Wi-Fi credentials are entered through the web flow and stored in Preferences. MQTT and LoRa settings can be changed through protected APIs and persisted. Arduino IoT Cloud requires credentials from the corresponding Thing.

Some feature settings are latched during module initialization. After changing settings that report `restartRequired`, restart the board for them to take effect.

## Known scope

The current OLED dashboard uses placeholder values, and `DummyTelemetry` is demonstration data. Physical sensor integration and production transport security are outside the current implementation. LoRa currently sends the node ID, sequence number, and demo telemetry value as JSON; received payloads are logged to Serial.
