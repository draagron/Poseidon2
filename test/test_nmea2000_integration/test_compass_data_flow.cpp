/**
 * @file test_compass_data_flow.cpp
 * @brief Integration test for Compass data flow from multiple PGNs
 *
 * Scenario: Compass receives data from 4 different PGNs and populates CompassData completely
 * - PGN 127250: Vessel Heading (true/magnetic heading) - 10 Hz
 * - PGN 127251: Rate of Turn (rateOfTurn) - 10 Hz
 * - PGN 127252: Heave (vertical displacement) - 10 Hz
 * - PGN 127257: Attitude (heel/pitch angles) - 10 Hz
 *
 * Validates: Complete Compass data structure population from NMEA2000 sources
 *
 * @see specs/010-nmea-2000-handling/tasks.md (T033)
 * @see specs/010-nmea-2000-handling/data-model.md (Compass Data section)
 */

#include <unity.h>
#include <NMEA2000.h>
#include <N2kMessages.h>
#include "../../src/components/BoatData.h"
#include "../../src/components/NMEA2000Handlers.h"
#include "../../src/utils/WebSocketLogger.h"

// Mock WebSocketLogger for testing
class MockLogger : public WebSocketLogger {
public:
    String lastLogLevel;
    String lastComponent;
    String lastEvent;
    String lastData;
    int logCount = 0;

    MockLogger() : WebSocketLogger(nullptr) {}

    void broadcastLog(LogLevel level, const String& component, const String& event, const String& data) override {
        lastLogLevel = (level == LogLevel::DEBUG) ? "DEBUG" :
                      (level == LogLevel::INFO) ? "INFO" :
                      (level == LogLevel::WARN) ? "WARN" : "ERROR";
        lastComponent = component;
        lastEvent = event;
        lastData = data;
        logCount++;
    }
};

void setUp(void) {}
void tearDown(void) {}

/**
 * @test Complete Compass data flow from all 4 PGNs
 *
 * Simulates receiving compass/attitude data from multiple PGN sources:
 * 1. PGN 127250 - Vessel Heading (true/magnetic heading)
 * 2. PGN 127251 - Rate of Turn
 * 3. PGN 127252 - Heave (vertical motion)
 * 4. PGN 127257 - Attitude (heel and pitch)
 */
void test_compass_complete_data_flow() {
    BoatData boatData;
    MockLogger logger;

    // Step 1: Receive PGN 127250 - True Heading
    tN2kMsg msg1;
    double trueHeading = 90.0 * DEG_TO_RAD;  // 90° East
    SetN2kPGN127250(msg1, 0, trueHeading, N2kDoubleNA, N2kDoubleNA, N2khr_true);
    HandleN2kPGN127250(msg1, &boatData, &logger);

    CompassData compass = boatData.getCompassData();
    TEST_ASSERT_DOUBLE_WITHIN(0.01, trueHeading, compass.trueHeading);
    TEST_ASSERT_TRUE(compass.available);

    // Step 2: Receive PGN 127250 - Magnetic Heading
    tN2kMsg msg2;
    double magneticHeading = 85.0 * DEG_TO_RAD;  // 85° (5° variation)
    SetN2kPGN127250(msg2, 0, magneticHeading, N2kDoubleNA, N2kDoubleNA, N2khr_magnetic);
    HandleN2kPGN127250(msg2, &boatData, &logger);

    compass = boatData.getCompassData();
    TEST_ASSERT_DOUBLE_WITHIN(0.01, magneticHeading, compass.magneticHeading);

    // Step 3: Receive PGN 127251 - Rate of Turn
    tN2kMsg msg3;
    double rateOfTurn = 0.05;  // 0.05 rad/s (starboard turn)
    SetN2kPGN127251(msg3, 0, rateOfTurn);
    HandleN2kPGN127251(msg3, &boatData, &logger);

    compass = boatData.getCompassData();
    TEST_ASSERT_DOUBLE_WITHIN(0.001, rateOfTurn, compass.rateOfTurn);

    // Step 4: Receive PGN 127252 - Heave
    tN2kMsg msg4;
    double heave = 0.5;  // 0.5m upward motion
    SetN2kPGN127252(msg4, 0, heave);
    HandleN2kPGN127252(msg4, &boatData, &logger);

    compass = boatData.getCompassData();
    TEST_ASSERT_DOUBLE_WITHIN(0.01, heave, compass.heave);

    // Step 5: Receive PGN 127257 - Attitude (heel and pitch)
    tN2kMsg msg5;
    double heelAngle = 10.0 * DEG_TO_RAD;   // 10° starboard heel
    double pitchAngle = 5.0 * DEG_TO_RAD;    // 5° bow up
    SetN2kPGN127257(msg5, 0, N2kDoubleNA, pitchAngle, heelAngle);  // yaw, pitch, roll
    HandleN2kPGN127257(msg5, &boatData, &logger);

    compass = boatData.getCompassData();
    TEST_ASSERT_DOUBLE_WITHIN(0.01, heelAngle, compass.heelAngle);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, pitchAngle, compass.pitchAngle);

    // Verify all Compass fields populated
    TEST_ASSERT_TRUE(compass.available);
    TEST_ASSERT_GREATER_THAN(0, compass.lastUpdate);
}

