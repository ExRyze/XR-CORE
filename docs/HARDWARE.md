# Hardware notes

## Board profile

The supplied Arduino IDE screenshot identifies **ESP32-WROOM-DA Module** on COM9. The sketch does not hard-code a serial port; select the port assigned by the host computer. The captured settings show 240 MHz CPU, 80 MHz flash, DIO mode, 4 MB flash, a custom partition scheme, and 921600 baud upload speed.

## OLED

The firmware uses Adafruit SSD1306 and Adafruit GFX over I2C. `OLEDDisplay::begin()` failure stops startup in an infinite delay loop. Confirm the display controller, address, dimensions, and I2C wiring against `OLED_Display.cpp` and the physical module. The OLED logo page reads `/logo.raw` from LittleFS as a 54 x 54 monochrome bitmap.

## BME680

`BME680Sensor` uses the same I2C bus as the OLED (`SDA_PIN` / `SCL_PIN`) and defaults to address `0x77`; use the constructor's address argument for modules strapped to `0x76`. The firmware reads temperature only and exposes it through the local web endpoint, MQTT telemetry, and OLED dashboard. Arduino IoT Cloud is not connected to the sensor.

## Status LEDs and provisioning button

The sketch assigns power LED GPIO 13 and data LED GPIO 12. The Wi-Fi manager assigns the AP button to GPIO 0 with `INPUT_PULLUP`; a pressed button reads LOW. GPIO 0 is also a boot-strapping pin on common ESP32 boards, so ensure attached circuitry does not interfere with reset/boot mode.

## LoRa

The default LoRa wiring is defined in `Config.h`:

| Signal | Default GPIO |
| --- | ---: |
| NSS / chip select | 5 |
| Reset | 14 |
| DIO0 | 2 |
| DIO1 | -1 (unused) |

RadioLib is configured for SX1276 unless `LORA_RADIO_TYPE` is overridden. Verify antenna, module voltage compatibility, GPIO routing, radio region/frequency, and power limits before transmitting. Board variants may require a different pin map.

## Flash and partition layout

`partitions.csv` is sized for 4 MB flash. It includes NVS, OTA metadata, two 0x180000 application slots, a 0xE0000 filesystem partition, and a 0x10000 coredump partition. Firmware and filesystem upload tools must use this same layout. Check the selected board's actual flash capacity before flashing.

## Wiring validation

This repository documents configured pins, not a complete schematic. Confirm the actual PCB and peripheral wiring before connecting hardware. Do not assume the default LoRa profile applies to every ESP32-WROOM-DA carrier board.
