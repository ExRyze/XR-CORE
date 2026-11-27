#pragma once

#include <Arduino.h>
#include <vector>

// One named measurement exposed by a sensor snapshot.
struct SensorReading
{
    String name;
    float value = 0.0f;
    String unit;
    bool valid = false;
};

// A sensor snapshot is copied as one coherent measurement transaction.
struct SensorSnapshot
{
    String sensorId;
    String filterName;
    bool initialized = false;
    bool online = false;
    bool valid = false;
    uint32_t timestamp = 0;
    std::vector<SensorReading> readings;
};

// Transport-neutral collection consumed by presentation and communication layers.
struct TelemetrySnapshot
{
    uint32_t timestamp = 0;
    std::vector<SensorSnapshot> sensors;
};
