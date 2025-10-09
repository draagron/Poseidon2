/**
 * @file ESP32SystemMetrics.h
 * @brief ESP32 hardware adapter for system metrics
 *
 * Implements ISystemMetrics using ESP32 platform APIs for memory and CPU metrics.
 *
 * Hardware: ESP32 (ESP32, ESP32-S2, ESP32-C3, ESP32-S3)
 * APIs: ESP.h (memory), FreeRTOS (CPU idle time)
 *
 * Constitutional Compliance:
 * - Principle I (Hardware Abstraction): Implements ISystemMetrics interface
 * - Principle II (Resource Management): Minimal overhead (~1-2% CPU for stats)
 *
 * @version 1.0.0
 * @date 2025-10-09
 */

#ifndef ESP32_SYSTEM_METRICS_H
#define ESP32_SYSTEM_METRICS_H

#include "hal/interfaces/ISystemMetrics.h"

/**
 * @brief ESP32 hardware implementation of ISystemMetrics
 *
 * Uses ESP32 platform APIs to retrieve system resource metrics:
 * - ESP.getFreeHeap() for RAM
 * - ESP.getSketchSize() for code size
 * - ESP.getFreeSketchSpace() for free flash
 * - FreeRTOS uxTaskGetSystemState() for CPU idle %
 * - millis() for timestamp
 */
class ESP32SystemMetrics : public ISystemMetrics {
public:
    /**
     * @brief Constructor
     */
    ESP32SystemMetrics();

    /**
     * @brief Destructor
     */
    ~ESP32SystemMetrics();

    // ISystemMetrics interface implementation
    uint32_t getFreeHeapBytes() override;
    uint32_t getSketchSizeBytes() override;
    uint32_t getFreeFlashBytes() override;
    uint8_t getCpuIdlePercent() override;
    unsigned long getMillis() override;
};

#endif // ESP32_SYSTEM_METRICS_H
