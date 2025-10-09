/**
 * @file MockSystemMetrics.h
 * @brief Mock implementation of ISystemMetrics for unit/integration tests
 *
 * Provides test-controllable mock of ESP32 system metrics. Allows tests to
 * set specific metric values and verify collection logic without actual hardware.
 *
 * Usage in tests:
 * @code
 * MockSystemMetrics mockMetrics;
 * mockMetrics.setFreeHeapBytes(250000);
 * mockMetrics.setSketchSizeBytes(850000);
 * mockMetrics.setFreeFlashBytes(1000000);
 * mockMetrics.setCpuIdlePercent(87);
 * mockMetrics.setMillis(5000);
 *
 * MetricsCollector collector(&mockMetrics);
 * DisplayMetrics metrics;
 * collector.collectMetrics(&metrics);
 *
 * TEST_ASSERT_EQUAL(250000, metrics.freeRamBytes);
 * TEST_ASSERT_EQUAL(87, metrics.cpuIdlePercent);
 * @endcode
 *
 * @version 1.0.0
 * @date 2025-10-08
 */

#ifndef MOCK_SYSTEM_METRICS_H
#define MOCK_SYSTEM_METRICS_H

#include "hal/interfaces/ISystemMetrics.h"

/**
 * @brief Mock system metrics for testing
 *
 * Returns test-controlled values for all metrics.
 * Enables deterministic testing without ESP32 hardware.
 */
class MockSystemMetrics : public ISystemMetrics {
private:
    uint32_t _freeHeapBytes;
    uint32_t _sketchSizeBytes;
    uint32_t _freeFlashBytes;
    uint8_t _cpuIdlePercent;
    unsigned long _millis;

public:
    /**
     * @brief Constructor - initialize with reasonable defaults
     */
    MockSystemMetrics()
        : _freeHeapBytes(250000),      // 244 KB free RAM (typical)
          _sketchSizeBytes(850000),    // 830 KB sketch size (typical)
          _freeFlashBytes(1000000),    // 976 KB free flash (typical)
          _cpuIdlePercent(87),         // 87% idle (lightly loaded)
          _millis(0) {
    }

    /**
     * @brief Reset mock to default values
     */
    void reset() {
        _freeHeapBytes = 250000;
        _sketchSizeBytes = 850000;
        _freeFlashBytes = 1000000;
        _cpuIdlePercent = 87;
        _millis = 0;
    }

    // Setter methods for test control

    /**
     * @brief Set free heap memory value
     * @param bytes Free heap in bytes (0-320KB typical for ESP32)
     */
    void setFreeHeapBytes(uint32_t bytes) {
        _freeHeapBytes = bytes;
    }

    /**
     * @brief Set sketch size value
     * @param bytes Sketch size in bytes (500KB-1.5MB typical)
     */
    void setSketchSizeBytes(uint32_t bytes) {
        _sketchSizeBytes = bytes;
    }

    /**
     * @brief Set free flash space value
     * @param bytes Free flash in bytes (0-1.9MB)
     */
    void setFreeFlashBytes(uint32_t bytes) {
        _freeFlashBytes = bytes;
    }

    /**
     * @brief Set CPU idle percentage
     * @param percent Idle percentage (0-100)
     */
    void setCpuIdlePercent(uint8_t percent) {
        _cpuIdlePercent = percent;
    }

    /**
     * @brief Set millis() timestamp
     * @param milliseconds Milliseconds since boot
     */
    void setMillis(unsigned long milliseconds) {
        _millis = milliseconds;
    }

    /**
     * @brief Advance millis() by delta
     * @param deltaMs Milliseconds to advance
     */
    void advanceMillis(unsigned long deltaMs) {
        _millis += deltaMs;
    }

    // ISystemMetrics interface implementation

    uint32_t getFreeHeapBytes() override {
        return _freeHeapBytes;
    }

    uint32_t getSketchSizeBytes() override {
        return _sketchSizeBytes;
    }

    uint32_t getFreeFlashBytes() override {
        return _freeFlashBytes;
    }

    uint8_t getCpuIdlePercent() override {
        return _cpuIdlePercent;
    }

    unsigned long getMillis() override {
        return _millis;
    }
};

#endif // MOCK_SYSTEM_METRICS_H
