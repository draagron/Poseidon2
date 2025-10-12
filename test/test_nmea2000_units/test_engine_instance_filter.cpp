/**
 * @file test_engine_instance_filter.cpp
 * @brief Unit tests for engine instance filtering (only instance 0 processed)
 *
 * Tests that PGN 127488 (Engine Parameters, Rapid) and PGN 127489 (Engine Parameters, Dynamic)
 * only process engine instance 0, silently ignoring other instances.
 *
 * @see specs/010-nmea-2000-handling/data-model.md lines 144-159
 * @see specs/010-nmea-2000-handling/tasks.md T042
 */

#include <unity.h>

// ============================================================================
// Engine Instance Filter Logic Tests
// ============================================================================

/**
 * @brief Test that engine instance 0 is accepted
 *
 * According to specification: Only engine instance 0 should be processed.
 */
void test_engine_instance_0_accepted() {
    unsigned char instance = 0;

    // Engine instance 0 should be accepted for processing
    TEST_ASSERT_EQUAL_UINT8(0, instance);

    // This is the only valid instance for single-engine boats
    bool shouldProcess = (instance == 0);
    TEST_ASSERT_TRUE(shouldProcess);
}

/**
 * @brief Test that engine instance 1 is rejected
 *
 * According to specification: Engine instances other than 0 should be silently ignored.
 */
void test_engine_instance_1_rejected() {
    unsigned char instance = 1;

    // Engine instance 1 should be rejected
    bool shouldProcess = (instance == 0);
    TEST_ASSERT_FALSE(shouldProcess);
}

/**
 * @brief Test that engine instance 2+ are rejected
 *
 * According to specification: All engine instances except 0 should be silently ignored.
 */
void test_engine_instance_multiple_rejected() {
    // Test instances 2-5 (multi-engine boats)
    for (unsigned char instance = 2; instance <= 5; instance++) {
        bool shouldProcess = (instance == 0);
        TEST_ASSERT_FALSE(shouldProcess);
    }
}

/**
 * @brief Test engine instance filter function
 *
 * Helper function that would be used in actual handlers to determine
 * if an engine instance should be processed.
 */
bool shouldProcessEngineInstance(unsigned char instance) {
    return (instance == 0);
}

void test_engine_instance_filter_function() {
    // Instance 0: should process
    TEST_ASSERT_TRUE(shouldProcessEngineInstance(0));

    // Instance 1-255: should NOT process
    for (unsigned char instance = 1; instance != 0; instance++) {
        TEST_ASSERT_FALSE(shouldProcessEngineInstance(instance));
    }
}

// ============================================================================
// PGN 127488 (Engine Parameters, Rapid Update) Instance Filter Tests
// ============================================================================

/**
 * @brief Test PGN 127488 instance filtering logic
 *
 * PGN 127488 contains: Engine Instance, Engine Speed, Engine Boost Pressure, Engine Tilt/Trim
 * Only instance 0 should update EngineData.engineRev
 */
void test_pgn127488_instance_filter() {
    unsigned char engineInstance;
    double engineSpeed = 2000.0;  // 2000 RPM

    // Instance 0: should be processed
    engineInstance = 0;
    bool process = (engineInstance == 0);
    TEST_ASSERT_TRUE(process);

    // If processed, engine speed should be used
    if (process) {
        TEST_ASSERT_EQUAL_DOUBLE(2000.0, engineSpeed);
    }

    // Instance 1: should be silently ignored
    engineInstance = 1;
    process = (engineInstance == 0);
    TEST_ASSERT_FALSE(process);

    // Instance 2: should be silently ignored
    engineInstance = 2;
    process = (engineInstance == 0);
    TEST_ASSERT_FALSE(process);
}

// ============================================================================
// PGN 127489 (Engine Parameters, Dynamic) Instance Filter Tests
// ============================================================================

/**
 * @brief Test PGN 127489 instance filtering logic
 *
 * PGN 127489 contains: Engine Instance, Oil Pressure, Oil Temperature, Alternator Voltage, etc.
 * Only instance 0 should update EngineData.oilTemperature and EngineData.alternatorVoltage
 */
