#pragma once

#include <Adafruit_BME680.h>
#include <Wire.h>
#include "Sensor_Interface.h"
#include "Sensor_Filter.h"

enum class BME680TemperatureFilter : uint8_t
{
    MOVING_AVERAGE,
    MEDIAN,
    CUMULATIVE_MEAN
};

struct BME680TemperatureData
{
    float rawTemperatureC = 0.0f;
    float filteredTemperatureC = 0.0f;
    bool valid = false;
    uint32_t timestamp = 0;
};

class BME680Sensor : public SensorInterface
{
public:
    explicit BME680Sensor(TwoWire &wire = Wire,
                          uint8_t address = BME68X_DEFAULT_ADDRESS,
                          uint32_t readIntervalMs = 2000,
                          uint8_t filterWindowSize = 5,
                          BME680TemperatureFilter filter = BME680TemperatureFilter::MOVING_AVERAGE);

    bool begin() override;
    bool update(uint32_t now) override;
    bool isInitialized() const override;
    bool isOnline() const override;
    bool isValid() const override;
    uint32_t lastUpdate() const override;
    SensorSnapshot getSnapshot() const override;
    void printData() const override;

    BME680TemperatureData getData() const;
    float getRawTemperature() const;
    float getFilteredTemperature() const;
    float getTemperature() const;
    uint32_t getReadInterval() const;
    bool setReadInterval(uint32_t intervalMs);
    uint8_t getFilterWindowSize() const;
    bool setTemperatureFilter(BME680TemperatureFilter filter);
    BME680TemperatureFilter getTemperatureFilter() const;
    const char *getTemperatureFilterName() const;

private:
    bool _startReading(uint32_t now);
    float _filterTemperature(float rawTemperatureC);
    void _setStatus(bool online, bool valid);

    Adafruit_BME680 _bme;
    const uint8_t _address;
    FilterAVG _averageFilter;
    FilterMED _medianFilter;
    FilterMEAN _cumulativeMeanFilter;
    BME680TemperatureFilter _temperatureFilter;
    const uint8_t _filterWindowSize;
    uint32_t _readIntervalMs;
    uint32_t _lastBeginAttemptMs = 0;
    uint32_t _lastCycleMs = 0;
    uint32_t _readingReadyAt = 0;
    bool _readingPending = false;

    mutable portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;
    bool _initialized = false;
    bool _online = false;
    BME680TemperatureData _data;
};
