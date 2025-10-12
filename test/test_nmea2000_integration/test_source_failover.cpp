/**
 * @file test_source_failover.cpp
 * @brief Integration test for NMEA2000 to NMEA0183 automatic failover
 *
 * Scenario: NMEA2000 GPS is active (10 Hz), then stops sending data
 * Validates: System automatically fails over to NMEA0183 GPS (1 Hz) backup source
 *
 * Failover Logic:
 * - Active source monitored for staleness (>5 seconds without update)
 * - When primary becomes stale, system switches to next-highest priority source
 * - When primary recovers, system switches back (higher frequency preferred)
 *
 * Expected Behavior:
 * 1. NMEA2000-GPS active (10 Hz) → GPS data updating
 * 2. NMEA2000-GPS stops → marked stale after 5 seconds
 * 3. NMEA0183-VH promoted → GPS data continues from backup
 * 4. NMEA2000-GPS recovers → automatically switches back
 *
 * @see specs/010-nmea-2000-handling/tasks.md (T038)
 * @see specs/010-nmea-2000-handling/data-model.md (Failover Logic)
 */

#ifdef UNIT_TEST

#include <unity.h>
#include <NMEA2000.h>
#include <N2kMessages.h>
#include "../../src/components/BoatData.h"
#include "../../src/components/SourcePrioritizer.h"
#include "../../src/components/NMEA2000Handlers.h"
#include "../../src/utils/WebSocketLogger.h"

#define STALE_THRESHOLD_MS 5000  // 5 seconds (from data-model.md)

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
 * @test NMEA2000 GPS failure → automatic failover to NMEA0183 GPS
 *
 * Simulates complete failure scenario:
 * - Phase 1: NMEA2000 active at 10 Hz (5 seconds)
 * - Phase 2: NMEA2000 stops, NMEA0183 continues (5+ seconds)
 * - Phase 3: Stale detection triggers failover
 * - Phase 4: GPS data continues from NMEA0183
 */
void test_nmea2000_to_nmea0183_failover() {
    // Setup
    SourcePrioritizer prioritizer;
    BoatData boatData(&prioritizer);
    MockLogger logger;

    int nmea0183_idx = prioritizer.registerSource("NMEA0183-VH", SensorType::GPS, ProtocolType::NMEA0183);
    int nmea2000_idx = prioritizer.registerSource("NMEA2000-GPS", SensorType::GPS, ProtocolType::NMEA2000);

    unsigned long baseTime = 0;

    // ========================================================================
    // PHASE 1: Establish NMEA2000 as primary (higher frequency)
    // ========================================================================

    // NMEA2000: 10 Hz for 5 seconds (50 updates)
    for (int i = 0; i < 50; i++) {
        unsigned long timestamp = baseTime + (i * 100);  // Every 100ms
        prioritizer.updateSourceTimestamp(nmea2000_idx, timestamp);

        tN2kMsg msg;
        SetN2kPGN129025(msg, 0, 37.7750, -122.4200);
        HandleN2kPGN129025(msg, &boatData, &logger);
    }

    // NMEA0183: 1 Hz for 5 seconds (5 updates) - running in background
    for (int i = 0; i < 5; i++) {
        unsigned long timestamp = baseTime + (i * 1000);  // Every 1000ms
        prioritizer.updateSourceTimestamp(nmea0183_idx, timestamp);
        boatData.updateGPS(37.7700, -122.4100, 0.0, 5.0, "NMEA0183-VH");
    }

    // Calculate priorities at t=5000ms
    prioritizer.updatePriorities(SensorType::GPS, baseTime + 5000);

    // Assert: NMEA2000 is active (higher frequency)
    int activeIdx = prioritizer.getActiveSource(SensorType::GPS);
    TEST_ASSERT_EQUAL_MESSAGE(nmea2000_idx, activeIdx,
        "NMEA2000-GPS should be active initially (10 Hz > 1 Hz)");

    // Assert: GPS data from NMEA2000
    GPSData gps = boatData.getGPSData();
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 37.7750, gps.latitude);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, -122.4200, gps.longitude);
    TEST_ASSERT_TRUE(gps.available);

    // ========================================================================
    // PHASE 2: NMEA2000 stops (failure simulation)
    // ========================================================================

    unsigned long failureTime = baseTime + 5000;

    // NMEA2000: NO UPDATES (simulating CAN bus failure or device offline)

    // NMEA0183: Continues at 1 Hz (10 more updates over 10 seconds)
    for (int i = 0; i < 10; i++) {
        unsigned long timestamp = failureTime + (i * 1000);
        prioritizer.updateSourceTimestamp(nmea0183_idx, timestamp);
        boatData.updateGPS(37.7701 + (i * 0.0001), -122.4101 + (i * 0.0001), 0.0, 5.0, "NMEA0183-VH");
    }

    // ========================================================================
    // PHASE 3: Stale detection (>5 seconds since NMEA2000 last update)
    // ========================================================================

    unsigned long checkTime = failureTime + STALE_THRESHOLD_MS + 1000;  // 6 seconds after failure

    // Check if NMEA2000 is stale
    bool nmea2000_stale = prioritizer.isSourceStale(nmea2000_idx, checkTime);
    TEST_ASSERT_TRUE_MESSAGE(nmea2000_stale,
        "NMEA2000-GPS should be marked stale after 5+ seconds without updates");

    // Check if NMEA0183 is still fresh
    bool nmea0183_stale = prioritizer.isSourceStale(nmea0183_idx, checkTime);
    TEST_ASSERT_FALSE_MESSAGE(nmea0183_stale,
        "NMEA0183-VH should NOT be stale (still updating)");

    // Trigger priority recalculation (failover)
    prioritizer.updatePriorities(SensorType::GPS, checkTime);

    // ========================================================================
    // PHASE 4: Verify failover to NMEA0183
    // ========================================================================

    // Assert: NMEA0183 is now active (failover occurred)
    activeIdx = prioritizer.getActiveSource(SensorType::GPS);
    TEST_ASSERT_EQUAL_MESSAGE(nmea0183_idx, activeIdx,
        "NMEA0183-VH should be active after NMEA2000-GPS became stale");

    // Assert: GPS data still available (from NMEA0183)
    gps = boatData.getGPSData();
    TEST_ASSERT_TRUE_MESSAGE(gps.available,
        "GPS data should still be available (failover to NMEA0183)");

    // GPS data should reflect NMEA0183's last update
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 37.7701 + (9 * 0.0001), gps.latitude);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, -122.4101 + (9 * 0.0001), gps.longitude);
}

