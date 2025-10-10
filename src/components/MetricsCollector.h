/**
 * @file MetricsCollector.h
 * @brief Component to gather system metrics via ISystemMetrics interface
 *
 * Collects ESP32 system resource metrics (RAM, flash, CPU) through the
 * ISystemMetrics HAL interface and populates DisplayMetrics struct.
 *
 * Constitutional Principle I: Hardware Abstraction Layer
 * - Depends on ISystemMetrics interface, not ESP32 hardware directly
 * - Enables testing with MockSystemMetrics on native platform
 *
 * @version 1.0.0
 * @date 2025-10-09
 */

#ifndef METRICS_COLLECTOR_H
#define METRICS_COLLECTOR_H

#include "hal/interfaces/ISystemMetrics.h"
#include "types/DisplayTypes.h"

/**
 * @brief Collects system metrics for display
 *
 * Queries ISystemMetrics interface to gather current system resource
 * metrics and populates a DisplayMetrics struct.
 */
class MetricsCollector {
private:
    ISystemMetrics* _systemMetrics;  ///< HAL interface for system metrics

public:
    /**
     * @brief Constructor with dependency injection
     *
     * @param systemMetrics Pointer to ISystemMetrics implementation
     *                      (ESP32SystemMetrics for production, MockSystemMetrics for tests)
     */
    MetricsCollector(ISystemMetrics* systemMetrics);

    /**
     * @brief Collect current system metrics
     *
     * Queries all system metrics via ISystemMetrics interface:
     * - Free heap bytes (RAM)
     * - Sketch size bytes (uploaded code size)
     * - Free flash bytes (available for OTA updates)
     * - CPU idle percentage (0-100)
     * - Current timestamp (millis())
     *
     * Updates the provided DisplayMetrics struct with fresh values.
     * Handles edge cases (0 values, overflow).
     *
     * @param metrics Pointer to DisplayMetrics struct to populate
     *
     * Usage:
     * @code
     * DisplayMetrics metrics = {0};
     * metricsCollector->collectMetrics(&metrics);
     * // metrics now contains current system state
     * @endcode
     */
    void collectMetrics(DisplayMetrics* metrics);
};

#endif // METRICS_COLLECTOR_H
