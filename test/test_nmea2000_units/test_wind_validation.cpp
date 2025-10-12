/**
 * @file test_wind_validation.cpp
 * @brief Unit tests for wind data validation (angle sign convention, speed conversion)
 *
 * Tests PGN 130306 (Wind Data) validation logic including wind angle wrapping,
 * speed validation, and m/s to knots conversion.
 *
 * @see src/utils/DataValidation.h
 * @see specs/010-nmea-2000-handling/tasks.md T040
 */

#include <unity.h>
#include "../../src/utils/DataValidation.h"
#include <math.h>

// ============================================================================
// Wind Angle Validation Tests (Sign Convention)
// ============================================================================

void test_wind_angle_sign_convention() {
    // Wind angle sign convention: positive = starboard, negative = port
    // Range: [-π, π] radians

    // Starboard side (positive angles)
    double angle = M_PI / 4.0;  // 45 degrees starboard
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, M_PI / 4.0, DataValidation::wrapWindAngle(angle));

    angle = M_PI / 2.0;  // 90 degrees starboard
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, M_PI / 2.0, DataValidation::wrapWindAngle(angle));

    // Port side (negative angles)
    angle = -M_PI / 4.0;  // 45 degrees port
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, -M_PI / 4.0, DataValidation::wrapWindAngle(angle));

    angle = -M_PI / 2.0;  // 90 degrees port
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, -M_PI / 2.0, DataValidation::wrapWindAngle(angle));

    // Directly aft (±180 degrees)
    angle = M_PI;  // 180 degrees
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, M_PI, DataValidation::wrapWindAngle(angle));

    angle = -M_PI;  // -180 degrees
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, -M_PI, DataValidation::wrapWindAngle(angle));
}

void test_wind_angle_wrapping() {
    // Wrapping angles > π (e.g., 270° = -90°)
    double angle = 3 * M_PI / 2.0;  // 270 degrees
    double wrapped = DataValidation::wrapWindAngle(angle);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, -M_PI / 2.0, wrapped);  // Should become -90 degrees

    // Wrapping angles < -π (e.g., -270° = 90°)
    angle = -3 * M_PI / 2.0;  // -270 degrees
    wrapped = DataValidation::wrapWindAngle(angle);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, M_PI / 2.0, wrapped);  // Should become 90 degrees

    // Wrapping angles > 2π (e.g., 450° = 90°)
    angle = 2.5 * M_PI;  // 450 degrees
    wrapped = DataValidation::wrapWindAngle(angle);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, M_PI / 2.0, wrapped);  // Should become 90 degrees

    // Wrapping angles < -2π (e.g., -450° = -90°)
    angle = -2.5 * M_PI;  // -450 degrees
    wrapped = DataValidation::wrapWindAngle(angle);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, -M_PI / 2.0, wrapped);  // Should become -90 degrees
}

void test_wind_angle_edge_cases() {
    // Zero angle (wind from bow)
    TEST_ASSERT_EQUAL_DOUBLE(0.0, DataValidation::wrapWindAngle(0.0));

    // Full rotation should wrap to zero
    double wrapped = DataValidation::wrapWindAngle(2 * M_PI);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 0.0, wrapped);

    // Negative full rotation should wrap to zero
    wrapped = DataValidation::wrapWindAngle(-2 * M_PI);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 0.0, wrapped);
}

// ============================================================================
// Wind Speed Validation Tests
// ============================================================================

void test_wind_speed_valid_range() {
    // Valid wind speed values [0, 100] knots
    TEST_ASSERT_TRUE(DataValidation::isValidWindSpeed(0.0));
    TEST_ASSERT_TRUE(DataValidation::isValidWindSpeed(10.0));       // Light breeze
    TEST_ASSERT_TRUE(DataValidation::isValidWindSpeed(25.0));       // Strong breeze
    TEST_ASSERT_TRUE(DataValidation::isValidWindSpeed(50.0));       // Storm conditions
    TEST_ASSERT_TRUE(DataValidation::isValidWindSpeed(100.0));      // Maximum allowed
}

void test_wind_speed_out_of_range() {
    // Invalid wind speed values
    TEST_ASSERT_FALSE(DataValidation::isValidWindSpeed(-1.0));      // Negative speed
    TEST_ASSERT_FALSE(DataValidation::isValidWindSpeed(-10.0));
    TEST_ASSERT_FALSE(DataValidation::isValidWindSpeed(101.0));     // Too high
    TEST_ASSERT_FALSE(DataValidation::isValidWindSpeed(200.0));     // Way too high
}

void test_wind_speed_clamping() {
    // Clamping out-of-range wind speeds
    TEST_ASSERT_EQUAL_DOUBLE(0.0, DataValidation::clampWindSpeed(-1.0));
    TEST_ASSERT_EQUAL_DOUBLE(0.0, DataValidation::clampWindSpeed(-10.0));
    TEST_ASSERT_EQUAL_DOUBLE(100.0, DataValidation::clampWindSpeed(101.0));
    TEST_ASSERT_EQUAL_DOUBLE(100.0, DataValidation::clampWindSpeed(200.0));

    // Valid wind speeds should not be clamped
    TEST_ASSERT_EQUAL_DOUBLE(10.0, DataValidation::clampWindSpeed(10.0));
    TEST_ASSERT_EQUAL_DOUBLE(25.0, DataValidation::clampWindSpeed(25.0));
    TEST_ASSERT_EQUAL_DOUBLE(50.0, DataValidation::clampWindSpeed(50.0));
}