/**
 * @test NMEA2000 recovery → automatic switch back from NMEA0183
 *
 * After failover to NMEA0183, when NMEA2000 recovers,
 * system should automatically switch back (higher frequency preferred).
 */
void test_nmea2000_recovery_switchback() {
    SourcePrioritizer prioritizer;
    BoatData boatData(&prioritizer);
    MockLogger logger;

    int nmea0183_idx = prioritizer.registerSource("NMEA0183-VH", SensorType::GPS, ProtocolType::NMEA0183);
    int nmea2000_idx = prioritizer.registerSource("NMEA2000-GPS", SensorType::GPS, ProtocolType::NMEA2000);

    unsigned long baseTime = 0;

    // Phase 1: NMEA2000 active
    for (int i = 0; i < 50; i++) {
        prioritizer.updateSourceTimestamp(nmea2000_idx, baseTime + (i * 100));
        tN2kMsg msg;
        SetN2kPGN129025(msg, 0, 37.7750, -122.4200);
        HandleN2kPGN129025(msg, &boatData, &logger);
    }

    for (int i = 0; i < 5; i++) {
        prioritizer.updateSourceTimestamp(nmea0183_idx, baseTime + (i * 1000));
        boatData.updateGPS(37.7700, -122.4100, 0.0, 5.0, "NMEA0183-VH");
    }

    prioritizer.updatePriorities(SensorType::GPS, baseTime + 5000);
    TEST_ASSERT_EQUAL(nmea2000_idx, prioritizer.getActiveSource(SensorType::GPS));

    // Phase 2: NMEA2000 fails, NMEA0183 takes over
    unsigned long failureTime = baseTime + 5000;

    for (int i = 0; i < 10; i++) {
        prioritizer.updateSourceTimestamp(nmea0183_idx, failureTime + (i * 1000));
        boatData.updateGPS(37.7700, -122.4100, 0.0, 5.0, "NMEA0183-VH");
    }

    unsigned long failoverTime = failureTime + STALE_THRESHOLD_MS + 1000;
    prioritizer.updatePriorities(SensorType::GPS, failoverTime);
    TEST_ASSERT_EQUAL(nmea0183_idx, prioritizer.getActiveSource(SensorType::GPS));

    // Phase 3: NMEA2000 recovers (starts sending again at 10 Hz)
    unsigned long recoveryTime = failoverTime + 1000;

    // NMEA2000 resumes at 10 Hz
    for (int i = 0; i < 30; i++) {
        prioritizer.updateSourceTimestamp(nmea2000_idx, recoveryTime + (i * 100));
        tN2kMsg msg;
        SetN2kPGN129025(msg, 0, 37.7751, -122.4201);
        HandleN2kPGN129025(msg, &boatData, &logger);
    }

    // NMEA0183 continues at 1 Hz
    for (int i = 0; i < 3; i++) {
        prioritizer.updateSourceTimestamp(nmea0183_idx, recoveryTime + (i * 1000));
        boatData.updateGPS(37.7700, -122.4100, 0.0, 5.0, "NMEA0183-VH");
    }

    // Recalculate priorities after recovery
    unsigned long checkTime = recoveryTime + 3000;
    prioritizer.updatePriorities(SensorType::GPS, checkTime);

    // Assert: NMEA2000 is active again (higher frequency)
    int activeIdx = prioritizer.getActiveSource(SensorType::GPS);
    TEST_ASSERT_EQUAL_MESSAGE(nmea2000_idx, activeIdx,
        "NMEA2000-GPS should be active again after recovery (10 Hz > 1 Hz)");

    // Assert: GPS data from NMEA2000
    GPSData gps = boatData.getGPSData();
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 37.7751, gps.latitude);
    TEST_ASSERT_TRUE(gps.available);
}

