# Build and upload

## Arduino IDE profile

Use the profile captured for the current project build:

| Option | Value |
| --- | --- |
| Board | ESP32-WROOM-DA Module |
| ESP32 Arduino core | 3.3.12 |
| Flash size | 4 MB (32 Mb) |
| Flash frequency / mode | 80 MHz / DIO |
| CPU frequency | 240 MHz |
| Partition scheme | Custom, using the project's `partitions.csv` |
| Upload speed | 921600 baud (lower it if uploads are unreliable) |
| Erase all flash before sketch upload | Enabled in the supplied IDE image |
| Programmer | Esptool |

Arduino IDE may require adding the custom partition scheme to the installed ESP32 platform's `tools/partitions` directory or using the board package's supported custom CSV mechanism. If the scheme does not appear in the menu, consult the installed ESP32 core documentation rather than choosing a differently sized partition layout. Select the same scheme for firmware and filesystem uploads.

The attached compile log used ESP32 core libraries WiFi, Networking, Preferences, Wire, SPI, FS, NetworkClientSecure, LittleFS, Hash, and Update at 3.3.12. Third-party library versions recorded for that build are:

| Library | Version |
| --- | ---: |
| Adafruit GFX Library | 1.13.0 |
| Adafruit BusIO | 1.17.4 |
| Adafruit SSD1306 | 2.5.17 |
| ESP Async WebServer | 3.12.1 |
| Async TCP | 3.6.1 |
| PubSubClient | 2.8 |
| Arduino_ConnectionHandler | 1.3.0 |
| Arduino_DebugUtils | 1.5.0 |
| ArduinoIoTCloud | 2.10.0 |
| Arduino_CloudUtils | 1.2.1 |
| ArduinoMqttClient | 0.1.8 |
| ArduinoHttpClient | 0.6.2 |
| RadioLib | 8.0.0 |
| ArduinoJson | 7.4.3 |
| Adafruit BME680 Library | Install through Arduino IDE Library Manager |

Install any missing libraries through Arduino IDE Library Manager or the library's supported installation method. The list reflects the supplied compile environment; not every library listed is necessarily used in every build configuration.

## Compile and upload the sketch

1. Open `XR-CORE.ino` in Arduino IDE.
2. Set the board profile and port, and confirm the custom partition scheme.
3. Update `Config.cpp` for this device and ensure secrets are not committed.
4. Compile, then upload the sketch.
5. Open Serial Monitor at 115200 baud to inspect startup messages.

The supplied build reports 1,332,391 bytes of program storage (7% of the reported 16 MB maximum) and 54,868 bytes of global dynamic memory (16% of 327,680 bytes). The IDE's reported maximum program storage and the selected 4 MB flash profile appear inconsistent; verify the active partition and board settings when reproducing the build. These figures are a historical compile result, not a guarantee for another toolchain.

## Build the dashboard

Node.js and npm are required. In a terminal:

```sh
cd "local web"
npm install
npm run build
```

Vite writes production pages and hashed assets into the root `data/` directory. Review the generated output before flashing. Do not remove `data/logo.raw`; the OLED reads it at runtime. The build preserves existing files in `data`, so stale hashed assets may remain; the generated HTML references the active bundle names.

For frontend development, run `npm run dev` from `local web/`. The development server is for UI iteration; hardware APIs and the production filesystem should be verified on the device.

## Upload LittleFS data

Install the ESP32 filesystem uploader compatible with Arduino IDE 2.x using the [Random Nerd Tutorials filesystem uploader guide](https://randomnerdtutorials.com/install-esp32-filesystem-uploader-arduino-ide/). The IDE image supplied with this project shows a custom partition menu and the ESP32 filesystem upload workflow.

1. Build the frontend so `data/` contains the production page and assets.
2. Preserve `data/logo.raw` and any needed runtime files such as `/mqtt-ca.crt`.
3. Select the correct board, serial port, and the same custom partition scheme as the firmware.
4. Use the uploader's **ESP32 Sketch Data Upload** command (menu wording depends on plugin version).
5. Upload/re-upload the firmware separately when it has changed.
6. Restart and check serial output and the dashboard URL.

Filesystem upload erases/replaces the filesystem partition. A CA certificate uploaded at runtime can therefore be lost when deploying a new filesystem image; re-upload or provision it afterward if TLS MQTT relies on it. The firmware also mounts LittleFS with format-on-failure enabled, which can erase filesystem contents if mounting fails.

## PNG to OLED raw conversion

The source image is `data/logo.png`; the firmware expects `data/logo.raw` as a 54 x 54, 1-bit, MSB-first bitmap. From the project root:

```sh
python -m pip install Pillow numpy
python tools/convert_logo.py
```

The script writes 378 bytes to `data/logo.raw` and an enlarged preview to `tools/logo_preview.png`. Ensure the generated raw file is included in the next LittleFS upload.
