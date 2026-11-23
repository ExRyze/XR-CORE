#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "Config.h"

class LoRaManager
{
public:
    bool begin();
    void loop(float telemetryValue);
    bool sendPacket(const String &payload);
    String lastReceivedPacket() const { return _lastReceivedPacket; }
    bool saveSettings(bool enabled, float frequencyMHz, uint32_t bandwidthKHz,
                      uint8_t spreadingFactor, uint8_t codingRate, int8_t txPower,
                      uint8_t syncWord, uint16_t preambleLength, bool crcEnabled,
                      const String &nodeId);
    bool clearSettings();
    String settingsJson() const;
    bool isReady() const { return _ready; }

private:
    void loadSettings();
    bool startListening();

    Preferences _preferences;
    bool _enabled = LORA_ENABLED;
    bool _ready = false;
    float _frequencyMHz = LORA_FREQUENCY_MHZ;
    uint32_t _bandwidthKHz = LORA_BANDWIDTH_KHZ;
    uint8_t _spreadingFactor = LORA_SPREADING_FACTOR;
    uint8_t _codingRate = LORA_CODING_RATE;
    int8_t _txPower = LORA_TX_POWER;
    uint8_t _syncWord = LORA_SYNC_WORD;
    uint16_t _preambleLength = LORA_PREAMBLE_LENGTH;
    bool _crcEnabled = LORA_CRC_ENABLED;
    String _nodeId = LORA_NODE_ID;
    String _lastReceivedPacket;
    uint32_t _lastTransmitMs = 0;
    uint32_t _transmitSequence = 0;
    bool _listening = false;
};
