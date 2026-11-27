// Copyright 2026 ExRyze
// SPDX-License-Identifier: Apache-2.0

#include <Arduino.h>
#include "Config.h"
#include "ESP_Device.h"
#include "WiFi_Manager.h"
#include "OLED_Display.h"
#include "Web_Config.h"
#include "MQTT_Manager.h"
#include "LoRa_Manager.h"
#include "Telemetry_Manager.h"
#include "Sensor_Manager.h"
#include "BME680_Sensor.h"
#include "ArduinoCloud_Manager.h"

ESPDevice espDevice;
WiFiManager wifi;
OLEDDisplay oled;
MQTTManager mqtt(MQTT_SERVER, MQTT_PORT, MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD);
LoRaManager lora;
SensorManager sensorManager;
TelemetryManager telemetry(sensorManager);
BME680Sensor bme680Sensor;
ArduinoCloudManager cloud;
WebConfig server(wifi, mqtt, lora, telemetry);

TaskHandle_t sensorTaskHandle;
TaskHandle_t networkTaskHandle;
TaskHandle_t mqttTaskHandle;

void initLedStatus()
{
#if defined(POWER_LED)
    pinMode(POWER_LED, OUTPUT);
    digitalWrite(POWER_LED, HIGH);
#endif

#if defined(DATA_LED)
    pinMode(DATA_LED, OUTPUT);
    digitalWrite(DATA_LED, HIGH);
#endif
}

void updateDataLed()
{
#if defined(DATA_LED)
    static unsigned long lastToggle = 0;
    static const uint32_t interval = 500;
    static bool ledState = false;

    if (wifi.isConnected())
    {
        if (millis() - lastToggle >= interval)
        {
            digitalWrite(DATA_LED, ledState ? HIGH : LOW);
            lastToggle = millis();
        }
    }
    else
    {
        digitalWrite(DATA_LED, LOW);
    }
#endif
}

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("[INFO] Booting...");
    espDevice.printSystemInfo();

    initLedStatus();

    // Initialize the OLED over I2C.
    if (!oled.begin())
    {
        Serial.println(F("[ERROR] OLED Display not found!"));
        while (true)
        {
            delay(1000);
        }
    }

    sensorManager.addSensor(bme680Sensor);
    sensorManager.begin();

    oled.showSplash(deviceBrand, String(deviceName) + " " + deviceVersion);

    wifi.begin();

    server.begin();

    Serial.printf("[INFO] Copyright by %s.\n", deviceBrand);
    Serial.printf("[INFO] %s %s started.\n", deviceName, deviceVersion);

    // Initialize FreeRTOS tasks.
    // Core 1: sensor and application logic.
    xTaskCreatePinnedToCore(
        sensorTask,        // Task function
        "SensorTask",      // Task name
        4096,              // Stack size in bytes
        NULL,              // Task parameter
        1,                 // Priority (0 = lowest)
        &sensorTaskHandle, // Task handle
        1                  // Core index
    );

    // Core 0: Wi-Fi and network connections (the ESP32 Wi-Fi stack runs here).
    xTaskCreatePinnedToCore(
        networkTask,
        "NetworkTask",
        4096,
        NULL,
        1,
        &networkTaskHandle,
        0);

    // Keep MQTT connection and TLS work off the Wi-Fi management task.
    xTaskCreatePinnedToCore(
        mqttTask,
        "MqttTask",
        8192,
        NULL,
        0,
        &mqttTaskHandle,
        1);
}

void sensorTask(void *parameter)
{
    const uint32_t PAGE_INTERVAL_MS = 20000UL;
    const uint8_t PAGE_COUNT = 3;
    uint8_t page = 0;
    uint32_t lastPageChangeMs = millis();

    for (;;)
    {
        if (sensorManager.update(millis()))
            telemetry.update();
        uint32_t now = millis();

        if (now - lastPageChangeMs >= PAGE_INTERVAL_MS)
        {
            page = (page + 1) % PAGE_COUNT;
            lastPageChangeMs = now;
        }

        oled.clear();
        oled.showStatusBar(wifi.isConnected(), wifi.getTime("%H:%M"));

        switch (page)
        {
        case 0: // Dashboard
        {
            const TelemetryPackage package = telemetry.getTemperaturePackage();

            const char *filterLabel = "AVG";
            if (package.filter == "MEDIAN")
                filterLabel = "MEDIAN";
            else if (package.filter == "CUMULATIVE_MEAN")
                filterLabel = "MEAN";

            oled.showKeyValue(1, "TEMP", package.valid ? String(package.filteredValue, 2) + " C" : "--");
            oled.showKeyValue(2, "RAW", package.valid ? String(package.rawValue, 2) + " C" : "--");
            oled.showKeyValue(3, "SENSOR", package.online ? "ONLINE" : "OFFLINE");
            oled.showKeyValue(4, "FILTER", filterLabel);
            break;
        }

        case 1: // WiFi Config
            if (wifi.getMode() == WiFiMode::ACCESS_POINT)
            {
                oled.showKeyValue(1, "SSID", AP_SSID);
                oled.showKeyValue(2, "IP  ", wifi.getIp());
                oled.showKeyValue(4, "PASS", wifi.getApPassword());
            }
            else
            {
                oled.showKeyValue(1, "SSID", wifi.getSavedSsid());
                oled.showKeyValue(2, "SIG", wifi.isConnected() ? String(wifi.getConnectedRssi()) + " dBm" : "N/A");
                oled.showKeyValue(3, "IP ", wifi.getIp());
                oled.showKeyValue(4, "MAC", WiFi.macAddress());
            }
            break;

        case 2: // Device Info
            oled.showKeyValue(1, "", deviceBrand, 64);
            oled.showKeyValue(2, "", deviceName, 64);
            oled.showKeyValue(3, "", "Version:", 64);
            oled.showKeyValue(4, "", deviceVersion, 64);
            oled.showImageFromFile("/logo.raw", 64, 10, 54, 54);
            break;
        }

        oled.update();

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
}

void networkTask(void *parameter)
{
    lora.begin();
    for (;;)
    {
        wifi.loop();
        if (wifi.isConnected())
        {
            cloud.begin();
            cloud.loop();
        }
        else
        {
            cloud.loop();
        }
        lora.loop(telemetry.value());
        updateDataLed();

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
}

void mqttTask(void *parameter)
{
    bool mqttInitialized = false;
    for (;;)
    {
        if (wifi.isConnected())
        {
            if (!mqttInitialized)
            {
                mqtt.begin();
                mqttInitialized = true;
            }
            else
            {
                mqtt.update(telemetry, lora.isReady(), wifi.getConnectedRssi());
            }
        }

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
}

void loop()
{
    // Empty loop() once all logic has been moved to the task.
    // loop() itself actually runs as a FreeRTOS task as well
    // (priority 1, core 1 on the default Arduino-ESP32 core).
    vTaskDelay(250 / portTICK_PERIOD_MS);
}
