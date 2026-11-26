// Author : ExRyze
// Device : ESP32
// Version : 1.1.0

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <esp_system.h>
#include <cstdlib>
#include "Web_Config.h"
#include "ESP_Device.h"
#include "Config.h"

namespace
{
    constexpr char MQTT_CA_PATH[] = "/mqtt-ca.crt";
    constexpr char MQTT_CA_BACKUP_PATH[] = "/mqtt-ca.backup.tmp";
    constexpr size_t MQTT_CA_MAX_SIZE = 8192;

    struct MqttCaUploadState
    {
        size_t bytesReceived;
        bool authorized;
        bool failed;
        bool complete;
        char temporaryPath[40];
    };
}

//  Constructor
WebConfig::WebConfig(WiFiManager &wifi, MQTTManager &mqtt, LoRaManager &lora, DummyTelemetry &telemetry)
    : _server(SERVER_PORT), _wifi(wifi), _mqtt(mqtt), _lora(lora), _telemetry(telemetry)
{
}

//  begin()
void WebConfig::begin()
{
    if (!LittleFS.begin(true))
    {
        Serial.println("[ERROR] LittleFS mount failed!");
        return;
    }

    Serial.println("[INFO] LittleFS initialized.");

    _server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request)
               { request->send(LittleFS, _isAuthenticated(request) ? "/index.html" : "/login.html", "text/html"); });

    // API: Auth
    _server.on("/api/auth/login", HTTP_POST, [this](AsyncWebServerRequest *request)
               {
                   const String username = request->hasParam("username", true) ? request->getParam("username", true)->value() : "";
                   const String password = request->hasParam("password", true) ? request->getParam("password", true)->value() : "";
                   if (username != WEB_AUTH_USERNAME || password != WEB_AUTH_PASSWORD)
                   {
                       request->send(401, "application/json", "{\"status\":\"error\",\"message\":\"Invalid username or password.\"}");
                       return;
                   }

                   char token[33];
                   snprintf(token, sizeof(token), "%08lx%08lx%08lx%08lx",
                            (unsigned long)esp_random(), (unsigned long)esp_random(),
                            (unsigned long)esp_random(), (unsigned long)esp_random());
                   _sessionToken = token;
                   AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "{\"status\":\"ok\"}");
                   response->addHeader("Set-Cookie", "XRSESSION=" + _sessionToken + "; Path=/; HttpOnly; SameSite=Strict; Max-Age=3600");
                   request->send(response); });

    _server.on("/api/auth/logout", HTTP_POST, [this](AsyncWebServerRequest *request)
               {
                   if (!_requireAuthentication(request)) return;
                   _sessionToken = "";
                   AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "{\"status\":\"ok\"}");
                   response->addHeader("Set-Cookie", "XRSESSION=; Path=/; HttpOnly; SameSite=Strict; Max-Age=0");
                   request->send(response); });

    // API: System
    _server.on("/api/system/status", HTTP_GET, [this](AsyncWebServerRequest *request)
               { if (_requireAuthentication(request)) request->send(200, "application/json", _jsonSystemStatus()); });

    _server.on("/api/system/restart", HTTP_POST, [this](AsyncWebServerRequest *request)
               {
                   if (!_requireAuthentication(request)) return;
                   if (!request->hasHeader("X-Requested-With") || request->getHeader("X-Requested-With")->value() != "XMLHttpRequest")
                   {
                       request->send(403, "application/json", "{\"status\":\"error\",\"message\":\"Request validation failed\"}");
                       return;
                   }
                   request->send(200, "application/json", "{\"status\":\"ok\"}");
                   delay(1000);
                   ESP.restart(); });

    // API: Device
    _server.on("/api/device/public-status", HTTP_GET, [](AsyncWebServerRequest *request)
               {
                   StaticJsonDocument<64> doc;
                   doc["name"] = deviceName;
                   String json;
                   serializeJson(doc, json);
                   request->send(200, "application/json", json); });

    _server.on("/api/device/status", HTTP_GET, [this](AsyncWebServerRequest *request)
               { if (_requireAuthentication(request)) request->send(200, "application/json", _jsonDeviceStatus()); });

    // API: Wifi
    _server.on("/api/wifi/status", HTTP_GET, [this](AsyncWebServerRequest *request)
               { if (_requireAuthentication(request)) request->send(200, "application/json", _jsonWiFiStatus()); });

    _server.on("/api/wifi/scan", HTTP_GET, [this](AsyncWebServerRequest *request)
               { if (_requireAuthentication(request)) request->send(200, "application/json", _jsonWiFiScan()); });

    _server.on("/api/wifi/save", HTTP_POST, [this](AsyncWebServerRequest *request)
               {
        if (!_requireAuthentication(request)) return;
        if (!_hasValidCsrfHeader(request)) {
            request->send(403, "application/json", "{\"status\":\"error\",\"message\":\"Request validation failed\"}");
            return;
        }

        if (!request->hasParam("ssid", true)) {
            request->send(400, "application/json", _jsonSSIDEmpty());
            return;
        }

        String ssid     = request->getParam("ssid", true)->value();
        String password  = request->hasParam("password", true)
                                ? request->getParam("password", true)->value()
                                : "";
        bool restartAfterSave = request->hasParam("restartAfterSave", true)
                                && request->getParam("restartAfterSave", true)->value() == "true";

        if (ssid.length() == 0) {
            request->send(400, "application/json", _jsonSSIDEmpty());
            return;
        }

        if (!_wifi.saveCredentials(ssid, password))
        {
            request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Unable to save Wi-Fi credentials\"}");
            return;
        }

        request->send(200, "application/json", _jsonSuccess());
        _wifi.applySavedCredentials(restartAfterSave); });

    _server.on("/api/wifi/clear", HTTP_POST, [this](AsyncWebServerRequest *request)
               {
                   if (!_requireAuthentication(request))
                       return;
                   if (!_hasValidCsrfHeader(request))
                   {
                       request->send(403, "application/json", "{\"status\":\"error\",\"message\":\"Request validation failed\"}");
                       return;
                   }
                   request->send(200, "application/json", _jsonSuccess());
                   _wifi.clearCredentials();
                   _wifi.forceApMode();
                   delay(1000);
                   // keep the device waiting in AP mode for reconfiguration
               });

    _server.on("/api/wifi/ap-password/clear", HTTP_POST, [this](AsyncWebServerRequest *request)
               {
                   if (!_requireAuthentication(request))
                       return;
                   if (!_hasValidCsrfHeader(request))
                   {
                       request->send(403, "application/json", "{\"status\":\"error\",\"message\":\"Request validation failed\"}");
                       return;
                   }
                   request->send(200, "application/json", _jsonSuccess());
                   _wifi.clearApPassword();
                   _wifi.forceApMode();
                   delay(1000); });

    // API: MQTT
    _server.on("/api/mqtt/settings", HTTP_GET, [this](AsyncWebServerRequest *request)
               { if (_requireAuthentication(request)) request->send(200, "application/json", _mqtt.settingsJson()); });

    _server.on("/api/mqtt/status", HTTP_GET, [this](AsyncWebServerRequest *request)
               { if (_requireAuthentication(request)) request->send(200, "application/json", _jsonMqttStatus()); });

    _server.on("/api/mqtt/ca/status", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
                   if (!_isAuthenticated(request))
                   {
                       request->send(401, "application/json", "{\"status\":\"error\",\"message\":\"Authentication required.\"}");
                       return;
                   }
                   request->send(200, "application/json", LittleFS.exists(MQTT_CA_PATH)
                       ? "{\"available\":true}"
                       : "{\"available\":false}"); });

    _server.on("/api/mqtt/ca/upload", HTTP_POST, [this](AsyncWebServerRequest *request)
               {
                   auto *state = static_cast<MqttCaUploadState *>(request->_tempObject);
                   if (!state)
                   {
                       request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"No CA certificate was uploaded.\"}");
                       return;
                   }

                   if (!state->authorized)
                   {
                       request->send(401, "application/json", "{\"status\":\"error\",\"message\":\"Authentication or request validation failed.\"}");
                   }
                   else if (state->failed || !state->complete || state->bytesReceived == 0)
                   {
                       if (state->temporaryPath[0] != '\0')
                           LittleFS.remove(state->temporaryPath);
                       request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"CA upload failed or exceeded the 8 KB limit.\"}");
                   }
                   else
                   {
                       File certificateFile = LittleFS.open(state->temporaryPath, "r");
                       String certificate = certificateFile ? certificateFile.readString() : "";
                       if (certificateFile)
                           certificateFile.close();

                       const bool validPem = certificate.startsWith("-----BEGIN CERTIFICATE-----") &&
                                              certificate.indexOf("-----END CERTIFICATE-----") >= 0;
                       bool saved = false;
                       if (validPem)
                       {
                           if (!_caUploadInProgress)
                           {
                               _caUploadInProgress = true;
                               const bool hadPrevious = LittleFS.exists(MQTT_CA_PATH);
                               if (!hadPrevious || LittleFS.rename(MQTT_CA_PATH, MQTT_CA_BACKUP_PATH))
                               {
                                   if (LittleFS.rename(state->temporaryPath, MQTT_CA_PATH))
                                   {
                                       LittleFS.remove(MQTT_CA_BACKUP_PATH);
                                       saved = true;
                                   }
                                   else if (hadPrevious)
                                   {
                                       LittleFS.rename(MQTT_CA_BACKUP_PATH, MQTT_CA_PATH);
                                   }
                               }
                               _caUploadInProgress = false;
                           }
                       }

                       LittleFS.remove(state->temporaryPath);
                       request->send(saved ? 200 : 400, "application/json",
                                     saved ? "{\"status\":\"ok\",\"restartRequired\":true}"
                                           : "{\"status\":\"error\",\"message\":\"The uploaded file is not a valid PEM certificate or could not be saved.\"}");
                   }

                   free(state);
                   request->_tempObject = nullptr; }, [this](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t length, bool final)
               {
                   (void)filename;
                   auto *state = static_cast<MqttCaUploadState *>(request->_tempObject);
                   if (index == 0)
                   {
                       if (state)
                       {
                           state->failed = true;
                           request->_tempFile.close();
                           if (state->temporaryPath[0] != '\0')
                               LittleFS.remove(state->temporaryPath);
                           return;
                       }

                       state = static_cast<MqttCaUploadState *>(calloc(1, sizeof(MqttCaUploadState)));
                       if (!state)
                       {
                           request->abort();
                           return;
                       }
                       request->_tempObject = state;
                       state->authorized = _isAuthenticated(request) && _hasValidCsrfHeader(request);
                       if (!state->authorized)
                           return;
                       snprintf(state->temporaryPath, sizeof(state->temporaryPath),
                                "/mqtt-ca-%08lx.tmp", (unsigned long)esp_random());
                       request->_tempFile = LittleFS.open(state->temporaryPath, "w");
                       if (!request->_tempFile)
                       {
                           state->failed = true;
                           return;
                       }
                   }

                   if (!state || !state->authorized || state->failed || state->temporaryPath[0] == '\0')
                       return;

                   if (length > 0)
                   {
                       if (state->bytesReceived + length > MQTT_CA_MAX_SIZE ||
                           request->_tempFile.write(data, length) != length)
                       {
                           state->failed = true;
                           request->_tempFile.close();
                           LittleFS.remove(state->temporaryPath);
                       }
                       else
                       {
                           state->bytesReceived += length;
                       }
                   }

                   if (final)
                   {
                       request->_tempFile.close();
                       state->complete = !state->failed;
                   } });

    _server.on("/api/mqtt/settings", HTTP_POST, [this](AsyncWebServerRequest *request)
               {
        if (!_requireAuthentication(request)) return;
        if (!_hasValidCsrfHeader(request)) {
            request->send(403, "application/json", "{\"status\":\"error\",\"message\":\"Request validation failed\"}");
            return;
        }

        String server = request->hasParam("server", true) ? request->getParam("server", true)->value() : "";
        server.trim();
        const String portValue = request->hasParam("port", true) ? request->getParam("port", true)->value() : "";
        const long port = portValue.toInt();
        String clientId = request->hasParam("clientId", true) ? request->getParam("clientId", true)->value() : "";
        clientId.trim();
        String baseTopic = request->hasParam("baseTopic", true) ? request->getParam("baseTopic", true)->value() : "";
        baseTopic.trim();
        const String username = request->hasParam("username", true) ? request->getParam("username", true)->value() : "";
        const String password = request->hasParam("password", true) ? request->getParam("password", true)->value() : "";
        const bool enabled = request->hasParam("enabled", true) && request->getParam("enabled", true)->value() == "true";
        const bool useTls = request->hasParam("tls", true) && request->getParam("tls", true)->value() == "true";
        const bool clearPassword = request->hasParam("clearPassword", true) && request->getParam("clearPassword", true)->value() == "true";

        if (port < 1 || port > 65535 || server.length() > 253 || clientId.length() > 128 ||
            baseTopic.length() > 128 || username.length() > 128 || password.length() > 128 ||
            (enabled && (server.length() == 0 || clientId.length() == 0))) {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Check the broker host, port, client ID, and field lengths.\"}");
            return;
        }
        if (!_mqtt.saveSettings(enabled, server, (uint16_t)port, clientId, baseTopic, username, password, useTls, clearPassword)) {
            request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Failed to save MQTT settings.\"}");
            return;
        }
        request->send(200, "application/json", "{\"status\":\"ok\",\"restartRequired\":true}"); });

    _server.on("/api/mqtt/settings/reset", HTTP_POST, [this](AsyncWebServerRequest *request)
               {
                   if (!_requireAuthentication(request)) return;
                   if (!_hasValidCsrfHeader(request)) {
                       request->send(403, "application/json", "{\"status\":\"error\",\"message\":\"Request validation failed\"}");
                       return;
                   }
                   if (!_mqtt.clearSettings()) {
                       request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Failed to reset MQTT settings.\"}");
                       return;
                   }
                   request->send(200, "application/json", "{\"status\":\"ok\",\"restartRequired\":true}"); });

    // API: Lora
    _server.on("/api/lora/settings", HTTP_GET, [this](AsyncWebServerRequest *request)
               { if (_requireAuthentication(request)) request->send(200, "application/json", _lora.settingsJson()); });

    _server.on("/api/lora/settings/reset", HTTP_POST, [this](AsyncWebServerRequest *request)
               {
                   if (!_requireAuthentication(request)) return;
                   if (!_hasValidCsrfHeader(request)) {
                       request->send(403, "application/json", "{\"status\":\"error\",\"message\":\"Request validation failed\"}");
                       return;
                   }
                   if (!_lora.clearSettings()) {
                       request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Failed to reset LoRa settings.\"}");
                       return;
                   }
                   request->send(200, "application/json", "{\"status\":\"ok\",\"restartRequired\":true}"); });

    _server.on("/api/lora/settings", HTTP_POST, [this](AsyncWebServerRequest *request)
               {
        if (!_requireAuthentication(request)) return;
        if (!_hasValidCsrfHeader(request)) {
            request->send(403, "application/json", "{\"status\":\"error\",\"message\":\"Request validation failed\"}");
            return;
        }

        const bool enabled = request->hasParam("enabled", true) && request->getParam("enabled", true)->value() == "true";
        const String frequencyValue = request->hasParam("frequency", true) ? request->getParam("frequency", true)->value() : "";
        const float frequency = frequencyValue.toFloat();
        const String bandwidthValue = request->hasParam("bandwidth", true) ? request->getParam("bandwidth", true)->value() : "";
        const long bandwidth = bandwidthValue.toInt();
        const String spreadingFactorValue = request->hasParam("spreadingFactor", true) ? request->getParam("spreadingFactor", true)->value() : "";
        const long spreadingFactor = spreadingFactorValue.toInt();
        const String codingRateValue = request->hasParam("codingRate", true) ? request->getParam("codingRate", true)->value() : "";
        const long codingRate = codingRateValue.toInt();
        const String txPowerValue = request->hasParam("txPower", true) ? request->getParam("txPower", true)->value() : "";
        const long txPower = txPowerValue.toInt();
        String syncWordValue = request->hasParam("syncWord", true) ? request->getParam("syncWord", true)->value() : "";
        syncWordValue.trim();
        if (syncWordValue.startsWith("0x") || syncWordValue.startsWith("0X")) syncWordValue.remove(0, 2);
        char *syncWordEnd = nullptr;
        const long syncWord = strtol(syncWordValue.c_str(), &syncWordEnd, 16);
        const String preambleLengthValue = request->hasParam("preambleLength", true) ? request->getParam("preambleLength", true)->value() : "";
        const long preambleLength = preambleLengthValue.toInt();
        const bool crcEnabled = request->hasParam("crcEnabled", true) && request->getParam("crcEnabled", true)->value() == "true";
        String nodeId = request->hasParam("nodeId", true) ? request->getParam("nodeId", true)->value() : "";
        nodeId.trim();

        if (frequency < 137.0f || frequency > 1020.0f || (bandwidth != 125 && bandwidth != 250 && bandwidth != 500) ||
            spreadingFactor < 7 || spreadingFactor > 12 || codingRate < 5 || codingRate > 8 ||
            txPower < 2 || txPower > 20 || preambleLength < 6 || preambleLength > 65535 ||
            syncWordValue.length() == 0 || syncWordValue.length() > 2 || !syncWordEnd || *syncWordEnd != '\0' || syncWord < 0 || syncWord > 255 ||
            nodeId.length() == 0 || nodeId.length() > 32) {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Check the LoRa frequency, bandwidth, spreading factor, coding rate, transmit power, sync word, preamble length, and node ID.\"}");
            return;
        }
        if (!_lora.saveSettings(enabled, frequency, static_cast<uint32_t>(bandwidth), static_cast<uint8_t>(spreadingFactor),
                                static_cast<uint8_t>(codingRate), static_cast<int8_t>(txPower), static_cast<uint8_t>(syncWord),
                                static_cast<uint16_t>(preambleLength), crcEnabled, nodeId)) {
            request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Failed to save LoRa settings.\"}");
            return;
        }
        request->send(200, "application/json", "{\"status\":\"ok\",\"restartRequired\":true}"); });

    // Telemetry
    // format: (/api/{sensor}/{command})
    _server.on("/api/dummy/telemetry", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
                   if (!_requireAuthentication(request)) return;
                   String json = "{\"value\":" + String(_telemetry.value(), 2) +
                                 ",\"unit\":\"°C\",\"updatedAt\":" + String(_telemetry.updatedAt()) + "}";
                   request->send(200, "application/json", json); });

    // Fallback
    _server.onNotFound([this](AsyncWebServerRequest *request)
                       {
                           if (request->method() == HTTP_GET && (request->url() == "/login" || request->url() == "/login.html"))
                           {
                               request->send(LittleFS, "/login.html", "text/html");
                               return;
                           }

                           if (request->method() == HTTP_GET && request->url() == "/index.html")
                           {
                               if (_requireAuthentication(request)) request->send(LittleFS, "/index.html", "text/html");
                               return;
                           }

                           if (request->method() == HTTP_GET && (request->url().startsWith("/assets/") || request->url() == "/logo.png"))
                           {
                               request->send(LittleFS, request->url());
                               return;
                           }

                           if (WEB_AP_AUTO_OPEN && request->method() == HTTP_GET &&
                               _wifi.getMode() == WiFiMode::ACCESS_POINT)
                           {
                               AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "");
                               response->addHeader("Location", "http://" + _wifi.getIp() + "/");
                               response->addHeader("Cache-Control", "no-store");
                               request->send(response);
                               return;
                           }

                           request->send(404, "application/json", _jsonPageNotFound()); });

    _server.begin();
    Serial.println("[INFO] AsyncWebServer started.");
}

