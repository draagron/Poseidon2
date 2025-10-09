/**
 * @file test_isystemmetrics.cpp
 * @brief Contract validation tests for ISystemMetrics interface
 *
 * Validates that MockSystemMetrics correctly implements the ISystemMetrics
 * interface contract. These tests ensure the interface behaves as expected.
 *
 * @version 1.0.0
 * @date 2025-10-09
 */

#include <unity.h>
#include "mocks/MockSystemMetrics.h"

// Global mock instance for tests
static MockSystemMetrics* mockMetrics = nullptr;

/**
 * @brief Test: getFreeHeapBytes() returns positive value within ESP32 limits
 *
 * ESP32 has 320KB RAM total. Free heap should be > 0 and <= 320KB.
 */
void test_getFreeHeapBytes_returns_positive_value() {
    mockMetrics = new MockSystemMetrics();
    mockMetrics->setFreeHeapBytes(250000);  // 244 KB

    uint32_t freeHeap = mockMetrics->getFreeHeapBytes();

    TEST_ASSERT_TRUE(freeHeap > 0);
    TEST_ASSERT_TRUE(freeHeap <= 320 * 1024);  // 320KB max
    TEST_ASSERT_EQUAL_UINT32(250000, freeHeap);

    delete mockMetrics;
}

/**
 * @brief Test: getSketchSizeBytes() returns positive value
 *
 * Sketch size should be > 0 (compiled code is never empty)
 */
void test_getSketchSizeBytes_returns_positive_value() {
    mockMetrics = new MockSystemMetrics();
    mockMetrics->setSketchSizeBytes(850000);  // 830 KB typical

    uint32_t sketchSize = mockMetrics->getSketchSizeBytes();

    TEST_ASSERT_TRUE(sketchSize > 0);
    TEST_ASSERT_EQUAL_UINT32(850000, sketchSize);

    delete mockMetrics;
}

/**
 * @brief Test: getFreeFlashBytes() returns valid value (>= 0)
 *
 * Free flash can be 0 if partition is full, but typically > 0
 */
void test_getFreeFlashBytes_returns_valid_value() {
    mockMetrics = new MockSystemMetrics();

    // Test typical case
    mockMetrics->setFreeFlashBytes(1000000);  // 976 KB
    uint32_t freeFlash = mockMetrics->getFreeFlashBytes();
    TEST_ASSERT_TRUE(freeFlash >= 0);
    TEST_ASSERT_EQUAL_UINT32(1000000, freeFlash);

    // Test edge case: partition full
    mockMetrics->setFreeFlashBytes(0);
    freeFlash = mockMetrics->getFreeFlashBytes();
    TEST_ASSERT_EQUAL_UINT32(0, freeFlash);

    delete mockMetrics;
}

/**
 * @brief Test: getCpuIdlePercent() returns value in range 0-100
 *
 * CPU idle percentage must be between 0% (fully loaded) and 100% (idle)
 */
void test_getCpuIdlePercent_returns_0_to_100() {
    mockMetrics = new MockSystemMetrics();

    // Test typical case
    mockMetrics->setCpuIdlePercent(87);
    uint8_t idle = mockMetrics->getCpuIdlePercent();
    TEST_ASSERT_TRUE(idle >= 0 && idle <= 100);
    TEST_ASSERT_EQUAL_UINT8(87, idle);

    // Test edge case: fully loaded
    mockMetrics->setCpuIdlePercent(0);
    idle = mockMetrics->getCpuIdlePercent();
    TEST_ASSERT_EQUAL_UINT8(0, idle);

    // Test edge case: completely idle
    mockMetrics->setCpuIdlePercent(100);
    idle = mockMetrics->getCpuIdlePercent();
    TEST_ASSERT_EQUAL_UINT8(100, idle);

    delete mockMetrics;
}

/**
 * @brief Test: getMillis() returns monotonically increasing value
 *
 * millis() timestamp should never decrease (except after ~49 day wraparound)
 */
void test_getMillis_returns_increasing_value() {
    mockMetrics = new MockSystemMetrics();

    mockMetrics->setMillis(1000);
    unsigned long first = mockMetrics->getMillis();

    mockMetrics->setMillis(2000);
    unsigned long second = mockMetrics->getMillis();

    TEST_ASSERT_TRUE(second >= first);
    TEST_ASSERT_EQUAL_UINT32(1000, first);
    TEST_ASSERT_EQUAL_UINT32(2000, second);

    delete mockMetrics;
}
