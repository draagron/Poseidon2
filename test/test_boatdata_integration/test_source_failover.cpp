/**
 * @file test_source_failover.cpp
 * @brief Integration test: Source failover (Scenario 3)
 *
 * Tests automatic failover when primary source becomes stale.
 *
 * Acceptance Criterion:
 * Given GPS-B is active (10 Hz) and GPS-A is standby (1 Hz),
 * when GPS-B stops sending data (>5s no updates),
 * then system automatically switches to GPS-A.
 *
 * @see specs/003-boatdata-feature-as/quickstart.md lines 125-172
 * @see specs/003-boatdata-feature-as/tasks.md T010
 */

#ifdef UNIT_TEST

#include <unity.h>
#include "../../src/components/BoatData.h"
#include "../../src/components/SourcePrioritizer.h"

#define STALE_THRESHOLD_MS 5000  // 5 seconds

void setUp(void) {}
void tearDown(void) {}

/**
 * @brief Test failover from high-frequency to low-frequency source
 *
 * Steps:
 * 1. Start with GPS-B active (higher frequency)
 * 2. Stop GPS-B updates (simulate failure)
 * 3. Continue GPS-A updates
 * 4. Wait >5 seconds (stale threshold)
 * 5. Trigger stale detection
 * 6. Assert GPS-B marked unavailable
 * 7. Assert GPS-A becomes active
 * 8. Assert GPS data still available
 */
void test_source_failover_on_stale() {
    // Setup
    SourcePrioritizer prioritizer;
    BoatData boatData(&prioritizer);

    int gpsA_idx = prioritizer.registerSource("GPS-A", SensorType::GPS, ProtocolType::NMEA0183);
    int gpsB_idx = prioritizer.registerSource("GPS-B", SensorType::GPS, ProtocolType::NMEA2000);

    unsigned long baseTime = 0;

    // Phase 1: Establish GPS-B as primary (higher frequency)
    for (int i = 0; i < 50; i++) {
        prioritizer.updateSourceTimestamp(gpsB_idx, baseTime + (i * 100));  // 10 Hz
        boatData.updateGPS(40.5, -73.5, 0.5, 6.0, "GPS-B");
    }

    // GPS-A also updating (lower frequency)
    for (int i = 0; i < 5; i++) {
        prioritizer.updateSourceTimestamp(gpsA_idx, baseTime + (i * 1000));  // 1 Hz
        boatData.updateGPS(40.4, -73.6, 0.4, 5.8, "GPS-A");
    }

    prioritizer.updatePriorities(SensorType::GPS, baseTime + 5000);

    // Assert: GPS-B is active
    int activeIdx = prioritizer.getActiveSource(SensorType::GPS);
    TEST_ASSERT_EQUAL_MESSAGE(gpsB_idx, activeIdx,
        "GPS-B should be active initially (higher frequency)");

    // Phase 2: GPS-B stops updating (failure simulation)
    unsigned long failureTime = baseTime + 5000;

    // GPS-A continues updating
    for (int i = 0; i < 10; i++) {
        unsigned long timestamp = failureTime + (i * 1000);
        prioritizer.updateSourceTimestamp(gpsA_idx, timestamp);
        boatData.updateGPS(40.4, -73.6, 0.4, 5.8, "GPS-A");
    }

    // Phase 3: Trigger stale detection after 5 seconds
    unsigned long checkTime = failureTime + STALE_THRESHOLD_MS + 1000;  // 6 seconds after GPS-B last update

    bool gpsB_stale = prioritizer.isSourceStale(gpsB_idx, checkTime);
    TEST_ASSERT_TRUE_MESSAGE(gpsB_stale,
        "GPS-B should be marked stale after 5+ seconds with no updates");

    // Update priorities to trigger failover
    prioritizer.updatePriorities(SensorType::GPS, checkTime);

    // Assert: GPS-A is now active (failover occurred)
    activeIdx = prioritizer.getActiveSource(SensorType::GPS);
    TEST_ASSERT_EQUAL_MESSAGE(gpsA_idx, activeIdx,
        "GPS-A should be active after GPS-B became stale");

    // Assert: GPS data still available (from GPS-A)
    GPSData gps = boatData.getGPSData();
    TEST_ASSERT_TRUE_MESSAGE(gps.available,
        "GPS data should still be available (failover to GPS-A)");
    TEST_ASSERT_FLOAT_WITHIN(0.001, 40.4, gps.latitude);
}

/**
 * @brief Test all sources stale - mark data unavailable
 *
 * If all sources become stale, GPS data should be marked unavailable.
 */
void test_all_sources_stale() {
    SourcePrioritizer prioritizer;
    BoatData boatData(&prioritizer);

    int gpsA_idx = prioritizer.registerSource("GPS-A", SensorType::GPS, ProtocolType::NMEA0183);
    int gpsB_idx = prioritizer.registerSource("GPS-B", SensorType::GPS, ProtocolType::NMEA2000);

    unsigned long baseTime = 0;

    // Both sources active initially
    prioritizer.updateSourceTimestamp(gpsA_idx, baseTime);
    prioritizer.updateSourceTimestamp(gpsB_idx, baseTime);
    boatData.updateGPS(40.0, -74.0, 0.0, 5.0, "GPS-A");
    boatData.updateGPS(40.1, -73.9, 0.1, 5.5, "GPS-B");

    prioritizer.updatePriorities(SensorType::GPS, baseTime + 1000);

    // Both sources stop updating
    unsigned long failureTime = baseTime + 1000;
    unsigned long checkTime = failureTime + STALE_THRESHOLD_MS + 1000;  // 6 seconds later

    // Trigger stale detection
    bool gpsA_stale = prioritizer.isSourceStale(gpsA_idx, checkTime);
    bool gpsB_stale = prioritizer.isSourceStale(gpsB_idx, checkTime);

    TEST_ASSERT_TRUE_MESSAGE(gpsA_stale, "GPS-A should be stale");
    TEST_ASSERT_TRUE_MESSAGE(gpsB_stale, "GPS-B should be stale");

    prioritizer.updatePriorities(SensorType::GPS, checkTime);

    // Assert: No active source
    int activeIdx = prioritizer.getActiveSource(SensorType::GPS);
    TEST_ASSERT_EQUAL_MESSAGE(-1, activeIdx,
        "No source should be active when all are stale");

    // Assert: GPS data marked unavailable
    GPSData gps = boatData.getGPSData();
    TEST_ASSERT_FALSE_MESSAGE(gps.available,
        "GPS data should be unavailable when all sources are stale");
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_source_failover_on_stale);
    RUN_TEST(test_all_sources_stale);

    return UNITY_END();
}

#endif // UNIT_TEST
