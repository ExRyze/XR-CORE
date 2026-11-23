#pragma once

#include <Arduino.h>

// Maximum storage reserved by each rolling-window filter.
static const uint8_t FILTER_MAX_SAMPLES = 32;

class FilterAVG
{
public:
    explicit FilterAVG(uint8_t windowSize = 5);
    float update(float newValue);
    void reset();
    uint8_t getSampleCount() const;

private:
    float _samples[FILTER_MAX_SAMPLES];
    uint8_t _windowSize;
    uint8_t _nextIndex;
    uint8_t _sampleCount;
    float _sum;
};

class FilterMED
{
public:
    explicit FilterMED(uint8_t windowSize = 5);
    float update(float newValue);
    void reset();
    uint8_t getSampleCount() const;

private:
    float _samples[FILTER_MAX_SAMPLES];
    uint8_t _windowSize;
    uint8_t _nextIndex;
    uint8_t _sampleCount;
};

// Cumulative arithmetic mean over all samples since construction or reset.
class FilterMEAN
{
public:
    FilterMEAN();
    float update(float newValue);
    void reset();
    uint32_t getSampleCount() const;

private:
    float _mean;
    uint32_t _sampleCount;
};
