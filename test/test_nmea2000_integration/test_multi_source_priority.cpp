/**
 * @file test_multi_source_priority.cpp
 * @brief Integration test for NMEA2000 vs NMEA0183 multi-source priority
 *
 * Scenario: System receives GPS data from both NMEA2000 (10 Hz) and NMEA0183 (1 Hz)
 * Validates: NMEA2000 source automatically selected as active due to higher frequency
 *
 * Multi-Source Prioritization Logic:
 * - Sources registered with SourcePrioritizer during initialization
 * - Update frequency measured over time window
 * - Highest frequency source becomes active
 * - Lower frequency sources remain as backup/failover
 *
 * Expected Behavior:
 * - NMEA2000-GPS (10 Hz) → Priority 1 (active)
 * - NMEA0183-VH (1 Hz) → Priority 2 (standby)
 *
 * @see specs/010-nmea-2000-handling/tasks.md (T037)
 * @see specs/010-nmea-2000-handling/data-model.md (Multi-Source Integration)
 */

#ifdef UNIT_TEST

#include <unity.h>
#include <NMEA2000.h>
#include <N2kMessages.h>
#include "../../src/components/BoatData.h"
#include "../../src/components/SourcePrioritizer.h"
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
 * @test NMEA2000 GPS (10 Hz) takes priority over NMEA0183 GPS (1 Hz)
 *
 * Simulates realistic scenario:
 * - NMEA2000 GPS broadcasting position at 10 Hz (PGN 129025)
 * - NMEA0183 VHF GPS broadcasting position at 1 Hz (simulated via direct updates)
 * - System should automatically select NMEA2000 as active source
 */
void test_nmea2000_priority_over_nmea0183() {
    // Setup: Create BoatData with source prioritizer
    SourcePrioritizer prioritizer;
    BoatData boatData(&prioritizer);
    MockLogger logger;

    // Register both GPS sources (as done in main.cpp)
    int nmea0183_idx = prioritizer.registerSource("NMEA0183-VH", SensorType::GPS, ProtocolType::NMEA0183);
    int nmea2000_idx = prioritizer.registerSource("NMEA2000-GPS", SensorType::GPS, ProtocolType::NMEA2000);

    TEST_ASSERT_GREATER_THAN_OR_EQUAL(0, nmea0183_idx);
    TEST_ASSERT_GREATER_THAN_OR_EQUAL(0, nmea2000_idx);

    // Simulate updates over 2 seconds
    unsigned long baseTime = 0;

    // NMEA0183-VH: 1 Hz (2 updates over 2 seconds)
    // Simulating VHF GPS providing position
    for (int i = 0; i < 2; i++) {
        unsigned long timestamp = baseTime + (i * 1000);  // Every 1000ms
        prioritizer.updateSourceTimestamp(nmea0183_idx, timestamp);

        // Direct GPS update (simulating NMEA0183 handler)
        boatData.updateGPS(37.7749, -122.4194, 0.0, 5.0, "NMEA0183-VH");
    }

    // NMEA2000-GPS: 10 Hz (20 updates over 2 seconds via PGN 129025)
    for (int i = 0; i < 20; i++) {
        unsigned long timestamp = baseTime + (i * 100);  // Every 100ms
        prioritizer.updateSourceTimestamp(nmea2000_idx, timestamp);

        // Create NMEA2000 PGN 129025 message
        tN2kMsg msg;
        double lat = 37.7749 + (i * 0.0001);  // Slightly changing position
        double lon = -122.4194 + (i * 0.0001);
        SetN2kPGN129025(msg, 0, lat, lon);

        // Process through handler (updates BoatData with NMEA2000-GPS source)
        HandleN2kPGN129025(msg, &boatData, &logger);
    }

    // Trigger priority recalculation
    prioritizer.updatePriorities(SensorType::GPS, baseTime + 2000);

    // Assert: NMEA2000-GPS should be active (10 Hz > 1 Hz)
    int activeSourceIdx = prioritizer.getActiveSource(SensorType::GPS);
    TEST_ASSERT_EQUAL_MESSAGE(nmea2000_idx, activeSourceIdx,
        "NMEA2000-GPS should be selected as active source (10 Hz > 1 Hz)");

    // Assert: GPS data should reflect NMEA2000's last update
    GPSData gps = boatData.getGPSData();
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 37.7749 + (19 * 0.0001), gps.latitude);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, -122.4194 + (19 * 0.0001), gps.longitude);
    TEST_ASSERT_TRUE(gps.available);
}

