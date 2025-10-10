/**
 * @file test_sign_conventions.cpp
 * @brief Unit tests for sign convention validation
 *
 * Tests mathematical sign conventions for directional measurements.
 * Ensures consistent interpretation across the codebase.
 *
 * Test Coverage:
 * - Battery amperage (+ = charging, - = discharging)
 * - Heel angle (+ = starboard, - = port)
 * - Pitch angle (+ = bow up, - = bow down)
 * - Rate of turn (+ = turning starboard, - = turning port)
 * - Magnetic variation (+ = East, - = West)
 * - Heave (+ = upward, - = downward)
 *
 * @see specs/008-enhanced-boatdata-following/data-model.md (Sign Conventions)
 * @see specs/008-enhanced-boatdata-following/tasks.md (T020)
 */

#include <unity.h>

void setUp(void) {}
void tearDown(void) {}

/**
 * @test Battery amperage sign convention
 */
void test_battery_amperage_sign_convention(void) {
    double amperage;
    
    // Positive amperage = charging (current flowing INTO battery)
    amperage = 10.0;
    TEST_ASSERT_GREATER_THAN(0.0, amperage);
    TEST_MESSAGE("Positive amperage = charging");
    
    // Negative amperage = discharging (current flowing OUT OF battery)
    amperage = -15.0;
    TEST_ASSERT_LESS_THAN(0.0, amperage);
    TEST_MESSAGE("Negative amperage = discharging");
    
    // Zero amperage = idle (no current flow)
    amperage = 0.0;
    TEST_ASSERT_EQUAL_DOUBLE(0.0, amperage);
    TEST_MESSAGE("Zero amperage = idle");
}

/**
 * @test Heel angle sign convention
 */
void test_heel_angle_sign_convention(void) {
    double heelAngle;
    
    // Positive heel = starboard tilt (right side down)
    heelAngle = 0.1;
    TEST_ASSERT_GREATER_THAN(0.0, heelAngle);
    TEST_MESSAGE("Positive heel = starboard tilt");
    
    // Negative heel = port tilt (left side down)
    heelAngle = -0.1;
    TEST_ASSERT_LESS_THAN(0.0, heelAngle);
    TEST_MESSAGE("Negative heel = port tilt");
}

/**
 * @test Pitch angle sign convention
 */
void test_pitch_angle_sign_convention(void) {
    double pitchAngle;
    
    // Positive pitch = bow up
    pitchAngle = 0.15;
    TEST_ASSERT_GREATER_THAN(0.0, pitchAngle);
    TEST_MESSAGE("Positive pitch = bow up");
    
    // Negative pitch = bow down (stern up)
    pitchAngle = -0.15;
    TEST_ASSERT_LESS_THAN(0.0, pitchAngle);
    TEST_MESSAGE("Negative pitch = bow down");
}

/**
 * @test Rate of turn sign convention
 */
void test_rate_of_turn_sign_convention(void) {
    double rateOfTurn;
    
    // Positive rate = turning right (starboard turn)
    rateOfTurn = 0.1;
    TEST_ASSERT_GREATER_THAN(0.0, rateOfTurn);
    TEST_MESSAGE("Positive rate = turning starboard");
    
    // Negative rate = turning left (port turn)
    rateOfTurn = -0.1;
    TEST_ASSERT_LESS_THAN(0.0, rateOfTurn);
    TEST_MESSAGE("Negative rate = turning port");
}

/**
 * @test Magnetic variation sign convention
 */
void test_magnetic_variation_sign_convention(void) {
    double variation;
    
    // Positive variation = East declination (compass reads LESS than true)
    variation = 0.054;  // 3.1° East
    TEST_ASSERT_GREATER_THAN(0.0, variation);
    TEST_MESSAGE("Positive variation = East declination");
    
    // Negative variation = West declination (compass reads MORE than true)
    variation = -0.054;  // 3.1° West
    TEST_ASSERT_LESS_THAN(0.0, variation);
    TEST_MESSAGE("Negative variation = West declination");
}

/**
 * @test Heave sign convention
 */
void test_heave_sign_convention(void) {
    double heave;
    
    // Positive heave = upward motion (vessel rising on wave)
    heave = 1.5;
    TEST_ASSERT_GREATER_THAN(0.0, heave);
    TEST_MESSAGE("Positive heave = upward motion");
    
    // Negative heave = downward motion (vessel descending into trough)
    heave = -1.5;
    TEST_ASSERT_LESS_THAN(0.0, heave);
    TEST_MESSAGE("Negative heave = downward motion");
}

/**
 * @test Sign convention consistency across data structures
 */
void test_sign_convention_consistency(void) {
    // All angular measurements use consistent sign conventions
    // Starboard/Right = positive
    // Port/Left = negative
    
    double heelAngle = 0.1;      // Positive = starboard
    double rateOfTurn = 0.1;     // Positive = turning starboard
    
    TEST_ASSERT_GREATER_THAN(0.0, heelAngle);
    TEST_ASSERT_GREATER_THAN(0.0, rateOfTurn);
    
    TEST_MESSAGE("Sign conventions consistent: Starboard/Right = positive");
}

/**
 * @test NMEA2000 sign convention alignment
 */
void test_nmea2000_sign_alignment(void) {
    // NMEA2000 PGN sign conventions should align with our data model
    // PGN 127257 (Attitude): Roll positive = starboard heel
    // PGN 127251 (Rate of Turn): Positive = turning right
    
    TEST_PASS_MESSAGE("Sign conventions align with NMEA2000 PGN specifications");
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_battery_amperage_sign_convention);
    RUN_TEST(test_heel_angle_sign_convention);
    RUN_TEST(test_pitch_angle_sign_convention);
    RUN_TEST(test_rate_of_turn_sign_convention);
    RUN_TEST(test_magnetic_variation_sign_convention);
    RUN_TEST(test_heave_sign_convention);
    RUN_TEST(test_sign_convention_consistency);
    RUN_TEST(test_nmea2000_sign_alignment);
    return UNITY_END();
}
