#include "LoRa_Manager.h"

#include <SPI.h>
#include <RadioLib.h>
#include <ArduinoJson.h>
#include "Config.h"

namespace
{
    volatile bool loraPacketReceived = false;

    void onLoRaPacketReceived()
    {
        loraPacketReceived = true;
    }

    LORA_RADIO_TYPE loraRadio = new Module(
        LORA_CS_PIN,
        LORA_IRQ_PIN,
        LORA_RESET_PIN,
        LORA_AUX_PIN,
        SPI,
        SPISettings(2000000, MSBFIRST, SPI_MODE0));
}

void LoRaManager::loadSettings()
{
    _enabled = LORA_ENABLED;
    _frequencyMHz = LORA_FREQUENCY_MHZ;
    _bandwidthKHz = LORA_BANDWIDTH_KHZ;
    _spreadingFactor = LORA_SPREADING_FACTOR;
    _codingRate = LORA_CODING_RATE;
    _txPower = LORA_TX_POWER;
    _syncWord = LORA_SYNC_WORD;
    _preambleLength = LORA_PREAMBLE_LENGTH;
    _crcEnabled = LORA_CRC_ENABLED;
    _nodeId = LORA_NODE_ID;

    if (!_preferences.begin("lora-config", false))
        return;

    // Save defaults to Preferences on first boot or when a key is missing.
    // Existing saved values are left unchanged.
    if (!_preferences.isKey("enabled"))
        _preferences.putBool("enabled", LORA_ENABLED);
    if (!_preferences.isKey("frequency"))
        _preferences.putFloat("frequency", LORA_FREQUENCY_MHZ);
    if (!_preferences.isKey("bandwidth"))
        _preferences.putUInt("bandwidth", LORA_BANDWIDTH_KHZ);
    if (!_preferences.isKey("sf"))
        _preferences.putUChar("sf", LORA_SPREADING_FACTOR);
    if (!_preferences.isKey("codingRate"))
        _preferences.putUChar("codingRate", LORA_CODING_RATE);
    if (!_preferences.isKey("txPower"))
        _preferences.putChar("txPower", LORA_TX_POWER);
    if (!_preferences.isKey("syncWord"))
        _preferences.putUChar("syncWord", LORA_SYNC_WORD);
    if (!_preferences.isKey("preamble"))
        _preferences.putUShort("preamble", LORA_PREAMBLE_LENGTH);
    if (!_preferences.isKey("crc"))
        _preferences.putBool("crc", LORA_CRC_ENABLED);
    if (!_preferences.isKey("nodeId"))
        _preferences.putString("nodeId", LORA_NODE_ID);

    _enabled = _preferences.getBool("enabled", LORA_ENABLED);
    _frequencyMHz = _preferences.getFloat("frequency", LORA_FREQUENCY_MHZ);
    _bandwidthKHz = _preferences.getUInt("bandwidth", LORA_BANDWIDTH_KHZ);
    _spreadingFactor = _preferences.getUChar("sf", LORA_SPREADING_FACTOR);
    _codingRate = _preferences.getUChar("codingRate", LORA_CODING_RATE);
    _txPower = _preferences.getChar("txPower", LORA_TX_POWER);
    _syncWord = _preferences.getUChar("syncWord", LORA_SYNC_WORD);
    _preambleLength = _preferences.getUShort("preamble", LORA_PREAMBLE_LENGTH);
    _crcEnabled = _preferences.getBool("crc", LORA_CRC_ENABLED);
    _nodeId = _preferences.getString("nodeId", LORA_NODE_ID);
    _preferences.end();
}

bool LoRaManager::begin()
{
    loadSettings();
    if (!_enabled)
        return true;

    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, LORA_CS_PIN);

#if LORA_RADIO_FAMILY == LORA_FAMILY_SX126X
    pinMode(LORA_RF_SWITCH_PIN, OUTPUT);
    digitalWrite(LORA_RF_SWITCH_PIN, HIGH);

    const int16_t tcxoState = loraRadio.setTCXO(LORA_TCXO_VOLTAGE);
    if (tcxoState != RADIOLIB_ERR_NONE)
    {
        Serial.printf("[ERROR] Failed to configure LoRa TCXO (RadioLib code %d).\n", tcxoState);
        return false;
    }