/**
 * @test Frequency calculation accuracy for NMEA2000 vs NMEA0183
 *
 * Verifies that the source prioritizer correctly measures update frequencies
 * for both NMEA2000 (10 Hz) and NMEA0183 (1 Hz) sources.
 */
void test_frequency_calculation_nmea2000_vs_nmea0183() {
    SourcePrioritizer prioritizer;

    // Register sources
    int nmea0183_idx = prioritizer.registerSource("NMEA0183-VH", SensorType::GPS, ProtocolType::NMEA0183);
    int nmea2000_idx = prioritizer.registerSource("NMEA2000-GPS", SensorType::GPS, ProtocolType::NMEA2000);

    // Simulate NMEA0183: 1 Hz over 3 seconds (3 updates)
    for (int i = 0; i < 3; i++) {
        prioritizer.updateSourceTimestamp(nmea0183_idx, i * 1000);
    }

    // Simulate NMEA2000: 10 Hz over 3 seconds (30 updates)
    for (int i = 0; i < 30; i++) {
        prioritizer.updateSourceTimestamp(nmea2000_idx, i * 100);
    }

    // Trigger priority calculation
    prioritizer.updatePriorities(SensorType::GPS, 3000);

    // Get measured frequencies
    double freq_nmea0183 = prioritizer.getSourceFrequency(nmea0183_idx);
    double freq_nmea2000 = prioritizer.getSourceFrequency(nmea2000_idx);

    // Assert approximate frequencies
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.5, 1.0, freq_nmea0183,
        "NMEA0183 frequency should be ~1 Hz");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(1.0, 10.0, freq_nmea2000,
        "NMEA2000 frequency should be ~10 Hz");

    // Assert NMEA2000 has higher frequency
    TEST_ASSERT_GREATER_THAN_MESSAGE(freq_nmea0183, freq_nmea2000,
        "NMEA2000 frequency should be greater than NMEA0183");

    // Assert NMEA2000 is selected as active
    int activeIdx = prioritizer.getActiveSource(SensorType::GPS);
    TEST_ASSERT_EQUAL(nmea2000_idx, activeIdx);
}

/**
 * @test Mixed data flow: NMEA2000 position + NMEA0183 position
 *
 * Validates that when both sources are active, the higher frequency source
 * (NMEA2000) data is used, but NMEA0183 data is still processed and available
 * for failover.
 */
void test_mixed_nmea2000_nmea0183_data_flow() {
    SourcePrioritizer prioritizer;
    BoatData boatData(&prioritizer);
    MockLogger logger;

    // Register sources
    int nmea0183_idx = prioritizer.registerSource("NMEA0183-VH", SensorType::GPS, ProtocolType::NMEA0183);
    int nmea2000_idx = prioritizer.registerSource("NMEA2000-GPS", SensorType::GPS, ProtocolType::NMEA2000);

    unsigned long baseTime = 0;

    // Interleaved updates (realistic scenario)
    // NMEA2000 at 10 Hz, NMEA0183 at 1 Hz

    // First NMEA0183 update (t=0)
    prioritizer.updateSourceTimestamp(nmea0183_idx, baseTime);
    boatData.updateGPS(37.7700, -122.4100, 0.0, 5.0, "NMEA0183-VH");

    // Ten NMEA2000 updates (t=0 to t=900ms)
    for (int i = 0; i < 10; i++) {
        unsigned long timestamp = baseTime + (i * 100);
        prioritizer.updateSourceTimestamp(nmea2000_idx, timestamp);

        tN2kMsg msg;
        SetN2kPGN129025(msg, 0, 37.7750 + (i * 0.0001), -122.4200 + (i * 0.0001));
        HandleN2kPGN129025(msg, &boatData, &logger);
    }

    // Second NMEA0183 update (t=1000ms)
    prioritizer.updateSourceTimestamp(nmea0183_idx, baseTime + 1000);
    boatData.updateGPS(37.7701, -122.4101, 0.0, 5.0, "NMEA0183-VH");

    // Ten more NMEA2000 updates (t=1000 to t=1900ms)
    for (int i = 10; i < 20; i++) {
        unsigned long timestamp = baseTime + (i * 100);
        prioritizer.updateSourceTimestamp(nmea2000_idx, timestamp);

        tN2kMsg msg;
        SetN2kPGN129025(msg, 0, 37.7750 + (i * 0.0001), -122.4200 + (i * 0.0001));
        HandleN2kPGN129025(msg, &boatData, &logger);
    }

    // Calculate priorities at t=2000ms
    prioritizer.updatePriorities(SensorType::GPS, baseTime + 2000);

    // Assert: NMEA2000 should be active
    int activeIdx = prioritizer.getActiveSource(SensorType::GPS);
    TEST_ASSERT_EQUAL(nmea2000_idx, activeIdx);

    // Assert: GPS data reflects NMEA2000's last update (higher priority)
    GPSData gps = boatData.getGPSData();
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 37.7750 + (19 * 0.0001), gps.latitude);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, -122.4200 + (19 * 0.0001), gps.longitude);
}

