#pragma once

#include <Arduino.h>
#include "Sensor_Types.h"

class SensorInterface
{
public:
    virtual ~SensorInterface() = default;

    virtual bool begin() = 0;
    virtual bool update(uint32_t now) = 0;
    virtual bool isInitialized() const = 0;
    virtual bool isOnline() const = 0;
    virtual bool isValid() const = 0;
    virtual uint32_t lastUpdate() const = 0;
    virtual SensorSnapshot getSnapshot() const = 0;
    virtual void printData() const = 0;
};
