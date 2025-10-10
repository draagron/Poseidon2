/**
 * @file test_display_formatter.cpp
 * @brief Unit tests for DisplayFormatter component
 *
 * Tests string formatting functions for display output.
 * Note: DisplayLayout.h already provides inline formatting functions,
 * but this validates they work correctly.
 *
 * @version 1.0.0
 * @date 2025-10-09
 */

#include <unity.h>
#include "utils/DisplayLayout.h"
#include <string.h>

/**
 * @brief Test: formatBytes() converts bytes to KB
 */
void test_formatBytes_converts_to_KB() {
    char buffer[10];

    // 250000 bytes = 244 KB (250000 / 1024 = 244.140625)
    formatBytes(250000, buffer);
    TEST_ASSERT_TRUE(strstr(buffer, "244") != nullptr);
    TEST_ASSERT_TRUE(strstr(buffer, "KB") != nullptr);
}

/**
 * @brief Test: formatBytes() handles small values
 */
void test_formatBytes_handles_small_values() {
    char buffer[10];

    // 500 bytes = 0 KB (rounds down)
    formatBytes(500, buffer);
    TEST_ASSERT_TRUE(strstr(buffer, "0") != nullptr);
    TEST_ASSERT_TRUE(strstr(buffer, "KB") != nullptr);
}

/**
 * @brief Test: formatBytes() handles large values
 */
void test_formatBytes_handles_large_values() {
    char buffer[10];

    // 1920000 bytes = 1875 KB (1920000 / 1024 = 1875)
    formatBytes(1920000, buffer);
    TEST_ASSERT_TRUE(strstr(buffer, "1875") != nullptr);
    TEST_ASSERT_TRUE(strstr(buffer, "KB") != nullptr);
}

/**
 * @brief Test: formatPercent() formats correctly
 */
void test_formatPercent_formats_correctly() {
    char buffer[5];

    formatPercent(87, buffer);
    TEST_ASSERT_TRUE(strstr(buffer, "87") != nullptr);
    TEST_ASSERT_TRUE(strstr(buffer, "%") != nullptr);
}

/**
 * @brief Test: formatPercent() handles 0 and 100
 */
void test_formatPercent_handles_0_and_100() {
    char buffer[5];

    // Test 0%
    formatPercent(0, buffer);
    TEST_ASSERT_TRUE(strstr(buffer, "0") != nullptr);
    TEST_ASSERT_TRUE(strstr(buffer, "%") != nullptr);

    // Test 100%
    formatPercent(100, buffer);
    TEST_ASSERT_TRUE(strstr(buffer, "100") != nullptr);
    TEST_ASSERT_TRUE(strstr(buffer, "%") != nullptr);
}

/**
 * @brief Test: formatIPAddress() formats correctly
 */
void test_formatIPAddress_formats_correctly() {
    char buffer[22];

    formatIPAddress("192.168.1.100", buffer);
    TEST_ASSERT_TRUE(strstr(buffer, "IP:") != nullptr);
    TEST_ASSERT_TRUE(strstr(buffer, "192.168.1.100") != nullptr);
}

/**
 * @brief Test: formatIPAddress() handles empty string
 */
void test_formatIPAddress_handles_empty() {
    char buffer[22];

    formatIPAddress("", buffer);
    TEST_ASSERT_TRUE(strstr(buffer, "IP:") != nullptr);
    TEST_ASSERT_TRUE(strstr(buffer, "---") != nullptr);
}
