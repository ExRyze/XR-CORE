#include "Sensor_Manager.h"

bool SensorManager::addSensor(SensorInterface &sensor)
{
    for (const SensorInterface *registered : _sensors)
    {
        if (registered == &sensor)
            return false;
    }

    _sensors.push_back(&sensor);
    return true;
}

void SensorManager::begin()
{
    for (SensorInterface *sensor : _sensors)
        sensor->begin();
}

bool SensorManager::update(uint32_t now)
{
    bool updated = false;
    for (SensorInterface *sensor : _sensors)
    {
        if (sensor->update(now))
            updated = true;
    }
    return updated;
}

TelemetrySnapshot SensorManager::getTelemetrySnapshot() const
{
    TelemetrySnapshot snapshot;
    snapshot.timestamp = millis();
    snapshot.sensors.reserve(_sensors.size());

    for (const SensorInterface *sensor : _sensors)
        snapshot.sensors.push_back(sensor->getSnapshot());

    return snapshot;
}
