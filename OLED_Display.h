#pragma once

#include <Wire.h>
#include <Adafruit_GFX.h>     // library: adafruit/Adafruit-GFX-Library
#include <Adafruit_SSD1306.h> // library: adafruit/Adafruit_SSD1306
#include "Config.h"

//  OLEDDisplay Class
class OLEDDisplay
{
public:
    // Constructor: creates the Adafruit_SSD1306 internally
    OLEDDisplay();

    // Must be called in setup(). Returns false if OLED not found.
    bool begin();

    // Basic helpers
    void clear();
    void update();
    bool interval();

    // Text helpers
    void setCursor(uint8_t x, uint8_t y);
    void setTextSize(uint8_t size);
    void println(const String &text);
    void print(const String &text);

    // Pre-built layouts

    // Shows a centered title + subtitle on a splash screen (loading screen)
    void showSplash(const String &title, const String &subtitle);

    // Shows a status bar on top screen (for WiFi status)
    void showStatusBar(const bool &status, const String &timeStr);

    // Shows a simple key–value row (e.g. "Key: ... value")
    // row: 0-based row index (row height = 10 px)
    // maxPx: right boundary of the row area, e.g. 64 or 128
    // offset: starting X position of the row area
    void showKeyValue(uint8_t row, const String &label, const String &value, int16_t maxPx = 128, int16_t offset = 0);

    // renders a custom bitmap image
    void showImage(const unsigned char *bitmap, int16_t x, int16_t y, int16_t w, int16_t h);

    // renders a custom bitmap image from a binary file on LittleFS
    bool showImageFromFile(const char *path, int16_t x, int16_t y, int16_t w, int16_t h);

    // Raw driver access
    // Escape hatch: returns reference to underlying Adafruit_SSD1306
    Adafruit_SSD1306 &driver();

private:
    Adafruit_SSD1306 _display;
    int16_t _scrollOffset;
    unsigned long _lastDisplayMs;
    static const unsigned long DISPLAY_INTERVAL_MS = 1000UL; // 1 second
};
