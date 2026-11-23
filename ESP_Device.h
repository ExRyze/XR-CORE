#pragma once

#include <Arduino.h>

class ESPDevice
{
public:
    // Memory & heap
    size_t getHeapSize() const;
    size_t getFreeHeap() const;
    size_t getMinFreeHeap() const;
    size_t getMaxAllocHeap() const;

    // Chip & system info
    String getChipModel() const;
    uint8_t getChipRevision() const;
    uint8_t getChipCores() const;
    uint32_t getCpuFreqMHz() const;
    uint32_t getCycleCount() const;
    String getSdkVersion() const;
    String getCoreVersion() const;
    uint64_t getEfuseMac() const;

    // Flash info
    size_t getFlashChipSize() const;
    uint32_t getFlashChipSpeed() const;
    FlashMode_t getFlashChipMode() const;
    uint32_t getFlashFrequencyMHz() const;

    // Execution / power
    void restart();
    // void deepSleep(uint64_t timeInUs);

    // Sketch info
    size_t getSketchSize() const;
    String getSketchMD5() const;
    size_t getFreeSketchSpace() const;

    // Utility
    void printSystemInfo() const;
};
