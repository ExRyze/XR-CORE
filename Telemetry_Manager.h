#pragma once

#include <Arduino.h>
#include "Sensor_Manager.h"
#include "Telemetry_Package.h"

class TelemetryManager
{
public:
    explicit TelemetryManager(const SensorManager &sensors);

    void update();
    TelemetryPackage getTemperaturePackage() const;
    void getHistory(uint32_t rangeMs, TelemetryHistory &history) const;

    // Compatibility accessor for the existing scalar LoRa payload.
    float value() const;

private:
    const SensorManager &_sensors;
    mutable portMUX_TYPE _historyMux = portMUX_INITIALIZER_UNLOCKED;
    TelemetryHistoryPoint _history[TELEMETRY_HISTORY_CAPACITY] = {};
    uint16_t _historyCount = 0;
    uint16_t _nextHistoryIndex = 0;
    uint32_t _lastRecordedTimestampMs = 0;
    bool _hasRecordedSample = false;
};
