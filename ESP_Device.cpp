#include "ESP_Device.h"

size_t ESPDevice::getHeapSize() const
{
    return ESP.getHeapSize();
}

size_t ESPDevice::getFreeHeap() const
{
    return ESP.getFreeHeap();
}

size_t ESPDevice::getMinFreeHeap() const
{
    return ESP.getMinFreeHeap();
}

size_t ESPDevice::getMaxAllocHeap() const
{
    return ESP.getMaxAllocHeap();
}

String ESPDevice::getChipModel() const
{
    return ESP.getChipModel();
}

uint8_t ESPDevice::getChipRevision() const
{
    return ESP.getChipRevision();
}

uint8_t ESPDevice::getChipCores() const
{
    return ESP.getChipCores();
}

uint32_t ESPDevice::getCpuFreqMHz() const
{
    return ESP.getCpuFreqMHz();
}

uint32_t ESPDevice::getCycleCount() const
{
    return ESP.getCycleCount();
}

String ESPDevice::getSdkVersion() const
{
    return ESP.getSdkVersion();
}

String ESPDevice::getCoreVersion() const
{
    return ESP.getCoreVersion();
}

uint64_t ESPDevice::getEfuseMac() const
{
    return ESP.getEfuseMac();
}

size_t ESPDevice::getFlashChipSize() const
{
    return ESP.getFlashChipSize();
}

uint32_t ESPDevice::getFlashChipSpeed() const
{
    return ESP.getFlashChipSpeed();
}

FlashMode_t ESPDevice::getFlashChipMode() const
{
    return ESP.getFlashChipMode();
}

uint32_t ESPDevice::getFlashFrequencyMHz() const
{
    return ESP.getFlashFrequencyMHz();
}

void ESPDevice::restart()
{
    ESP.restart();
}

// void ESPDevice::deepSleep(uint64_t timeInUs)
// {
//     ESP.deepSleep(timeInUs);
// }

size_t ESPDevice::getSketchSize() const
{
    return ESP.getSketchSize();
}

String ESPDevice::getSketchMD5() const
{
    return ESP.getSketchMD5();
}

size_t ESPDevice::getFreeSketchSpace() const
{
    return ESP.getFreeSketchSpace();
}

void ESPDevice::printSystemInfo() const
{
    Serial.println("[ESP System]");
    Serial.printf("Chip Model      : %s\n", getChipModel().c_str());
    Serial.printf("Chip Revision   : %u\n", getChipRevision());
    Serial.printf("Cores           : %u\n", getChipCores());
    Serial.printf("CPU Freq        : %u MHz\n", getCpuFreqMHz());
    Serial.printf("SDK Version     : %s\n", getSdkVersion().c_str());
    Serial.printf("Core Version    : %s\n", getCoreVersion().c_str());
    Serial.printf("Heap Size       : %u bytes\n", (unsigned int)getHeapSize());
    Serial.printf("Free Heap       : %u bytes\n", (unsigned int)getFreeHeap());
    Serial.printf("Min Free Heap   : %u bytes\n", (unsigned int)getMinFreeHeap());
    Serial.printf("Flash Size      : %u bytes\n", (unsigned int)getFlashChipSize());
    Serial.printf("Flash Speed     : %u Hz\n", getFlashChipSpeed());
    Serial.printf("Sketch Size     : %u bytes\n", (unsigned int)getSketchSize());
    Serial.printf("Sketch MD5      : %s\n", getSketchMD5().c_str());
    Serial.printf("Free Sketch     : %u bytes\n", (unsigned int)getFreeSketchSpace());
}
