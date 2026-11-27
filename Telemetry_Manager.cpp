#include "Telemetry_Manager.h"
#include <cmath>

namespace
{
constexpr char BME680_SENSOR_ID[] = "bme680";
constexpr char TEMPERATURE_READING[] = "temperature";
constexpr char RAW_TEMPERATURE_READING[] = "temperature_raw";
}

TelemetryManager::TelemetryManager(const SensorManager &sensors)
    : _sensors(sensors)
{
}

void TelemetryManager::update()
{
    const TelemetryPackage package = getTemperaturePackage();
    if (!package.valid)
        return;

    portENTER_CRITICAL(&_historyMux);
    if (!_hasRecordedSample || package.timestampMs != _lastRecordedTimestampMs)
    {
        _history[_nextHistoryIndex] = {package.timestampMs, package.rawValue, package.filteredValue};
        _nextHistoryIndex = (_nextHistoryIndex + 1) % TELEMETRY_HISTORY_CAPACITY;
        if (_historyCount < TELEMETRY_HISTORY_CAPACITY)
            ++_historyCount;
        _lastRecordedTimestampMs = package.timestampMs;
        _hasRecordedSample = true;
    }
    portEXIT_CRITICAL(&_historyMux);
}

void TelemetryManager::getHistory(uint32_t rangeMs, TelemetryHistory &history) const
{
    const uint32_t now = millis();
    history.count = 0;

    portENTER_CRITICAL(&_historyMux);
    const uint16_t oldestIndex = (_nextHistoryIndex + TELEMETRY_HISTORY_CAPACITY - _historyCount) % TELEMETRY_HISTORY_CAPACITY;
    for (uint16_t i = 0; i < _historyCount; ++i)
    {
        const uint16_t index = (oldestIndex + i) % TELEMETRY_HISTORY_CAPACITY;
        const TelemetryHistoryPoint &point = _history[index];
        if (now - point.timestampMs <= rangeMs)
            history.points[history.count++] = point;
    }
    portEXIT_CRITICAL(&_historyMux);
}

TelemetryPackage TelemetryManager::getTemperaturePackage() const
{
    TelemetryPackage package;
    package.source = BME680_SENSOR_ID;
    package.measurement = TEMPERATURE_READING;
    package.unit = "C";

    const TelemetrySnapshot snapshot = _sensors.getTelemetrySnapshot();
    for (const SensorSnapshot &sensor : snapshot.sensors)
    {
        if (sensor.sensorId != BME680_SENSOR_ID)
            continue;

        package.source = sensor.sensorId;
        package.initialized = sensor.initialized;
        package.online = sensor.online;
        package.timestampMs = sensor.timestamp;
        package.filter = sensor.filterName;

        bool rawFound = false;
        bool filteredFound = false;
        bool rawValid = false;
        bool filteredValid = false;
        for (const SensorReading &reading : sensor.readings)
        {
            if (reading.name == RAW_TEMPERATURE_READING)
            {
                package.rawValue = reading.value;
                package.unit = reading.unit;
                rawFound = true;
                rawValid = reading.valid;
            }
            else if (reading.name == TEMPERATURE_READING)
            {
                package.filteredValue = reading.value;
                package.unit = reading.unit;
                filteredFound = true;
                filteredValid = reading.valid;
            }
        }

        package.valid = sensor.valid && sensor.initialized && sensor.online &&
                        rawFound && filteredFound && rawValid && filteredValid &&
                        std::isfinite(package.rawValue) && std::isfinite(package.filteredValue);
        return package;
    }

    return package;
}

float TelemetryManager::value() const
{
    const TelemetryPackage package = getTemperaturePackage();
    return package.valid ? package.filteredValue : 0.0f;
}
