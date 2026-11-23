#pragma once

// Author : ExRyze
// Device : ESP32
// Version : 1.1.0

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "WiFi_Manager.h"
#include "MQTT_Manager.h"
#include "LoRa_Manager.h"
#include "DummyTelemetry.h"

const uint16_t SERVER_PORT = 80;

class WebConfig
{
public:
    WebConfig(WiFiManager &wifi, MQTTManager &mqtt, LoRaManager &lora, DummyTelemetry &telemetry);

    void begin();

private:
    AsyncWebServer _server;
    WiFiManager &_wifi;
    MQTTManager &_mqtt;
    LoRaManager &_lora;
    DummyTelemetry &_telemetry;
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
    String _jsonSSIDEmpty();
    String _jsonSuccess();
};
