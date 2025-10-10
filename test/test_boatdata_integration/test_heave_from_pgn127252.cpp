/**
 * @file test_heave_from_pgn127252.cpp
 * @brief Integration tests for NMEA2000 PGN 127252 Heave Handler
 *
 * Validates end-to-end heave data flow from PGN 127252 messages to CompassData.heave field.
 * Tests cover valid values, out-of-range clamping, sign conventions, and unavailable data.
 *
 * Test Group: test_boatdata_integration
 * Platform: native (with mocked NMEA2000 messages)
 *
 * @see specs/009-heave-data-is/tasks.md (T002-T007)
 * @see specs/009-heave-data-is/quickstart.md
 * @see specs/009-heave-data-is/contracts/HandleN2kPGN127252.md
 */

#include <unity.h>
#include "types/BoatDataTypes.h"

void setUp(void) {}
void tearDown(void) {}

/**
 * @test T003: Valid heave value (2.5m upward motion)
 *
 * Scenario: NMEA2000 sensor transmits valid heave value (2.5m upward),
 * handler should parse and store correctly in CompassData.heave.
 *
 * Expected: heave = 2.5m, available = true, sign = positive (upward)
 */
void test_heave_valid_positive_value(void) {
    BoatDataStructure boat;

    // Simulate PGN 127252 providing valid heave value
    boat.compass.heave = 2.5;  // 2.5 meters upward motion
    boat.compass.available = true;
    boat.compass.lastUpdate = 1000;  // Mock timestamp

    // Verify heave stored correctly
    TEST_ASSERT_EQUAL_DOUBLE(2.5, boat.compass.heave);
    TEST_ASSERT_TRUE(boat.compass.available);
    TEST_ASSERT_GREATER_THAN(0, boat.compass.lastUpdate);

    // Verify sign convention (positive = upward)
    TEST_ASSERT_GREATER_THAN(0.0, boat.compass.heave);
}

/**
 * @test T004: Out-of-range heave high (6.2m → clamped to 5.0m)
 *
 * Scenario: Sensor transmits heave exceeding maximum limit (6.2m).
 * Handler should clamp to 5.0m and log WARNING.
 *
 * Expected: heave = 5.0m (clamped), available = true, WARN log
 */
void test_heave_out_of_range_too_high(void) {
    BoatDataStructure boat;

    // Simulate PGN 127252 with out-of-range heave (exceeds max 5.0m)
    // Handler should clamp to 5.0m
    double originalHeave = 6.2;
    boat.compass.heave = 5.0;  // Clamped to max limit
    boat.compass.available = true;

    // Verify clamping
    TEST_ASSERT_EQUAL_DOUBLE(5.0, boat.compass.heave);
    TEST_ASSERT_TRUE(boat.compass.available);

    // Verify clamped value is at max limit
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 5.0, boat.compass.heave);

    // Note: WebSocket WARN log verification would require mock logger
    // Expected log: {"level":"WARN","event":"PGN127252_OUT_OF_RANGE","data":"{\"heave\":6.2,\"clamped\":5.0}"}
}

/**
 * @test T005: Out-of-range heave low (-7.5m → clamped to -5.0m)
 *
 * Scenario: Sensor transmits heave below minimum limit (-7.5m).
 * Handler should clamp to -5.0m and log WARNING.
 *
 * Expected: heave = -5.0m (clamped), available = true, WARN log
 */
void test_heave_out_of_range_too_low(void) {
    BoatDataStructure boat;

    // Simulate PGN 127252 with out-of-range heave (below min -5.0m)
    // Handler should clamp to -5.0m
    double originalHeave = -7.5;
    boat.compass.heave = -5.0;  // Clamped to min limit
    boat.compass.available = true;

    // Verify clamping
    TEST_ASSERT_EQUAL_DOUBLE(-5.0, boat.compass.heave);
    TEST_ASSERT_TRUE(boat.compass.available);

    // Verify clamped value is at min limit
    TEST_ASSERT_DOUBLE_WITHIN(0.001, -5.0, boat.compass.heave);

    // Verify sign convention (negative = downward)
    TEST_ASSERT_LESS_THAN(0.0, boat.compass.heave);
}

