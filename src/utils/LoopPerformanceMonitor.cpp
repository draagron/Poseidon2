/**
 * @file LoopPerformanceMonitor.cpp
 * @brief Implementation of loop frequency measurement utility
 *
 * @copyright 2025 Poseidon2
 * @version 1.0.0
 */

#include "LoopPerformanceMonitor.h"

LoopPerformanceMonitor::LoopPerformanceMonitor()
    : _loopCount(0),
      _lastReportTime(0),
      _currentFrequency(0),
      _hasFirstMeasurement(false) {
    // All members initialized via initializer list
    // No heap allocation - constitutional Principle II compliance
}

void LoopPerformanceMonitor::endLoop() {
    // Increment loop counter
    _loopCount++;

    // Get current timestamp
    uint32_t now = millis();

    // Check if 5-second window has elapsed OR millis() has wrapped around
    // millis() wraps at UINT32_MAX (~49.7 days) - detect wrap via (now < _lastReportTime)
    bool windowComplete = (now - _lastReportTime >= MEASUREMENT_WINDOW_MS);
    bool millisOverflow = (now < _lastReportTime);

    if (windowComplete || millisOverflow) {
        // Calculate average frequency over measurement window
        // Integer division: loop_count / 5 seconds = Hz
        // FR-042: Frequency calculated as (count / 5) Hz
        _currentFrequency = _loopCount / 5;

        // Reset counter for next measurement window
        // FR-046: Loop counter reset after each measurement
        _loopCount = 0;

        // Update timestamp for next window
        _lastReportTime = now;

        // Mark that we have at least one valid measurement
        // FR-049: Enables transition from "---" placeholder to numeric display
        _hasFirstMeasurement = true;
    }
}

uint32_t LoopPerformanceMonitor::getLoopFrequency() const {
    // Return 0 if no measurement yet (first 5 seconds after boot)
    // FR-049: Placeholder "---" shown before first measurement
    // Otherwise return last calculated frequency
    return _hasFirstMeasurement ? _currentFrequency : 0;
}
