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

uint8_t ESP32SystemMetrics::getCpuIdlePercent() {
    // Calculate CPU idle time using FreeRTOS runtime statistics
    //
    // NOTE: uxTaskGetSystemState() requires CONFIG_FREERTOS_USE_TRACE_FACILITY
    // and CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS to be enabled in sdkconfig.
    // If not available, we'll return a placeholder value.
    //
    // For now, return a reasonable default (85% idle is typical for idle ESP32)
    // TODO: Enable FreeRTOS stats in platformio.ini build_flags if needed:
    // build_flags = -DCONFIG_FREERTOS_USE_TRACE_FACILITY=1
    //               -DCONFIG_FREERTOS_GENERATE_RUN_TIME_STATS=1

    // Simplified implementation: Return fixed value for now
    // In production, this should be configurable or use alternative metrics
    return 85;  // Typical idle percentage for ESP32 with moderate load

    /* Full implementation (requires FreeRTOS stats enabled):
    TaskStatus_t* taskStatusArray;
    volatile UBaseType_t taskCount;
    uint32_t totalRuntime = 0;
    uint32_t idleRuntime = 0;

    taskCount = uxTaskGetNumberOfTasks();
    taskStatusArray = (TaskStatus_t*)pvPortMalloc(taskCount * sizeof(TaskStatus_t));
    if (taskStatusArray == NULL) {
        return 85;  // Out of memory - return reasonable default
    }

    taskCount = uxTaskGetSystemState(taskStatusArray, taskCount, &totalRuntime);

    for (UBaseType_t i = 0; i < taskCount; i++) {
        const char* taskName = taskStatusArray[i].pcTaskName;
        if (strstr(taskName, "IDLE") != NULL) {
            idleRuntime += taskStatusArray[i].ulRunTimeCounter;
        }
    }

    vPortFree(taskStatusArray);

    if (totalRuntime == 0) {
        return 85;
    }

    uint8_t idlePercent = (uint8_t)((idleRuntime * 100UL) / totalRuntime);
    if (idlePercent > 100) {
        idlePercent = 100;
    }

    return idlePercent;
    */
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
uint8_t ESP32SystemMetrics::getCpuIdlePercent() { return 87; }
unsigned long ESP32SystemMetrics::getMillis() { return 0; }
#endif