#endif

    ConfigLoRa_t config = {};
    config.frequency = static_cast<uint32_t>(_frequencyMHz * 1000000.0f);
    config.bandwidth = _bandwidthKHz * 1000U;
    config.spreadingFactor = _spreadingFactor;
    config.codingRate = _codingRate;
    config.syncWord = _syncWord;
    config.power = _txPower;
    config.preambleLength = _preambleLength;

    Serial.printf(
        "[DEBUG] LoRa begin config: frequency=%.3f MHz, bandwidth=%.3f kHz, SF=%u, CR=%u, power=%d dBm, preamble=%u\n",
        static_cast<double>(config.frequency) / 1000000.0,
        static_cast<double>(config.bandwidth) / 1000.0,
        static_cast<unsigned>(config.spreadingFactor),
        static_cast<unsigned>(config.codingRate),
        static_cast<int>(config.power),
        static_cast<unsigned>(config.preambleLength));

    const int16_t radioState = loraRadio.begin(config);
    if (radioState != RADIOLIB_ERR_NONE)
    {
        if (radioState == RADIOLIB_ERR_INVALID_BANDWIDTH)
            Serial.printf("[ERROR] LoRa initialization rejected bandwidth %.3f kHz (RadioLib code %d).\n",
                          static_cast<double>(config.bandwidth) / 1000.0, radioState);
        else
            Serial.printf("[ERROR] LoRa radio initialization failed (RadioLib code %d). Check the radio configuration and module connection.\n", radioState);
        return false;
    }

#if LORA_RADIO_FAMILY == LORA_FAMILY_SX126X
    const int16_t rfSwitchState = loraRadio.setDio2AsRfSwitch(true);
    if (rfSwitchState != RADIOLIB_ERR_NONE)
    {
        Serial.printf("[ERROR] Failed to configure SX126x DIO2 RF switch (RadioLib code %d).\n", rfSwitchState);
        return false;
    }
    const int16_t crcState = loraRadio.setCRC(_crcEnabled ? 2 : 0);
#else
    const int16_t crcState = loraRadio.setCRC(_crcEnabled);
#endif
    _ready = crcState == RADIOLIB_ERR_NONE;
    if (_ready)
    {
        Serial.printf("[INFO] LoRa radio ready at %.3f MHz for node %s.\n", _frequencyMHz, _nodeId.c_str());
        _listening = startListening();
    }
    else
        Serial.printf("[ERROR] Failed to apply LoRa CRC setting (RadioLib code %d).\n", crcState);
    return _ready;
}

bool LoRaManager::startListening()
{
    loraPacketReceived = false;
#if LORA_RADIO_FAMILY == LORA_FAMILY_SX126X
    // Wio-SX1262 RF_SW is active-high for receive and low for transmit.
    digitalWrite(LORA_RF_SWITCH_PIN, HIGH);
    loraRadio.setDio1Action(onLoRaPacketReceived);
#else
    loraRadio.setDio0Action(onLoRaPacketReceived);
#endif
    const int16_t state = loraRadio.startReceive();
    if (state != RADIOLIB_ERR_NONE)
    {
        Serial.printf("[ERROR] Failed to start LoRa receiver (RadioLib code %d).\n", state);
        return false;
    }
    return true;
}

bool LoRaManager::sendPacket(const String &payload)
{
    if (!_ready || payload.length() == 0)
        return false;

    _listening = false;
    String transmitPayload = payload;
#if LORA_RADIO_FAMILY == LORA_FAMILY_SX126X
    // The module's external RF switch must be in transmit mode while the radio sends.
    digitalWrite(LORA_RF_SWITCH_PIN, LOW);
#endif
    const int16_t state = loraRadio.transmit(transmitPayload);
    if (state != RADIOLIB_ERR_NONE)
    {
        Serial.printf("[ERROR] LoRa transmit failed (RadioLib code %d).\n", state);
        _listening = startListening();
        return false;
    }

    Serial.printf("[INFO] LoRa transmitted %u bytes.\n", static_cast<unsigned>(payload.length()));
    _listening = startListening();
    return true;
}

void LoRaManager::loop(float telemetryValue)
{
    if (!_ready)
        return;

    if (loraPacketReceived)
    {
        loraPacketReceived = false;
        String packet;
        const int16_t state = loraRadio.readData(packet);
        if (state == RADIOLIB_ERR_NONE)
        {
            _lastReceivedPacket = packet;
            Serial.printf("[INFO] LoRa received: %s\n", packet.c_str());
        }
        else
        {
            Serial.printf("[WARN] LoRa receive failed (RadioLib code %d).\n", state);
        }
        _listening = startListening();
    }

    const uint32_t now = millis();
    if (now - _lastTransmitMs >= 10000UL)
    {
        _lastTransmitMs = now;
        String payload = "{\"nodeId\":\"" + _nodeId + "\",\"sequence\":" + String(++_transmitSequence) +
                         ",\"value\":" + String(telemetryValue, 2) + "}";
        sendPacket(payload);
    }
    else if (!_listening)
    {
        _listening = startListening();
    }
}

