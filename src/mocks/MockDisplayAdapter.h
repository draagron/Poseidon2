/**
 * @file MockDisplayAdapter.h
 * @brief Mock implementation of IDisplayAdapter for unit/integration tests
 *
 * Provides test-controllable mock of OLED display adapter. Tracks method calls
 * in internal state for verification, without actual hardware rendering.
 *
 * Usage in tests:
 * @code
 * MockDisplayAdapter mockDisplay;
 * mockDisplay.setInitResult(true);  // Simulate successful init
 * DisplayManager manager(&mockDisplay, &mockMetrics);
 * manager.init();
 * TEST_ASSERT_TRUE(mockDisplay.wasCleared());
 * TEST_ASSERT_TRUE(mockDisplay.wasTextRendered("WiFi: Connected"));
 * @endcode
 *
 * @version 1.0.0
 * @date 2025-10-08
 */

#ifndef MOCK_DISPLAY_ADAPTER_H
#define MOCK_DISPLAY_ADAPTER_H

#include "hal/interfaces/IDisplayAdapter.h"
#include <string.h>  // For strstr, strlen

/**
 * @brief Mock display adapter for testing
 *
 * Simulates display operations by tracking state internally.
 * No actual rendering occurs, enabling fast native platform tests.
 */
class MockDisplayAdapter : public IDisplayAdapter {
private:
    bool _isReady;
    bool _initResult;
    bool _wasCleared;
    bool _displayCalled;
    uint8_t _cursorX;
    uint8_t _cursorY;
    uint8_t _textSize;
    char _renderedText[512];  // Buffer to track all printed text
    int _renderedTextLen;

public:
    /**
     * @brief Constructor - initialize mock state
     */
    MockDisplayAdapter()
        : _isReady(false),
          _initResult(true),  // Default: init succeeds
          _wasCleared(false),
          _displayCalled(false),
          _cursorX(0),
          _cursorY(0),
          _textSize(1),
          _renderedTextLen(0) {
        _renderedText[0] = '\0';
    }

    /**
     * @brief Set init() return value
     *
     * Controls whether next init() call succeeds or fails.
     *
     * @param result true = init succeeds, false = init fails (I2C error)
     */
    void setInitResult(bool result) {
        _initResult = result;
    }

    /**
     * @brief Reset mock state
     *
     * Clears all tracked state for next test case.
     */
    void reset() {
        _isReady = false;
        _initResult = true;
        _wasCleared = false;
        _displayCalled = false;
        _cursorX = 0;
        _cursorY = 0;
        _textSize = 1;
        _renderedText[0] = '\0';
        _renderedTextLen = 0;
    }

    // Query methods for test verification

    /**
     * @brief Check if clear() was called
     * @return true if clear() was called since last reset
     */
    bool wasCleared() const {
        return _wasCleared;
    }

    /**
     * @brief Check if display() was called
     * @return true if display() was called since last reset
     */
    bool wasDisplayCalled() const {
        return _displayCalled;
    }

    /**
     * @brief Check if specific text was rendered
     *
     * @param text Text to search for in rendered output
     * @return true if text was printed via print()
     */
    bool wasTextRendered(const char* text) const {
        return strstr(_renderedText, text) != nullptr;
    }

    /**
     * @brief Get current cursor X position
     * @return Cursor X coordinate (0-127)
     */
    uint8_t getCursorX() const {
        return _cursorX;
    }

    /**
     * @brief Get current cursor Y position
     * @return Cursor Y coordinate (0-63)
     */
    uint8_t getCursorY() const {
        return _cursorY;
    }

    /**
     * @brief Get current text size
     * @return Text size (1-4)
     */
    uint8_t getTextSize() const {
        return _textSize;
    }

    /**
     * @brief Get all rendered text
     * @return Null-terminated string of all text printed via print()
     */
    const char* getRenderedText() const {
        return _renderedText;
    }

    // IDisplayAdapter interface implementation

    bool init() override {
        _isReady = _initResult;
        return _initResult;
    }

    void clear() override {
        _wasCleared = true;
        _renderedText[0] = '\0';
        _renderedTextLen = 0;
    }

    void setCursor(uint8_t x, uint8_t y) override {
        _cursorX = x;
        _cursorY = y;
    }

    void setTextSize(uint8_t size) override {
        _textSize = size;
    }

    void print(const char* text) override {
        // Append text to rendered buffer
        int textLen = strlen(text);
        if (_renderedTextLen + textLen < sizeof(_renderedText) - 1) {
            strcpy(_renderedText + _renderedTextLen, text);
            _renderedTextLen += textLen;
        }
        // Note: Real display advances cursor, but mock doesn't track precise position
    }

    void display() override {
        _displayCalled = true;
    }

    bool isReady() const override {
        return _isReady;
    }
};

#endif // MOCK_DISPLAY_ADAPTER_H
