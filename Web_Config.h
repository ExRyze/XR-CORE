#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "WiFi_Manager.h"
#include "MQTT_Manager.h"
#include "LoRa_Manager.h"
#include "Telemetry_Manager.h"

const uint16_t SERVER_PORT = 80;

class WebConfig
{
public:
    WebConfig(WiFiManager &wifi, MQTTManager &mqtt, LoRaManager &lora, TelemetryManager &telemetry);

    void begin();

private:
    AsyncWebServer _server;
    WiFiManager &_wifi;
    MQTTManager &_mqtt;
    LoRaManager &_lora;
    TelemetryManager &_telemetry;
    String _sessionToken;
    bool _caUploadInProgress = false;

    bool _hasValidCsrfHeader(AsyncWebServerRequest *request) const;
    bool _isAuthenticated(AsyncWebServerRequest *request) const;
    bool _requireAuthentication(AsyncWebServerRequest *request) const;

    // JSON responses
    String _jsonPageNotFound();

    String _jsonSystemStatus();
    String _jsonMqttStatus();
    String _jsonWiFiStatus();
    String _jsonWiFiScan();
    String _jsonDeviceStatus();
    String _jsonBME680Telemetry() const;
    String _jsonBME680History(uint32_t rangeMs) const;
    String _jsonSSIDEmpty();
    String _jsonSuccess();
};
