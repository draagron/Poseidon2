/**
 * @file LoopPerformanceMonitor.h
 * @brief Lightweight utility for measuring main loop frequency
 *
 * Tracks loop iterations over 5-second windows and calculates average frequency.
 * Designed for minimal overhead (< 1% performance impact) with static allocation only.
 *
 * Constitutional Compliance:
 * - Principle II (Resource Management): 16 bytes static, zero heap allocation
 * - Principle VI (Always-On): Continuous measurement, no sleep modes
 * - Principle VII (Fail-Safe): Handles millis() overflow gracefully
 *
 * @copyright 2025 Poseidon2
 * @version 1.0.0
 */

#ifndef LOOP_PERFORMANCE_MONITOR_H
#define LOOP_PERFORMANCE_MONITOR_H

#include <Arduino.h>

/**
 * @class LoopPerformanceMonitor
 * @brief Measures main loop frequency over 5-second windows
 *
 * Usage pattern:
 * @code
 * LoopPerformanceMonitor monitor;
 *
 * void loop() {
 *     // ... application logic ...
 *     monitor.endLoop();  // Call at end of each iteration
 * }
 *
 * void displayFrequency() {
 *     uint32_t freq = monitor.getLoopFrequency();
 *     Serial.printf("Loop: %u Hz\n", freq);
 * }
 * @endcode
 *
 * Memory footprint: 16 bytes (3× uint32_t + 1× bool + 3 bytes padding)
 * Performance overhead: < 6 µs per loop iteration (< 0.1% for 5ms loops)
 */
class LoopPerformanceMonitor {
public:
    /**
     * @brief Constructor - initializes all counters to zero
     *
     * Initial state:
     * - Loop count: 0
     * - Last report time: 0
     * - Current frequency: 0
     * - Has first measurement: false (returns 0 until first 5-second window completes)
     */
    LoopPerformanceMonitor();

    /**
     * @brief Records end of loop iteration and calculates frequency every 5 seconds
     *
     * Call this method at the END of each main loop iteration. The method:
     * 1. Increments loop counter
     * 2. Checks if 5 seconds have elapsed since last report
     * 3. Calculates frequency (loop_count / 5) if window completed
     * 4. Resets counter for next measurement window
     * 5. Handles millis() overflow (wrap-around at ~49.7 days)
     *
     * Performance: ~5 µs per call (2× millis() + increment + conditional)
     *
     * @note Thread-safe for single-core usage (not ISR-safe)
     * @note Must be called every loop iteration for accurate measurement
     */
    void endLoop();

    /**
     * @brief Returns last calculated loop frequency in Hz
     *
     * @return uint32_t Loop frequency in Hz
     *         - Returns 0 if no measurement yet (first 5 seconds after boot)
     *         - Returns average frequency over last 5-second window
     *         - Typical range: 100-500 Hz for ESP32 with ReactESP
     *         - Values < 10 Hz indicate system hang or blocking operations
     *         - Values > 2000 Hz are unexpected (log warning)
     *
     * Performance: < 1 µs (simple member access)
     *
     * @note This is a read-only operation with no side effects
     */
    uint32_t getLoopFrequency() const;

private:
    uint32_t _loopCount;              ///< Accumulated loop iterations in current 5-second window
    uint32_t _lastReportTime;         ///< millis() timestamp of last frequency calculation
    uint32_t _currentFrequency;       ///< Last calculated frequency in Hz (0 = not measured yet)
    bool _hasFirstMeasurement;        ///< False until first 5-second window completes

    /**
     * @brief Measurement window duration in milliseconds
     *
     * Constitutional requirement FR-043: 5-second update interval
     */
    static constexpr uint32_t MEASUREMENT_WINDOW_MS = 5000;
};

#endif // LOOP_PERFORMANCE_MONITOR_H
