#pragma once

#include <Arduino.h>
#include <vector>
#include "Sensor_Interface.h"

class SensorManager
{
public:
    // Sensors are externally owned and must outlive this manager.
    bool addSensor(SensorInterface &sensor);
    void begin();
    bool update(uint32_t now);
    TelemetrySnapshot getTelemetrySnapshot() const;

private:
    std::vector<SensorInterface *> _sensors;
};
