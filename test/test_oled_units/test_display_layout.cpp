/**
 * @file test_display_layout.cpp
 * @brief Unit tests for DisplayLayout utility functions
 *
 * Tests text positioning calculations and formatting helpers.
 *
 * @version 1.0.0
 * @date 2025-10-09
 */

#include <unity.h>
#include "utils/DisplayLayout.h"
#include <string.h>

/**
 * @brief Test: getLineY() returns correct Y positions
 *
 * Line 0 = y=0, Line 1 = y=10, Line 2 = y=20, ..., Line 5 = y=50
 */
void test_getLineY_returns_correct_positions() {
    TEST_ASSERT_EQUAL_UINT8(0, getLineY(0));
    TEST_ASSERT_EQUAL_UINT8(10, getLineY(1));
    TEST_ASSERT_EQUAL_UINT8(20, getLineY(2));
    TEST_ASSERT_EQUAL_UINT8(30, getLineY(3));
    TEST_ASSERT_EQUAL_UINT8(40, getLineY(4));
    TEST_ASSERT_EQUAL_UINT8(50, getLineY(5));
}

/**
 * @brief Test: getLineY() handles invalid line (clamps to last line)
 */
void test_getLineY_handles_invalid_line() {
    // Line 6 is out of bounds (max is 5), should clamp to line 5 (y=50)
    TEST_ASSERT_EQUAL_UINT8(50, getLineY(6));
    TEST_ASSERT_EQUAL_UINT8(50, getLineY(10));  // Way out of bounds
}

/**
 * @brief Test: formatFlashUsage() formats correctly
 *
 * Example: 850000 used, 1920000 total → "830/1875KB"
 */
void test_formatFlashUsage_formats_correctly() {
    char buffer[20];

    formatFlashUsage(850000, 1920000, buffer);

    // 850000 / 1024 = 830KB, 1920000 / 1024 = 1875KB
    TEST_ASSERT_TRUE(strstr(buffer, "830") != nullptr);
    TEST_ASSERT_TRUE(strstr(buffer, "1875") != nullptr);
    TEST_ASSERT_TRUE(strstr(buffer, "KB") != nullptr);
    TEST_ASSERT_TRUE(strstr(buffer, "/") != nullptr);
}

/**
 * @brief Test: getAnimationIcon() cycles correctly
 *
 * State 0 = '/', 1 = '-', 2 = '\', 3 = '|'
 */
void test_getAnimationIcon_cycles_correctly() {
    TEST_ASSERT_EQUAL_CHAR('/', getAnimationIcon(0));
    TEST_ASSERT_EQUAL_CHAR('-', getAnimationIcon(1));
    TEST_ASSERT_EQUAL_CHAR('\\', getAnimationIcon(2));
    TEST_ASSERT_EQUAL_CHAR('|', getAnimationIcon(3));

    // Test wraparound (state % 4)
    TEST_ASSERT_EQUAL_CHAR('/', getAnimationIcon(4));
    TEST_ASSERT_EQUAL_CHAR('-', getAnimationIcon(5));
}