bool WebConfig::_isAuthenticated(AsyncWebServerRequest *request) const
{
    if (_sessionToken.length() == 0 || !request->hasHeader("Cookie"))
        return false;
    const String cookies = request->getHeader("Cookie")->value();
    const String cookie = "XRSESSION=" + _sessionToken;
    int position = cookies.indexOf(cookie);
    while (position >= 0)
    {
        const int end = position + cookie.length();
        if ((position == 0 || cookies[position - 1] == ';' || cookies[position - 1] == ' ') &&
            (end == cookies.length() || cookies[end] == ';' || cookies[end] == ' '))
            return true;
        position = cookies.indexOf(cookie, position + 1);
    }
    return false;
}

bool WebConfig::_requireAuthentication(AsyncWebServerRequest *request) const
{
    if (_isAuthenticated(request))
        return true;
    request->send(401, "application/json", "{\"status\":\"error\",\"message\":\"Authentication required.\"}");
    return false;
}

bool WebConfig::_hasValidCsrfHeader(AsyncWebServerRequest *request) const
{
    return request->hasHeader("X-Requested-With") &&
           request->getHeader("X-Requested-With")->value() == "XMLHttpRequest";
}

String WebConfig::_jsonPageNotFound()
{
    StaticJsonDocument<64> doc;

    doc["status"] = "error";
    doc["message"] = "Page not found";

    String json;
    serializeJson(doc, json);

    return json;
}

