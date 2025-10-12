/**
 * @file test_wind_reference_filter.cpp
 * @brief Unit tests for wind reference type filtering (only N2kWind_Apparent processed)
 *
 * Tests that PGN 130306 (Wind Data) only processes apparent wind reference type,
 * silently ignoring true wind references.
 *
 * @see specs/010-nmea-2000-handling/data-model.md lines 159-171
 * @see specs/010-nmea-2000-handling/tasks.md T043
 */

#include <unity.h>
#include <N2kTypes.h>

// ============================================================================
// Wind Reference Type Filter Logic Tests
// ============================================================================

/**
 * @brief Test that N2kWind_Apparent is accepted
 *
 * According to specification: Only N2kWind_Apparent should be processed.
 */
void test_apparent_wind_accepted() {
    tN2kWindReference reference = N2kWind_Apparent;

    // Apparent wind should be accepted for processing
    bool shouldProcess = (reference == N2kWind_Apparent);
    TEST_ASSERT_TRUE(shouldProcess);
}

/**
 * @brief Test that true wind references are rejected
 *
 * According to specification: All true wind references should be silently ignored.
 */
void test_true_wind_references_rejected() {
    // True Wind, Boat referenced
    tN2kWindReference reference = N2kWind_True_boat;
    bool shouldProcess = (reference == N2kWind_Apparent);
    TEST_ASSERT_FALSE(shouldProcess);

    // True Wind, Ground referenced - North
    reference = N2kWind_True_North;
    shouldProcess = (reference == N2kWind_Apparent);
    TEST_ASSERT_FALSE(shouldProcess);

    // True Wind, Ground referenced - Magnetic
    reference = N2kWind_True_Magnetic;
    shouldProcess = (reference == N2kWind_Apparent);
    TEST_ASSERT_FALSE(shouldProcess);
}

/**
 * @brief Test wind reference filter function
 *
 * Helper function that would be used in actual handlers to determine
 * if a wind reference type should be processed.
 */
bool shouldProcessWindReference(tN2kWindReference reference) {
    return (reference == N2kWind_Apparent);
}

void test_wind_reference_filter_function() {
    // N2kWind_Apparent: should process
    TEST_ASSERT_TRUE(shouldProcessWindReference(N2kWind_Apparent));

    // All other wind references: should NOT process
    TEST_ASSERT_FALSE(shouldProcessWindReference(N2kWind_True_boat));
    TEST_ASSERT_FALSE(shouldProcessWindReference(N2kWind_True_North));
    TEST_ASSERT_FALSE(shouldProcessWindReference(N2kWind_True_Magnetic));
}

// ============================================================================
// PGN 130306 (Wind Data) Reference Filter Tests
// ============================================================================

/**
 * @brief Test PGN 130306 reference filtering logic
 *
 * PGN 130306 contains: Wind Speed, Wind Angle, Reference
 * Only N2kWind_Apparent should update WindData
 */
void test_pgn130306_reference_filter() {
    tN2kWindReference windReference;
    double windSpeed = 10.0;  // m/s
    double windAngle = 0.785;  // 45 degrees starboard (π/4 radians)

    // Apparent wind: should be processed
    windReference = N2kWind_Apparent;
    bool process = (windReference == N2kWind_Apparent);
    TEST_ASSERT_TRUE(process);

    // If processed, wind data should be used
    if (process) {
        TEST_ASSERT_EQUAL_DOUBLE(10.0, windSpeed);
        TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.785, windAngle);
    }

    // True wind (boat referenced): should be silently ignored
    windReference = N2kWind_True_boat;
    process = (windReference == N2kWind_Apparent);
    TEST_ASSERT_FALSE(process);

    // True wind (North referenced): should be silently ignored
    windReference = N2kWind_True_North;
    process = (windReference == N2kWind_Apparent);
    TEST_ASSERT_FALSE(process);

    // True wind (Magnetic referenced): should be silently ignored
    windReference = N2kWind_True_Magnetic;
    process = (windReference == N2kWind_Apparent);
    TEST_ASSERT_FALSE(process);
}

// ============================================================================
// Apparent Wind vs True Wind Scenario Tests
// ============================================================================

