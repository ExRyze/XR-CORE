#include "BME680_Sensor.h"
#include <cmath>

namespace
{
constexpr uint32_t DEFAULT_READ_INTERVAL_MS = 2000UL;
constexpr uint32_t INITIALIZATION_RETRY_INTERVAL_MS = 5000UL;
constexpr uint8_t DEFAULT_FILTER_WINDOW_SIZE = 5;
constexpr char SENSOR_ID[] = "bme680";
constexpr char TEMPERATURE_NAME[] = "temperature";
constexpr char TEMPERATURE_UNIT[] = "C";
constexpr char RAW_TEMPERATURE_NAME[] = "temperature_raw";

uint8_t clampFilterWindowSize(uint8_t requestedSize)
{
    if (requestedSize == 0)
        return DEFAULT_FILTER_WINDOW_SIZE;
    return requestedSize > FILTER_MAX_SAMPLES ? FILTER_MAX_SAMPLES : requestedSize;
}
}

BME680Sensor::BME680Sensor(TwoWire &wire, uint8_t address,
                           uint32_t readIntervalMs, uint8_t filterWindowSize,
                           BME680TemperatureFilter filter)
    : _bme(&wire),
      _address(address),
      _averageFilter(clampFilterWindowSize(filterWindowSize)),
      _medianFilter(clampFilterWindowSize(filterWindowSize)),
      _temperatureFilter(filter),
      _filterWindowSize(clampFilterWindowSize(filterWindowSize)),
      _readIntervalMs(readIntervalMs == 0 ? DEFAULT_READ_INTERVAL_MS : readIntervalMs)
{
    if (_temperatureFilter != BME680TemperatureFilter::MOVING_AVERAGE &&
        _temperatureFilter != BME680TemperatureFilter::MEDIAN &&
        _temperatureFilter != BME680TemperatureFilter::CUMULATIVE_MEAN)
        _temperatureFilter = BME680TemperatureFilter::MOVING_AVERAGE;
}

bool BME680Sensor::begin()
{
    _lastBeginAttemptMs = millis();
    const bool deviceReady = _bme.begin(_address);
    const bool temperatureReady = deviceReady && _bme.setTemperatureOversampling(BME680_OS_8X);
    const bool pressureDisabled = temperatureReady && _bme.setPressureOversampling(BME680_OS_NONE);
    const bool humidityDisabled = pressureDisabled && _bme.setHumidityOversampling(BME680_OS_NONE);
    const bool gasDisabled = humidityDisabled && _bme.setGasHeater(0, 0);
    const bool ready = deviceReady && temperatureReady && pressureDisabled && humidityDisabled && gasDisabled;

    _readingPending = false;
    _lastCycleMs = millis() - _readIntervalMs;
    _averageFilter.reset();
    _medianFilter.reset();
    _cumulativeMeanFilter.reset();
    portENTER_CRITICAL(&_mux);
    _initialized = ready;
    _online = ready;
    _data.valid = false;
    portEXIT_CRITICAL(&_mux);

    if (ready)
        Serial.println(F("[INFO] BME680 temperature sensor initialized."));
    else
        Serial.println(F("[ERROR] BME680 initialization or temperature configuration failed."));

    return ready;
}

bool BME680Sensor::update(uint32_t now)
{
    if (!isInitialized())
    {
        if (now - _lastBeginAttemptMs >= INITIALIZATION_RETRY_INTERVAL_MS)
            begin();
        return false;
    }

    if (_readingPending)
    {
        if (static_cast<int32_t>(now - _readingReadyAt) < 0)
            return false;

        _readingPending = false;
        if (!_bme.endReading() || !std::isfinite(_bme.temperature))
        {
            _setStatus(false, false);
            return false;
        }

        const float rawTemperature = _bme.temperature;
        const float filteredTemperature = _filterTemperature(rawTemperature);
        portENTER_CRITICAL(&_mux);
        _online = true;
        _data.rawTemperatureC = rawTemperature;
        _data.filteredTemperatureC = filteredTemperature;
        _data.valid = true;
        _data.timestamp = now;
        portEXIT_CRITICAL(&_mux);
        // Serial.printf("[BME680] Temperature: %.2f C\n", filteredTemperature);
        return true;
    }

    if (now - _lastCycleMs < _readIntervalMs)
        return false;

    return _startReading(now);
}

bool BME680Sensor::_startReading(uint32_t now)
{
    _lastCycleMs = now;
    _readingReadyAt = _bme.beginReading();
    if (_readingReadyAt == 0)
    {
        _setStatus(false, false);
        return false;
    }

    _readingPending = true;
    return false;
}

void BME680Sensor::_setStatus(bool online, bool valid)
{
    portENTER_CRITICAL(&_mux);
    _online = online;
    _data.valid = valid;
    portEXIT_CRITICAL(&_mux);
}

