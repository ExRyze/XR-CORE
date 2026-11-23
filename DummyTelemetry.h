#pragma once

#include <Arduino.h>

class DummyTelemetry
{
public:
    void update();
    float value() const;
    uint32_t updatedAt() const;

private:
    mutable portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;
    float _value = 0.0f;
    uint32_t _updatedAt = 0;
    uint32_t _lastUpdateMs = 0;
};
