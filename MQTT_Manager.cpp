#include "MQTT_Manager.h"
#include "Config.h"
#include <ArduinoJson.h>
#include <esp_system.h>
#include <time.h>

MQTTManager *MQTTManager::_instance = nullptr;

MQTTManager::MQTTManager(const char *server, uint16_t port,
                         const char *clientId, const char *user, const char *pass)
    : _mqtt(_wifiClient), _server(server), _port(port),
      _clientId(clientId), _user(user), _pass(pass), _baseTopic(MQTT_BASE_TOPIC), _enabled(MQTT_ENABLED)
{
    _instance = this;
}

bool MQTTManager::begin()
{
    _clientReady = false;
    if (_preferences.begin("mqtt-config", true))
    {
        _enabled = _preferences.getBool("enabled", MQTT_ENABLED);
        _server = _preferences.getString("server", _server);
        _port = _preferences.getUShort("port", _port);
        _clientId = _preferences.getString("clientId", _clientId);
        _baseTopic = _preferences.getString("baseTopic", MQTT_BASE_TOPIC);
        if (_baseTopic.length() == 0)
            _baseTopic = MQTT_BASE_TOPIC;
        _user = _preferences.getString("username", _user);
        _pass = _preferences.getString("password", _pass);
        _useTls = _preferences.getBool("tls", MQTT_USE_TLS);
        _preferences.end();
    }

    for (auto &subscription : _subscriptions)
        subscription.topic = _resolveTopic(subscription.topic);

    std::map<String, MessageCallback> resolvedCallbacks;
    for (const auto &entry : _topicCallbacks)
        resolvedCallbacks[_resolveTopic(entry.first)] = entry.second;
    _topicCallbacks.swap(resolvedCallbacks);

    if (!_enabled || _server.length() == 0 || _clientId.length() == 0)
        return false;

    if (_useTls)
    {
        File certificateFile = LittleFS.open("/mqtt-ca.crt", "r");
        if (!certificateFile || certificateFile.size() == 0 || certificateFile.size() > 8192)
        {
            Serial.println("[ERROR] MQTT CA certificate is missing or has an invalid size.");
            return false;
        }
        _caCert = certificateFile.readString();
        certificateFile.close();
        if (!_caCert.startsWith("-----BEGIN CERTIFICATE-----") ||
            _caCert.indexOf("-----END CERTIFICATE-----") < 0)
        {
            Serial.println("[ERROR] MQTT CA certificate is not valid PEM text.");
            _caCert = "";
            return false;
        }
        _secureClient.setCACert(_caCert.c_str());
        _mqtt.setClient(_secureClient);
    }
    else
    {
        _mqtt.setClient(_wifiClient);
    }

    _mqtt.setServer(_server.c_str(), _port);
    _mqtt.setCallback(MQTTManager::staticCallback);
    _mqtt.setSocketTimeout(3);
    if (!_mqtt.setBufferSize(1024))
    {
        Serial.println("[ERROR] Failed to allocate the MQTT packet buffer.");
        return false;
    }
    _clientReady = true;
    return true;
}

bool MQTTManager::saveSettings(bool enabled, const String &server, uint16_t port,
                               const String &clientId, const String &baseTopic,
                               const String &username, const String &password,
                               bool useTls, bool clearPassword)
{
    if (enabled && (server.length() == 0 || clientId.length() == 0 || port == 0))
        return false;
    if (!_preferences.begin("mqtt-config", false))
        return false;

    _preferences.putBool("enabled", enabled);
    _preferences.putString("server", server);
    _preferences.putUShort("port", port);
    _preferences.putString("clientId", clientId);
    _preferences.putString("baseTopic", baseTopic.length() ? baseTopic : MQTT_BASE_TOPIC);
    _preferences.putString("username", username);
    _preferences.putBool("tls", useTls);
    if (clearPassword)
        _preferences.remove("password");
    else if (password.length() > 0)
        _preferences.putString("password", password);
    _preferences.end();
    return true;
}

