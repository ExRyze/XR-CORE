#pragma once
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <LittleFS.h>
#include <functional>
#include <map>
#include <vector>

// Provides MQTT publishing, subscriptions, and message callbacks.
// Only one MQTTManager instance is supported because callbacks use a static instance pointer.
class MQTTManager
{
public:
    using MessageCallback = std::function<void(const String &topic, const String &payload)>;

    MQTTManager(const char *server, uint16_t port,
                const char *clientId, const char *user, const char *pass);

    bool begin();
    void loop();
    bool saveSettings(bool enabled, const String &server, uint16_t port,
                      const String &clientId, const String &baseTopic,
                      const String &username, const String &password,
                      bool useTls, bool clearPassword = false);
    bool clearSettings();
    String settingsJson() const;

    // Publish a message to a topic.
    bool publish(const String &topic, const String &payload, bool retain = false);

    // Manage topic subscriptions.
    bool subscribe(const String &topic, uint8_t qos = 0);
    bool unsubscribe(const String &topic);

    // Register a callback for an exact topic or wildcard pattern.
    void onMessage(const String &topicPattern, MessageCallback cb);

    // Register a callback used when no topic-specific callback matches.
    void onMessage(MessageCallback cb);

    bool isConnected();

private:
    String _resolveTopic(const String &topic) const;
    void reconnect();
    void resubscribeAll();

    static void staticCallback(char *topic, byte *payload, unsigned int length);
    void handleMessage(char *topic, byte *payload, unsigned int length);
    static bool topicMatches(const String &pattern, const String &topic);

    WiFiClient _wifiClient;
    WiFiClientSecure _secureClient;
    PubSubClient _mqtt;
    String _server;
    uint16_t _port;
    String _clientId;
    String _user;
    String _pass;
    String _caCert;
    String _baseTopic;
    bool _useTls = false;
    Preferences _preferences;
    unsigned long _lastReconnectAttempt = 0;

    struct Subscription
    {
        String topic;
        uint8_t qos;
    };
    std::vector<Subscription> _subscriptions;
    std::map<String, MessageCallback> _topicCallbacks;
    MessageCallback _defaultCallback = nullptr;

    static MQTTManager *_instance;
    bool _enabled = false;
};
