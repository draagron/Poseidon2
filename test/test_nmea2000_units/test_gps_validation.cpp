/**
 * @file test_gps_validation.cpp
 * @brief Unit tests for GPS data validation (latitude, longitude, COG, SOG, variation)
 *
 * Tests PGN 129025 (Position Rapid Update) and PGN 129026 (COG/SOG Rapid Update)
 * validation logic including clamping and range checks.
 *
 * @see src/utils/DataValidation.h
 * @see specs/010-nmea-2000-handling/tasks.md T039
 */

#include <unity.h>
#include "../../src/utils/DataValidation.h"
#include <math.h>

// ============================================================================
// Latitude Validation Tests
// ============================================================================

void test_latitude_valid_range() {
    // Valid latitude values
    TEST_ASSERT_TRUE(DataValidation::isValidLatitude(0.0));
    TEST_ASSERT_TRUE(DataValidation::isValidLatitude(37.7749));     // San Francisco
    TEST_ASSERT_TRUE(DataValidation::isValidLatitude(-33.8688));    // Sydney
    TEST_ASSERT_TRUE(DataValidation::isValidLatitude(90.0));        // North Pole
    TEST_ASSERT_TRUE(DataValidation::isValidLatitude(-90.0));       // South Pole
}

void test_latitude_out_of_range() {
    // Invalid latitude values
    TEST_ASSERT_FALSE(DataValidation::isValidLatitude(91.0));       // Too far north
    TEST_ASSERT_FALSE(DataValidation::isValidLatitude(-91.0));      // Too far south
    TEST_ASSERT_FALSE(DataValidation::isValidLatitude(150.0));      // Way out of range
    TEST_ASSERT_FALSE(DataValidation::isValidLatitude(-150.0));
}

void test_latitude_clamping() {
    // Clamping out-of-range latitudes
    TEST_ASSERT_EQUAL_DOUBLE(90.0, DataValidation::clampLatitude(91.0));
    TEST_ASSERT_EQUAL_DOUBLE(-90.0, DataValidation::clampLatitude(-91.0));
    TEST_ASSERT_EQUAL_DOUBLE(90.0, DataValidation::clampLatitude(150.0));
    TEST_ASSERT_EQUAL_DOUBLE(-90.0, DataValidation::clampLatitude(-150.0));

    // Valid latitudes should not be clamped
    TEST_ASSERT_EQUAL_DOUBLE(37.7749, DataValidation::clampLatitude(37.7749));
    TEST_ASSERT_EQUAL_DOUBLE(-33.8688, DataValidation::clampLatitude(-33.8688));
}

// ============================================================================
// Longitude Validation Tests
// ============================================================================

void test_longitude_valid_range() {
    // Valid longitude values
    TEST_ASSERT_TRUE(DataValidation::isValidLongitude(0.0));
    TEST_ASSERT_TRUE(DataValidation::isValidLongitude(-122.4194));  // San Francisco
    TEST_ASSERT_TRUE(DataValidation::isValidLongitude(151.2093));   // Sydney
    TEST_ASSERT_TRUE(DataValidation::isValidLongitude(180.0));      // Dateline
    TEST_ASSERT_TRUE(DataValidation::isValidLongitude(-180.0));     // Dateline
}

void test_longitude_out_of_range() {
    // Invalid longitude values
    TEST_ASSERT_FALSE(DataValidation::isValidLongitude(181.0));     // Too far east
    TEST_ASSERT_FALSE(DataValidation::isValidLongitude(-181.0));    // Too far west
    TEST_ASSERT_FALSE(DataValidation::isValidLongitude(200.0));
    TEST_ASSERT_FALSE(DataValidation::isValidLongitude(-200.0));
}

void test_longitude_clamping() {
    // Clamping out-of-range longitudes
    TEST_ASSERT_EQUAL_DOUBLE(180.0, DataValidation::clampLongitude(181.0));
    TEST_ASSERT_EQUAL_DOUBLE(-180.0, DataValidation::clampLongitude(-181.0));
    TEST_ASSERT_EQUAL_DOUBLE(180.0, DataValidation::clampLongitude(200.0));
    TEST_ASSERT_EQUAL_DOUBLE(-180.0, DataValidation::clampLongitude(-200.0));

    // Valid longitudes should not be clamped
    TEST_ASSERT_EQUAL_DOUBLE(-122.4194, DataValidation::clampLongitude(-122.4194));
    TEST_ASSERT_EQUAL_DOUBLE(151.2093, DataValidation::clampLongitude(151.2093));
}

// ============================================================================
// COG (Course Over Ground) Validation Tests
// ============================================================================

void test_cog_valid_range() {
    // Valid COG values [0, 2π]
    TEST_ASSERT_TRUE(DataValidation::isValidCOG(0.0));
    TEST_ASSERT_TRUE(DataValidation::isValidCOG(M_PI / 2.0));       // 90 degrees (East)
    TEST_ASSERT_TRUE(DataValidation::isValidCOG(M_PI));             // 180 degrees (South)
    TEST_ASSERT_TRUE(DataValidation::isValidCOG(3 * M_PI / 2.0));   // 270 degrees (West)
    TEST_ASSERT_TRUE(DataValidation::isValidCOG(2 * M_PI));         // 360 degrees (North)
}

