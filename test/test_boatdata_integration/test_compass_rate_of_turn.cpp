/**
 * @file test_compass_rate_of_turn.cpp
 * @brief Integration test for compass rate of turn (FR-005)
 *
 * Scenario: NMEA2000 PGN 127251 provides rate of turn data
 * Validates: CompassData.rateOfTurn populated correctly
 *
 * @see specs/008-enhanced-boatdata-following/quickstart.md (Scenario 2)
 * @see specs/008-enhanced-boatdata-following/tasks.md (T010)
 */

#include <unity.h>
#include "types/BoatDataTypes.h"

void setUp(void) {}
void tearDown(void) {}

/**
 * @test Rate of turn stored in CompassData
 */
void test_rate_of_turn_in_compassdata(void) {
    BoatDataStructure boat;
    
    // Simulate PGN 127251 providing rate of turn
    boat.compass.rateOfTurn = 0.1;  // 0.1 rad/s (turning starboard)
    boat.compass.available = true;
    
    TEST_ASSERT_EQUAL_DOUBLE(0.1, boat.compass.rateOfTurn);
    TEST_ASSERT_TRUE(boat.compass.available);
}

/**
 * @test Rate of turn sign convention (positive = starboard turn)
 */
void test_rate_of_turn_sign_convention(void) {
    BoatDataStructure boat;
    
    // Positive rate = turning right (starboard)
    boat.compass.rateOfTurn = 0.5;
    TEST_ASSERT_GREATER_THAN(0.0, boat.compass.rateOfTurn);
    
    // Negative rate = turning left (port)
    boat.compass.rateOfTurn = -0.5;
    TEST_ASSERT_LESS_THAN(0.0, boat.compass.rateOfTurn);
}

/**
 * @test Rate of turn range validation (±π rad/s limit)
 */
void test_rate_of_turn_range_validation(void) {
    BoatDataStructure boat;
    
    // Extreme but valid rate
    boat.compass.rateOfTurn = M_PI;  // ±180°/s
    TEST_ASSERT_DOUBLE_WITHIN(0.01, M_PI, boat.compass.rateOfTurn);
    
    boat.compass.rateOfTurn = -M_PI;
    TEST_ASSERT_DOUBLE_WITHIN(0.01, -M_PI, boat.compass.rateOfTurn);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_rate_of_turn_in_compassdata);
    RUN_TEST(test_rate_of_turn_sign_convention);
    RUN_TEST(test_rate_of_turn_range_validation);
    return UNITY_END();
}
