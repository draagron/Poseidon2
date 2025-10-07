/**
 * @file test_multi_source_priority.cpp
 * @brief Integration test: Multi-source GPS priority (Scenario 2)
 *
 * Tests automatic prioritization based on update frequency.
 *
 * Acceptance Criterion:
 * Given GPS-A (NMEA0183, 1 Hz) and GPS-B (NMEA2000, 10 Hz) both providing valid data,
 * then the system uses GPS-B data (higher frequency = higher priority).
 *
 * @see specs/003-boatdata-feature-as/quickstart.md lines 73-122
 * @see specs/003-boatdata-feature-as/tasks.md T009
 */

#ifdef UNIT_TEST

#include <unity.h>
#include "../../src/components/BoatData.h"
#include "../../src/components/SourcePrioritizer.h"

void setUp(void) {}
void tearDown(void) {}

/**
 * @brief Test multi-source GPS with automatic priority selection
 *
 * Steps:
 * 1. Register GPS-A (NMEA0183) and GPS-B (NMEA2000)
 * 2. Simulate updates: GPS-A at 1 Hz, GPS-B at 10 Hz
 * 3. Trigger priority recalculation
 * 4. Assert GPS-B selected as active source (higher frequency)
 */
void test_multi_source_automatic_priority() {
    // Setup: Create BoatData with source prioritizer
    SourcePrioritizer prioritizer;
    BoatData boatData(&prioritizer);

    // Register two GPS sources
    int gpsA_idx = prioritizer.registerSource("GPS-NMEA0183", SensorType::GPS, ProtocolType::NMEA0183);
    int gpsB_idx = prioritizer.registerSource("GPS-NMEA2000", SensorType::GPS, ProtocolType::NMEA2000);

    TEST_ASSERT_GREATER_THAN_OR_EQUAL(0, gpsA_idx);
    TEST_ASSERT_GREATER_THAN_OR_EQUAL(0, gpsB_idx);

    // Simulate updates over 2 seconds
    // GPS-A: 1 Hz (2 updates total)
    // GPS-B: 10 Hz (20 updates total)

    unsigned long baseTime = 0;

    // GPS-A updates (once per second)
    for (int i = 0; i < 2; i++) {
        unsigned long timestamp = baseTime + (i * 1000);
        prioritizer.updateSourceTimestamp(gpsA_idx, timestamp);
        boatData.updateGPS(40.0, -74.0, 0.0, 5.0, "GPS-NMEA0183");
    }

    // GPS-B updates (10 per second, for 2 seconds = 20 updates)
    for (int i = 0; i < 20; i++) {
        unsigned long timestamp = baseTime + (i * 100);  // Every 100ms
        prioritizer.updateSourceTimestamp(gpsB_idx, timestamp);
        boatData.updateGPS(40.1, -73.9, 0.1, 5.5, "GPS-NMEA2000");
    }

    // Trigger priority recalculation
    prioritizer.updatePriorities(SensorType::GPS, baseTime + 2000);

    // Assert: GPS-B should be active (higher frequency)
    int activeSourceIdx = prioritizer.getActiveSource(SensorType::GPS);
    TEST_ASSERT_EQUAL_MESSAGE(gpsB_idx, activeSourceIdx,
        "GPS-B should be selected as active source (10 Hz > 1 Hz)");

    // Assert: GPS data should reflect GPS-B's last update
    GPSData gps = boatData.getGPSData();
    TEST_ASSERT_FLOAT_WITHIN(0.001, 40.1, gps.latitude);
    TEST_ASSERT_FLOAT_WITHIN(0.001, -73.9, gps.longitude);
}

/**
 * @brief Test frequency calculation correctness
 *
 * Verifies that update frequency is calculated correctly.
 */
void test_frequency_calculation() {
    SourcePrioritizer prioritizer;

    int gpsA_idx = prioritizer.registerSource("GPS-A", SensorType::GPS, ProtocolType::NMEA0183);
    int gpsB_idx = prioritizer.registerSource("GPS-B", SensorType::GPS, ProtocolType::NMEA2000);

    // GPS-A: 1 Hz (1 update/second)
    prioritizer.updateSourceTimestamp(gpsA_idx, 0);
    prioritizer.updateSourceTimestamp(gpsA_idx, 1000);
    prioritizer.updateSourceTimestamp(gpsA_idx, 2000);

    // GPS-B: 10 Hz (10 updates/second)
    for (int i = 0; i < 30; i++) {
        prioritizer.updateSourceTimestamp(gpsB_idx, i * 100);  // Every 100ms
    }

    prioritizer.updatePriorities(SensorType::GPS, 3000);

    // Get frequencies
    double freqA = prioritizer.getSourceFrequency(gpsA_idx);
    double freqB = prioritizer.getSourceFrequency(gpsB_idx);

    // Assert approximate frequencies (with tolerance for calculation method)
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.5, 1.0, freqA,
        "GPS-A frequency should be ~1 Hz");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(1.0, 10.0, freqB,
        "GPS-B frequency should be ~10 Hz");

    // Assert higher frequency source is prioritized
    TEST_ASSERT_GREATER_THAN_MESSAGE(freqA, freqB,
        "GPS-B frequency should be greater than GPS-A");
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_multi_source_automatic_priority);
    RUN_TEST(test_frequency_calculation);

    return UNITY_END();
}

#endif // UNIT_TEST
