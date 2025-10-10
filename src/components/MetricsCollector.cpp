/**
 * @file MetricsCollector.cpp
 * @brief Implementation of MetricsCollector component
 *
 * @version 1.0.0
 * @date 2025-10-09
 */

#include "MetricsCollector.h"

MetricsCollector::MetricsCollector(ISystemMetrics* systemMetrics)
    : _systemMetrics(systemMetrics) {
    // Constructor: Store HAL interface pointer
}

void MetricsCollector::collectMetrics(DisplayMetrics* metrics) {
    if (metrics == nullptr || _systemMetrics == nullptr) {
        return;  // Graceful handling of null pointers
    }

    // Query all system metrics via HAL interface
    metrics->freeRamBytes = _systemMetrics->getFreeHeapBytes();
    metrics->sketchSizeBytes = _systemMetrics->getSketchSizeBytes();
    metrics->freeFlashBytes = _systemMetrics->getFreeFlashBytes();
    metrics->cpuIdlePercent = _systemMetrics->getCpuIdlePercent();

    // Update timestamp
    metrics->lastUpdate = _systemMetrics->getMillis();

    // Note: animationState is NOT updated here - it's managed by DisplayManager
    // to support independent 1-second animation cycle (FR-016a)
}