bool MQTTManager::clearSettings()
{
    if (!_preferences.begin("mqtt-config", false))
        return false;

    _preferences.clear();
    _preferences.end();

    _enabled = MQTT_ENABLED;
    _clientReady = false;
    _server = "";
    _port = 1883;
    _clientId = "";
    _baseTopic = MQTT_BASE_TOPIC;
    _user = "";
    _pass = "";
    _useTls = MQTT_USE_TLS;
    return true;
}

String MQTTManager::settingsJson() const
{
    Preferences preferences;
    StaticJsonDocument<384> document;
    if (preferences.begin("mqtt-config", true))
    {
        document["enabled"] = preferences.getBool("enabled", MQTT_ENABLED);
        document["server"] = preferences.getString("server", _server);
        document["port"] = preferences.getUShort("port", _port);
        document["clientId"] = preferences.getString("clientId", _clientId);
        document["baseTopic"] = preferences.getString("baseTopic", MQTT_BASE_TOPIC);
        document["username"] = preferences.getString("username", _user);
        document["hasPassword"] = preferences.getString("password", _pass).length() > 0;
        document["tls"] = preferences.getBool("tls", MQTT_USE_TLS);
        preferences.end();
    }
    else
    {
        document["enabled"] = _enabled;
        document["server"] = _server;
        document["port"] = _port;
        document["clientId"] = _clientId;
        document["baseTopic"] = _baseTopic;
        document["username"] = _user;
        document["hasPassword"] = _pass.length() > 0;
        document["tls"] = _useTls;
    }
    String json;
    serializeJson(document, json);
    return json;
}

void MQTTManager::reconnect()
{
    if (!_enabled || !_clientReady)
        return;

    if (millis() - _lastReconnectAttempt < 5000)
        return;
    _lastReconnectAttempt = millis();

    const String willTopic = _resolveTopic("status");
    StaticJsonDocument<128> willDocument;
    willDocument["schemaVersion"] = 1;
    willDocument["deviceId"] = _clientId;
    willDocument["state"] = "offline";
    String willPayload;
    serializeJson(willDocument, willPayload);
    if (_mqtt.connect(_clientId.c_str(), _user.c_str(), _pass.c_str(),
                      willTopic.c_str(), 1, true, willPayload.c_str()))
    {
        _statusPublished = false;
        _lastStatusAttemptMs = millis();
        publishStatus(true);
        if (!_infoPublished)
            _infoPublished = publishInfo();
        resubscribeAll();
    }
}

void MQTTManager::resubscribeAll()
{
    for (const auto &sub : _subscriptions)
    {
        _mqtt.subscribe(sub.topic.c_str(), sub.qos);
    }
}

void MQTTManager::loop()
{
    if (!_enabled || !_clientReady)
        return;

    if (!_mqtt.connected())
    {
        reconnect();
    }
    else
    {
        _mqtt.loop();
    }
}

void MQTTManager::update(const TelemetryManager &telemetry, bool loraReady, int wifiRssi)
{
    _reportedLoraReady = loraReady;
    _reportedWifiRssi = wifiRssi;
    loop();
    if (!_mqtt.connected())
        return;

    const uint32_t now = millis();
    if (now - _lastTelemetryMs >= _telemetryIntervalMs)
    {
        const TelemetryPackage package = telemetry.getTemperaturePackage();
        if (package.valid)
        {
            if (publishTelemetry(package))
                _lastTelemetryMs = now;
        }
        else
        {
            _lastTelemetryMs = now;
        }
    }
    if ((!_statusPublished || now - _lastStatusMs >= 60000UL) &&
        now - _lastStatusAttemptMs >= 5000UL)
    {
        _lastStatusAttemptMs = now;
        publishStatus(true);
    }
    if (!_infoPublished && now - _lastInfoAttemptMs >= 5000UL)
    {
        _lastInfoAttemptMs = now;
        _infoPublished = publishInfo();
    }
}

namespace
{
    bool getMqttTimestamp(time_t &timestamp)
    {
        timestamp = time(nullptr);
        return timestamp >= 1577836800;
    }

