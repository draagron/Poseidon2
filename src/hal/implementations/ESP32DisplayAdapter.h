/**
 * @file ESP32DisplayAdapter.h
 * @brief ESP32 hardware adapter for SSD1306 OLED display
 *
 * Implements IDisplayAdapter using Adafruit_SSD1306 library for
 * 128x64 monochrome OLED on I2C Bus 2 (SDA=GPIO21, SCL=GPIO22).
 *
 * Hardware: SH-ESP32 board with SSD1306 OLED at I2C address 0x3C
 * Library: Adafruit_SSD1306, Adafruit_GFX, Wire (I2C)
 *
 * Constitutional Compliance:
 * - Principle I (Hardware Abstraction): Implements IDisplayAdapter interface
 * - Principle II (Resource Management): 1KB framebuffer (justified for display)
 * - Principle VII (Fail-Safe): init() returns false on I2C error
 *
 * @version 1.0.0
 * @date 2025-10-09
 */

#ifndef ESP32_DISPLAY_ADAPTER_H
#define ESP32_DISPLAY_ADAPTER_H

#include "hal/interfaces/IDisplayAdapter.h"

#ifdef ARDUINO
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#endif

/**
 * @brief ESP32 hardware implementation of IDisplayAdapter
 *
 * Uses Adafruit_SSD1306 library to control 128x64 OLED display
 * via I2C Bus 2 on ESP32.
 */
class ESP32DisplayAdapter : public IDisplayAdapter {
private:
#ifdef ARDUINO
    Adafruit_SSD1306* _display;  ///< Adafruit SSD1306 driver instance
#endif
    bool _isReady;                ///< True if init() succeeded

public:
    /**
     * @brief Constructor - initializes display object
     *
     * Creates Adafruit_SSD1306 instance for 128x64 display.
     */
    ESP32DisplayAdapter();

    /**
     * @brief Destructor - clean up display object
     */
    ~ESP32DisplayAdapter();

    // IDisplayAdapter interface implementation
    bool init() override;
    void clear() override;
    void setCursor(uint8_t x, uint8_t y) override;
    void setTextSize(uint8_t size) override;
    void print(const char* text) override;
    void display() override;
    bool isReady() const override;
};

#endif // ESP32_DISPLAY_ADAPTER_H
