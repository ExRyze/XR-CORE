#include "DummyTelemetry.h"
#include <esp_system.h>

void DummyTelemetry::update()
{
    const uint32_t now = millis();
    if (now - _lastUpdateMs < 1000UL)
        return;

    _lastUpdateMs = now;
    const float nextValue = 20.0f + static_cast<float>(esp_random() % 1001) / 100.0f;
    portENTER_CRITICAL(&_mux);
    _value = nextValue;
    _updatedAt = now;
    portEXIT_CRITICAL(&_mux);
}

float DummyTelemetry::value() const
{
    portENTER_CRITICAL(&_mux);
    const float result = _value;
    portEXIT_CRITICAL(&_mux);
    return result;
}

uint32_t DummyTelemetry::updatedAt() const
{
    portENTER_CRITICAL(&_mux);
    const uint32_t result = _updatedAt;
    portEXIT_CRITICAL(&_mux);
    return result;
}
