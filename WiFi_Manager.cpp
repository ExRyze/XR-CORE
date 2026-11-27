#include "WiFi_Manager.h"
#include <esp_system.h>

//  Constructor
WiFiManager::WiFiManager()
    : _mode(WiFiMode::CONNECTING), _connectStartTime(0), _apPassword(""),
      _lastButtonReading(HIGH), _stableButtonState(HIGH), _lastButtonChangeMs(0),
      _scanActive(false), _scanWasApMode(false)
{
}

//  begin()
void WiFiManager::begin()
{
    if (AP_BUTTON_PIN >= 0)
    {
        pinMode(AP_BUTTON_PIN, INPUT_PULLUP);
        _lastButtonReading = digitalRead(AP_BUTTON_PIN);
        _stableButtonState = _lastButtonReading;
    }
    else
    {
        _lastButtonReading = HIGH;
        _stableButtonState = HIGH;
    }
    _lastButtonChangeMs = millis();

    if (isButtonPressed())
    {
        forceApMode();
        return;
    }

    Serial.println("[INFO] Checking credential...");
    String savedSsid, savedPassword;
    bool hasCredentials = _loadCredentials(savedSsid, savedPassword);

    if (hasCredentials)
    {
        Serial.println("[INFO] Wi-Fi credentials are available in NVS.");
        _startStationMode(savedSsid, savedPassword);
    }
    else
    {
        Serial.println("[INFO] No Wi-Fi credentials are available; starting AP mode.");
        _startAccessPointMode();
    }

    while (millis() - _connectStartTime.load() < _connectTimeoutMs)
    {
        if (_mode.load() == WiFiMode::ACCESS_POINT)
            break;

        if (isButtonPressed())
        {
            forceApMode();
            break;
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            _mode.store(WiFiMode::STATION);
            Serial.printf("[INFO] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
            _initTime();
            break;
        }

        delay(20);
    }

    if (_mode.load() == WiFiMode::CONNECTING)
    {
        Serial.println("[INFO] Timeout, starting AP mode...");
        _startAccessPointMode();
    }
}

// Handle button presses and complete asynchronous connection attempts.
void WiFiManager::loop()
{
    uint32_t now = millis();
    if (_dnsServerStarted)
    {
        _dnsServer.processNextRequest();
    }

    if (AP_BUTTON_PIN >= 0)
    {
        bool buttonReading = digitalRead(AP_BUTTON_PIN);

        if (buttonReading != _lastButtonReading)
        {
            _lastButtonReading = buttonReading;
            _lastButtonChangeMs = now;
        }

        if (now - _lastButtonChangeMs >= _buttonDebounceMs && buttonReading != _stableButtonState)
        {
            _stableButtonState = buttonReading;
            if (_stableButtonState == LOW)
            {
                Serial.println("[INFO] AP button pressed; switching to Access Point mode.");
                forceApMode();
            }
        }
    }

    if (_mode.load() == WiFiMode::CONNECTING)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            _mode.store(WiFiMode::STATION);
            Serial.printf("[INFO] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
            _initTime();
        }
        else if (now - _connectStartTime.load() >= _connectTimeoutMs)
        {
            Serial.println("[INFO] Connection timed out; starting AP mode.");
            _startAccessPointMode();
            _connectStartTime.store(now);
        }
    }
}

// Persist Wi-Fi credentials in NVS.
bool WiFiManager::saveCredentials(const String &ssid, const String &password)
{
    if (ssid.length() == 0 || !_prefs.begin("wifi-config", false))
    {
        Serial.println("[ERROR] Unable to open Wi-Fi credential storage.");
        return false;
    }

    const bool hadSsid = _prefs.isKey("ssid");
    const String previousSsid = _prefs.getString("ssid", "");
    const String previousPassword = _prefs.getString("password", "");
    const size_t ssidBytes = _prefs.putString("ssid", ssid);
    _prefs.putString("password", password);
    const bool saved = ssidBytes > 0 &&
                       _prefs.isKey("password") &&
                       _prefs.getString("ssid", "") == ssid &&
                       _prefs.getString("password", "") == password;

    if (!saved)
    {
        if (hadSsid)
        {
            _prefs.putString("ssid", previousSsid);
            _prefs.putString("password", previousPassword);
        }
        else
        {
            _prefs.remove("ssid");
            _prefs.remove("password");
        }
        _prefs.end();
        Serial.println("[ERROR] Failed to save Wi-Fi credentials to NVS.");
        return false;
    }
    _prefs.end();

    Serial.println("[INFO] Wi-Fi credentials saved to NVS.");
    return true;
}

