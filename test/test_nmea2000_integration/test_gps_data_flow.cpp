/**
 * @file test_gps_data_flow.cpp
 * @brief Integration test for GPS data flow from multiple PGNs
 *
 * Scenario: GPS receives data from 4 different PGNs and populates GPSData completely
 * - PGN 129025: Position Rapid Update (lat/lon) - 10 Hz
 * - PGN 129026: COG/SOG Rapid Update (course/speed) - 10 Hz
 * - PGN 129029: GNSS Position Data (all fields including variation) - 1 Hz
 * - PGN 127258: Magnetic Variation (alternative variation source) - 1 Hz
 *
 * Validates: Complete GPS data structure population from NMEA2000 sources
 *
 * @see specs/010-nmea-2000-handling/tasks.md (T032)
 * @see specs/010-nmea-2000-handling/data-model.md (GPS Data section)
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
 * @test Complete GPS data flow from all 4 PGNs
 *
 * Simulates receiving GPS data from multiple PGN sources in typical order:
 * 1. PGN 129025 - Position (10 Hz rapid updates)
 * 2. PGN 129026 - COG/SOG (10 Hz rapid updates)
 * 3. PGN 129029 - GNSS Position with variation (1 Hz complete data)
 * 4. PGN 127258 - Magnetic variation (1 Hz alternative source)
 */
void test_gps_complete_data_flow() {
    BoatData boatData;
    MockLogger logger;

    // Step 1: Receive PGN 129025 - Position Rapid Update
    tN2kMsg msg1;
    SetN2kPGN129025(msg1, 0, 37.7749, -122.4194);  // San Francisco
    HandleN2kPGN129025(msg1, &boatData, &logger);

    GPSData gps = boatData.getGPSData();
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 37.7749, gps.latitude);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, -122.4194, gps.longitude);
    TEST_ASSERT_TRUE(gps.available);

    // Step 2: Receive PGN 129026 - COG/SOG Rapid Update
    tN2kMsg msg2;
    double cogRad = 90.0 * DEG_TO_RAD;  // 90° East
    double sogMs = 5.0 / 1.94384;        // 5 knots in m/s
    SetN2kPGN129026(msg2, 0, N2khr_true, cogRad, sogMs);
    HandleN2kPGN129026(msg2, &boatData, &logger);

    gps = boatData.getGPSData();
    TEST_ASSERT_DOUBLE_WITHIN(0.01, cogRad, gps.cog);
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 5.0, gps.sog);  // Should be converted to knots

    // Step 3: Receive PGN 129029 - GNSS Position Data (with variation)
    tN2kMsg msg3;
    double variation = -0.054;  // 3.1° West
    SetN2kPGN129029(msg3, 0, 0, tN2kGNSStype::N2kGNSSt_GPS, tN2kGNSSmethod::N2kGNSSm_GNSS,
                    12, 37.7749, -122.4194, 10.5, N2kGNSSt_GPS, N2kGNSSm_GNSSfix,
                    12, 1.2, 0.8, N2kDoubleNA, 0, tN2kPRNUsageStatus::N2kDD426_Yes, 0, variation);
    HandleN2kPGN129029(msg3, &boatData, &logger);

    gps = boatData.getGPSData();
    TEST_ASSERT_DOUBLE_WITHIN(0.001, variation, gps.variation);

    // Step 4: Receive PGN 127258 - Magnetic Variation (alternative source)
    tN2kMsg msg4;
    double variation2 = -0.052;  // 3.0° West (slightly different)
    SetN2kPGN127258(msg4, 0, N2kmagvar_Manual, 0, variation2);
    HandleN2kPGN127258(msg4, &boatData, &logger);

    gps = boatData.getGPSData();
    // Variation should be updated to latest value
    TEST_ASSERT_DOUBLE_WITHIN(0.001, variation2, gps.variation);

    // Verify all GPS fields populated
    TEST_ASSERT_TRUE(gps.available);
    TEST_ASSERT_GREATER_THAN(0, gps.lastUpdate);
}

/**
 * @test GPS data accumulation with partial updates
 *
 * Validates that GPS data accumulates from multiple partial updates
 * without overwriting existing valid data fields.
 */
void test_gps_partial_updates() {
    BoatData boatData;
    MockLogger logger;

    // Initial state: GPS data should be unavailable
    GPSData gps = boatData.getGPSData();
    TEST_ASSERT_FALSE(gps.available);

    // First update: Position only (PGN 129025)
    tN2kMsg msg1;
    SetN2kPGN129025(msg1, 0, 37.7749, -122.4194);
    HandleN2kPGN129025(msg1, &boatData, &logger);

    gps = boatData.getGPSData();
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 37.7749, gps.latitude);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, -122.4194, gps.longitude);
    // COG/SOG should still be default (0.0)
    TEST_ASSERT_EQUAL_DOUBLE(0.0, gps.cog);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, gps.sog);

    // Second update: COG/SOG (PGN 129026)
    tN2kMsg msg2;
    double cogRad = 45.0 * DEG_TO_RAD;
    double sogMs = 3.0 / 1.94384;
    SetN2kPGN129026(msg2, 0, N2khr_true, cogRad, sogMs);
    HandleN2kPGN129026(msg2, &boatData, &logger);

    gps = boatData.getGPSData();
    // Position should be preserved
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 37.7749, gps.latitude);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, -122.4194, gps.longitude);
    // COG/SOG now updated
    TEST_ASSERT_DOUBLE_WITHIN(0.01, cogRad, gps.cog);
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 3.0, gps.sog);
}

