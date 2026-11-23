#include "ArduinoCloud_Manager.h"

#include "Config.h"
#include <esp_system.h>

namespace
{
    ArduinoCloudManager *cloudManager = nullptr;

    // void function() {}
}

void ArduinoCloudManager::initProperties()
{
    ArduinoCloud.setBoardId(CLOUD_DEVICE_ID);
    ArduinoCloud.setSecretDeviceKey(CLOUD_DEVICE_KEY);
    // ArduinoCloud.addProperty(variable_a, READWRITE, ON_CHANGE, function);
    // ArduinoCloud.addProperty(variable_b, READ, ON_CHANGE, nullptr);
}

bool ArduinoCloudManager::begin()
{
    if (_initialized || !ARDUINO_CLOUD_ENABLED || CLOUD_DEVICE_ID[0] == '\0' ||
        CLOUD_DEVICE_KEY[0] == '\0' || !WiFi.isConnected())
    {
        return _initialized;
    }

    // Reuse the active Wi-Fi credentials managed by WiFiManager.
    _connection = new WiFiConnectionHandler(WiFi.SSID().c_str(), WiFi.psk().c_str());
    cloudManager = this;
    initProperties();
    ArduinoCloud.begin(*_connection);
    setDebugMessageLevel(2);
    ArduinoCloud.printDebugInfo();
    _initialized = true;
    return true;
}

void ArduinoCloudManager::loop()
{
    if (_initialized)
    {
        ArduinoCloud.update();
    }
}

bool ArduinoCloudManager::isConnected() const
{
    return _initialized && ArduinoCloud.connected();
}
