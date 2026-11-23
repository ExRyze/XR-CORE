// Author : ExRyze
// Device : OLED 0.96 inch
// Version : 1.1.0

#include "OLED_Display.h"
#include <LittleFS.h>
#include <memory>

//  Constructor
OLEDDisplay::OLEDDisplay()
    : _display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET),
      _scrollOffset(OLED_WIDTH), // start scroll from right edge
      _lastDisplayMs(0)
{
}

//  begin()
bool OLEDDisplay::begin()
{
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(OLED_I2C_CLOCK);

    if (!_display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR))
    {
        return false;
    }

    _display.clearDisplay();
    _display.setTextColor(SSD1306_WHITE);
    _display.display();
    return true;
}

//  Basic helpers
void OLEDDisplay::clear()
{
    _display.clearDisplay();
}

void OLEDDisplay::update()
{
    _display.display();
}

bool OLEDDisplay::interval()
{
    unsigned long now = millis();

    // Interval display
    if (now - _lastDisplayMs < DISPLAY_INTERVAL_MS)
    {
        return false;
    }

    _lastDisplayMs = now;
    return true;
}

// Text helpers
void OLEDDisplay::setCursor(uint8_t x, uint8_t y)
{
    _display.setCursor(x, y);
}

void OLEDDisplay::setTextSize(uint8_t size)
{
    _display.setTextSize(size);
}

void OLEDDisplay::println(const String &text)
{
    _display.println(text);
}

void OLEDDisplay::print(const String &text)
{
    _display.print(text);
}

// Pre-built layouts

//  showSplash()
//  Centered title (size 2) + subtitle (size 1)
void OLEDDisplay::showSplash(const String &title, const String &subtitle)
{
    _display.clearDisplay();

    // Title (large, centered)
    _display.setTextSize(2);
    int16_t x1, y1;
    uint16_t tw, th;
    _display.getTextBounds(title.c_str(), 0, 0, &x1, &y1, &tw, &th);
    int16_t titleX = max(0, (OLED_WIDTH - (int16_t)tw) / 2);
    int16_t titleY = 20;
    _display.setCursor(titleX, titleY);
    _display.print(title);

    // Subtitle (small, centered)
    _display.setTextSize(1);
    _display.getTextBounds(subtitle.c_str(), 0, 0, &x1, &y1, &tw, &th);
    int16_t subX = max(0, (OLED_WIDTH - (int16_t)tw) / 2);
    int16_t subY = titleY + th + 14; // 14 px gap below title
    _display.setCursor(subX, subY);
    _display.print(subtitle);

    // Decorative divider under title
    _display.drawFastHLine(10, titleY + 20, OLED_WIDTH - 20, SSD1306_WHITE);

    _display.display();
}

//  showStatusBar()
// Show the connection status and time above a divider line.
void OLEDDisplay::showStatusBar(const bool &status, const String &timeStr)
{
    int16_t x1, y1;
    uint16_t tw, th;

    // Clear the status bar area (height is 10 pixels: 0 to 8 inclusive)
    _display.fillRect(0, 0, OLED_WIDTH, 10, SSD1306_BLACK);

    // Status - Left-aligned
    String statusStr = status ? "Station" : "Access Point";
    _display.setCursor(0, 0);
    _display.print(statusStr);

    // Time - Right-aligned
    _display.getTextBounds(timeStr.c_str(), 0, 0, &x1, &y1, &tw, &th);
    int16_t timeX = OLED_WIDTH - tw;
    _display.setCursor(timeX, 0);
    _display.print(timeStr);

    // Bottom dividing line
    _display.drawFastHLine(0, 8, OLED_WIDTH, SSD1306_WHITE);
}

// showKeyValue()
// Label on left, value on right with a custom max pixel width parameter.
void OLEDDisplay::showKeyValue(uint8_t row, const String &label, const String &value, int16_t maxPx, int16_t offset)
{
    const uint8_t ROW_H = 10;
    uint8_t y = row * ROW_H + 10; // Start below the status bar (y = 10)

    int16_t x1, y1;
    uint16_t tw, th;

    _display.setTextSize(1);

    // The area for this row is [offset .. offset + maxPx)
    int16_t areaStartX = offset;
    int16_t areaEndX = areaStartX + maxPx;

    // Clear only the exact rendered area for this row.
    _display.fillRect(areaStartX, y, maxPx, ROW_H, SSD1306_BLACK);

    if (label.length() == 0)
    {
        // Value-only row: keep it aligned to the left edge of this area.
        _display.setCursor(areaStartX, y);
        _display.print(value);
        return;
    }

    _display.setCursor(areaStartX, y);
    _display.print(label + ":");

    _display.getTextBounds(value.c_str(), 0, 0, &x1, &y1, &tw, &th);

    int16_t valueX = areaEndX - tw;
    if (valueX < areaStartX + 10)
        valueX = areaStartX + 10;

    _display.setCursor(valueX, y);
    _display.print(value);
}

// showImage()
// Renders a custom bitmap image at the specified x, y position with given width and height.
void OLEDDisplay::showImage(const unsigned char *bitmap, int16_t x, int16_t y, int16_t w, int16_t h)
{
    // Clear the exact area where the image will be rendered to prevent smearing
    _display.fillRect(x, y, w, h, SSD1306_BLACK);

    // Draw the bitmap image
    _display.drawBitmap(x, y, bitmap, w, h, SSD1306_WHITE);
}

// showImageFromFile()
// Displays an image from a file stored in LittleFS.
bool OLEDDisplay::showImageFromFile(const char *path, int16_t x, int16_t y, int16_t w, int16_t h)
{
    if (path == nullptr || !LittleFS.exists(path))
    {
        return false;
    }

    File file = LittleFS.open(path, "r");
    if (!file)
    {
        return false;
    }

    size_t fileSize = file.size();
    if (fileSize == 0)
    {
        file.close();
        return false;
    }

    size_t bytesNeeded = ((size_t)w + 7U) / 8U * (size_t)h;
    if (bytesNeeded == 0 || fileSize < bytesNeeded)
    {
        file.close();
        return false;
    }

    std::unique_ptr<uint8_t[]> buffer(new uint8_t[bytesNeeded]);
    if (!buffer)
    {
        file.close();
        return false;
    }

    size_t bytesRead = file.read(buffer.get(), bytesNeeded);
    file.close();

    if (bytesRead != bytesNeeded)
    {
        return false;
    }

    _display.fillRect(x, y, w, h, SSD1306_BLACK);
    _display.drawBitmap(x, y, buffer.get(), w, h, SSD1306_WHITE);
    return true;
}

//  driver() — raw access
Adafruit_SSD1306 &OLEDDisplay::driver()
{
    return _display;
}