/**
 * @test Compass heading routing based on reference type
 *
 * PGN 127250 can provide either true or magnetic heading.
 * Validates that the handler correctly routes to the appropriate field.
 */
void test_compass_heading_routing() {
    BoatData boatData;
    MockLogger logger;

    // Test 1: True heading routing
    tN2kMsg msg1;
    double trueHeading = 45.0 * DEG_TO_RAD;
    SetN2kPGN127250(msg1, 0, trueHeading, N2kDoubleNA, N2kDoubleNA, N2khr_true);
    HandleN2kPGN127250(msg1, &boatData, &logger);

    CompassData compass = boatData.getCompassData();
    TEST_ASSERT_DOUBLE_WITHIN(0.01, trueHeading, compass.trueHeading);
    // Magnetic heading should remain at default (0.0)
    TEST_ASSERT_EQUAL_DOUBLE(0.0, compass.magneticHeading);

    // Test 2: Magnetic heading routing
    tN2kMsg msg2;
    double magneticHeading = 40.0 * DEG_TO_RAD;
    SetN2kPGN127250(msg2, 0, magneticHeading, N2kDoubleNA, N2kDoubleNA, N2khr_magnetic);
    HandleN2kPGN127250(msg2, &boatData, &logger);

    compass = boatData.getCompassData();
    TEST_ASSERT_DOUBLE_WITHIN(0.01, magneticHeading, compass.magneticHeading);
    // True heading should be preserved from previous update
    TEST_ASSERT_DOUBLE_WITHIN(0.01, trueHeading, compass.trueHeading);
}

/**
 * @test Compass rate of turn sign convention
 *
 * Validates that positive rate = starboard turn, negative = port turn.
 * Tests extreme values and sign changes.
 */
void test_compass_rate_of_turn_sign_convention() {
    BoatData boatData;
    MockLogger logger;

    // Test 1: Starboard turn (positive)
    tN2kMsg msg1;
    double starboardRate = 0.1;  // 0.1 rad/s starboard
    SetN2kPGN127251(msg1, 0, starboardRate);
    HandleN2kPGN127251(msg1, &boatData, &logger);

    CompassData compass = boatData.getCompassData();
    TEST_ASSERT_DOUBLE_WITHIN(0.001, starboardRate, compass.rateOfTurn);
    TEST_ASSERT_GREATER_THAN(0.0, compass.rateOfTurn);

    // Test 2: Port turn (negative)
    tN2kMsg msg2;
    double portRate = -0.1;  // 0.1 rad/s port
    SetN2kPGN127251(msg2, 0, portRate);
    HandleN2kPGN127251(msg2, &boatData, &logger);

    compass = boatData.getCompassData();
    TEST_ASSERT_DOUBLE_WITHIN(0.001, portRate, compass.rateOfTurn);
    TEST_ASSERT_LESS_THAN(0.0, compass.rateOfTurn);

    // Test 3: No turn (zero)
    tN2kMsg msg3;
    SetN2kPGN127251(msg3, 0, 0.0);
    HandleN2kPGN127251(msg3, &boatData, &logger);

    compass = boatData.getCompassData();
    TEST_ASSERT_EQUAL_DOUBLE(0.0, compass.rateOfTurn);
}

