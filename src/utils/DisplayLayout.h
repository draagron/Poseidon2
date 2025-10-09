/**
 * @file DisplayLayout.h
 * @brief Text positioning and formatting utilities for 128x64 OLED display
 *
 * Header-only utility functions for layout calculations, string formatting,
 * and PROGMEM constants. All functions are inline for zero runtime overhead.
 *
 * Constitutional Principle II: Resource-Aware Development
 * - All label constants stored in PROGMEM (saves RAM)
 * - Inline functions avoid function call overhead
 * - Fixed-size buffers prevent heap allocation
 *
 * @version 1.0.0
 * @date 2025-10-08
 */

#ifndef DISPLAY_LAYOUT_H
#define DISPLAY_LAYOUT_H

#include <stdint.h>
#include <Arduino.h>  // For F() macro and PROGMEM

// Screen dimensions
#define SCREEN_WIDTH 128   ///< Display width in pixels
#define SCREEN_HEIGHT 64   ///< Display height in pixels

// Font size 1 character dimensions
#define CHAR_WIDTH 6       ///< Characters are 5px wide + 1px spacing
#define LINE_HEIGHT 10     ///< Lines are 7px tall + 3px spacing

// Maximum text dimensions
#define MAX_CHARS_PER_LINE 21  ///< 128 / 6 = 21 characters per line
#define MAX_LINES 6            ///< 64 / 10 = 6.4 lines (use 6 for clean layout)

/**
 * @brief Get Y-coordinate for text line number
 *
 * Calculates pixel Y position for line 0-5 on the display.
 * Line 0 is at top (y=0), Line 5 is at bottom (y=50).
 *
 * @param lineNum Line number (0-5)
 * @return Y-coordinate in pixels (0, 10, 20, 30, 40, 50)
 *
 * Usage:
 * @code
 * display->setCursor(0, getLineY(0));  // Line 0 at y=0
 * display->print("WiFi: Connected");
 * display->setCursor(0, getLineY(1));  // Line 1 at y=10
 * display->print("IP: 192.168.1.100");
 * @endcode
 */
inline uint8_t getLineY(uint8_t lineNum) {
    if (lineNum >= MAX_LINES) {
        return (MAX_LINES - 1) * LINE_HEIGHT;  // Clamp to last line
    }
    return lineNum * LINE_HEIGHT;
}

/**
 * @brief Format bytes as KB string
 *
 * Converts byte count to kilobytes with "KB" suffix.
 * Example: 250000 bytes → "244KB"
 *
 * @param bytes Number of bytes to format
 * @param buffer Output buffer (must be at least 10 chars)
 *
 * Usage:
 * @code
 * char ramStr[10];
 * formatBytes(245000, ramStr);  // "239KB"
 * display->print(ramStr);
 * @endcode
 */
inline void formatBytes(uint32_t bytes, char* buffer) {
    uint32_t kilobytes = bytes / 1024;
    snprintf(buffer, 10, "%luKB", kilobytes);
}

/**
 * @brief Format percentage with % suffix
 *
 * Formats integer percentage value with "%" suffix.
 * Example: 87 → "87%"
 *
 * @param value Percentage value (0-100)
 * @param buffer Output buffer (must be at least 5 chars)
 *
 * Usage:
 * @code
 * char cpuStr[5];
 * formatPercent(87, cpuStr);  // "87%"
 * display->print(cpuStr);
 * @endcode
 */
inline void formatPercent(uint8_t value, char* buffer) {
    snprintf(buffer, 5, "%u%%", value);
}

/**
 * @brief Format IP address or placeholder
 *
 * Formats IP address with "IP: " prefix, or "IP: ---" if empty.
 *
 * @param ip IP address string (e.g., "192.168.1.100") or "" for empty
 * @param buffer Output buffer (must be at least 22 chars)
 *
 * Usage:
 * @code
 * char ipStr[22];
 * formatIPAddress("192.168.1.100", ipStr);  // "IP: 192.168.1.100"
 * formatIPAddress("", ipStr);               // "IP: ---"
 * display->print(ipStr);
 * @endcode
 */
inline void formatIPAddress(const char* ip, char* buffer) {
    if (ip[0] == '\0') {
        snprintf(buffer, 22, "IP: ---");
    } else {
        snprintf(buffer, 22, "IP: %s", ip);
    }
}

/**
 * @brief Format flash usage as "used/total KB"
 *
 * Formats flash memory usage with both used and total space.
 * Example: 850000 bytes used, 1920000 total → "830/1875KB"
 *
 * @param usedBytes Flash space used (sketch size)
 * @param totalBytes Total flash space available
 * @param buffer Output buffer (must be at least 20 chars)
 *
 * Usage:
 * @code
 * char flashStr[20];
 * formatFlashUsage(850000, 1920000, flashStr);  // "830/1875KB"
 * display->print(flashStr);
 * @endcode
 */
inline void formatFlashUsage(uint32_t usedBytes, uint32_t totalBytes, char* buffer) {
    uint32_t usedKB = usedBytes / 1024;
    uint32_t totalKB = totalBytes / 1024;
    snprintf(buffer, 20, "%lu/%luKB", usedKB, totalKB);
}

/**
 * @brief Get animation icon character
 *
 * Returns rotating icon character for given animation state.
 * State cycles: 0=/, 1=-, 2=\, 3=|
 *
 * @param state Animation state (0-3)
 * @return Icon character ('/', '-', '\\', '|')
 *
 * Usage:
 * @code
 * uint8_t animState = 0;  // Increments every 1 second
 * char icon = getAnimationIcon(animState);  // '/'
 * display->print("[ ");
 * display->print(&icon);
 * display->print(" ]");
 * @endcode
 */
inline char getAnimationIcon(uint8_t state) {
    switch (state % 4) {
        case 0: return '/';
        case 1: return '-';
        case 2: return '\\';
        case 3: return '|';
        default: return '?';  // Should never happen
    }
}

// PROGMEM string constants (stored in flash, not RAM)
const char LABEL_WIFI[] PROGMEM = "WiFi: ";
const char LABEL_RAM[] PROGMEM = "RAM: ";
const char LABEL_FLASH[] PROGMEM = "Flash: ";
const char LABEL_CPU[] PROGMEM = "CPU Idle: ";
const char LABEL_CONNECTING[] PROGMEM = "Connecting...";
const char LABEL_DISCONNECTED[] PROGMEM = "Disconnected";
const char LABEL_BOOTING[] PROGMEM = "Booting...";
const char LABEL_BOOT_COMPLETE[] PROGMEM = "Boot complete!";
const char LABEL_POSEIDON[] PROGMEM = "Poseidon2 Gateway";

#endif // DISPLAY_LAYOUT_H
