#pragma once

#include <Arduino.h>

// Device Information
extern const char *deviceBrand;
extern const char *deviceName;
extern const char *deviceVersion;

// NTP Config
extern const long GMT_OFFSET_SEC;
extern const int DAYLIGHT_OFFSET_SEC;

// WiFi Config
extern const char *WIFI_SSID;
extern const char *WIFI_PASSWORD;

// Access Point Config
extern const char *AP_SSID;
extern const char *AP_PASS;

// Local Web Config
extern const char *WEB_AUTH_USERNAME;
extern const char *WEB_AUTH_PASSWORD;
extern const bool WEB_AP_AUTO_OPEN;

// MQTT Config
extern const bool MQTT_ENABLED;
extern const char *MQTT_SERVER;
extern const uint16_t MQTT_PORT;
extern const bool MQTT_USE_TLS;
extern const char *MQTT_CLIENT_ID;
extern const char *MQTT_BASE_TOPIC;
extern const uint32_t MQTT_TELEMETRY_INTERVAL_MS;
extern const char *MQTT_USERNAME;
extern const char *MQTT_PASSWORD;

// LoRa Config
extern const bool LORA_ENABLED;
extern const float LORA_FREQUENCY_MHZ;
extern const uint32_t LORA_BANDWIDTH_KHZ;
extern const uint8_t LORA_SPREADING_FACTOR;
extern const uint8_t LORA_CODING_RATE;
extern const int8_t LORA_TX_POWER;
extern const uint8_t LORA_SYNC_WORD;
extern const uint16_t LORA_PREAMBLE_LENGTH;
extern const bool LORA_CRC_ENABLED;
extern const char *LORA_NODE_ID;

// LoRa family options
#define LORA_FAMILY_SX127X 1
#define LORA_FAMILY_SX126X 2

// Select the installed radio family and edit the matching ESP32 pin profile.
#define LORA_RADIO_FAMILY LORA_FAMILY_SX126X

// Arduino Cloud Config
extern const bool ARDUINO_CLOUD_ENABLED;
extern const char *CLOUD_DEVICE_ID;
extern const char *CLOUD_DEVICE_KEY;

// OLED Display Config
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_RESET -1
#define OLED_I2C_ADDR 0x3C
#define OLED_I2C_CLOCK 100000UL // 100 kHz

// GPIOs                    // Shared pins format: connection_PIN; Dedicated pins format module_connection_PIN
// #define POWER_LED 15     // Simply use 3.3V > Led > GND.
// #define DATA_LED 2       // Use matching ESP32 onboard data led (blue led)
#define AP_BUTTON_PIN -1

// UART Pins
#define TX_PIN 17
#define RX_PIN 16

// I2C Pins
#define SDA_PIN 21
#define SCL_PIN 22

// SPI Pins
#define SCK_PIN 18
#define MISO_PIN 19
#define MOSI_PIN 23

// Conditional pins from LoRa families
#if LORA_RADIO_FAMILY == LORA_FAMILY_SX127X
#define LORA_RADIO_TYPE SX1276
// SX127x: IRQ is DIO0; auxiliary pin is DIO1 (used by CAD/blocking receive).
#define LORA_CS_PIN 5
#define LORA_RESET_PIN 4
#define LORA_IRQ_PIN 13
#define LORA_AUX_PIN 14
#define LORA_RF_SWITCH_PIN -1
#elif LORA_RADIO_FAMILY == LORA_FAMILY_SX126X
#define LORA_RADIO_TYPE SX1262
// SX126x: IRQ is DIO1; auxiliary pin is BUSY.
#define LORA_CS_PIN 5
#define LORA_RESET_PIN 4
#define LORA_IRQ_PIN 13
#define LORA_AUX_PIN 14
#define LORA_RF_SWITCH_PIN 27
#define LORA_TCXO_VOLTAGE RadioLibTCXOVoltage_t::Voltage1V8
#else
#error "Unsupported LORA_RADIO_FAMILY. Select LORA_FAMILY_SX127X or LORA_FAMILY_SX126X."
#endif