void WiFiManager::applySavedCredentials(bool restartAfterSave)
{
    if (restartAfterSave)
    {
        Serial.println("[INFO] Restarting device to apply WiFi changes...");
        delay(1000);
        ESP.restart();
        return;
    }

    reconnectNow();
}

void WiFiManager::reconnectNow()
{
    String ssid, password;
    if (!_loadCredentials(ssid, password) || ssid.length() == 0)
    {
        Serial.println("[INFO] No saved credentials, switching to AP mode...");
        _startAccessPointMode();
        return;
    }

    Serial.printf("[INFO] Reconnecting to saved WiFi: %s\n", ssid.c_str());
    _startStationMode(ssid, password);
}

//  clearCredentials()
void WiFiManager::clearCredentials()
{
    _prefs.begin("wifi-config", false);
    _prefs.remove("ssid");
    _prefs.remove("password");
    _prefs.remove("credentialsReset");
    _prefs.end();

    _apPassword = "";
    Serial.println("[WARN] Wi-Fi credentials cleared.");
}

void WiFiManager::clearApPassword()
{
    _prefs.begin("device-config", false);
    _prefs.remove("apPassword");
    _prefs.end();

    _apPassword = "";
    Serial.println("[WARN] AP password cleared.");
}

//  Getters
WiFiMode WiFiManager::getMode() const { return _mode.load(); }
bool WiFiManager::isConnected() const { return _mode.load() == WiFiMode::STATION && WiFi.status() == WL_CONNECTED; }

bool WiFiManager::isButtonPressed() const
{
    if (AP_BUTTON_PIN < 0)
    {
        return false;
    }

    if (digitalRead(AP_BUTTON_PIN) == LOW)
    {
        return true;
    }

    return false;
}

String WiFiManager::getIp() const
{
    if (_mode.load() == WiFiMode::ACCESS_POINT)
        return WiFi.softAPIP().toString();
    return WiFi.localIP().toString();
}

String WiFiManager::getSavedSsid() const
{
    _prefs.begin("wifi-config", true);
    String ssid = _prefs.getString("ssid", "");
    _prefs.end();
    return ssid;
}

String WiFiManager::getApPassword()
{
    if (AP_PASS == nullptr)
    {
        return "";
    }

    if (AP_PASS[0] != '\0')
    {
        return String(AP_PASS);
    }

    if (_apPassword.length() >= 12)
    {
        return _apPassword;
    }

    if (!_prefs.begin("device-config", false))
    {
        return "";
    }

    String storedPassword = _prefs.getString("apPassword", "");
    if (storedPassword.length() >= 12 && storedPassword.length() <= 63)
    {
        _apPassword = storedPassword;
        _prefs.end();
        return _apPassword;
    }

    static const char alphabet[] = "ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz23456789";
    char generatedPassword[13];
    size_t index = 0;
    while (index < sizeof(generatedPassword) - 1)
    {
        uint32_t randomValue = esp_random();
        uint8_t characterIndex = randomValue & 0x3F;
        if (characterIndex < sizeof(alphabet) - 1)
        {
            generatedPassword[index++] = alphabet[characterIndex];
        }
    }
    generatedPassword[sizeof(generatedPassword) - 1] = '\0';

    if (_prefs.putString("apPassword", generatedPassword) > 0)
    {
        _apPassword = generatedPassword;
    }

    _prefs.end();
    return _apPassword;
}

void WiFiManager::_initTime()
{
    // Configure NTP client (pool.ntp.org)
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, "pool.ntp.org", "time.nist.gov");
    Serial.println("[INFO] Synchronizing time via NTP...");
}