    const char *resetReasonName(esp_reset_reason_t reason)
    {
        switch (reason)
        {
        case ESP_RST_POWERON: return "poweron";
        case ESP_RST_EXT: return "external";
        case ESP_RST_SW: return "software";
        case ESP_RST_PANIC: return "panic";
        case ESP_RST_INT_WDT: return "interrupt_wdt";
        case ESP_RST_TASK_WDT: return "task_wdt";
        case ESP_RST_WDT: return "wdt";
        case ESP_RST_DEEPSLEEP: return "deep_sleep";
        case ESP_RST_BROWNOUT: return "brownout";
        case ESP_RST_SDIO: return "sdio";
        default: return "unknown";
        }
    }
}

bool MQTTManager::publishTelemetry(const TelemetryPackage &package)
{
    time_t timestamp = 0;
    const bool timeSynced = getMqttTimestamp(timestamp);
    StaticJsonDocument<512> document;
    document["schemaVersion"] = 1;
    document["deviceId"] = _clientId;
    document["seq"] = ++_sequence;
    document["ts"] = timeSynced ? static_cast<uint32_t>(timestamp) : 0;
    document["timeSynced"] = timeSynced;
    document["source"] = package.source;
    document["measurement"] = package.measurement;
    document["rawValue"] = package.rawValue;
    document["filteredValue"] = package.filteredValue;
    document["unit"] = package.unit;
    document["filter"] = package.filter;
    document["initialized"] = package.initialized;
    document["online"] = package.online;
    document["valid"] = package.valid;
    document["timestampMs"] = package.timestampMs;
    String payload;
    serializeJson(document, payload);
    return publish("telemetry", payload, false);
}

bool MQTTManager::publishStatus(bool force)
{
    const uint32_t now = millis();
    if (!force && now - _lastStatusMs < 60000UL)
        return true;
    _lastStatusAttemptMs = now;

    time_t timestamp = 0;
    const bool timeSynced = getMqttTimestamp(timestamp);
    StaticJsonDocument<256> document;
    document["schemaVersion"] = 1;
    document["deviceId"] = _clientId;
    document["state"] = "online";
    document["ts"] = timeSynced ? static_cast<uint32_t>(timestamp) : 0;
    document["timeSynced"] = timeSynced;
    document["uptime"] = millis() / 1000UL;
    document["wifiRssi"] = _reportedWifiRssi;
    document["loraReady"] = _reportedLoraReady;
    document["freeHeap"] = ESP.getFreeHeap();
    document["minFreeHeap"] = ESP.getMinFreeHeap();
    String payload;
    serializeJson(document, payload);
    if (!publish("status", payload, true))
        return false;
    _lastStatusMs = now;
    _statusPublished = true;
    return true;
}

bool MQTTManager::publishInfo()
{
    time_t timestamp = 0;
    const bool timeSynced = getMqttTimestamp(timestamp);
    StaticJsonDocument<512> document;
    document["schemaVersion"] = 1;
    document["deviceId"] = _clientId;
    document["ts"] = timeSynced ? static_cast<uint32_t>(timestamp) : 0;
    document["timeSynced"] = timeSynced;
    document["brand"] = deviceBrand;
    document["model"] = deviceName;
    document["firmwareVersion"] = deviceVersion;
    document["sdkVersion"] = ESP.getSdkVersion();
    document["chipModel"] = ESP.getChipModel();
    document["chipRevision"] = ESP.getChipRevision();
    document["chipCores"] = ESP.getChipCores();
    document["cpuFrequency"] = ESP.getCpuFreqMHz();
    document["flashSize"] = ESP.getFlashChipSize();
    document["sketchSize"] = ESP.getSketchSize();
    document["sketchMd5"] = ESP.getSketchMD5();
    document["loraNodeId"] = LORA_NODE_ID;
    document["loraFrequency"] = LORA_FREQUENCY_MHZ;
    document["resetReason"] = resetReasonName(esp_reset_reason());
    String payload;
    serializeJson(document, payload);
    return publish("info", payload, true);
}

bool MQTTManager::publish(const String &topic, const String &payload, bool retain)
{
    if (!_mqtt.connected())
        return false;
    const String resolvedTopic = _resolveTopic(topic);
    return _mqtt.publish(resolvedTopic.c_str(), payload.c_str(), retain);
}