void test_cog_wrapping() {
    // Wrapping negative angles
    double wrapped = DataValidation::wrapAngle2Pi(-M_PI / 2.0);     // -90 degrees
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 3 * M_PI / 2.0, wrapped);     // Should become 270 degrees

    // Wrapping angles > 2π
    wrapped = DataValidation::wrapAngle2Pi(2.5 * M_PI);             // 450 degrees
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 0.5 * M_PI, wrapped);         // Should become 90 degrees

    // Wrapping multiple rotations
    wrapped = DataValidation::wrapAngle2Pi(3 * 2 * M_PI + M_PI / 4.0);  // 3 rotations + 45 degrees
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, M_PI / 4.0, wrapped);         // Should become 45 degrees
}

// ============================================================================
// SOG (Speed Over Ground) Validation Tests
// ============================================================================

void test_sog_valid_range() {
    // Valid SOG values [0, 100] knots
    TEST_ASSERT_TRUE(DataValidation::isValidSOG(0.0));
    TEST_ASSERT_TRUE(DataValidation::isValidSOG(5.2));              // Typical sailing speed
    TEST_ASSERT_TRUE(DataValidation::isValidSOG(25.0));             // Fast powerboat
    TEST_ASSERT_TRUE(DataValidation::isValidSOG(50.0));             // Very fast
    TEST_ASSERT_TRUE(DataValidation::isValidSOG(100.0));            // Maximum allowed
}

void test_sog_out_of_range() {
    // Invalid SOG values
    TEST_ASSERT_FALSE(DataValidation::isValidSOG(-1.0));            // Negative speed
    TEST_ASSERT_FALSE(DataValidation::isValidSOG(101.0));           // Too fast
    TEST_ASSERT_FALSE(DataValidation::isValidSOG(200.0));           // Way too fast
}

void test_sog_clamping() {
    // Clamping out-of-range SOG
    TEST_ASSERT_EQUAL_DOUBLE(0.0, DataValidation::clampSOG(-1.0));
    TEST_ASSERT_EQUAL_DOUBLE(0.0, DataValidation::clampSOG(-10.0));
    TEST_ASSERT_EQUAL_DOUBLE(100.0, DataValidation::clampSOG(101.0));
    TEST_ASSERT_EQUAL_DOUBLE(100.0, DataValidation::clampSOG(200.0));

    // Valid SOG should not be clamped
    TEST_ASSERT_EQUAL_DOUBLE(5.2, DataValidation::clampSOG(5.2));
    TEST_ASSERT_EQUAL_DOUBLE(25.0, DataValidation::clampSOG(25.0));
}

// ============================================================================
// Magnetic Variation Validation Tests
// ============================================================================

void test_variation_valid_range() {
    // Valid variation values [-30°, 30°] = [-0.5236, 0.5236] radians
    TEST_ASSERT_TRUE(DataValidation::isValidVariation(0.0));
    TEST_ASSERT_TRUE(DataValidation::isValidVariation(0.1));        // ~5.7 degrees East
    TEST_ASSERT_TRUE(DataValidation::isValidVariation(-0.1));       // ~5.7 degrees West
    TEST_ASSERT_TRUE(DataValidation::isValidVariation(0.5));        // ~28.6 degrees East
    TEST_ASSERT_TRUE(DataValidation::isValidVariation(-0.5));       // ~28.6 degrees West
}

void test_variation_out_of_range() {
    // Invalid variation values (> ±30°)
    double max_variation = 30.0 * M_PI / 180.0;  // 30 degrees in radians
    TEST_ASSERT_FALSE(DataValidation::isValidVariation(max_variation + 0.01));
    TEST_ASSERT_FALSE(DataValidation::isValidVariation(-max_variation - 0.01));
    TEST_ASSERT_FALSE(DataValidation::isValidVariation(1.0));       // ~57 degrees
    TEST_ASSERT_FALSE(DataValidation::isValidVariation(-1.0));
}

void test_variation_clamping() {
    double max_variation = 30.0 * M_PI / 180.0;  // 30 degrees in radians

    // Clamping out-of-range variation
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, max_variation, DataValidation::clampVariation(1.0));
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, -max_variation, DataValidation::clampVariation(-1.0));
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, max_variation, DataValidation::clampVariation(0.6));
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, -max_variation, DataValidation::clampVariation(-0.6));

    // Valid variation should not be clamped
    TEST_ASSERT_EQUAL_DOUBLE(0.1, DataValidation::clampVariation(0.1));
    TEST_ASSERT_EQUAL_DOUBLE(-0.1, DataValidation::clampVariation(-0.1));
}

// ============================================================================
// Test Suite Setup
// ============================================================================

void setUp(void) {
    // Set up before each test (if needed)
}

void tearDown(void) {
    // Clean up after each test (if needed)
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // Latitude tests
    RUN_TEST(test_latitude_valid_range);
    RUN_TEST(test_latitude_out_of_range);
    RUN_TEST(test_latitude_clamping);

    // Longitude tests
    RUN_TEST(test_longitude_valid_range);
    RUN_TEST(test_longitude_out_of_range);
    RUN_TEST(test_longitude_clamping);

    // COG tests
    RUN_TEST(test_cog_valid_range);
    RUN_TEST(test_cog_wrapping);

    // SOG tests
    RUN_TEST(test_sog_valid_range);
    RUN_TEST(test_sog_out_of_range);
    RUN_TEST(test_sog_clamping);

    // Variation tests
    RUN_TEST(test_variation_valid_range);
    RUN_TEST(test_variation_out_of_range);
    RUN_TEST(test_variation_clamping);

    return UNITY_END();
}
