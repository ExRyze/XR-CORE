#pragma once

#include <Arduino.h>
#include <Arduino_ConnectionHandler.h>
#include <ArduinoIoTCloud.h>

// Owns the Arduino IoT Cloud properties and connection lifecycle.
class ArduinoCloudManager
{
public:
    bool begin();
    void loop();
    bool isConnected() const;

    // Cloud properties based on the AWLR Thing reference.
    // String/float/int/bool variables;

private:
    void initProperties();
    WiFiConnectionHandler *_connection = nullptr;
    bool _initialized = false;
};
