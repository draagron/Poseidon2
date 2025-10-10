/**
 * @file ISystemMetrics.h
 * @brief Hardware Abstraction Layer interface for ESP32 system metrics
 *
 * This interface abstracts ESP32 platform APIs for retrieving system resource
 * metrics (RAM, flash, CPU), enabling mock-first testing on native platform.
 *
 * Constitutional Principle I: Hardware Abstraction Layer
 * - All ESP32 platform API calls abstracted through this interface
 * - Mock implementation (MockSystemMetrics) enables unit/integration tests
 * - Business logic (MetricsCollector) depends only on this interface
 *
 * @version 1.0.0
 * @date 2025-10-08
 */

#ifndef I_SYSTEM_METRICS_H
#define I_SYSTEM_METRICS_H

#include <stdint.h>

/**
 * @brief System metrics interface for ESP32 platform APIs
 *
 * Abstracts ESP32-specific memory and CPU APIs to enable testable
 * metrics collection logic.
 *
 * Implementation: ESP32SystemMetrics (uses ESP.h and FreeRTOS APIs)
 * Mock: MockSystemMetrics (for unit/integration tests)
 */
class ISystemMetrics {
public:
    virtual ~ISystemMetrics() = default;

    /**
     * @brief Get free heap memory in bytes
     *
     * Returns the amount of free RAM available on the ESP32 heap.
     *
     * Implementation: ESP.getFreeHeap()
     * Typical values: 50KB-280KB depending on allocated objects
     *
     * @return Free heap memory in bytes
     *
     * @see FR-011: Display free RAM on OLED (formatted as KB)
     */
    virtual uint32_t getFreeHeapBytes() = 0;

    /**
     * @brief Get sketch (uploaded code) size in bytes
     *
     * Returns the size of the compiled and uploaded firmware image.
     *
     * Implementation: ESP.getSketchSize()
     * Typical values: 500KB-1.5MB depending on features/libraries
     *
     * @return Sketch size in bytes
     *
     * @see FR-012: Display flash usage (sketch size) on OLED
     */
    virtual uint32_t getSketchSizeBytes() = 0;

    /**
     * @brief Get free flash space available for OTA updates
     *
     * Returns the amount of flash memory available for future firmware updates.
     * This is the space remaining in the OTA partition.
     *
     * Implementation: ESP.getFreeSketchSpace()
     * Typical values: 400KB-1.4MB depending on sketch size
     *
     * @return Free flash space in bytes
     *
     * @see FR-013: Display free flash space on OLED
     */
    virtual uint32_t getFreeFlashBytes() = 0;

    /**
     * @brief Get CPU idle time percentage
     *
     * Returns the percentage of time the CPU spends in the IDLE task
     * (0% = fully loaded, 100% = completely idle).
     *
     * Implementation:
     * - Use FreeRTOS vTaskGetRunTimeStats() or uxTaskGetSystemState()
     * - Calculate: (IDLE task runtime / total runtime) * 100
     * - Requires CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS=y in sdkconfig
     *   (enabled by default in ESP32 Arduino core)
     *
     * Performance impact: ~1-2% CPU overhead for runtime stats collection
     * Update frequency: Every 5 seconds (FR-016)
     *
     * @return CPU idle time percentage (0-100)
     *
     * @see FR-015: Display CPU idle % on OLED
     * @see research.md: FreeRTOS CPU idle time statistics
     */
    virtual uint8_t getCpuIdlePercent() = 0;

    /**
     * @brief Get current millisecond timestamp
     *
     * Returns the number of milliseconds since system boot.
     * Wraps after approximately 49 days.
     *
     * Implementation: millis() (Arduino core)
     *
     * Usage:
     * - Timestamp for DisplayMetrics.lastUpdate
     * - Delta calculations: (unsigned long)(currentMillis - lastMillis) handles wrapping
     *
     * @return Milliseconds since boot
     */
    virtual unsigned long getMillis() = 0;
};

#endif // I_SYSTEM_METRICS_H