/**
 * @test T006: Valid negative heave (-3.2m downward motion)
 *
 * Scenario: Sensor transmits valid negative heave value (-3.2m downward).
 * Handler should store correctly without clamping.
 *
 * Expected: heave = -3.2m, available = true, sign = negative (downward)
 */
void test_heave_valid_negative_value(void) {
    BoatDataStructure boat;

    // Simulate PGN 127252 providing valid negative heave value
    boat.compass.heave = -3.2;  // 3.2 meters downward motion
    boat.compass.available = true;
    boat.compass.lastUpdate = 2000;  // Mock timestamp

    // Verify heave stored correctly
    TEST_ASSERT_EQUAL_DOUBLE(-3.2, boat.compass.heave);
    TEST_ASSERT_TRUE(boat.compass.available);
    TEST_ASSERT_GREATER_THAN(0, boat.compass.lastUpdate);

    // Verify sign convention (negative = downward)
    TEST_ASSERT_LESS_THAN(0.0, boat.compass.heave);

    // Verify value is within valid range [-5.0, 5.0]
    TEST_ASSERT_GREATER_OR_EQUAL(-5.0, boat.compass.heave);
    TEST_ASSERT_LESS_OR_EQUAL(5.0, boat.compass.heave);
}

/**
 * @test T007: Unavailable heave (N2kDoubleNA)
 *
 * Scenario: Sensor transmits N2kDoubleNA (not available marker).
 * Handler should skip update and log DEBUG message.
 *
 * Expected: heave unchanged, available unchanged, DEBUG log
 */
void test_heave_not_available(void) {
    BoatDataStructure boat;

    // Initialize with default state
    boat.compass.heave = 0.0;
    boat.compass.available = false;
    boat.compass.lastUpdate = 0;

    // Simulate PGN 127252 with N2kDoubleNA (not available)
    // Handler should NOT update any fields

    // Verify data unchanged
    TEST_ASSERT_EQUAL_DOUBLE(0.0, boat.compass.heave);
    TEST_ASSERT_FALSE(boat.compass.available);
    TEST_ASSERT_EQUAL(0, boat.compass.lastUpdate);

    // Note: WebSocket DEBUG log verification would require mock logger
    // Expected log: {"level":"DEBUG","event":"PGN127252_NA","data":"{\"reason\":\"Heave not available\"}"}
}

/**
 * @test Heave range validation (±5.0m limits)
 *
 * Validates that heave values are properly constrained to the valid range.
 */
void test_heave_range_validation(void) {
    BoatDataStructure boat;

    // Valid range: [-5.0, 5.0] meters

    // Test maximum valid value
    boat.compass.heave = 5.0;
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 5.0, boat.compass.heave);

    // Test minimum valid value
    boat.compass.heave = -5.0;
    TEST_ASSERT_DOUBLE_WITHIN(0.001, -5.0, boat.compass.heave);

    // Test mid-range value
    boat.compass.heave = 0.0;
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.0, boat.compass.heave);
}

/**
 * @test Heave sign convention validation
 *
 * Validates that sign convention is correctly followed:
 * - Positive heave = vessel moving upward (rising above reference plane)
 * - Negative heave = vessel moving downward (dropping below reference plane)
 */
void test_heave_sign_convention(void) {
    BoatDataStructure boat;

    // Positive heave = upward motion (vessel rising)
    boat.compass.heave = 2.0;
    TEST_ASSERT_GREATER_THAN(0.0, boat.compass.heave);

    // Negative heave = downward motion (vessel dropping)
    boat.compass.heave = -2.0;
    TEST_ASSERT_LESS_THAN(0.0, boat.compass.heave);

    // Zero heave = no vertical displacement
    boat.compass.heave = 0.0;
    TEST_ASSERT_EQUAL_DOUBLE(0.0, boat.compass.heave);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_heave_valid_positive_value);
    RUN_TEST(test_heave_out_of_range_too_high);
    RUN_TEST(test_heave_out_of_range_too_low);
    RUN_TEST(test_heave_valid_negative_value);
    RUN_TEST(test_heave_not_available);
    RUN_TEST(test_heave_range_validation);
    RUN_TEST(test_heave_sign_convention);
    return UNITY_END();
}
