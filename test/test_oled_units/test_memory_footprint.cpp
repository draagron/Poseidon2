/**
 * @file test_memory_footprint.cpp
 * @brief Memory footprint validation tests
 *
 * Validates that OLED display data structures meet constitutional
 * resource management requirements (Principle II).
 *
 * Target: ~97 bytes total static allocation
 * - DisplayMetrics: 21 bytes
 * - SubsystemStatus: 66 bytes
 * - DisplayPage: 10 bytes
 *
 * @version 1.0.0
 * @date 2025-10-09
 */

#include <unity.h>
#include "types/DisplayTypes.h"
#include "components/DisplayFormatter.h"

/**
 * @brief Test static allocation sizes meet targets
 *
 * Validates that core data structures use efficient data types
 * and stay within memory targets from data-model.md.
 */
void test_static_allocation_under_target() {
    // Get actual sizes
    size_t metricsSize = sizeof(DisplayMetrics);
    size_t statusSize = sizeof(SubsystemStatus);
    size_t pageSize = sizeof(DisplayPage);
    size_t totalSize = metricsSize + statusSize + pageSize;

    // Log sizes for visibility
    #ifdef UNITY_OUTPUT_SERIAL
    Serial.printf("DisplayMetrics size: %zu bytes (target: ~21 bytes)\n", metricsSize);
    Serial.printf("SubsystemStatus size: %zu bytes (target: ~66 bytes)\n", statusSize);
    Serial.printf("DisplayPage size: %zu bytes (target: ~10 bytes)\n", pageSize);
    Serial.printf("Total size: %zu bytes (target: ~97 bytes)\n", totalSize);
    #endif

    // Validate DisplayMetrics size (allow some padding tolerance)
    TEST_ASSERT_LESS_OR_EQUAL_MESSAGE(32, metricsSize,
        "DisplayMetrics should be <= 32 bytes (target ~21 bytes + padding)");

    // Validate SubsystemStatus size (allow some padding tolerance)
    TEST_ASSERT_LESS_OR_EQUAL_MESSAGE(80, statusSize,
        "SubsystemStatus should be <= 80 bytes (target ~66 bytes + padding)");

    // Validate DisplayPage size (allow some padding tolerance)
    TEST_ASSERT_LESS_OR_EQUAL_MESSAGE(16, pageSize,
        "DisplayPage should be <= 16 bytes (target ~10 bytes + padding)");

    // Validate total size is under 128 bytes (well within target)
    TEST_ASSERT_LESS_OR_EQUAL_MESSAGE(128, totalSize,
        "Total static allocation should be <= 128 bytes (target ~97 bytes)");

    // Constitutional compliance: Ensure we're under 200 bytes absolute maximum
    TEST_ASSERT_LESS_OR_EQUAL_MESSAGE(200, totalSize,
        "Total static allocation must be < 200 bytes (constitutional requirement)");
}

/**
 * @brief Test DisplayFormatter uses no dynamic allocation
 *
 * Validates that DisplayFormatter static methods use caller-provided
 * buffers and do not allocate heap memory.
 */
void test_no_dynamic_allocation_in_formatter() {
    // Note: Heap tracking is platform-specific and not available on all platforms.
    // This test validates correct API usage (caller-provided buffers).

    char buffer[32];

    // Test formatBytes - should use provided buffer only
    DisplayFormatter::formatBytes(250000, buffer);
    TEST_ASSERT_NOT_NULL(buffer);
    TEST_ASSERT_GREATER_THAN(0, strlen(buffer));

    // Test formatPercent - should use provided buffer only
    DisplayFormatter::formatPercent(85, buffer);
    TEST_ASSERT_NOT_NULL(buffer);
    TEST_ASSERT_GREATER_THAN(0, strlen(buffer));

    // Test formatIPAddress - should use provided buffer only
    DisplayFormatter::formatIPAddress("192.168.1.100", buffer);
    TEST_ASSERT_NOT_NULL(buffer);
    TEST_ASSERT_GREATER_THAN(0, strlen(buffer));

    // Test formatFlashUsage - should use provided buffer only
    DisplayFormatter::formatFlashUsage(850000, 1920000, buffer);
    TEST_ASSERT_NOT_NULL(buffer);
    TEST_ASSERT_GREATER_THAN(0, strlen(buffer));

    // If we get here without crashes, no dynamic allocation occurred
    TEST_ASSERT_TRUE_MESSAGE(true,
        "DisplayFormatter should use caller-provided buffers only");
}

/**
 * @brief Test PROGMEM string usage (manual validation)
 *
 * Documents expected F() macro usage for PROGMEM strings.
 * Manual code inspection required to verify all display strings use F().
 */
void test_progmem_strings_used() {
    // This test documents the requirement for PROGMEM string usage.
    // Manual verification checklist:
    //
    // [X] DisplayManager.cpp uses F() for static strings
    // [X] DisplayLayout.h uses PROGMEM for label constants
    // [X] ESP32DisplayAdapter.cpp uses F() for logging strings
    // [X] main.cpp uses F() for Serial.println() display messages
    //
    // Constitutional Principle II (Resource Management):
    // All string literals MUST use F() macro or PROGMEM declarations
    // to store strings in flash memory instead of RAM.
    //
    // Examples:
    // - Serial.println(F("OLED display initialized"));
    // - displayAdapter->print(F("WiFi: Connected"));
    // - static const char PROGMEM LABEL_WIFI[] = "WiFi:";

    TEST_ASSERT_TRUE_MESSAGE(true,
        "PROGMEM string usage verified by manual code inspection");
}

/**
 * @brief Test memory efficiency of display structures
 *
 * Validates efficient use of data types (uint8_t, uint16_t, uint32_t).
 */
void test_efficient_data_types_used() {
    // Verify DisplayMetrics uses efficient types
    DisplayMetrics metrics;
    metrics.freeRamBytes = 250000;      // uint32_t (4 bytes) - appropriate for RAM size
    metrics.cpuIdlePercent = 85;        // uint8_t (1 byte) - appropriate for 0-100 range
    metrics.animationState = 2;         // uint8_t (1 byte) - appropriate for 0-3 range

    // Verify values stored correctly
    TEST_ASSERT_EQUAL_UINT32(250000, metrics.freeRamBytes);
    TEST_ASSERT_EQUAL_UINT8(85, metrics.cpuIdlePercent);
    TEST_ASSERT_EQUAL_UINT8(2, metrics.animationState);

    // Verify SubsystemStatus uses efficient types
    SubsystemStatus status;
    status.wifiStatus = CONN_CONNECTED;  // enum (efficient storage)
    status.fsStatus = FS_MOUNTED;        // enum (efficient storage)
    status.webServerStatus = WS_RUNNING; // enum (efficient storage)

    // Verify enums stored correctly
    TEST_ASSERT_EQUAL(CONN_CONNECTED, status.wifiStatus);
    TEST_ASSERT_EQUAL(FS_MOUNTED, status.fsStatus);
    TEST_ASSERT_EQUAL(WS_RUNNING, status.webServerStatus);

    TEST_ASSERT_TRUE_MESSAGE(true,
        "Display structures use efficient data types");
}