bool MQTTManager::subscribe(const String &topic, uint8_t qos)
{
    const String resolvedTopic = _resolveTopic(topic);
    for (auto &sub : _subscriptions)
    {
        if (sub.topic == resolvedTopic)
        {
            sub.qos = qos;
            return _mqtt.connected() ? _mqtt.subscribe(resolvedTopic.c_str(), qos) : true;
        }
    }

    _subscriptions.push_back({resolvedTopic, qos});

    if (_mqtt.connected())
    {
        return _mqtt.subscribe(resolvedTopic.c_str(), qos);
    }
    return true;
}

bool MQTTManager::unsubscribe(const String &topic)
{
    const String resolvedTopic = _resolveTopic(topic);
    for (auto it = _subscriptions.begin(); it != _subscriptions.end(); ++it)
    {
        if (it->topic == resolvedTopic)
        {
            _subscriptions.erase(it);
            break;
        }
    }
    _topicCallbacks.erase(resolvedTopic);

    if (_mqtt.connected())
    {
        return _mqtt.unsubscribe(resolvedTopic.c_str());
    }
    return true;
}

void MQTTManager::onMessage(const String &topicPattern, MessageCallback cb)
{
    _topicCallbacks[_resolveTopic(topicPattern)] = cb;
}

String MQTTManager::_resolveTopic(const String &topic) const
{
    String base = _baseTopic;
    String resolvedTopic = topic;
    base.trim();
    resolvedTopic.trim();

    while (base.endsWith("/"))
        base.remove(base.length() - 1);
    while (resolvedTopic.startsWith("/"))
        resolvedTopic.remove(0, 1);

    if (base.length() == 0)
        return resolvedTopic;
    if (resolvedTopic == base || resolvedTopic.startsWith(base + "/"))
        return resolvedTopic;
    if (resolvedTopic.length() == 0)
        return base;

    return base + "/" + resolvedTopic;
}

void MQTTManager::onMessage(MessageCallback cb)
{
    _defaultCallback = cb;
}

// Forward PubSubClient's callback to this instance.
void MQTTManager::staticCallback(char *topic, byte *payload, unsigned int length)
{
    if (_instance)
    {
        _instance->handleMessage(topic, payload, length);
    }
}

// Route an incoming message to the matching callback.
void MQTTManager::handleMessage(char *topic, byte *payload, unsigned int length)
{
    String topicStr(topic);
    String payloadStr;
    payloadStr.reserve(length);
    for (unsigned int i = 0; i < length; i++)
    {
        payloadStr += (char)payload[i];
    }

    // Check for an exact topic match first.
    auto exact = _topicCallbacks.find(topicStr);
    if (exact != _topicCallbacks.end())
    {
        exact->second(topicStr, payloadStr);
        return;
    }

    // Check registered wildcard patterns.
    for (const auto &entry : _topicCallbacks)
    {
        if (topicMatches(entry.first, topicStr))
        {
            entry.second(topicStr, payloadStr);
            return;
        }
    }

    // Use the default callback when no topic-specific callback matches.
    if (_defaultCallback)
    {
        _defaultCallback(topicStr, payloadStr);
    }
}

// Match MQTT single-level (+) and multi-level (#) wildcards.
bool MQTTManager::topicMatches(const String &pattern, const String &topic)
{
    if (pattern.indexOf('+') < 0 && pattern.indexOf('#') < 0)
    {
        return pattern == topic;
    }

    int pStart = 0, tStart = 0;
    while (pStart <= (int)pattern.length())
    {
        int pEnd = pattern.indexOf('/', pStart);
        String pLevel = (pEnd == -1) ? pattern.substring(pStart) : pattern.substring(pStart, pEnd);

        if (pLevel == "#")
            return true;

        if (tStart > (int)topic.length())
            return false;

        int tEnd = topic.indexOf('/', tStart);
        String tLevel = (tEnd == -1) ? topic.substring(tStart) : topic.substring(tStart, tEnd);

        if (pLevel != "+" && pLevel != tLevel)
            return false;

        bool pDone = (pEnd == -1);
        bool tDone = (tEnd == -1);
        if (pDone != tDone)
            return false;
        if (pDone && tDone)
            return true;

        pStart = pEnd + 1;
        tStart = tEnd + 1;
    }
    return false;
}

bool MQTTManager::isConnected()
{
    return _mqtt.connected();
}
