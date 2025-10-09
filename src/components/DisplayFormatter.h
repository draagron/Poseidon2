/**
 * @file DisplayFormatter.h
 * @brief Component to format metrics as display strings
 *
 * Provides static utility methods for formatting DisplayMetrics and
 * SubsystemStatus data as human-readable strings for OLED display.
 *
 * Note: Most formatting functions are already provided as inline functions
 * in utils/DisplayLayout.h. This component wraps them for consistency and
 * provides additional formatting helpers.
 *
 * Constitutional Principle II: Resource-Aware Development
 * - All methods are static (no heap allocation for instances)
 * - Uses caller-provided buffers (no dynamic allocation)
 * - Safe string formatting with snprintf() bounds checking
 *
 * @version 1.0.0
 * @date 2025-10-09
 */

#ifndef DISPLAY_FORMATTER_H
#define DISPLAY_FORMATTER_H

#include "types/DisplayTypes.h"
#include "utils/DisplayLayout.h"
#include <stdint.h>

/**
 * @brief Static utility class for display string formatting
 *
 * All methods are static - no instance required.
 * Formats DisplayMetrics and SubsystemStatus for OLED display.
 */
class DisplayFormatter {
public:
    /**
     * @brief Format bytes as KB string
     *
     * Converts byte count to kilobytes with "KB" suffix.
     * Example: 250000 bytes → "244KB"
     *
     * @param bytes Number of bytes to format
     * @param buffer Output buffer (must be at least 10 chars)
     *
     * Delegates to inline function from DisplayLayout.h
     */
    static void formatBytes(uint32_t bytes, char* buffer) {
        ::formatBytes(bytes, buffer);  // Call inline function
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
     * Delegates to inline function from DisplayLayout.h
     */
    static void formatPercent(uint8_t value, char* buffer) {
        ::formatPercent(value, buffer);  // Call inline function
    }

    /**
     * @brief Format IP address or placeholder
     *
     * Formats IP address with "IP: " prefix, or "IP: ---" if empty.
     *
     * @param ip IP address string (e.g., "192.168.1.100") or "" for empty
     * @param buffer Output buffer (must be at least 22 chars)
     *
     * Delegates to inline function from DisplayLayout.h
     */
    static void formatIPAddress(const char* ip, char* buffer) {
        ::formatIPAddress(ip, buffer);  // Call inline function
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
     * Delegates to inline function from DisplayLayout.h
     */
    static void formatFlashUsage(uint32_t usedBytes, uint32_t totalBytes, char* buffer) {
        ::formatFlashUsage(usedBytes, totalBytes, buffer);  // Call inline function
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
     * Delegates to inline function from DisplayLayout.h
     */
    static char getAnimationIcon(uint8_t state) {
        return ::getAnimationIcon(state);  // Call inline function
    }
};

#endif // DISPLAY_FORMATTER_H
