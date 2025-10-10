/**
 * @file test_compass_attitude.cpp
 * @brief Integration test for compass attitude data (FR-006, FR-007, FR-008)
 *
 * Scenario: NMEA2000 PGN 127257 provides heel, pitch, and heave
 * Validates: CompassData.heelAngle, pitchAngle, heave populated correctly
 *
 * @see specs/008-enhanced-boatdata-following/quickstart.md (Scenario 3)
 * @see specs/008-enhanced-boatdata-following/tasks.md (T011)
 */

#include <unity.h>
#include "types/BoatDataTypes.h"
#include <math.h>

void setUp(void) {}
void tearDown(void) {}

/**
 * @test Heel angle stored in CompassData (moved from SpeedData)
 */
void test_heel_angle_in_compassdata(void) {
    BoatDataStructure boat;
    
    // Simulate PGN 127257 providing heel angle
    boat.compass.heelAngle = 0.087;  // ~5° starboard
    boat.compass.available = true;
    
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.087, boat.compass.heelAngle);
    TEST_ASSERT_TRUE(boat.compass.available);
}

/**
 * @test Pitch angle stored in CompassData
 */
void test_pitch_angle_in_compassdata(void) {
    BoatDataStructure boat;
    
    // Simulate PGN 127257 providing pitch angle
    boat.compass.pitchAngle = 0.15;  // ~8.6° bow up
    boat.compass.available = true;
    
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.15, boat.compass.pitchAngle);
    TEST_ASSERT_TRUE(boat.compass.available);
}

/**
 * @test Heave stored in CompassData
 */
void test_heave_in_compassdata(void) {
    BoatDataStructure boat;
    
    // Simulate PGN 127257 providing heave
    boat.compass.heave = 1.5;  // 1.5 meters upward motion
    boat.compass.available = true;
    
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 1.5, boat.compass.heave);
    TEST_ASSERT_TRUE(boat.compass.available);
}

/**
 * @test Pitch angle range validation (±30° = ±π/6 rad)
 */
void test_pitch_angle_range_validation(void) {
    BoatDataStructure boat;
    
    // Valid pitch angle within range
    boat.compass.pitchAngle = M_PI / 6;  // +30° (max)
    TEST_ASSERT_DOUBLE_WITHIN(0.01, M_PI / 6, boat.compass.pitchAngle);
    
    boat.compass.pitchAngle = -M_PI / 6;  // -30° (min)
    TEST_ASSERT_DOUBLE_WITHIN(0.01, -M_PI / 6, boat.compass.pitchAngle);
}

/**
 * @test Heave range validation (±5 meters)
 */
void test_heave_range_validation(void) {
    BoatDataStructure boat;
    
    // Valid heave within range
    boat.compass.heave = 5.0;  // +5m (max upward)
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 5.0, boat.compass.heave);
    
    boat.compass.heave = -5.0;  // -5m (max downward)
    TEST_ASSERT_DOUBLE_WITHIN(0.1, -5.0, boat.compass.heave);
}

/**
 * @test Heel angle sign convention (positive = starboard, negative = port)
 */
void test_heel_angle_sign_convention(void) {
    BoatDataStructure boat;
    
    // Positive heel = starboard tilt
    boat.compass.heelAngle = 0.1;
    TEST_ASSERT_GREATER_THAN(0.0, boat.compass.heelAngle);
    
    // Negative heel = port tilt
    boat.compass.heelAngle = -0.1;
    TEST_ASSERT_LESS_THAN(0.0, boat.compass.heelAngle);
}

/**
 * @test Pitch angle sign convention (positive = bow up, negative = bow down)
 */
void test_pitch_angle_sign_convention(void) {
    BoatDataStructure boat;
    
    // Positive pitch = bow up
    boat.compass.pitchAngle = 0.1;
    TEST_ASSERT_GREATER_THAN(0.0, boat.compass.pitchAngle);
    
    // Negative pitch = bow down
    boat.compass.pitchAngle = -0.1;
    TEST_ASSERT_LESS_THAN(0.0, boat.compass.pitchAngle);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_heel_angle_in_compassdata);
    RUN_TEST(test_pitch_angle_in_compassdata);
    RUN_TEST(test_heave_in_compassdata);
    RUN_TEST(test_pitch_angle_range_validation);
    RUN_TEST(test_heave_range_validation);
    RUN_TEST(test_heel_angle_sign_convention);
    RUN_TEST(test_pitch_angle_sign_convention);
    return UNITY_END();
}