bool BME680Sensor::isInitialized() const
{
    portENTER_CRITICAL(&_mux);
    const bool result = _initialized;
    portEXIT_CRITICAL(&_mux);
    return result;
}

bool BME680Sensor::isOnline() const
{
    portENTER_CRITICAL(&_mux);
    const bool result = _online;
    portEXIT_CRITICAL(&_mux);
    return result;
}

bool BME680Sensor::isValid() const
{
    portENTER_CRITICAL(&_mux);
    const bool result = _data.valid;
    portEXIT_CRITICAL(&_mux);
    return result;
}

uint32_t BME680Sensor::lastUpdate() const
{
    portENTER_CRITICAL(&_mux);
    const uint32_t result = _data.timestamp;
    portEXIT_CRITICAL(&_mux);
    return result;
}

BME680TemperatureData BME680Sensor::getData() const
{
    portENTER_CRITICAL(&_mux);
    const BME680TemperatureData result = _data;
    portEXIT_CRITICAL(&_mux);
    return result;
}

float BME680Sensor::getRawTemperature() const
{
    return getData().rawTemperatureC;
}

float BME680Sensor::getFilteredTemperature() const
{
    return getData().filteredTemperatureC;
}

float BME680Sensor::getTemperature() const
{
    return getFilteredTemperature();
}

SensorSnapshot BME680Sensor::getSnapshot() const
{
    BME680TemperatureData data;
    bool initialized;
    bool online;
    portENTER_CRITICAL(&_mux);
    data = _data;
    initialized = _initialized;
    online = _online;
    portEXIT_CRITICAL(&_mux);

    SensorSnapshot snapshot;
    snapshot.sensorId = SENSOR_ID;
    snapshot.filterName = getTemperatureFilterName();
    snapshot.initialized = initialized;
    snapshot.online = online;
    snapshot.valid = data.valid;
    snapshot.timestamp = data.timestamp;
    snapshot.readings.push_back({TEMPERATURE_NAME, data.filteredTemperatureC, TEMPERATURE_UNIT, data.valid});
    snapshot.readings.push_back({RAW_TEMPERATURE_NAME, data.rawTemperatureC, TEMPERATURE_UNIT, data.valid});
    return snapshot;
}

void BME680Sensor::printData() const
{
    const BME680TemperatureData data = getData();
    Serial.printf("[INFO] BME680 initialized=%s online=%s valid=%s raw=%.2f C filtered=%.2f C filter=%s timestamp=%lu\n",
                  isInitialized() ? "true" : "false",
                  isOnline() ? "true" : "false",
                  data.valid ? "true" : "false",
                  data.rawTemperatureC,
                  data.filteredTemperatureC,
                  getTemperatureFilterName(),
                  static_cast<unsigned long>(data.timestamp));
}

uint32_t BME680Sensor::getReadInterval() const
{
    return _readIntervalMs;
}

uint8_t BME680Sensor::getFilterWindowSize() const
{
    return _filterWindowSize;
}

bool BME680Sensor::setReadInterval(uint32_t intervalMs)
{
    if (intervalMs == 0)
        return false;

    _readIntervalMs = intervalMs;
    return true;
}

bool BME680Sensor::setTemperatureFilter(BME680TemperatureFilter filter)
{
    if (filter != BME680TemperatureFilter::MOVING_AVERAGE &&
        filter != BME680TemperatureFilter::MEDIAN &&
        filter != BME680TemperatureFilter::CUMULATIVE_MEAN)
        return false;

    _temperatureFilter = filter;
    _averageFilter.reset();
    _medianFilter.reset();
    _cumulativeMeanFilter.reset();
    portENTER_CRITICAL(&_mux);
    _data.valid = false;
    portEXIT_CRITICAL(&_mux);
    return true;
}

BME680TemperatureFilter BME680Sensor::getTemperatureFilter() const
{
    return _temperatureFilter;
}

const char *BME680Sensor::getTemperatureFilterName() const
{
    switch (_temperatureFilter)
    {
    case BME680TemperatureFilter::MOVING_AVERAGE:
        return "MOVING_AVERAGE";
    case BME680TemperatureFilter::MEDIAN:
        return "MEDIAN";
    case BME680TemperatureFilter::CUMULATIVE_MEAN:
        return "CUMULATIVE_MEAN";
    default:
        return "MOVING_AVERAGE";
    }
}

float BME680Sensor::_filterTemperature(float rawTemperatureC)
{
    switch (_temperatureFilter)
    {
    case BME680TemperatureFilter::MEDIAN:
        return _medianFilter.update(rawTemperatureC);
    case BME680TemperatureFilter::CUMULATIVE_MEAN:
        return _cumulativeMeanFilter.update(rawTemperatureC);
    case BME680TemperatureFilter::MOVING_AVERAGE:
    default:
        return _averageFilter.update(rawTemperatureC);
    }
}