String WebConfig::_jsonSystemStatus()
{
    ESPDevice device;
    StaticJsonDocument<256> doc;

    doc["chipModel"] = device.getChipModel();
    doc["chipRevision"] = device.getChipRevision();
    doc["chipCores"] = device.getChipCores();
    doc["cpuFreqMHz"] = device.getCpuFreqMHz();
    doc["heapSize"] = (unsigned long)device.getHeapSize();
    doc["freeHeap"] = (unsigned long)device.getFreeHeap();
    doc["minFreeHeap"] = (unsigned long)device.getMinFreeHeap();
    doc["maxAllocHeap"] = (unsigned long)device.getMaxAllocHeap();
    doc["flashChipSize"] = (unsigned long)device.getFlashChipSize();
    doc["flashChipSpeed"] = (unsigned long)device.getFlashChipSpeed();
    doc["flashChipMode"] = (int)device.getFlashChipMode();
    doc["flashFrequencyMHz"] = device.getFlashFrequencyMHz();
    doc["sdkVersion"] = device.getSdkVersion();
    doc["coreVersion"] = device.getCoreVersion();

    String efuseMac = String(device.getEfuseMac(), HEX);
    efuseMac.toLowerCase();
    if (efuseMac.length() == 12)
    {
        String formattedMac;
        for (int i = 0; i < efuseMac.length(); i += 2)
        {
            if (i > 0)
                formattedMac += ":";
            formattedMac += efuseMac.substring(i, i + 2);
        }
        doc["efuseMac"] = formattedMac;
    }
    else
    {
        doc["efuseMac"] = efuseMac;
    }

    doc["sketchSize"] = (unsigned long)device.getSketchSize();
    doc["sketchMD5"] = device.getSketchMD5();
    doc["freeSketchSpace"] = (unsigned long)device.getFreeSketchSpace();

    String json;
    serializeJson(doc, json);
    return json;
}

