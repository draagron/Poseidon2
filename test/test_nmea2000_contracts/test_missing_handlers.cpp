/**
 * @file test_missing_handlers.cpp
 * @brief Contract tests for missing NMEA2000 PGN handlers
 *
 * Tests the 5 missing handler functions to verify they follow HandlerFunctionContract.md:
 * - PGN 129025: Position Rapid Update
 * - PGN 129026: COG/SOG Rapid Update
 * - PGN 127250: Vessel Heading
 * - PGN 127258: Magnetic Variation
 * - PGN 130306: Wind Data
 *
 * Expected: Tests will FAIL initially (handlers not yet implemented)
 * Expected: Tests will PASS after handler implementation
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

// ============================================================================
// PGN 129025 - Position Rapid Update Tests
// ============================================================================

void test_pgn129025_valid_position() {
    // Create mock BoatData and logger
    BoatData boatData;
    MockLogger logger;

    // Create test message with valid position (San Francisco)
    tN2kMsg testMsg;
    SetN2kPGN129025(testMsg, 0, 37.7749, -122.4194);

    // Call handler (should exist after implementation)
    HandleN2kPGN129025(testMsg, &boatData, &logger);

    // Verify BoatData updated
    GPSData gps = boatData.getGPSData();
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 37.7749, gps.latitude);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, -122.4194, gps.longitude);
    TEST_ASSERT_TRUE(gps.available);
    TEST_ASSERT_GREATER_THAN(0, gps.lastUpdate);

    // Verify DEBUG log
    TEST_ASSERT_EQUAL_STRING("DEBUG", logger.lastLogLevel.c_str());
    TEST_ASSERT_EQUAL_STRING("PGN129025_UPDATE", logger.lastEvent.c_str());
}

void test_pgn129025_nullptr_handling() {
    tN2kMsg testMsg;
    SetN2kPGN129025(testMsg, 0, 37.7749, -122.4194);

    // Should not crash with nullptr parameters
    HandleN2kPGN129025(testMsg, nullptr, nullptr);
    TEST_PASS();
}

// ============================================================================
// PGN 129026 - COG/SOG Rapid Update Tests
// ============================================================================

void test_pgn129026_valid_cog_sog() {
    BoatData boatData;
    MockLogger logger;

    // Create test message with COG=90° (East), SOG=5 knots
    tN2kMsg testMsg;
    double cogRad = 90.0 * DEG_TO_RAD;  // 90 degrees in radians
    double sogMs = 5.0 / 1.94384;        // 5 knots in m/s
    SetN2kPGN129026(testMsg, 0, N2khr_true, cogRad, sogMs);

    // Call handler
    HandleN2kPGN129026(testMsg, &boatData, &logger);

    // Verify BoatData updated
    GPSData gps = boatData.getGPSData();
    TEST_ASSERT_DOUBLE_WITHIN(0.01, cogRad, gps.cog);
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 5.0, gps.sog);  // Should be in knots
    TEST_ASSERT_TRUE(gps.available);

    // Verify DEBUG log
    TEST_ASSERT_EQUAL_STRING("DEBUG", logger.lastLogLevel.c_str());
    TEST_ASSERT_EQUAL_STRING("PGN129026_UPDATE", logger.lastEvent.c_str());
}

void test_pgn129026_nullptr_handling() {
    tN2kMsg testMsg;
    SetN2kPGN129026(testMsg, 0, N2khr_true, 1.57, 2.5);

    // Should not crash with nullptr parameters
    HandleN2kPGN129026(testMsg, nullptr, nullptr);
    TEST_PASS();
}

// ============================================================================
// PGN 127250 - Vessel Heading Tests
// ============================================================================

void test_pgn127250_true_heading() {
    BoatData boatData;
    MockLogger logger;

    // Create test message with true heading 90° (East)
    tN2kMsg testMsg;
    double headingRad = 90.0 * DEG_TO_RAD;
    SetN2kPGN127250(testMsg, 0, headingRad, N2kDoubleNA, N2kDoubleNA, N2khr_true);

    // Call handler
    HandleN2kPGN127250(testMsg, &boatData, &logger);

    // Verify BoatData updated (should go to trueHeading)
    CompassData compass = boatData.getCompassData();
    TEST_ASSERT_DOUBLE_WITHIN(0.01, headingRad, compass.trueHeading);
    TEST_ASSERT_TRUE(compass.available);

    // Verify DEBUG log
    TEST_ASSERT_EQUAL_STRING("DEBUG", logger.lastLogLevel.c_str());
    TEST_ASSERT_EQUAL_STRING("PGN127250_UPDATE", logger.lastEvent.c_str());
}

void test_pgn127250_magnetic_heading() {
    BoatData boatData;
    MockLogger logger;

    // Create test message with magnetic heading 85°
    tN2kMsg testMsg;
    double headingRad = 85.0 * DEG_TO_RAD;
    SetN2kPGN127250(testMsg, 0, headingRad, N2kDoubleNA, N2kDoubleNA, N2khr_magnetic);

    // Call handler
    HandleN2kPGN127250(testMsg, &boatData, &logger);

    // Verify BoatData updated (should go to magneticHeading)
    CompassData compass = boatData.getCompassData();
    TEST_ASSERT_DOUBLE_WITHIN(0.01, headingRad, compass.magneticHeading);
    TEST_ASSERT_TRUE(compass.available);
}

void test_pgn127250_nullptr_handling() {
    tN2kMsg testMsg;
    SetN2kPGN127250(testMsg, 0, 1.57, N2kDoubleNA, N2kDoubleNA, N2khr_true);

    // Should not crash with nullptr parameters
    HandleN2kPGN127250(testMsg, nullptr, nullptr);
    TEST_PASS();
}

// ============================================================================
// PGN 127258 - Magnetic Variation Tests
// ============================================================================

void test_pgn127258_valid_variation() {
    BoatData boatData;
    MockLogger logger;

    // Create test message with variation -15° (15° West)
    tN2kMsg testMsg;
    double variationRad = -15.0 * DEG_TO_RAD;
    SetN2kPGN127258(testMsg, 0, N2kmagvar_Manual, 0, variationRad);

    // Call handler
    HandleN2kPGN127258(testMsg, &boatData, &logger);

    // Verify BoatData updated
    GPSData gps = boatData.getGPSData();
    TEST_ASSERT_DOUBLE_WITHIN(0.01, variationRad, gps.variation);
    TEST_ASSERT_TRUE(gps.available);

    // Verify DEBUG log
    TEST_ASSERT_EQUAL_STRING("DEBUG", logger.lastLogLevel.c_str());
    TEST_ASSERT_EQUAL_STRING("PGN127258_UPDATE", logger.lastEvent.c_str());
}

void test_pgn127258_nullptr_handling() {
    tN2kMsg testMsg;
    SetN2kPGN127258(testMsg, 0, N2kmagvar_Manual, 0, -0.26);

    // Should not crash with nullptr parameters
    HandleN2kPGN127258(testMsg, nullptr, nullptr);
    TEST_PASS();
}

// ============================================================================
// PGN 130306 - Wind Data Tests
// ============================================================================

void test_pgn130306_apparent_wind() {
    BoatData boatData;
    MockLogger logger;

    // Create test message with apparent wind: 45° starboard, 15 knots
    tN2kMsg testMsg;
    double windAngleRad = 45.0 * DEG_TO_RAD;
    double windSpeedMs = 15.0 / 1.94384;  // 15 knots in m/s
    SetN2kPGN130306(testMsg, 0, windSpeedMs, windAngleRad, N2kWind_Apparent);

    // Call handler
    HandleN2kPGN130306(testMsg, &boatData, &logger);

    // Verify BoatData updated
    WindData wind = boatData.getWindData();
    TEST_ASSERT_DOUBLE_WITHIN(0.01, windAngleRad, wind.apparentWindAngle);
    TEST_ASSERT_DOUBLE_WITHIN(0.5, 15.0, wind.apparentWindSpeed);  // Should be in knots
    TEST_ASSERT_TRUE(wind.available);

    // Verify DEBUG log
    TEST_ASSERT_EQUAL_STRING("DEBUG", logger.lastLogLevel.c_str());
    TEST_ASSERT_EQUAL_STRING("PGN130306_UPDATE", logger.lastEvent.c_str());
}

void test_pgn130306_ignores_true_wind() {
    BoatData boatData;
    MockLogger logger;

    // Create test message with TRUE wind (should be ignored)
    tN2kMsg testMsg;
    SetN2kPGN130306(testMsg, 0, 10.0, 1.0, N2kWind_True_boat);

    // Call handler
    HandleN2kPGN130306(testMsg, &boatData, &logger);

    // Verify BoatData NOT updated (wind reference type is not Apparent)
    WindData wind = boatData.getWindData();
    TEST_ASSERT_FALSE(wind.available);

    // Should have no log (silently ignored)
    TEST_ASSERT_EQUAL(0, logger.logCount);
}

void test_pgn130306_nullptr_handling() {
    tN2kMsg testMsg;
    SetN2kPGN130306(testMsg, 0, 10.0, 0.78, N2kWind_Apparent);

    // Should not crash with nullptr parameters
    HandleN2kPGN130306(testMsg, nullptr, nullptr);
    TEST_PASS();
}

// ============================================================================
// Test Runner
// ============================================================================

void setUp(void) {
    // Set up runs before each test
}

void tearDown(void) {
    // Tear down runs after each test
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // PGN 129025 tests
    RUN_TEST(test_pgn129025_valid_position);
    RUN_TEST(test_pgn129025_nullptr_handling);

    // PGN 129026 tests
    RUN_TEST(test_pgn129026_valid_cog_sog);
    RUN_TEST(test_pgn129026_nullptr_handling);

    // PGN 127250 tests
    RUN_TEST(test_pgn127250_true_heading);
    RUN_TEST(test_pgn127250_magnetic_heading);
    RUN_TEST(test_pgn127250_nullptr_handling);

    // PGN 127258 tests
    RUN_TEST(test_pgn127258_valid_variation);
    RUN_TEST(test_pgn127258_nullptr_handling);

    // PGN 130306 tests
    RUN_TEST(test_pgn130306_apparent_wind);
    RUN_TEST(test_pgn130306_ignores_true_wind);
    RUN_TEST(test_pgn130306_nullptr_handling);

    return UNITY_END();
}
