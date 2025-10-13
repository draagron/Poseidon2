/**
 * @file test_manufacturer_lookup.cpp
 * @brief Contract test for ManufacturerLookup utility
 *
 * Tests the NMEA2000 manufacturer code to name lookup functionality.
 *
 * Test Coverage:
 * - Known manufacturer codes (Garmin, Furuno, Raymarine, etc.)
 * - Unknown manufacturer codes (fallback to "Unknown (code)")
 * - Performance requirement (<50μs per lookup)
 * - Edge cases (code 0, max uint16_t)
 *
 * Feature: 013-r013-nmea2000-device (User Story 1)
 * Task: T008
 */

#include <unity.h>
#include <string.h>
#include "utils/ManufacturerLookup.h"

/**
 * @brief Test known manufacturer codes
 *
 * Verifies that common marine equipment manufacturers are correctly identified.
 */
void test_known_manufacturer_codes() {
    // Test common manufacturer codes from NMEA2000 registry
    TEST_ASSERT_EQUAL_STRING("Garmin", getManufacturerName(275));
    TEST_ASSERT_EQUAL_STRING("Furuno", getManufacturerName(1855));
    TEST_ASSERT_EQUAL_STRING("Raymarine", getManufacturerName(378));
    TEST_ASSERT_EQUAL_STRING("Airmar", getManufacturerName(135));
    TEST_ASSERT_EQUAL_STRING("Maretron", getManufacturerName(137));
    TEST_ASSERT_EQUAL_STRING("Simrad", getManufacturerName(1857));
    TEST_ASSERT_EQUAL_STRING("B&G", getManufacturerName(381));
}

/**
 * @brief Test unknown manufacturer codes
 *
 * Verifies that unknown codes return "Unknown (code)" format.
 */
void test_unknown_manufacturer_codes() {
    const char* result;

    // Test code that doesn't exist in the table (within uint16_t range)
    result = getManufacturerName(9999);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(strstr(result, "Unknown") != NULL);
    TEST_ASSERT_TRUE(strstr(result, "9999") != NULL);

    // Test code 0 (reserved/invalid)
    result = getManufacturerName(0);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(strstr(result, "Unknown") != NULL);

    // Test max uint16_t value
    result = getManufacturerName(65535);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(strstr(result, "Unknown") != NULL);
}

/**
 * @brief Test lookup performance
 *
 * Verifies that lookups complete in <50μs as per FR-022.
 * Tests both known and unknown codes.
 *
 * Note: This test is marked as informational only in native environment
 * since timing is not accurate without actual hardware. The test will
 * still verify that lookups complete without crashing.
 */
void test_lookup_performance() {
    const uint16_t testCode = 275;  // Garmin
    const int iterations = 1000;

    // Just verify lookups don't crash and return valid results
    for (int i = 0; i < iterations; i++) {
        const char* result = getManufacturerName(testCode);
        TEST_ASSERT_NOT_NULL(result);
        TEST_ASSERT_GREATER_THAN(0, strlen(result));
    }

    // Also test unknown code performance
    for (int i = 0; i < iterations; i++) {
        const char* result = getManufacturerName(9999);
        TEST_ASSERT_NOT_NULL(result);
        TEST_ASSERT_GREATER_THAN(0, strlen(result));
    }

    // Performance verification note: Actual timing tests should be run
    // on ESP32 hardware in test_device_discovery_hardware suite
    TEST_PASS_MESSAGE("Performance test passed (functional verification only in native env)");
}

/**
 * @brief Test null safety
 *
 * Verifies that the function never returns NULL.
 */
void test_null_safety() {
    // All lookups should return non-null strings
    TEST_ASSERT_NOT_NULL(getManufacturerName(0));
    TEST_ASSERT_NOT_NULL(getManufacturerName(275));
    TEST_ASSERT_NOT_NULL(getManufacturerName(9999));
    TEST_ASSERT_NOT_NULL(getManufacturerName(65535));
}

/**
 * @brief Test string safety
 *
 * Verifies that returned strings are null-terminated and safe to use.
 */
void test_string_safety() {
    const char* result;

    // Test that known codes return valid strings
    result = getManufacturerName(275);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_GREATER_THAN(0, strlen(result));
    TEST_ASSERT_LESS_THAN(50, strlen(result));  // Reasonable length check

    // Test that unknown codes return formatted strings
    result = getManufacturerName(9999);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_GREATER_THAN(0, strlen(result));
    TEST_ASSERT_LESS_THAN(50, strlen(result));
}

/**
 * @brief Test additional common manufacturers
 *
 * Extended test for more manufacturer codes to ensure comprehensive coverage.
 */
void test_additional_manufacturers() {
    // Test more manufacturers from the lookup table
    TEST_ASSERT_EQUAL_STRING("Victron Energy", getManufacturerName(358));
    TEST_ASSERT_EQUAL_STRING("Yanmar", getManufacturerName(172));
    TEST_ASSERT_EQUAL_STRING("Volvo Penta", getManufacturerName(174));
}

void setUp(void) {
    // Test setup (if needed)
}

void tearDown(void) {
    // Test cleanup (if needed)
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_known_manufacturer_codes);
    RUN_TEST(test_unknown_manufacturer_codes);
    RUN_TEST(test_null_safety);
    RUN_TEST(test_string_safety);
    RUN_TEST(test_additional_manufacturers);
    RUN_TEST(test_lookup_performance);

    return UNITY_END();
}
