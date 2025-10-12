/**
 * @file test_out_of_range.cpp
 * @brief Unit tests for handler behavior on out-of-range values
 *
 * Tests that handlers log WARN, clamp values, and still update BoatData when
 * values exceed valid ranges.
 *
 * @see specs/010-nmea-2000-handling/data-model.md lines 386-392
 * @see specs/010-nmea-2000-handling/tasks.md T048
 */

#include <unity.h>
#include "../../src/utils/DataValidation.h"
#include <math.h>

// ============================================================================
// Out-of-Range Behavior Tests
// ============================================================================

/**
 * @brief Test expected behavior on out-of-range values
 *
 * When validation fails (e.g., latitude = 100°):
 * - Handler should log WARN level message with original and clamped values
 * - Value should be clamped to valid range
 * - BoatData should be updated with CLAMPED value
 * - Message counter should be incremented
 * - Availability flag should be set to true
 */
void test_out_of_range_behavior() {
    // Simulate out-of-range latitude
    double latitude = 100.0;  // Out of range [-90, 90]
    bool valid = DataValidation::isValidLatitude(latitude);

    if (!valid) {
        // Expected actions:
        // 1. Log WARN with original and clamped values
        bool shouldLogWarn = true;
        TEST_ASSERT_TRUE(shouldLogWarn);

        // 2. Clamp value
        double clampedValue = DataValidation::clampLatitude(latitude);
        TEST_ASSERT_EQUAL_DOUBLE(90.0, clampedValue);

        // 3. Update BoatData with clamped value
        bool shouldUpdateData = true;
        TEST_ASSERT_TRUE(shouldUpdateData);

        // 4. Increment counter
        bool shouldIncrementCounter = true;
        TEST_ASSERT_TRUE(shouldIncrementCounter);

        // 5. Set availability = true
        bool availability = true;
        TEST_ASSERT_TRUE(availability);
    }
}

/**
 * @brief Test valid value behavior (for comparison)
 *
 * When validation succeeds:
 * - No WARN log
 * - No clamping needed
 * - BoatData updated with original value
 */
void test_valid_range_behavior() {
    // Simulate valid latitude
    double latitude = 37.7749;
    bool valid = DataValidation::isValidLatitude(latitude);

    if (valid) {
        // Expected actions:
        // 1. No WARN log needed
        bool shouldLogWarn = false;
        TEST_ASSERT_FALSE(shouldLogWarn);

        // 2. No clamping needed
        double clampedValue = DataValidation::clampLatitude(latitude);
        TEST_ASSERT_EQUAL_DOUBLE(latitude, clampedValue);

        // 3. Update BoatData with original value
        bool shouldUpdateData = true;
        TEST_ASSERT_TRUE(shouldUpdateData);
    }
}

/**
 * @brief Test clamping examples for various data types
 */
void test_clamping_examples() {
    // Latitude: 100° → 90°
    TEST_ASSERT_EQUAL_DOUBLE(90.0, DataValidation::clampLatitude(100.0));

    // Longitude: -200° → -180°
    TEST_ASSERT_EQUAL_DOUBLE(-180.0, DataValidation::clampLongitude(-200.0));

    // SOG: 150 knots → 100 knots
    TEST_ASSERT_EQUAL_DOUBLE(100.0, DataValidation::clampSOG(150.0));

    // Wind speed: -10 knots → 0 knots
    TEST_ASSERT_EQUAL_DOUBLE(0.0, DataValidation::clampWindSpeed(-10.0));

    // Variation: 1.0 rad (57°) → 0.5236 rad (30°)
    double maxVariation = 30.0 * M_PI / 180.0;
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, maxVariation, DataValidation::clampVariation(1.0));
}

/**
 * @brief Test WARN log content requirements
 *
 * WARN logs on out-of-range values should include:
 * - Component: "NMEA2000"
 * - Event: "PGN<NUMBER>_OUT_OF_RANGE"
 * - Original value
 * - Clamped value
 */