/**
 * @test NMEA2000 Compass priority over NMEA0183 Compass
 *
 * Similar to GPS, but for compass/heading data.
 * NMEA2000 compass at 10 Hz should take priority over NMEA0183 autopilot at 1 Hz.
 */
void test_nmea2000_compass_priority() {
    SourcePrioritizer prioritizer;
    BoatData boatData(&prioritizer);
    MockLogger logger;

    // Register compass sources
    int nmea0183_idx = prioritizer.registerSource("NMEA0183-AP", SensorType::COMPASS, ProtocolType::NMEA0183);
    int nmea2000_idx = prioritizer.registerSource("NMEA2000-COMPASS", SensorType::COMPASS, ProtocolType::NMEA2000);

    unsigned long baseTime = 0;

    // NMEA0183-AP: 1 Hz (autopilot heading)
    for (int i = 0; i < 2; i++) {
        unsigned long timestamp = baseTime + (i * 1000);
        prioritizer.updateSourceTimestamp(nmea0183_idx, timestamp);
        // Simulate NMEA0183 compass update (not shown - would be from NMEA0183 handler)
    }

    // NMEA2000-COMPASS: 10 Hz (PGN 127250 - Vessel Heading)
    for (int i = 0; i < 20; i++) {
        unsigned long timestamp = baseTime + (i * 100);
        prioritizer.updateSourceTimestamp(nmea2000_idx, timestamp);

        tN2kMsg msg;
        double heading = (90.0 + i) * DEG_TO_RAD;  // Changing heading
        SetN2kPGN127250(msg, 0, heading, N2kDoubleNA, N2kDoubleNA, N2khr_true);
        HandleN2kPGN127250(msg, &boatData, &logger);
    }

    // Calculate priorities
    prioritizer.updatePriorities(SensorType::COMPASS, baseTime + 2000);

    // Assert: NMEA2000-COMPASS should be active
    int activeIdx = prioritizer.getActiveSource(SensorType::COMPASS);
    TEST_ASSERT_EQUAL(nmea2000_idx, activeIdx);

    // Assert: Compass data reflects NMEA2000 heading
    CompassData compass = boatData.getCompassData();
    double expectedHeading = (90.0 + 19) * DEG_TO_RAD;
    TEST_ASSERT_DOUBLE_WITHIN(0.01, expectedHeading, compass.trueHeading);
    TEST_ASSERT_TRUE(compass.available);
}

/**
 * @test Equal frequency sources - first registered wins
 *
 * Edge case: If two sources have identical update frequencies,
 * the first-registered source should be selected.
 */
void test_equal_frequency_first_registered_wins() {
    SourcePrioritizer prioritizer;

    // Register two sources (order matters)
    int source1_idx = prioritizer.registerSource("GPS-Source-1", SensorType::GPS, ProtocolType::NMEA2000);
    int source2_idx = prioritizer.registerSource("GPS-Source-2", SensorType::GPS, ProtocolType::NMEA2000);

    // Both sources update at exactly 5 Hz
    for (int i = 0; i < 10; i++) {
        prioritizer.updateSourceTimestamp(source1_idx, i * 200);  // Every 200ms
        prioritizer.updateSourceTimestamp(source2_idx, i * 200);  // Every 200ms
    }

    prioritizer.updatePriorities(SensorType::GPS, 2000);

    // Get frequencies (should be equal)
    double freq1 = prioritizer.getSourceFrequency(source1_idx);
    double freq2 = prioritizer.getSourceFrequency(source2_idx);
    TEST_ASSERT_FLOAT_WITHIN(0.1, freq1, freq2);

    // Assert: First registered source wins
    int activeIdx = prioritizer.getActiveSource(SensorType::GPS);
    TEST_ASSERT_EQUAL_MESSAGE(source1_idx, activeIdx,
        "When frequencies are equal, first-registered source should be selected");
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_nmea2000_priority_over_nmea0183);
    RUN_TEST(test_frequency_calculation_nmea2000_vs_nmea0183);
    RUN_TEST(test_mixed_nmea2000_nmea0183_data_flow);
    RUN_TEST(test_nmea2000_compass_priority);
    RUN_TEST(test_equal_frequency_first_registered_wins);
    return UNITY_END();
}

#endif // UNIT_TEST
