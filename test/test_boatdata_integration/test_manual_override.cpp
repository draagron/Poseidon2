/**
 * @file test_manual_override.cpp
 * @brief Integration test: Manual priority override (Scenario 4)
 *
 * Tests user-initiated manual source priority override.
 *
 * Acceptance Criterion:
 * Given GPS-B is auto-prioritized (higher frequency),
 * when user manually sets GPS-A as preferred source,
 * then GPS-A becomes active regardless of frequency.
 *
 * @see specs/003-boatdata-feature-as/quickstart.md lines 175-214
 * @see specs/003-boatdata-feature-as/tasks.md T011
 */

#ifdef UNIT_TEST

#include <unity.h>
#include "../../src/components/BoatData.h"
#include "../../src/components/SourcePrioritizer.h"

void setUp(void) {}
void tearDown(void) {}

/**
 * @brief Test manual override forces lower-frequency source active
 *
 * Steps:
 * 1. Establish GPS-B as automatic priority (higher frequency)
 * 2. Simulate web API call: setManualOverride(GPS-A)
 * 3. Assert GPS-A becomes active despite lower frequency
 * 4. Assert GPS-A has manual override flag set
 * 5. Assert GPS-B still available but not active
 */
void test_manual_override_forces_source_active() {
    // Setup
    SourcePrioritizer prioritizer;
    BoatData boatData(&prioritizer);

    int gpsA_idx = prioritizer.registerSource("GPS-A", SensorType::GPS, ProtocolType::NMEA0183);
    int gpsB_idx = prioritizer.registerSource("GPS-B", SensorType::GPS, ProtocolType::NMEA2000);

    unsigned long baseTime = 0;

    // Establish GPS-B as higher frequency (10 Hz vs 1 Hz)
    for (int i = 0; i < 50; i++) {
        prioritizer.updateSourceTimestamp(gpsB_idx, baseTime + (i * 100));  // 10 Hz
        boatData.updateGPS(40.5, -73.5, 0.5, 6.0, "GPS-B");
    }

    for (int i = 0; i < 5; i++) {
        prioritizer.updateSourceTimestamp(gpsA_idx, baseTime + (i * 1000));  // 1 Hz
        boatData.updateGPS(40.4, -73.6, 0.4, 5.8, "GPS-A");
    }

    prioritizer.updatePriorities(SensorType::GPS, baseTime + 5000);

    // Assert: GPS-B is active (automatic priority)
    int activeIdx = prioritizer.getActiveSource(SensorType::GPS);
    TEST_ASSERT_EQUAL_MESSAGE(gpsB_idx, activeIdx,
        "GPS-B should be active (higher frequency)");

    // Action: User sets manual override for GPS-A
    prioritizer.setManualOverride(gpsA_idx);

    // Assert: GPS-A is now active (manual override)
    activeIdx = prioritizer.getActiveSource(SensorType::GPS);
    TEST_ASSERT_EQUAL_MESSAGE(gpsA_idx, activeIdx,
        "GPS-A should be active after manual override");

    // Assert: GPS-A has manual override flag
    bool hasOverride = prioritizer.hasManualOverride(gpsA_idx);
    TEST_ASSERT_TRUE_MESSAGE(hasOverride,
        "GPS-A should have manual override flag set");

    // Assert: GPS-B still available (not removed, just not active)
    bool gpsB_available = prioritizer.isSourceAvailable(gpsB_idx);
    TEST_ASSERT_TRUE_MESSAGE(gpsB_available,
        "GPS-B should still be available (not active but present)");
}

/**
 * @brief Test clearing manual override restores automatic priority
 *
 * Ensures system returns to frequency-based priority after manual override cleared.
 */
void test_clear_manual_override_restores_automatic() {
    SourcePrioritizer prioritizer;
    BoatData boatData(&prioritizer);

    int gpsA_idx = prioritizer.registerSource("GPS-A", SensorType::GPS, ProtocolType::NMEA0183);
    int gpsB_idx = prioritizer.registerSource("GPS-B", SensorType::GPS, ProtocolType::NMEA2000);

    unsigned long baseTime = 0;

    // Establish GPS-B as higher frequency
    for (int i = 0; i < 50; i++) {
        prioritizer.updateSourceTimestamp(gpsB_idx, baseTime + (i * 100));
        boatData.updateGPS(40.5, -73.5, 0.5, 6.0, "GPS-B");
    }

    for (int i = 0; i < 5; i++) {
        prioritizer.updateSourceTimestamp(gpsA_idx, baseTime + (i * 1000));
        boatData.updateGPS(40.4, -73.6, 0.4, 5.8, "GPS-A");
    }

    prioritizer.updatePriorities(SensorType::GPS, baseTime + 5000);

    // Set manual override to GPS-A
    prioritizer.setManualOverride(gpsA_idx);
    TEST_ASSERT_EQUAL(gpsA_idx, prioritizer.getActiveSource(SensorType::GPS));

    // Clear manual override
    prioritizer.clearManualOverride(SensorType::GPS);

    // Recalculate priorities
    prioritizer.updatePriorities(SensorType::GPS, baseTime + 5000);

    // Assert: GPS-B should be active again (automatic priority restored)
    int activeIdx = prioritizer.getActiveSource(SensorType::GPS);
    TEST_ASSERT_EQUAL_MESSAGE(gpsB_idx, activeIdx,
        "GPS-B should be active after clearing manual override (higher frequency)");
}

/**
 * @brief Test manual override persists across priority recalculations
 *
 * Ensures manual override is not overridden by automatic priority updates.
 */
void test_manual_override_persists() {
    SourcePrioritizer prioritizer;
    BoatData boatData(&prioritizer);

    int gpsA_idx = prioritizer.registerSource("GPS-A", SensorType::GPS, ProtocolType::NMEA0183);
    int gpsB_idx = prioritizer.registerSource("GPS-B", SensorType::GPS, ProtocolType::NMEA2000);

    unsigned long baseTime = 0;

    // Establish GPS-B as higher frequency
    for (int i = 0; i < 50; i++) {
        prioritizer.updateSourceTimestamp(gpsB_idx, baseTime + (i * 100));
    }

    for (int i = 0; i < 5; i++) {
        prioritizer.updateSourceTimestamp(gpsA_idx, baseTime + (i * 1000));
    }

    prioritizer.updatePriorities(SensorType::GPS, baseTime + 5000);

    // Set manual override to GPS-A
    prioritizer.setManualOverride(gpsA_idx);
    TEST_ASSERT_EQUAL(gpsA_idx, prioritizer.getActiveSource(SensorType::GPS));

    // Trigger multiple priority recalculations
    for (int i = 0; i < 10; i++) {
        unsigned long checkTime = baseTime + 6000 + (i * 1000);
        prioritizer.updatePriorities(SensorType::GPS, checkTime);

        // Assert: GPS-A still active (manual override persists)
        int activeIdx = prioritizer.getActiveSource(SensorType::GPS);
        TEST_ASSERT_EQUAL_MESSAGE(gpsA_idx, activeIdx,
            "Manual override should persist across priority updates");
    }
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_manual_override_forces_source_active);
    RUN_TEST(test_clear_manual_override_restores_automatic);
    RUN_TEST(test_manual_override_persists);

    return UNITY_END();
}

#endif // UNIT_TEST