// Return the current local time using the requested format.
String WiFiManager::getTime(const char *format) const
{
    struct tm timeinfo;

    // Check if time is successfully obtained from system (getLocalTime returns false if year < 1970 or uninitialized)
    if (!getLocalTime(&timeinfo))
    {
        // Fallback strategy when offline (AP mode or no internet yet)
        if (_mode.load() == WiFiMode::ACCESS_POINT)
        {
            return "00:00";
        }
        return "Syncing...";
    }

    char buffer[64];
    strftime(buffer, sizeof(buffer), format, &timeinfo);
    return String(buffer);
}

// Load saved Wi-Fi credentials.
bool WiFiManager::_loadCredentials(String &ssid, String &password)
{
    if (_prefs.begin("wifi-config", true))
    {
        ssid = _prefs.getString("ssid", "");
        password = _prefs.getString("password", "");
        _prefs.end();
    }
    else
    {
        // A read-only open fails when this namespace has not been created yet.
        ssid = "";
        password = "";
        Serial.println("[INFO] Wi-Fi credential namespace is not present in NVS yet.");
    }

    if (ssid.length() > 0)
    {
        return true;
    }

    // Copy configured defaults into Preferences before using them.
    if (WIFI_SSID != nullptr && WIFI_SSID[0] != '\0')
    {
        ssid = String(WIFI_SSID);
        password = (WIFI_PASSWORD != nullptr) ? String(WIFI_PASSWORD) : String("");

        if (!_prefs.begin("wifi-config", false))
        {
            ssid = "";
            password = "";
            Serial.println("[ERROR] Unable to open NVS to save default Wi-Fi credentials.");
            return false;
        }

        const size_t ssidBytes = _prefs.putString("ssid", ssid);
        _prefs.putString("password", password);
        const bool saved = ssidBytes > 0 &&
                           _prefs.isKey("password") &&
                           _prefs.getString("ssid", "") == ssid &&
                           _prefs.getString("password", "") == password;
        if (!saved)
        {
            _prefs.remove("ssid");
            _prefs.remove("password");
            ssid = "";
            password = "";
            _prefs.end();
            Serial.println("[ERROR] Failed to persist default Wi-Fi credentials; starting AP mode.");
            return false;
        }
        _prefs.end();

        Serial.println("[INFO] Default Wi-Fi credentials copied from Config.cpp to NVS.");
        return true;
    }

    ssid = "";
    password = "";
    return false;
}

// Start Wi-Fi Station mode.
void WiFiManager::_startStationMode(const String &ssid, const String &password)
{
    if (_dnsServerStarted)
    {
        _dnsServer.stop();
        _dnsServerStarted = false;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    _mode.store(WiFiMode::CONNECTING);
    _connectStartTime.store(millis());

    Serial.printf("[INFO] Connecting to %s...\n", ssid.c_str());
}

// Start Wi-Fi Access Point mode.
bool WiFiManager::_startAccessPointMode()
{
    if (_dnsServerStarted)
    {
        _dnsServer.stop();
        _dnsServerStarted = false;
    }

    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);

    bool started = false;

    if (AP_PASS == nullptr)
    {
        started = WiFi.softAP(AP_SSID);
    }
    else if (AP_PASS[0] == '\0')
    {
        String apPassword = getApPassword();
        if (apPassword.length() >= 12)
        {
            started = WiFi.softAP(AP_SSID, apPassword.c_str());
        }
        else
        {
            Serial.println("[ERROR] Unable to load or create the Access Point password.");
        }
    }
    else
    {
        started = WiFi.softAP(AP_SSID, AP_PASS);
    }

    if (!started)
    {
        Serial.println("[ERROR] Failed to start the Access Point.");
        return false;
    }

    _mode.store(WiFiMode::ACCESS_POINT);
    if (WEB_AP_AUTO_OPEN)
    {
        _dnsServerStarted = _dnsServer.start(53, "*", WiFi.softAPIP());
        if (_dnsServerStarted)
        {
            Serial.printf("[INFO] Captive portal DNS started at %s.\n", WiFi.softAPIP().toString().c_str());
        }
        else
        {
            Serial.println("[WARN] Captive portal DNS failed to start; use the AP IP address directly.");
        }
    }
    Serial.printf("[INFO] AP mode activated - SSID: %s, IP: %s\n",
                  AP_SSID, getIp().c_str());
    return true;
}