String WebConfig::_jsonWiFiStatus()
{
    StaticJsonDocument<128> doc;

    doc["mode"] = String(_wifi.getMode() == WiFiMode::ACCESS_POINT ? "AP" : "STA");
    doc["ip"] = _wifi.getIp();
    doc["savedSsid"] = _wifi.getSavedSsid();
    doc["isConnected"] = _wifi.isConnected();
    doc["rssi"] = _wifi.getConnectedRssi();
    doc["signalQuality"] = _wifi.getSignalQuality();

    String json;
    serializeJson(doc, json);

    return json;
}

String WebConfig::_jsonMqttStatus()
{
    StaticJsonDocument<64> doc;
    doc["connected"] = _mqtt.isConnected();

    String json;
    serializeJson(doc, json);
    return json;
}

String WebConfig::_jsonWiFiScan()
{
    if (!_wifi.isScanActive())
    {
        int startResult = _wifi.startScan(false);
        if (startResult < 0 && startResult != WIFI_SCAN_RUNNING)
        {
            StaticJsonDocument<96> errorDoc;
            errorDoc["status"] = "error";
            errorDoc["message"] = "WiFi scan failed";
            String errorJson;
            serializeJson(errorDoc, errorJson);
            return errorJson;
        }
    }

    int count = _wifi.getScanCount();
    if (count == WIFI_SCAN_RUNNING)
    {
        return "{\"status\":\"scanning\"}";
    }

    if (count < 0)
    {
        _wifi.finishScan();
        StaticJsonDocument<96> errorDoc;
        errorDoc["status"] = "error";
        errorDoc["message"] = "WiFi scan failed";
        String errorJson;
        serializeJson(errorDoc, errorJson);
        return errorJson;
    }

    StaticJsonDocument<512> doc;
    JsonArray arr = doc.to<JsonArray>();

    for (int i = 0; i < count; i++)
    {
        JsonObject item = arr.createNestedObject();
        item["ssid"] = _wifi.getScanSsid(i);
        item["rssi"] = _wifi.getScanRssi(i);
        item["encryptionType"] = _wifi.getScanEncryptionType(i);
        item["channel"] = WiFi.channel(i);
    }

    String json;
    serializeJson(doc, json);
    _wifi.finishScan();
    return json;
}

String WebConfig::_jsonDeviceStatus()
{
    StaticJsonDocument<128> doc;
    doc["brand"] = deviceBrand;
    doc["name"] = deviceName;
    doc["version"] = deviceVersion;

    String json;
    serializeJson(doc, json);
    return json;
}

String WebConfig::_jsonSSIDEmpty()
{
    StaticJsonDocument<64> doc;
    doc["status"] = "error";
    doc["message"] = "SSID is required";

    String json;
    serializeJson(doc, json);
    return json;
}

String WebConfig::_jsonSuccess()
{
    StaticJsonDocument<64> doc;
    doc["status"] = "ok";

    String json;
    serializeJson(doc, json);
    return json;
}
