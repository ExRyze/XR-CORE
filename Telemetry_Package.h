#pragma once

#include <Arduino.h>

// Validated, transport-neutral representation of one sensor measurement.
struct TelemetryPackage
{
    String source;
    String measurement;
    String unit;
    String filter;
    bool initialized = false;
    bool online = false;
    bool valid = false;
    uint32_t timestampMs = 0;
    float rawValue = 0.0f;
    float filteredValue = 0.0f;
};

constexpr uint16_t TELEMETRY_HISTORY_CAPACITY = 180;

struct TelemetryHistoryPoint
{
    uint32_t timestampMs = 0;
    float rawValue = 0.0f;
    float filteredValue = 0.0f;
};

struct TelemetryHistory
{
    TelemetryHistoryPoint points[TELEMETRY_HISTORY_CAPACITY];
    uint16_t count = 0;
};
