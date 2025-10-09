/**
 * @file IDisplayAdapter.h
 * @brief Hardware Abstraction Layer interface for OLED display
 *
 * This interface abstracts the SSD1306 OLED display hardware, enabling
 * mock-first testing on native platform without physical hardware.
 *
 * Constitutional Principle I: Hardware Abstraction Layer
 * - All display operations abstracted through this interface
 * - Mock implementation (MockDisplayAdapter) enables unit/integration tests
 * - Business logic (DisplayManager, DisplayFormatter) depends only on this interface
 *
 * @version 1.0.0
 * @date 2025-10-08
 */

#ifndef I_DISPLAY_ADAPTER_H
#define I_DISPLAY_ADAPTER_H

#include <stdint.h>

/**
 * @brief Display adapter interface for OLED hardware
 *
 * Abstracts low-level OLED operations (I2C communication, framebuffer management,
 * text rendering) to enable testable display logic.
 *
 * Implementation: ESP32DisplayAdapter (uses Adafruit_SSD1306 library)
 * Mock: MockDisplayAdapter (for unit/integration tests)
 */
class IDisplayAdapter {
public:
    virtual ~IDisplayAdapter() = default;

    /**
     * @brief Initialize display hardware
     *
     * Performs I2C initialization, allocates framebuffer, and configures
     * display parameters (contrast, orientation, etc.).
     *
     * Implementation notes:
     * - ESP32DisplayAdapter: Calls Adafruit_SSD1306::begin(SSD1306_SWITCHCAPVCC, 0x3C)
     * - I2C Bus 2: SDA=GPIO21, SCL=GPIO22 (SH-ESP32 pinout)
     * - Framebuffer: 1KB allocated on heap (128x64 monochrome)
     *
     * @return true if initialization succeeds, false on I2C error or allocation failure
     *
     * @see FR-026: Must log failure via WebSocket if init fails
     * @see FR-027: System continues without display if init fails (graceful degradation)
     */
    virtual bool init() = 0;

    /**
     * @brief Clear display buffer
     *
     * Clears the framebuffer to black (all pixels off). Does NOT update
     * the physical display until display() is called.
     *
     * Usage pattern:
     * @code
     * display->clear();           // Clear buffer
     * display->setCursor(0, 0);   // Position cursor
     * display->print("Hello");    // Draw text to buffer
     * display->display();         // Push buffer to hardware
     * @endcode
     */
    virtual void clear() = 0;

    /**
     * @brief Set text cursor position
     *
     * Sets the starting position for the next print() call.
     *
     * @param x Column position (0-127 pixels, typically 0-20 for characters)
     * @param y Row position (0-63 pixels, typically 0, 10, 20, 30, 40, 50 for 6 lines)
     *
     * Font size 1: 6 pixels/char width, 10 pixels/line height
     * Line positions: y = 0, 10, 20, 30, 40, 50 (6 lines total)
     */
    virtual void setCursor(uint8_t x, uint8_t y) = 0;

    /**
     * @brief Set text size (font scale factor)
     *
     * @param size Font scale factor (1-4)
     *   - size=1: 5x7 pixels per character (21 chars/line)
     *   - size=2: 10x14 pixels per character (10 chars/line)
     *   - size=3: 15x21 pixels per character (7 chars/line)
     *   - size=4: 20x28 pixels per character (5 chars/line)
     *
     * Recommendation: Use size=1 for maximum information density (FR-022 resource efficiency)
     */
    virtual void setTextSize(uint8_t size) = 0;

    /**
     * @brief Print string to display buffer
     *
     * Renders text to the framebuffer at the current cursor position.
     * Does NOT update physical display until display() is called.
     *
     * @param text Null-terminated string to render
     *
     * Constitutional compliance:
     * - Use F() macro or PROGMEM for string literals (Principle II, FR-023)
     * - Example: `display->print(F("WiFi: Connected"))`
     *
     * @note Cursor advances automatically after printing
     */
    virtual void print(const char* text) = 0;

    /**
     * @brief Push framebuffer to physical display
     *
     * Performs I2C transaction to transfer the framebuffer to the OLED hardware.
     * This is the only method that actually updates the visible display.
     *
     * Performance:
     * - I2C transaction: ~10ms typical @ 400kHz
     * - Full screen refresh: Acceptable for 1-5 second update intervals (FR-016, FR-016a)
     *
     * @note Must be called after rendering operations (clear, setCursor, print)
     * to make changes visible.
     */
    virtual void display() = 0;

    /**
     * @brief Check if display is initialized and ready
     *
     * @return true if init() succeeded and display is ready for rendering, false otherwise
     *
     * Use case: Skip rendering if display failed to initialize (graceful degradation, FR-027)
     * @code
     * if (displayAdapter->isReady()) {
     *     displayAdapter->clear();
     *     displayAdapter->print(F("System running"));
     *     displayAdapter->display();
     * }
     * @endcode
     */
    virtual bool isReady() const = 0;
};

#endif // I_DISPLAY_ADAPTER_H
