/**
 * @file ESP32SystemMetrics.cpp
 * @brief Implementation of ESP32SystemMetrics
 *
 * @version 1.0.0
 * @date 2025-10-09
 */

#include "ESP32SystemMetrics.h"

#ifdef ARDUINO
#include <Arduino.h>
#include <Esp.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

ESP32SystemMetrics::ESP32SystemMetrics() {
    // Constructor: No initialization needed
}

ESP32SystemMetrics::~ESP32SystemMetrics() {
    // Destructor: No cleanup needed
}

uint32_t ESP32SystemMetrics::getFreeHeapBytes() {
    return ESP.getFreeHeap();
}

uint32_t ESP32SystemMetrics::getSketchSizeBytes() {
    return ESP.getSketchSize();
}

uint32_t ESP32SystemMetrics::getFreeFlashBytes() {
    return ESP.getFreeSketchSpace();
}

uint32_t ESP32SystemMetrics::getLoopFrequency() {
    /**
     * Returns the main loop frequency in Hz.
     *
     * Delegates to LoopPerformanceMonitor which tracks loop iterations
     * over 5-second windows and calculates average frequency.
     *
     * @return uint32_t Loop frequency in Hz (0 = not yet measured)
     *
     * Performance: < 1 µs (simple member access)
     * Constitutional compliance: FR-042 (frequency = count / 5)
     */
    return _loopMonitor.getLoopFrequency();
}

void ESP32SystemMetrics::instrumentLoop() {
    /**
     * Instruments the main loop for performance measurement.
     *
     * Call this method at the END of each main loop iteration.
     * Increments loop counter and calculates frequency every 5 seconds.
     *
     * Performance: ~5 µs per call (< 0.1% overhead for typical 5ms loops)
     * Constitutional compliance: FR-041 (loop iteration count measured)
     */
    _loopMonitor.endLoop();
}

unsigned long ESP32SystemMetrics::getMillis() {
    return millis();
}

#else
// Stub implementation for native platform (tests use MockSystemMetrics)
ESP32SystemMetrics::ESP32SystemMetrics() {}
ESP32SystemMetrics::~ESP32SystemMetrics() {}
uint32_t ESP32SystemMetrics::getFreeHeapBytes() { return 250000; }
uint32_t ESP32SystemMetrics::getSketchSizeBytes() { return 850000; }
uint32_t ESP32SystemMetrics::getFreeFlashBytes() { return 1000000; }
uint32_t ESP32SystemMetrics::getLoopFrequency() { return 212; }  // Typical value for tests
void ESP32SystemMetrics::instrumentLoop() { _loopMonitor.endLoop(); }
unsigned long ESP32SystemMetrics::getMillis() { return 0; }
#endif