bool LoRaManager::saveSettings(bool enabled, float frequencyMHz, uint32_t bandwidthKHz,
                               uint8_t spreadingFactor, uint8_t codingRate, int8_t txPower,
                               uint8_t syncWord, uint16_t preambleLength, bool crcEnabled,
                               const String &nodeId)
{
    if (frequencyMHz < 137.0f || frequencyMHz > 1020.0f ||
        (bandwidthKHz != 125 && bandwidthKHz != 250 && bandwidthKHz != 500) ||
        spreadingFactor < 7 || spreadingFactor > 12 || codingRate < 5 || codingRate > 8 ||
        txPower < 2 || txPower > 20 || preambleLength < 6 ||
        nodeId.length() == 0 || nodeId.length() > 32)
        return false;

    if (!_preferences.begin("lora-config", false))
        return false;
    _preferences.putBool("enabled", enabled);
    _preferences.putFloat("frequency", frequencyMHz);
    _preferences.putUInt("bandwidth", bandwidthKHz);
    _preferences.putUChar("sf", spreadingFactor);
    _preferences.putUChar("codingRate", codingRate);
    _preferences.putChar("txPower", txPower);
    _preferences.putUChar("syncWord", syncWord);
    _preferences.putUShort("preamble", preambleLength);
    _preferences.putBool("crc", crcEnabled);
    _preferences.putString("nodeId", nodeId);
    _preferences.end();

    _enabled = enabled;
    _frequencyMHz = frequencyMHz;
    _bandwidthKHz = bandwidthKHz;
    _spreadingFactor = spreadingFactor;
    _codingRate = codingRate;
    _txPower = txPower;
    _syncWord = syncWord;
    _preambleLength = preambleLength;
    _crcEnabled = crcEnabled;
    _nodeId = nodeId;
    return true;
}

bool LoRaManager::clearSettings()
{
    if (!_preferences.begin("lora-config", false))
        return false;

    _preferences.clear();
    _preferences.end();

    return saveSettings(
        LORA_ENABLED,
        LORA_FREQUENCY_MHZ,
        LORA_BANDWIDTH_KHZ,
        LORA_SPREADING_FACTOR,
        LORA_CODING_RATE,
        LORA_TX_POWER,
        LORA_SYNC_WORD,
        LORA_PREAMBLE_LENGTH,
        LORA_CRC_ENABLED,
        LORA_NODE_ID);
}

String LoRaManager::settingsJson() const
{
    Preferences preferences;
    StaticJsonDocument<320> document;
    if (preferences.begin("lora-config", true))
    {
        document["enabled"] = preferences.getBool("enabled", LORA_ENABLED);
        document["frequency"] = preferences.getFloat("frequency", LORA_FREQUENCY_MHZ);
        document["bandwidth"] = preferences.getUInt("bandwidth", LORA_BANDWIDTH_KHZ);
        document["spreadingFactor"] = preferences.getUChar("sf", LORA_SPREADING_FACTOR);
        document["codingRate"] = preferences.getUChar("codingRate", LORA_CODING_RATE);
        document["txPower"] = preferences.getChar("txPower", LORA_TX_POWER);
        document["syncWord"] = preferences.getUChar("syncWord", LORA_SYNC_WORD);
        document["preambleLength"] = preferences.getUShort("preamble", LORA_PREAMBLE_LENGTH);
        document["crcEnabled"] = preferences.getBool("crc", LORA_CRC_ENABLED);
        document["nodeId"] = preferences.getString("nodeId", LORA_NODE_ID);
        preferences.end();
    }
    else
    {
        document["enabled"] = LORA_ENABLED;
        document["frequency"] = LORA_FREQUENCY_MHZ;
        document["bandwidth"] = LORA_BANDWIDTH_KHZ;
        document["spreadingFactor"] = LORA_SPREADING_FACTOR;
        document["codingRate"] = LORA_CODING_RATE;
        document["txPower"] = LORA_TX_POWER;
        document["syncWord"] = LORA_SYNC_WORD;
        document["preambleLength"] = LORA_PREAMBLE_LENGTH;
        document["crcEnabled"] = LORA_CRC_ENABLED;
        document["nodeId"] = LORA_NODE_ID;
    }
    document["initialized"] = _ready;
    String json;
    serializeJson(document, json);
    return json;
}