/**
 * @test Compass heave sign convention
 *
 * Validates that positive heave = upward motion, negative = downward motion.
 */
void test_compass_heave_sign_convention() {
    BoatData boatData;
    MockLogger logger;

    // Test 1: Upward motion (positive)
    tN2kMsg msg1;
    double upwardHeave = 1.5;  // 1.5m upward
    SetN2kPGN127252(msg1, 0, upwardHeave);
    HandleN2kPGN127252(msg1, &boatData, &logger);

    CompassData compass = boatData.getCompassData();
    TEST_ASSERT_DOUBLE_WITHIN(0.01, upwardHeave, compass.heave);
    TEST_ASSERT_GREATER_THAN(0.0, compass.heave);

    // Test 2: Downward motion (negative)
    tN2kMsg msg2;
    double downwardHeave = -2.0;  // 2.0m downward
    SetN2kPGN127252(msg2, 0, downwardHeave);
    HandleN2kPGN127252(msg2, &boatData, &logger);

    compass = boatData.getCompassData();
    TEST_ASSERT_DOUBLE_WITHIN(0.01, downwardHeave, compass.heave);
    TEST_ASSERT_LESS_THAN(0.0, compass.heave);

    // Test 3: No vertical motion (zero)
    tN2kMsg msg3;
    SetN2kPGN127252(msg3, 0, 0.0);
    HandleN2kPGN127252(msg3, &boatData, &logger);

    compass = boatData.getCompassData();
    TEST_ASSERT_EQUAL_DOUBLE(0.0, compass.heave);
}

/**
 * @test Compass attitude sign conventions (heel and pitch)
 *
 * Validates:
 * - Heel: Positive = starboard tilt, Negative = port tilt
 * - Pitch: Positive = bow up, Negative = bow down
 */
void test_compass_attitude_sign_conventions() {
    BoatData boatData;
    MockLogger logger;

    // Test 1: Starboard heel, bow up
    tN2kMsg msg1;
    double starboardHeel = 15.0 * DEG_TO_RAD;  // 15° starboard
    double bowUp = 10.0 * DEG_TO_RAD;          // 10° bow up
    SetN2kPGN127257(msg1, 0, N2kDoubleNA, bowUp, starboardHeel);
    HandleN2kPGN127257(msg1, &boatData, &logger);

    CompassData compass = boatData.getCompassData();
    TEST_ASSERT_DOUBLE_WITHIN(0.01, starboardHeel, compass.heelAngle);
    TEST_ASSERT_GREATER_THAN(0.0, compass.heelAngle);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, bowUp, compass.pitchAngle);
    TEST_ASSERT_GREATER_THAN(0.0, compass.pitchAngle);

    // Test 2: Port heel, bow down
    tN2kMsg msg2;
    double portHeel = -20.0 * DEG_TO_RAD;      // 20° port
    double bowDown = -5.0 * DEG_TO_RAD;        // 5° bow down
    SetN2kPGN127257(msg2, 0, N2kDoubleNA, bowDown, portHeel);
    HandleN2kPGN127257(msg2, &boatData, &logger);

    compass = boatData.getCompassData();
    TEST_ASSERT_DOUBLE_WITHIN(0.01, portHeel, compass.heelAngle);
    TEST_ASSERT_LESS_THAN(0.0, compass.heelAngle);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, bowDown, compass.pitchAngle);
    TEST_ASSERT_LESS_THAN(0.0, compass.pitchAngle);

    // Test 3: Level (no heel, no pitch)
    tN2kMsg msg3;
    SetN2kPGN127257(msg3, 0, N2kDoubleNA, 0.0, 0.0);
    HandleN2kPGN127257(msg3, &boatData, &logger);

    compass = boatData.getCompassData();
    TEST_ASSERT_EQUAL_DOUBLE(0.0, compass.heelAngle);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, compass.pitchAngle);
}