// ============================================================================
// Speed Conversion Tests (m/s → knots)
// ============================================================================

void test_mps_to_knots_conversion() {
    // Conversion factor: 1 m/s = 1.9438444924406 knots

    // Zero speed
    TEST_ASSERT_EQUAL_DOUBLE(0.0, DataValidation::mpsToKnots(0.0));

    // 1 m/s = 1.9438444924406 knots
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 1.9438444924406, DataValidation::mpsToKnots(1.0));

    // 5 m/s ≈ 9.72 knots (typical sailing wind)
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 9.72, DataValidation::mpsToKnots(5.0));

    // 10 m/s ≈ 19.44 knots (strong breeze)
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 19.44, DataValidation::mpsToKnots(10.0));

    // 25 m/s ≈ 48.60 knots (storm conditions)
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 48.60, DataValidation::mpsToKnots(25.0));
}

void test_mps_to_knots_precision() {
    // Test conversion precision with known values

    // 2.5 m/s ≈ 4.86 knots
    double knots = DataValidation::mpsToKnots(2.5);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 4.86, knots);

    // 7.5 m/s ≈ 14.58 knots
    knots = DataValidation::mpsToKnots(7.5);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 14.58, knots);

    // 12.5 m/s ≈ 24.30 knots
    knots = DataValidation::mpsToKnots(12.5);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 24.30, knots);
}

// ============================================================================
// Integration Tests (Wind Angle + Speed)
// ============================================================================

void test_wind_data_starboard_beam() {
    // Wind from starboard beam (90 degrees = π/2 radians)
    double angle = M_PI / 2.0;
    double speedMps = 10.0;  // 10 m/s

    // Validate angle (should be valid)
    double wrappedAngle = DataValidation::wrapWindAngle(angle);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, M_PI / 2.0, wrappedAngle);

    // Convert speed to knots
    double speedKnots = DataValidation::mpsToKnots(speedMps);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 19.44, speedKnots);

    // Validate speed (should be valid)
    TEST_ASSERT_TRUE(DataValidation::isValidWindSpeed(speedKnots));
}

void test_wind_data_port_quarter() {
    // Wind from port quarter (135 degrees aft port = -135° = -3π/4 radians)
    double angle = -3 * M_PI / 4.0;
    double speedMps = 7.5;  // 7.5 m/s

    // Validate angle (should be valid, no wrapping needed)
    double wrappedAngle = DataValidation::wrapWindAngle(angle);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, -3 * M_PI / 4.0, wrappedAngle);

    // Convert speed to knots
    double speedKnots = DataValidation::mpsToKnots(speedMps);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 14.58, speedKnots);

    // Validate speed (should be valid)
    TEST_ASSERT_TRUE(DataValidation::isValidWindSpeed(speedKnots));
}

void test_wind_data_headwind() {
    // Wind from bow (0 degrees)
    double angle = 0.0;
    double speedMps = 15.0;  // 15 m/s (strong headwind)

    // Validate angle (should be valid)
    double wrappedAngle = DataValidation::wrapWindAngle(angle);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, wrappedAngle);

    // Convert speed to knots
    double speedKnots = DataValidation::mpsToKnots(speedMps);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 29.16, speedKnots);

    // Validate speed (should be valid)
    TEST_ASSERT_TRUE(DataValidation::isValidWindSpeed(speedKnots));
}

void test_wind_data_tailwind() {
    // Wind from stern (180 degrees = π radians)
    double angle = M_PI;
    double speedMps = 5.0;  // 5 m/s (light tailwind)

    // Validate angle (should be valid)
    double wrappedAngle = DataValidation::wrapWindAngle(angle);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, M_PI, wrappedAngle);

    // Convert speed to knots
    double speedKnots = DataValidation::mpsToKnots(speedMps);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 9.72, speedKnots);

    // Validate speed (should be valid)
    TEST_ASSERT_TRUE(DataValidation::isValidWindSpeed(speedKnots));
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

    // Wind angle tests
    RUN_TEST(test_wind_angle_sign_convention);
    RUN_TEST(test_wind_angle_wrapping);
    RUN_TEST(test_wind_angle_edge_cases);

    // Wind speed tests
    RUN_TEST(test_wind_speed_valid_range);
    RUN_TEST(test_wind_speed_out_of_range);
    RUN_TEST(test_wind_speed_clamping);

    // Speed conversion tests
    RUN_TEST(test_mps_to_knots_conversion);
    RUN_TEST(test_mps_to_knots_precision);

    // Integration tests
    RUN_TEST(test_wind_data_starboard_beam);
    RUN_TEST(test_wind_data_port_quarter);
    RUN_TEST(test_wind_data_headwind);
    RUN_TEST(test_wind_data_tailwind);

    return UNITY_END();
}