/**
 * @test Both sources fail → GPS data marked unavailable
 *
 * If both NMEA2000 and NMEA0183 become stale,
 * GPS data should be marked unavailable (no valid data).
 */
void test_both_sources_stale_gps_unavailable() {
    SourcePrioritizer prioritizer;
    BoatData boatData(&prioritizer);
    MockLogger logger;

    int nmea0183_idx = prioritizer.registerSource("NMEA0183-VH", SensorType::GPS, ProtocolType::NMEA0183);
    int nmea2000_idx = prioritizer.registerSource("NMEA2000-GPS", SensorType::GPS, ProtocolType::NMEA2000);

    unsigned long baseTime = 0;

    // Both sources active initially
    prioritizer.updateSourceTimestamp(nmea0183_idx, baseTime);
    prioritizer.updateSourceTimestamp(nmea2000_idx, baseTime);

    boatData.updateGPS(37.7700, -122.4100, 0.0, 5.0, "NMEA0183-VH");

    tN2kMsg msg;
    SetN2kPGN129025(msg, 0, 37.7750, -122.4200);
    HandleN2kPGN129025(msg, &boatData, &logger);

    prioritizer.updatePriorities(SensorType::GPS, baseTime + 1000);

    // Both sources stop (simulating complete GPS failure)
    unsigned long failureTime = baseTime + 1000;
    unsigned long checkTime = failureTime + STALE_THRESHOLD_MS + 1000;  // 6 seconds later

    // Check staleness
    bool nmea0183_stale = prioritizer.isSourceStale(nmea0183_idx, checkTime);
    bool nmea2000_stale = prioritizer.isSourceStale(nmea2000_idx, checkTime);

    TEST_ASSERT_TRUE_MESSAGE(nmea0183_stale, "NMEA0183 should be stale");
    TEST_ASSERT_TRUE_MESSAGE(nmea2000_stale, "NMEA2000 should be stale");

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

/**
 * @test Compass failover: NMEA2000 → NMEA0183
 *
 * Similar to GPS, but for compass/heading data.
 * When NMEA2000 compass fails, system should failover to NMEA0183 autopilot.
 */
void test_compass_failover_nmea2000_to_nmea0183() {
    SourcePrioritizer prioritizer;
    BoatData boatData(&prioritizer);
    MockLogger logger;

    int nmea0183_idx = prioritizer.registerSource("NMEA0183-AP", SensorType::COMPASS, ProtocolType::NMEA0183);
    int nmea2000_idx = prioritizer.registerSource("NMEA2000-COMPASS", SensorType::COMPASS, ProtocolType::NMEA2000);

    unsigned long baseTime = 0;

    // Phase 1: NMEA2000 compass active at 10 Hz
    for (int i = 0; i < 50; i++) {
        prioritizer.updateSourceTimestamp(nmea2000_idx, baseTime + (i * 100));
        tN2kMsg msg;
        double heading = 90.0 * DEG_TO_RAD;
        SetN2kPGN127250(msg, 0, heading, N2kDoubleNA, N2kDoubleNA, N2khr_true);
        HandleN2kPGN127250(msg, &boatData, &logger);
    }

    // NMEA0183 autopilot at 1 Hz (background)
    for (int i = 0; i < 5; i++) {
        prioritizer.updateSourceTimestamp(nmea0183_idx, baseTime + (i * 1000));
        // Simulate NMEA0183 compass update (would come from NMEA0183 handler)
    }

    prioritizer.updatePriorities(SensorType::COMPASS, baseTime + 5000);
    TEST_ASSERT_EQUAL(nmea2000_idx, prioritizer.getActiveSource(SensorType::COMPASS));

    // Phase 2: NMEA2000 compass fails
    unsigned long failureTime = baseTime + 5000;

    // NMEA0183 continues
    for (int i = 0; i < 10; i++) {
        prioritizer.updateSourceTimestamp(nmea0183_idx, failureTime + (i * 1000));
        // Continue autopilot updates
    }

    // Trigger failover
    unsigned long checkTime = failureTime + STALE_THRESHOLD_MS + 1000;
    prioritizer.updatePriorities(SensorType::COMPASS, checkTime);

    // Assert: NMEA0183 autopilot now active
    int activeIdx = prioritizer.getActiveSource(SensorType::COMPASS);
    TEST_ASSERT_EQUAL_MESSAGE(nmea0183_idx, activeIdx,
        "NMEA0183-AP should be active after NMEA2000-COMPASS failure");
}

/**
 * @test Partial failover: GPS fails, Compass continues
 *
 * Validates that failover is per-sensor-type.
 * GPS failover should not affect Compass data.
 */
void test_partial_failover_gps_only() {
    SourcePrioritizer prioritizer;
    BoatData boatData(&prioritizer);
    MockLogger logger;

    // Register GPS sources
    int nmea0183_gps_idx = prioritizer.registerSource("NMEA0183-VH", SensorType::GPS, ProtocolType::NMEA0183);
    int nmea2000_gps_idx = prioritizer.registerSource("NMEA2000-GPS", SensorType::GPS, ProtocolType::NMEA2000);

    // Register Compass sources
    int nmea0183_compass_idx = prioritizer.registerSource("NMEA0183-AP", SensorType::COMPASS, ProtocolType::NMEA0183);
    int nmea2000_compass_idx = prioritizer.registerSource("NMEA2000-COMPASS", SensorType::COMPASS, ProtocolType::NMEA2000);

    unsigned long baseTime = 0;

    // Both GPS and Compass from NMEA2000 active initially
    for (int i = 0; i < 50; i++) {
        prioritizer.updateSourceTimestamp(nmea2000_gps_idx, baseTime + (i * 100));
        prioritizer.updateSourceTimestamp(nmea2000_compass_idx, baseTime + (i * 100));

        tN2kMsg gpsMsg;
        SetN2kPGN129025(gpsMsg, 0, 37.7750, -122.4200);
        HandleN2kPGN129025(gpsMsg, &boatData, &logger);

        tN2kMsg compassMsg;
        SetN2kPGN127250(compassMsg, 0, 90.0 * DEG_TO_RAD, N2kDoubleNA, N2kDoubleNA, N2khr_true);
        HandleN2kPGN127250(compassMsg, &boatData, &logger);
    }

    prioritizer.updatePriorities(SensorType::GPS, baseTime + 5000);
    prioritizer.updatePriorities(SensorType::COMPASS, baseTime + 5000);

    // NMEA2000 GPS fails, but Compass continues
    unsigned long failureTime = baseTime + 5000;

    // NMEA0183 GPS takes over
    for (int i = 0; i < 10; i++) {
        prioritizer.updateSourceTimestamp(nmea0183_gps_idx, failureTime + (i * 1000));
        boatData.updateGPS(37.7700, -122.4100, 0.0, 5.0, "NMEA0183-VH");
    }

    // NMEA2000 Compass continues (no failure)
    for (int i = 0; i < 100; i++) {
        prioritizer.updateSourceTimestamp(nmea2000_compass_idx, failureTime + (i * 100));
        tN2kMsg compassMsg;
        SetN2kPGN127250(compassMsg, 0, 90.0 * DEG_TO_RAD, N2kDoubleNA, N2kDoubleNA, N2khr_true);
        HandleN2kPGN127250(compassMsg, &boatData, &logger);
    }

    unsigned long checkTime = failureTime + STALE_THRESHOLD_MS + 1000;
    prioritizer.updatePriorities(SensorType::GPS, checkTime);
    prioritizer.updatePriorities(SensorType::COMPASS, checkTime);

    // Assert: GPS failed over to NMEA0183
    TEST_ASSERT_EQUAL(nmea0183_gps_idx, prioritizer.getActiveSource(SensorType::GPS));

    // Assert: Compass still using NMEA2000
    TEST_ASSERT_EQUAL(nmea2000_compass_idx, prioritizer.getActiveSource(SensorType::COMPASS));

    // Assert: Both data types still available
    GPSData gps = boatData.getGPSData();
    CompassData compass = boatData.getCompassData();
    TEST_ASSERT_TRUE(gps.available);
    TEST_ASSERT_TRUE(compass.available);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_nmea2000_to_nmea0183_failover);
    RUN_TEST(test_nmea2000_recovery_switchback);
    RUN_TEST(test_both_sources_stale_gps_unavailable);
    RUN_TEST(test_compass_failover_nmea2000_to_nmea0183);
    RUN_TEST(test_partial_failover_gps_only);
    return UNITY_END();
}

#endif // UNIT_TEST
