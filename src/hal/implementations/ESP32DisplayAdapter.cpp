/**
 * @file ESP32DisplayAdapter.cpp
 * @brief Implementation of ESP32DisplayAdapter for SSD1306 OLED
 *
 * @version 1.0.0
 * @date 2025-10-09
 */

#include "ESP32DisplayAdapter.h"

#ifdef ARDUINO
#include <Arduino.h>

// Display configuration constants
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1         // No reset pin on SH-ESP32
#define SCREEN_ADDRESS 0x3C   // I2C address for 128x64 OLED
#define OLED_SDA 21           // GPIO21 for I2C Bus 2
#define OLED_SCL 22           // GPIO22 for I2C Bus 2
#define I2C_CLOCK_SPEED 400000  // 400kHz fast mode

ESP32DisplayAdapter::ESP32DisplayAdapter()
    : _display(nullptr), _isReady(false) {
    // Create Adafruit_SSD1306 instance
    _display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
}

ESP32DisplayAdapter::~ESP32DisplayAdapter() {
    // Clean up display object
    if (_display != nullptr) {
        delete _display;
        _display = nullptr;
    }
}

bool ESP32DisplayAdapter::init() {
    if (_display == nullptr) {
        _isReady = false;
        return false;
    }

    // Initialize I2C Bus 2 (SDA=GPIO21, SCL=GPIO22)
    Wire.begin(OLED_SDA, OLED_SCL);
    Wire.setClock(I2C_CLOCK_SPEED);

    // Initialize SSD1306 display
    // begin() returns true on success, false on I2C error
    bool success = _display->begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);

    if (success) {
        // Configure display defaults
        _display->clearDisplay();
        _display->setTextSize(1);      // Font size 1 = 5x7 pixels
        _display->setTextColor(SSD1306_WHITE);  // White text on black background
        _display->setCursor(0, 0);
        _display->display();  // Push clear buffer to hardware
        _isReady = true;
    } else {
        _isReady = false;
    }

    return success;
}

void ESP32DisplayAdapter::clear() {
    if (_display != nullptr && _isReady) {
        _display->clearDisplay();
    }
}

void ESP32DisplayAdapter::setCursor(uint8_t x, uint8_t y) {
    if (_display != nullptr && _isReady) {
        _display->setCursor(x, y);
    }
}

void ESP32DisplayAdapter::setTextSize(uint8_t size) {
    if (_display != nullptr && _isReady) {
        _display->setTextSize(size);
    }
}

void ESP32DisplayAdapter::print(const char* text) {
    if (_display != nullptr && _isReady && text != nullptr) {
        _display->print(text);
    }
}

void ESP32DisplayAdapter::display() {
    if (_display != nullptr && _isReady) {
        _display->display();  // Push framebuffer to OLED via I2C
    }
}

bool ESP32DisplayAdapter::isReady() const {
    return _isReady;
}

#else
// Stub implementation for native platform (tests use MockDisplayAdapter)
ESP32DisplayAdapter::ESP32DisplayAdapter() : _isReady(false) {}
ESP32DisplayAdapter::~ESP32DisplayAdapter() {}
bool ESP32DisplayAdapter::init() { return false; }
void ESP32DisplayAdapter::clear() {}
void ESP32DisplayAdapter::setCursor(uint8_t x, uint8_t y) { (void)x; (void)y; }
void ESP32DisplayAdapter::setTextSize(uint8_t size) { (void)size; }
void ESP32DisplayAdapter::print(const char* text) { (void)text; }
void ESP32DisplayAdapter::display() {}
bool ESP32DisplayAdapter::isReady() const { return false; }
#endif