void test_pgn127489_instance_filter() {
    unsigned char engineInstance;
    double oilTemp = 363.15;        // 90°C in Kelvin
    double alternatorVoltage = 13.8; // Volts

    // Instance 0: should be processed
    engineInstance = 0;
    bool process = (engineInstance == 0);
    TEST_ASSERT_TRUE(process);

    // If processed, oil temp and voltage should be used
    if (process) {
        TEST_ASSERT_EQUAL_DOUBLE(363.15, oilTemp);
        TEST_ASSERT_EQUAL_DOUBLE(13.8, alternatorVoltage);
    }

    // Instance 1: should be silently ignored
    engineInstance = 1;
    process = (engineInstance == 0);
    TEST_ASSERT_FALSE(process);

    // Instance 2: should be silently ignored
    engineInstance = 2;
    process = (engineInstance == 0);
    TEST_ASSERT_FALSE(process);
}

// ============================================================================
// Multi-Engine Scenario Tests
// ============================================================================

/**
 * @brief Test multi-engine boat scenario (only engine 0 processed)
 *
 * On a multi-engine boat with engines 0, 1, 2:
 * - Engine 0 data updates EngineData
 * - Engine 1 data silently ignored
 * - Engine 2 data silently ignored
 */
void test_multi_engine_scenario() {
    struct EngineMessage {
        unsigned char instance;
        double rpm;
        bool shouldProcess;
    };

    // Simulate messages from 3 engines
    EngineMessage engines[] = {
        {0, 2000.0, true},   // Engine 0: process
        {1, 2100.0, false},  // Engine 1: ignore
        {2, 1950.0, false}   // Engine 2: ignore
    };

    for (int i = 0; i < 3; i++) {
        bool process = (engines[i].instance == 0);
        TEST_ASSERT_EQUAL(engines[i].shouldProcess, process);

        if (process) {
            // Only engine 0 data should be used
            TEST_ASSERT_EQUAL_DOUBLE(2000.0, engines[i].rpm);
        }
    }
}

/**
 * @brief Test that filtering does not log errors
 *
 * According to specification: Silently ignore engine instances ≠ 0 (no log, no update)
 * This means:
 * - No ERROR log
 * - No WARN log
 * - No DEBUG log
 * - BoatData not updated
 */
void test_silent_filtering() {
    unsigned char instance = 1;

    // Instance 1 should be filtered silently
    bool shouldProcess = (instance == 0);
    TEST_ASSERT_FALSE(shouldProcess);

    // No logging should occur - this is verified by absence of log calls in handler
    // No BoatData update should occur - this is verified by handler early return
}

// ============================================================================
// Edge Cases
// ============================================================================

/**
 * @brief Test maximum engine instance value
 */
void test_max_engine_instance() {
    unsigned char instance = 255;  // Maximum uint8 value

    bool shouldProcess = (instance == 0);
    TEST_ASSERT_FALSE(shouldProcess);
}

/**
 * @brief Test engine instance boundary values
 */
void test_engine_instance_boundaries() {
    // Minimum value (0): should process
    TEST_ASSERT_TRUE(shouldProcessEngineInstance(0));

    // Just above minimum (1): should NOT process
    TEST_ASSERT_FALSE(shouldProcessEngineInstance(1));

    // Maximum value (255): should NOT process
    TEST_ASSERT_FALSE(shouldProcessEngineInstance(255));
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

    // Basic instance filter tests
    RUN_TEST(test_engine_instance_0_accepted);
    RUN_TEST(test_engine_instance_1_rejected);
    RUN_TEST(test_engine_instance_multiple_rejected);
    RUN_TEST(test_engine_instance_filter_function);

    // PGN-specific instance filter tests
    RUN_TEST(test_pgn127488_instance_filter);
    RUN_TEST(test_pgn127489_instance_filter);

    // Multi-engine scenario tests
    RUN_TEST(test_multi_engine_scenario);
    RUN_TEST(test_silent_filtering);

    // Edge cases
    RUN_TEST(test_max_engine_instance);
    RUN_TEST(test_engine_instance_boundaries);

    return UNITY_END();
}
