#include "Config.h"
#include <Arduino.h>

// Device Information
const char *deviceBrand = "ExRyze";
const char *deviceName = "XR-CORE";
const char *deviceVersion = "1.1.0";

// NTP Config
const long GMT_OFFSET_SEC = 8 * 3600;
const int DAYLIGHT_OFFSET_SEC = 0;

// WiFi Config
// nullptr SSID keeps the device in AP mode on first boot.
// Set both values to use firmware credentials on first boot.
const char *WIFI_SSID = nullptr;
const char *WIFI_PASSWORD = nullptr;

// Access Point Config
const char *AP_SSID = "XR-CORE";
const char *AP_PASS = nullptr; // nullptr: open AP; "": random password; "...": use it directly.

// Local Web Config
const char *WEB_AUTH_USERNAME = "root";
const char *WEB_AUTH_PASSWORD = "";
const bool WEB_AP_AUTO_OPEN = true;

// MQTT Config
// Feature defaults; can be changed in local web config settings; saved in Preferences.
const bool MQTT_ENABLED = false;
const char *MQTT_SERVER = "mqtt.local";
const uint16_t MQTT_PORT = 8883;
const bool MQTT_USE_TLS = true;
const char *MQTT_CLIENT_ID = "XR-CORE-01";
const char *MQTT_BASE_TOPIC = "exryze/xrcore/001";
const uint32_t MQTT_TELEMETRY_INTERVAL_MS = 5000UL;
const char *MQTT_USERNAME = "exryze-xrcore-001";
const char *MQTT_PASSWORD = "xrcore001";

// LoRa Config
// Feature defaults; can be changed in local web config settings; saved in Preferences.
const bool LORA_ENABLED = false;
const float LORA_FREQUENCY_MHZ = 920.0f;
const uint32_t LORA_BANDWIDTH_KHZ = 125;
const uint8_t LORA_SPREADING_FACTOR = 9;
const uint8_t LORA_CODING_RATE = 7;
const int8_t LORA_TX_POWER = 2;
const uint8_t LORA_SYNC_WORD = 0x12;
const uint16_t LORA_PREAMBLE_LENGTH = 8;
const bool LORA_CRC_ENABLED = true;
const char *LORA_NODE_ID = "XR-CORE-001";

// Arduino Cloud Config
// Feature defaults; can't be changed in local web config settings; CONFIDENTIAL.
const bool ARDUINO_CLOUD_ENABLED = false;
const char *CLOUD_DEVICE_ID = "";
const char *CLOUD_DEVICE_KEY = "";
