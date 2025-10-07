/**
 * @file test_single_gps_source.cpp
 * @brief Integration test: Single source GPS data (Scenario 1)
 *
 * Tests that a single GPS source can update and retrieve position data.
 *
 * Acceptance Criterion:
 * Given the system has one GPS sensor connected via NMEA0183,
 * when the GPS sensor sends position data (latitude/longitude),
 * then the system stores and makes available the current position data.
 *
 * @see specs/003-boatdata-feature-as/quickstart.md lines 30-70
 * @see specs/003-boatdata-feature-as/tasks.md T008
 */

#ifdef UNIT_TEST

#include <unity.h>
#include <math.h>
#include "../../src/components/BoatData.h"
#include "../../src/components/SourcePrioritizer.h"
#include "../../src/mocks/MockSourcePrioritizer.h"

/**
 * Test setup: Create BoatData instance with source prioritizer
 */
void setUp(void) {
    // Test-specific setup if needed
}

/**
 * Test teardown
 */
void tearDown(void) {
    // Test-specific cleanup if needed
}

/**
 * @brief Test single GPS source update and retrieval
 *
 * Steps:
 * 1. Create mock NMEA0183 GPS source
 * 2. Send position data (New York City: 40.7128°N, 74.0060°W)
 * 3. Assert update accepted
 * 4. Assert GPS data stored and retrievable with correct values
 * 5. Assert timestamp recorded
 */
void test_single_gps_source_update() {
    // Setup: Create BoatData with source prioritizer
    MockSourcePrioritizer prioritizer;
    BoatData boatData(&prioritizer);

    // Test data (New York City)
    const char* sourceId = "GPS-NMEA0183";
    double lat = 40.7128;   // Latitude (decimal degrees, North)
    double lon = -74.0060;  // Longitude (decimal degrees, West)
    double cog = 1.571;     // Course over ground (π/2 rad, 90°, heading East)
    double sog = 5.5;       // Speed over ground (knots)

    // Action: Update GPS data
    bool accepted = boatData.updateGPS(lat, lon, cog, sog, sourceId);

    // Assert: Update accepted
    TEST_ASSERT_TRUE_MESSAGE(accepted, "GPS update should be accepted");

    // Assert: GPS data stored correctly
    GPSData gps = boatData.getGPSData();

    TEST_ASSERT_TRUE_MESSAGE(gps.available, "GPS data should be available");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.0001, lat, gps.latitude,
        "Latitude should match input");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.0001, lon, gps.longitude,
        "Longitude should match input");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01, cog, gps.cog,
        "COG should match input");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.1, sog, gps.sog,
        "SOG should match input");
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, gps.lastUpdate,
        "Timestamp should be recorded");
}

/**
 * @brief Test GPS data unavailable before any updates
 *
 * Ensures initial state is correct (no data available).
 */
void test_gps_unavailable_before_update() {
    // Setup
    MockSourcePrioritizer prioritizer;
    BoatData boatData(&prioritizer);

    // Action: Read GPS data before any updates
    GPSData gps = boatData.getGPSData();

    // Assert: GPS not available initially
    TEST_ASSERT_FALSE_MESSAGE(gps.available, "GPS should not be available before first update");
}

/**
 * @brief Test multiple updates from same source
 *
 * Ensures subsequent updates overwrite previous values correctly.
 */
void test_multiple_updates_same_source() {
    // Setup
    MockSourcePrioritizer prioritizer;
    BoatData boatData(&prioritizer);
    const char* sourceId = "GPS-NMEA0183";

    // Action: First update
    bool accepted1 = boatData.updateGPS(40.0, -74.0, 0.0, 5.0, sourceId);
    TEST_ASSERT_TRUE(accepted1);

    GPSData gps1 = boatData.getGPSData();
    unsigned long timestamp1 = gps1.lastUpdate;

    // Small delay to ensure timestamp changes
    // (In real hardware: delay(10); in test: simulate time passage)

    // Action: Second update with different values
    bool accepted2 = boatData.updateGPS(40.5, -73.5, 1.57, 6.0, sourceId);
    TEST_ASSERT_TRUE(accepted2);

    GPSData gps2 = boatData.getGPSData();

    // Assert: New values overwrite old values
    TEST_ASSERT_FLOAT_WITHIN(0.0001, 40.5, gps2.latitude);
    TEST_ASSERT_FLOAT_WITHIN(0.0001, -73.5, gps2.longitude);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.57, gps2.cog);
    TEST_ASSERT_FLOAT_WITHIN(0.1, 6.0, gps2.sog);

    // Note: Timestamp comparison skipped in unit test (would require time mocking)
}

/**
 * Main test runner
 */
int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_gps_unavailable_before_update);
    RUN_TEST(test_single_gps_source_update);
    RUN_TEST(test_multiple_updates_same_source);

    return UNITY_END();
}

#endif // UNIT_TEST
