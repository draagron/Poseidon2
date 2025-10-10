/**
 * @file DisplayFormatter.cpp
 * @brief Implementation of display formatting utilities
 *
 * @version 1.0.0
 * @date 2025-10-10
 */

#include "DisplayFormatter.h"
#include <stdio.h>

void DisplayFormatter::formatFrequency(uint32_t frequency, char* buffer) {
    /**
     * Format loop frequency according to FR-045 requirements:
     * - 0 Hz: "---" placeholder (no measurement yet)
     * - 1-999 Hz: Integer display, no decimals
     * - ≥1000 Hz: Abbreviated "X.Xk" format with 1 decimal
     *
     * Constitutional compliance:
     * - FR-045: Integer display unless >999 Hz
     * - FR-049: Placeholder "---" before first measurement
     * - Principle II: Static allocation, no heap usage
     */

    if (frequency == 0) {
        // No measurement yet - show placeholder
        snprintf(buffer, 8, "---");
    } else if (frequency < 1000) {
        // Normal range: 1-999 Hz - show integer
        snprintf(buffer, 8, "%lu", (unsigned long)frequency);
    } else {
        // High frequency: ≥1000 Hz - show abbreviated kilohert with 1 decimal
        // Example: 1500 Hz → "1.5k"
        float kHz = frequency / 1000.0f;
        snprintf(buffer, 8, "%.1fk", kHz);
    }
}