/**
 * @test GPS high-frequency updates (10 Hz simulation)
 *
 * Simulates rapid GPS position updates (PGN 129025 at 10 Hz).
 * Validates that BoatData correctly handles high-frequency updates.
 */
void test_gps_high_frequency_updates() {
    BoatData boatData;
    MockLogger logger;

    // Simulate 10 rapid position updates (as if received at 10 Hz)
    for (int i = 0; i < 10; i++) {
        tN2kMsg msg;
        double lat = 37.7749 + (i * 0.0001);  // Moving slightly north
        double lon = -122.4194 + (i * 0.0001); // Moving slightly east
        SetN2kPGN129025(msg, 0, lat, lon);
        HandleN2kPGN129025(msg, &boatData, &logger);

        // Verify each update is processed
        GPSData gps = boatData.getGPSData();
        TEST_ASSERT_DOUBLE_WITHIN(0.00001, lat, gps.latitude);
        TEST_ASSERT_DOUBLE_WITHIN(0.00001, lon, gps.longitude);
        TEST_ASSERT_TRUE(gps.available);
    }

    // Verify logger captured all updates
    TEST_ASSERT_EQUAL_INT(10, logger.logCount);
}

/**
 * @test GPS variation from two sources (PGN 129029 vs 127258)
 *
 * Validates that both PGN 129029 and PGN 127258 can update GPSData.variation,
 * with the latest update taking precedence.
 */
void test_gps_variation_dual_sources() {
    BoatData boatData;
    MockLogger logger;

    // Source 1: PGN 129029 provides variation
    tN2kMsg msg1;
    double variation1 = -0.054;  // 3.1° West
    SetN2kPGN129029(msg1, 0, 0, tN2kGNSStype::N2kGNSSt_GPS, tN2kGNSSmethod::N2kGNSSm_GNSS,
                    12, 37.7749, -122.4194, 10.5, N2kGNSSt_GPS, N2kGNSSm_GNSSfix,
                    12, 1.2, 0.8, N2kDoubleNA, 0, tN2kPRNUsageStatus::N2kDD426_Yes, 0, variation1);
    HandleN2kPGN129029(msg1, &boatData, &logger);

    GPSData gps = boatData.getGPSData();
    TEST_ASSERT_DOUBLE_WITHIN(0.001, variation1, gps.variation);

    // Source 2: PGN 127258 provides different variation (overrides)
    tN2kMsg msg2;
    double variation2 = -0.060;  // 3.4° West
    SetN2kPGN127258(msg2, 0, N2kmagvar_Manual, 0, variation2);
    HandleN2kPGN127258(msg2, &boatData, &logger);

    gps = boatData.getGPSData();
    TEST_ASSERT_DOUBLE_WITHIN(0.001, variation2, gps.variation);

    // Source 1 updates again (should override source 2)
    double variation3 = -0.055;  // 3.15° West
    SetN2kPGN129029(msg1, 0, 0, tN2kGNSStype::N2kGNSSt_GPS, tN2kGNSSmethod::N2kGNSSm_GNSS,
                    12, 37.7749, -122.4194, 10.5, N2kGNSSt_GPS, N2kGNSSm_GNSSfix,
                    12, 1.2, 0.8, N2kDoubleNA, 0, tN2kPRNUsageStatus::N2kDD426_Yes, 0, variation3);
    HandleN2kPGN129029(msg1, &boatData, &logger);

    gps = boatData.getGPSData();
    TEST_ASSERT_DOUBLE_WITHIN(0.001, variation3, gps.variation);
}

/**
 * @test GPS data validation and range checking
 *
 * Validates that GPS handlers properly validate latitude/longitude ranges
 * and handle edge cases (poles, international date line).
 */
void test_gps_range_validation() {
    BoatData boatData;
    MockLogger logger;

    // Test 1: North Pole (max latitude)
    tN2kMsg msg1;
    SetN2kPGN129025(msg1, 0, 90.0, 0.0);
    HandleN2kPGN129025(msg1, &boatData, &logger);
    GPSData gps = boatData.getGPSData();
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 90.0, gps.latitude);

    // Test 2: South Pole (min latitude)
    tN2kMsg msg2;
    SetN2kPGN129025(msg2, 0, -90.0, 0.0);
    HandleN2kPGN129025(msg2, &boatData, &logger);
    gps = boatData.getGPSData();
    TEST_ASSERT_DOUBLE_WITHIN(0.001, -90.0, gps.latitude);

    // Test 3: International Date Line (max longitude)
    tN2kMsg msg3;
    SetN2kPGN129025(msg3, 0, 0.0, 180.0);
    HandleN2kPGN129025(msg3, &boatData, &logger);
    gps = boatData.getGPSData();
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 180.0, gps.longitude);

    // Test 4: International Date Line (min longitude)
    tN2kMsg msg4;
    SetN2kPGN129025(msg4, 0, 0.0, -180.0);
    HandleN2kPGN129025(msg4, &boatData, &logger);
    gps = boatData.getGPSData();
    TEST_ASSERT_DOUBLE_WITHIN(0.001, -180.0, gps.longitude);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_gps_complete_data_flow);
    RUN_TEST(test_gps_partial_updates);
    RUN_TEST(test_gps_high_frequency_updates);
    RUN_TEST(test_gps_variation_dual_sources);
    RUN_TEST(test_gps_range_validation);
    return UNITY_END();
}