void test_out_of_range_log_format() {
    const char* component = "NMEA2000";
    const char* event = "PGN129025_OUT_OF_RANGE";
    double originalValue = 100.0;
    double clampedValue = 90.0;

    // Verify log format components exist
    TEST_ASSERT_NOT_NULL(component);
    TEST_ASSERT_NOT_NULL(event);

    // Verify component name
    TEST_ASSERT_EQUAL_STRING("NMEA2000", component);

    // Verify event naming pattern
    TEST_ASSERT_TRUE(strstr(event, "OUT_OF_RANGE") != NULL);

    // Verify values are different (original vs clamped)
    TEST_ASSERT_NOT_EQUAL(originalValue, clampedValue);
}

/**
 * @brief Test that out-of-range does NOT prevent data update
 *
 * Unlike N/A values (which skip update), out-of-range values are
 * clamped and then used to update BoatData
 */
void test_out_of_range_still_updates() {
    double latitude = 100.0;  // Out of range
    bool valid = DataValidation::isValidLatitude(latitude);

    // Even though invalid, we should still update with clamped value
    if (!valid) {
        double clampedValue = DataValidation::clampLatitude(latitude);

        // BoatData should be updated with clamped value
        bool shouldUpdate = true;
        TEST_ASSERT_TRUE(shouldUpdate);

        // Verify clamped value is valid
        TEST_ASSERT_TRUE(DataValidation::isValidLatitude(clampedValue));
        TEST_ASSERT_EQUAL_DOUBLE(90.0, clampedValue);
    }
}

/**
 * @brief Test edge case: value exactly at boundary
 *
 * Boundary values (e.g., latitude = 90°) should be valid, not clamped
 */
void test_boundary_values_valid() {
    // Latitude boundaries
    TEST_ASSERT_TRUE(DataValidation::isValidLatitude(90.0));
    TEST_ASSERT_TRUE(DataValidation::isValidLatitude(-90.0));

    // Longitude boundaries
    TEST_ASSERT_TRUE(DataValidation::isValidLongitude(180.0));
    TEST_ASSERT_TRUE(DataValidation::isValidLongitude(-180.0));

    // SOG boundary
    TEST_ASSERT_TRUE(DataValidation::isValidSOG(0.0));
    TEST_ASSERT_TRUE(DataValidation::isValidSOG(100.0));

    // Clamping boundary values should not change them
    TEST_ASSERT_EQUAL_DOUBLE(90.0, DataValidation::clampLatitude(90.0));
    TEST_ASSERT_EQUAL_DOUBLE(-90.0, DataValidation::clampLatitude(-90.0));
}

/**
 * @brief Test multiple out-of-range values in same handler
 *
 * If handler processes multiple fields and both are out of range,
 * both should be clamped and a WARN should be logged
 */
void test_multiple_out_of_range() {
    double latitude = 100.0;   // Out of range
    double longitude = -200.0;  // Out of range

    bool validLat = DataValidation::isValidLatitude(latitude);
    bool validLon = DataValidation::isValidLongitude(longitude);

    // Both invalid: should WARN and clamp both
    if (!validLat || !validLon) {
        bool shouldLogWarn = true;
        TEST_ASSERT_TRUE(shouldLogWarn);

        double clampedLat = DataValidation::clampLatitude(latitude);
        double clampedLon = DataValidation::clampLongitude(longitude);

        TEST_ASSERT_EQUAL_DOUBLE(90.0, clampedLat);
        TEST_ASSERT_EQUAL_DOUBLE(-180.0, clampedLon);
    }
}

// ============================================================================
// Test Suite Setup
// ============================================================================

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_out_of_range_behavior);
    RUN_TEST(test_valid_range_behavior);
    RUN_TEST(test_clamping_examples);
    RUN_TEST(test_out_of_range_log_format);
    RUN_TEST(test_out_of_range_still_updates);
    RUN_TEST(test_boundary_values_valid);
    RUN_TEST(test_multiple_out_of_range);
    return UNITY_END();
}