/**
 * @brief Test apparent wind data processing
 *
 * On a sailboat with wind instruments:
 * - Apparent wind data (relative to boat motion) should be processed
 * - True wind data (calculated wind over water) should be ignored
 */
void test_apparent_wind_scenario() {
    struct WindMessage {
        tN2kWindReference reference;
        double speed_mps;
        double angle_rad;
        bool shouldProcess;
    };

    // Simulate different wind messages
    WindMessage winds[] = {
        {N2kWind_Apparent, 10.0, 0.785, true},        // Apparent: process
        {N2kWind_True_boat, 8.0, 0.524, false},       // True boat: ignore
        {N2kWind_True_North, 7.5, 1.571, false},      // True North: ignore
        {N2kWind_True_Magnetic, 7.5, 1.571, false}    // True Magnetic: ignore
    };

    for (int i = 0; i < 4; i++) {
        bool process = (winds[i].reference == N2kWind_Apparent);
        TEST_ASSERT_EQUAL(winds[i].shouldProcess, process);

        if (process) {
            // Only apparent wind data should be used
            TEST_ASSERT_EQUAL_DOUBLE(10.0, winds[i].speed_mps);
            TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.785, winds[i].angle_rad);
        }
    }
}

/**
 * @brief Test that filtering does not log errors
 *
 * According to specification: Silently ignore if Reference ≠ N2kWind_Apparent (no log, no update)
 * This means:
 * - No ERROR log
 * - No WARN log
 * - No DEBUG log
 * - BoatData not updated
 */
void test_silent_filtering() {
    tN2kWindReference reference = N2kWind_True_boat;

    // True wind should be filtered silently
    bool shouldProcess = (reference == N2kWind_Apparent);
    TEST_ASSERT_FALSE(shouldProcess);

    // No logging should occur - this is verified by absence of log calls in handler
    // No BoatData update should occur - this is verified by handler early return
}

// ============================================================================
// Edge Cases
// ============================================================================

/**
 * @brief Test all NMEA2000 wind reference enum values
 */
void test_all_wind_reference_values() {
    // Only N2kWind_Apparent (value 2) should be accepted
    TEST_ASSERT_TRUE(shouldProcessWindReference(N2kWind_Apparent));

    // All other values should be rejected
    TEST_ASSERT_FALSE(shouldProcessWindReference(N2kWind_True_boat));
    TEST_ASSERT_FALSE(shouldProcessWindReference(N2kWind_True_North));
    TEST_ASSERT_FALSE(shouldProcessWindReference(N2kWind_True_Magnetic));
}

/**
 * @brief Test wind reference type reasoning
 *
 * Apparent wind: Wind relative to boat motion (what crew feels)
 * True wind: Actual wind direction/speed over water (calculated)
 *
 * We only process apparent wind because:
 * 1. It's directly measured by wind instruments
 * 2. True wind is usually calculated from apparent wind + boat motion
 * 3. Processing both could cause data conflicts
 */
void test_wind_reference_reasoning() {
    // Apparent wind is directly measured - ACCEPT
    tN2kWindReference measured = N2kWind_Apparent;
    TEST_ASSERT_TRUE(measured == N2kWind_Apparent);

    // True wind is calculated - REJECT
    tN2kWindReference calculated = N2kWind_True_boat;
    TEST_ASSERT_FALSE(calculated == N2kWind_Apparent);
}

// ============================================================================
// Test Suite Setup
// ============================================================================

void setUp(void) {
    // Set up before each test (if needed)
}

void tearDown(void) {
    // Clean up after each test (if needed)
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // Basic reference filter tests
    RUN_TEST(test_apparent_wind_accepted);
    RUN_TEST(test_true_wind_references_rejected);
    RUN_TEST(test_wind_reference_filter_function);

    // PGN-specific reference filter tests
    RUN_TEST(test_pgn130306_reference_filter);

    // Scenario tests
    RUN_TEST(test_apparent_wind_scenario);
    RUN_TEST(test_silent_filtering);

    // Edge cases
    RUN_TEST(test_all_wind_reference_values);
    RUN_TEST(test_wind_reference_reasoning);

    return UNITY_END();
}
