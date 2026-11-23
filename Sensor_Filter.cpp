#include "Sensor_Filter.h"

namespace
{
uint8_t clampWindowSize(uint8_t requestedSize)
{
    if (requestedSize == 0)
        return 1;
    return requestedSize > FILTER_MAX_SAMPLES ? FILTER_MAX_SAMPLES : requestedSize;
}
}

FilterAVG::FilterAVG(uint8_t windowSize)
    : _windowSize(clampWindowSize(windowSize)), _nextIndex(0), _sampleCount(0), _sum(0.0f)
{
}

float FilterAVG::update(float newValue)
{
    if (_sampleCount == _windowSize)
        _sum -= _samples[_nextIndex];
    else
        ++_sampleCount;

    _samples[_nextIndex] = newValue;
    _sum += newValue;
    _nextIndex = (_nextIndex + 1) % _windowSize;
    return _sum / _sampleCount;
}

void FilterAVG::reset()
{
    _nextIndex = 0;
    _sampleCount = 0;
    _sum = 0.0f;
}

uint8_t FilterAVG::getSampleCount() const
{
    return _sampleCount;
}

FilterMED::FilterMED(uint8_t windowSize)
    : _windowSize(clampWindowSize(windowSize)), _nextIndex(0), _sampleCount(0)
{
}

float FilterMED::update(float newValue)
{
    _samples[_nextIndex] = newValue;
    _nextIndex = (_nextIndex + 1) % _windowSize;
    if (_sampleCount < _windowSize)
        ++_sampleCount;

    float sorted[FILTER_MAX_SAMPLES];
    for (uint8_t i = 0; i < _sampleCount; ++i)
        sorted[i] = _samples[i];

    // Insertion sort is efficient for the small fixed-size window.
    for (uint8_t i = 1; i < _sampleCount; ++i)
    {
        float value = sorted[i];
        int8_t j = static_cast<int8_t>(i) - 1;
        while (j >= 0 && sorted[j] > value)
        {
            sorted[j + 1] = sorted[j];
            --j;
        }
        sorted[j + 1] = value;
    }

    if (_sampleCount % 2 != 0)
        return sorted[_sampleCount / 2];
    return (sorted[_sampleCount / 2 - 1] + sorted[_sampleCount / 2]) / 2.0f;
}

void FilterMED::reset()
{
    _nextIndex = 0;
    _sampleCount = 0;
}

uint8_t FilterMED::getSampleCount() const
{
    return _sampleCount;
}

FilterMEAN::FilterMEAN() : _mean(0.0f), _sampleCount(0)
{
}

float FilterMEAN::update(float newValue)
{
    ++_sampleCount;
    _mean += (newValue - _mean) / static_cast<float>(_sampleCount);
    return _mean;
}

void FilterMEAN::reset()
{
    _mean = 0.0f;
    _sampleCount = 0;
}

uint32_t FilterMEAN::getSampleCount() const
{
    return _sampleCount;
}
