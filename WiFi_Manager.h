#pragma once

// Author : ExRyze
// Device : ESP32
// Version : 1.1.0

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <atomic>
#include <time.h>
#include "Config.h"

extern const char *AP_SSID;
extern const char *AP_PASS;

enum class WiFiMode
{
    CONNECTING,
    STATION,
    ACCESS_POINT
};

class WiFiManager
{
public:
    WiFiManager();

    void begin();
    void loop();

    void saveCredentials(const String &ssid, const String &password, bool restartAfterSave = false);
    void reconnectNow();
    void clearCredentials();
    void clearApPassword();
    void forceApMode();

    WiFiMode getMode() const;
    bool isConnected() const;
    bool isButtonPressed() const;
    String getIp() const;
    String getSavedSsid() const;
    String getApPassword();
    String getTime(const char *format = "%H:%M:%S") const;

    // Scan nearby Wi-Fi networks and read their signal strength.
    int scanNetworks(bool async = false, bool showHidden = false);
    int startScan(bool showHidden = false);
    bool isScanActive() const;
    void finishScan();
    int getScanCount() const;
    String getScanSsid(uint8_t index) const;
    int getScanRssi(uint8_t index) const;
    int getScanEncryptionType(uint8_t index) const;
    int getConnectedRssi() const;
    int getSignalQuality() const;

private:
    mutable Preferences _prefs;
    std::atomic<WiFiMode> _mode;

    std::atomic<uint32_t> _connectStartTime;
    const uint32_t _connectTimeoutMs = 60 * 1000; // 1 minute
    String _apPassword;
    bool _lastButtonReading;
    bool _stableButtonState;
    uint32_t _lastButtonChangeMs;
    const uint32_t _buttonDebounceMs = 40;
    bool _scanActive;
    bool _scanWasApMode;

    void _initTime(); // Configure NTP time synchronization.

    bool _loadCredentials(String &ssid, String &password);
    bool _startAccessPointMode();
    void _startStationMode(const String &ssid, const String &password);
};