/**
 * @test Compass high-frequency updates (10 Hz simulation)
 *
 * Simulates rapid compass heading updates (PGN 127250 at 10 Hz).
 * Validates that BoatData correctly handles high-frequency updates.
 */
void test_compass_high_frequency_updates() {
    BoatData boatData;
    MockLogger logger;

    // Simulate 10 rapid heading updates (turning starboard)
    for (int i = 0; i < 10; i++) {
        tN2kMsg msg;
        double heading = (90.0 + i * 5.0) * DEG_TO_RAD;  // 90°, 95°, 100°, ...
        SetN2kPGN127250(msg, 0, heading, N2kDoubleNA, N2kDoubleNA, N2khr_true);
        HandleN2kPGN127250(msg, &boatData, &logger);

        // Verify each update is processed
        CompassData compass = boatData.getCompassData();
        TEST_ASSERT_DOUBLE_WITHIN(0.01, heading, compass.trueHeading);
        TEST_ASSERT_TRUE(compass.available);
    }

    // Verify logger captured all updates
    TEST_ASSERT_EQUAL_INT(10, logger.logCount);
}

/**
 * @test Compass extreme attitude angles
 *
 * Validates handling of extreme (but valid) heel and pitch angles
 * that might occur in rough seas.
 */
void test_compass_extreme_attitudes() {
    BoatData boatData;
    MockLogger logger;

    // Test 1: Extreme heel (45° - knockdown scenario)
    tN2kMsg msg1;
    double extremeHeel = 45.0 * DEG_TO_RAD;
    SetN2kPGN127257(msg1, 0, N2kDoubleNA, 0.0, extremeHeel);
    HandleN2kPGN127257(msg1, &boatData, &logger);

    CompassData compass = boatData.getCompassData();
    TEST_ASSERT_DOUBLE_WITHIN(0.01, extremeHeel, compass.heelAngle);

    // Test 2: Extreme pitch (30° - heavy seas)
    tN2kMsg msg2;
    double extremePitch = 30.0 * DEG_TO_RAD;
    SetN2kPGN127257(msg2, 0, N2kDoubleNA, extremePitch, 0.0);
    HandleN2kPGN127257(msg2, &boatData, &logger);

    compass = boatData.getCompassData();
    TEST_ASSERT_DOUBLE_WITHIN(0.01, extremePitch, compass.pitchAngle);

    // Test 3: Large heave (4m wave)
    tN2kMsg msg3;
    double largeHeave = 4.0;  // 4m vertical displacement
    SetN2kPGN127252(msg3, 0, largeHeave);
    HandleN2kPGN127252(msg3, &boatData, &logger);

    compass = boatData.getCompassData();
    TEST_ASSERT_DOUBLE_WITHIN(0.01, largeHeave, compass.heave);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_compass_complete_data_flow);
    RUN_TEST(test_compass_heading_routing);
    RUN_TEST(test_compass_rate_of_turn_sign_convention);
    RUN_TEST(test_compass_heave_sign_convention);
    RUN_TEST(test_compass_attitude_sign_conventions);
    RUN_TEST(test_compass_high_frequency_updates);
    RUN_TEST(test_compass_extreme_attitudes);
    return UNITY_END();
}