int WiFiManager::scanNetworks(bool async, bool showHidden)
{
    if (async)
    {
        return startScan(showHidden);
    }

    if (_scanActive)
    {
        return WiFi.scanComplete();
    }

    bool wasApMode = (_mode.load() == WiFiMode::ACCESS_POINT);

    if (wasApMode)
    {
        WiFi.mode(WIFI_AP_STA);
    }
    else
    {
        WiFi.mode(WIFI_STA);
    }

    int networkCount = WiFi.scanNetworks(false, showHidden);

    if (networkCount < 0)
    {
        Serial.println("[WARN] WiFi scan failed.");
        if (wasApMode)
        {
            WiFi.mode(WIFI_AP);
            _startAccessPointMode();
        }
        else
        {
            WiFi.mode(WIFI_STA);
        }
        return 0;
    }

    Serial.printf("[INFO] Found %d WiFi networks.\n", networkCount);
    for (int i = 0; i < networkCount; i++)
    {
        Serial.printf("[%d] %s | RSSI: %d dBm | CH: %d | ENC: %d\n",
                      i,
                      WiFi.SSID(i).c_str(),
                      WiFi.RSSI(i),
                      WiFi.channel(i),
                      WiFi.encryptionType(i));
    }

    if (wasApMode)
    {
        WiFi.mode(WIFI_AP);
        _startAccessPointMode();
    }
    else
    {
        WiFi.mode(WIFI_STA);
    }

    return networkCount;
}

int WiFiManager::startScan(bool showHidden)
{
    if (_scanActive)
    {
        return WiFi.scanComplete();
    }

    _scanWasApMode = (_mode.load() == WiFiMode::ACCESS_POINT);
    WiFi.mode(_scanWasApMode ? WIFI_AP_STA : WIFI_STA);

    int scanStatus = WiFi.scanNetworks(true, showHidden);
    _scanActive = true;
    if (scanStatus < 0 && scanStatus != WIFI_SCAN_RUNNING)
    {
        Serial.println("[WARN] WiFi scan failed.");
        finishScan();
    }

    return scanStatus;
}

bool WiFiManager::isScanActive() const
{
    return _scanActive;
}

void WiFiManager::finishScan()
{
    if (!_scanActive)
    {
        return;
    }

    if (_scanWasApMode)
    {
        WiFi.mode(WIFI_AP);
        _startAccessPointMode();
    }
    else
    {
        WiFi.mode(WIFI_STA);
    }

    WiFi.scanDelete();
    _scanActive = false;
}

int WiFiManager::getScanCount() const
{
    return WiFi.scanComplete();
}

String WiFiManager::getScanSsid(uint8_t index) const
{
    if (index >= WiFi.scanComplete())
        return "";

    return WiFi.SSID(index);
}

int WiFiManager::getScanRssi(uint8_t index) const
{
    if (index >= WiFi.scanComplete())
        return -9999;

    return WiFi.RSSI(index);
}

int WiFiManager::getScanEncryptionType(uint8_t index) const
{
    if (index >= WiFi.scanComplete())
        return -1;

    return WiFi.encryptionType(index);
}

int WiFiManager::getConnectedRssi() const
{
    if (!isConnected())
        return -9999;

    return WiFi.RSSI();
}

int WiFiManager::getSignalQuality() const
{
    int rssi = getConnectedRssi();
    if (rssi <= -100)
        return 0;
    if (rssi >= -50)
        return 100;

    // Linear interpolation in the common Wi‑Fi range: -100 dBm to -50 dBm
    // maps to 0% to 100%.
    return ((rssi + 100) * 100) / 50;
}

// Manually switch to Access Point mode.
void WiFiManager::forceApMode()
{
    Serial.println("[INFO] Starting AP mode...");
    _startAccessPointMode();
}
