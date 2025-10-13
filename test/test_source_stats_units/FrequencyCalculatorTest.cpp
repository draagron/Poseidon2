/**
 * @file FrequencyCalculatorTest.cpp
 * @brief Unit tests for FrequencyCalculator utility
 *
 * Tests frequency calculation accuracy, edge cases, and circular buffer management.
 *
 * Test Coverage:
 * - 10Hz source (100ms intervals): Expected 9.0-11.0 Hz (±10% tolerance)
 * - 1Hz source (1000ms intervals): Expected 0.9-1.1 Hz
 * - Insufficient samples (<2): Expected 0.0 Hz
 * - Buffer wrapping: Verify circular buffer behavior
 * - Edge cases: Zero interval, single sample
 *
 * @see specs/012-sources-stats-and/contracts/FrequencyCalculatorContract.md
 */

#include <unity.h>
#include <stdint.h>
#include <cmath>

// Include implementation directly for native testing
#include "../../src/utils/FrequencyCalculator.h"
#include "../../src/utils/FrequencyCalculator.cpp"

/**
 * @brief Test frequency calculation for 10Hz source (ideal case)
 *
 * Scenario: GPS updating every 100ms (10 samples)
 * Expected: 10.0 Hz ±10% (9.0-11.0 Hz)
 */
void test_calculate_10hz_source() {
    uint32_t buffer[10] = {1000, 1100, 1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900};
    double freq = FrequencyCalculator::calculate(buffer, 10);

    // Verify frequency is within ±10% tolerance
    TEST_ASSERT_FLOAT_WITHIN(1.0, 10.0, freq);  // 10.0 ± 1.0
}

/**
 * @brief Test frequency calculation for 1Hz source
 *
 * Scenario: NMEA 0183 sentence updating every 1 second (10 samples)
 * Expected: 1.0 Hz ±10% (0.9-1.1 Hz)
 */
void test_calculate_1hz_source() {
    uint32_t buffer[10] = {1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000};
    double freq = FrequencyCalculator::calculate(buffer, 10);

    TEST_ASSERT_FLOAT_WITHIN(0.1, 1.0, freq);  // 1.0 ± 0.1
}

/**
 * @brief Test with insufficient samples (count < 2)
 *
 * Scenario: Source discovered but only 1 sample collected
 * Expected: 0.0 Hz (frequency undefined)
 */
void test_calculate_insufficient_samples() {
    uint32_t buffer[10] = {1000};
    double freq = FrequencyCalculator::calculate(buffer, 1);

    TEST_ASSERT_EQUAL_FLOAT(0.0, freq);
}

/**
 * @brief Test with exactly 2 samples (minimum valid)
 *
 * Scenario: Source with 2 samples, 100ms apart
 * Expected: 10.0 Hz
 */
void test_calculate_two_samples() {
    uint32_t buffer[10] = {1000, 1100};
    double freq = FrequencyCalculator::calculate(buffer, 2);

    TEST_ASSERT_FLOAT_WITHIN(0.1, 10.0, freq);
}

/**
 * @brief Test with zero interval (all timestamps identical)
 *
 * Scenario: Buffer corruption or timing error
 * Expected: 0.0 Hz (guard against division by zero)
 */
void test_calculate_zero_interval() {
    uint32_t buffer[10] = {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000};
    double freq = FrequencyCalculator::calculate(buffer, 10);

    TEST_ASSERT_EQUAL_FLOAT(0.0, freq);
}

/**
 * @brief Test variable frequency source (jitter)
 *
 * Scenario: Source with 80-120ms jitter (10 samples)
 * Expected: ~10 Hz (rolling average smooths jitter)
 */
void test_calculate_variable_frequency() {
    uint32_t buffer[10] = {1000, 1080, 1200, 1280, 1400, 1480, 1600, 1680, 1800, 1880};
    double freq = FrequencyCalculator::calculate(buffer, 10);

    // Total interval: 1880 - 1000 = 880ms
    // Avg interval: 880 / 9 = 97.78ms
    // Frequency: 1000 / 97.78 = 10.23 Hz
    TEST_ASSERT_FLOAT_WITHIN(1.0, 10.0, freq);  // Should be ~10 Hz ±10%
}

/**
 * @brief Test addTimestamp inserts value at correct index
 */
void test_addTimestamp_inserts_value() {
    uint32_t buffer[10] = {0};
    uint8_t index = 0;
    bool full = false;

    FrequencyCalculator::addTimestamp(buffer, index, full, 1234);

    TEST_ASSERT_EQUAL_UINT32(1234, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(1, index);
    TEST_ASSERT_FALSE(full);
}

/**
 * @brief Test addTimestamp wraps around at index 10
 */
void test_addTimestamp_wraps_at_10() {
    uint32_t buffer[10] = {0};
    uint8_t index = 0;
    bool full = false;

    // Add 10 timestamps
    for (int i = 0; i < 10; i++) {
        FrequencyCalculator::addTimestamp(buffer, index, full, 1000 + i * 100);
    }

    // Verify buffer wrapped back to 0
    TEST_ASSERT_EQUAL_UINT8(0, index);

    // Verify full flag set
    TEST_ASSERT_TRUE(full);

    // Verify all values inserted correctly
    for (int i = 0; i < 10; i++) {
        TEST_ASSERT_EQUAL_UINT32(1000 + i * 100, buffer[i]);
    }
}

/**
 * @brief Test addTimestamp circular behavior after wrapping
 */
void test_addTimestamp_circular_overwrites() {
    uint32_t buffer[10] = {0};
    uint8_t index = 0;
    bool full = false;

    // Fill buffer (10 samples)
    for (int i = 0; i < 10; i++) {
        FrequencyCalculator::addTimestamp(buffer, index, full, i);
    }

    // Add 11th sample - should overwrite buffer[0]
    FrequencyCalculator::addTimestamp(buffer, index, full, 999);

    TEST_ASSERT_EQUAL_UINT32(999, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(1, index);
    TEST_ASSERT_TRUE(full);  // Remains true
}

/**
 * @brief Test edge case: millis() rollover simulation
 *
 * Note: millis() rolls over at 49.7 days (UINT32_MAX ms).
 * Frequency calculation will be incorrect for one buffer cycle, then self-corrects.
 * This test documents expected behavior but does not fail on rollover.
 */
void test_calculate_rollover_simulation() {
    // Simulate timestamps near rollover
    uint32_t buffer[10] = {
        0xFFFFFF00, 0xFFFFFF64, 0xFFFFFFC8, 0xFFFFFFFF,  // Before rollover
        0x00000032, 0x00000096, 0x000000FA, 0x0000015E,  // After rollover
        0x000001C2, 0x00000226
    };

    // This will calculate incorrect frequency due to wraparound
    // (last - first = 0x226 - 0xFFFFFF00 = huge negative, wraps to huge positive)
    double freq = FrequencyCalculator::calculate(buffer, 10);

    // Just verify no crash or NaN - frequency will be wrong but will self-correct
    // after buffer fills with post-rollover timestamps
    TEST_ASSERT_FALSE(isnan(freq));
    TEST_ASSERT_FALSE(isinf(freq));
}

// =============================================================================
// Test Registration
// =============================================================================

// Register tests with Unity framework
extern "C" void run_frequency_calculator_tests() {
    // Frequency calculation tests
    RUN_TEST(test_calculate_10hz_source);
    RUN_TEST(test_calculate_1hz_source);
    RUN_TEST(test_calculate_insufficient_samples);
    RUN_TEST(test_calculate_two_samples);
    RUN_TEST(test_calculate_zero_interval);
    RUN_TEST(test_calculate_variable_frequency);

    // Circular buffer tests
    RUN_TEST(test_addTimestamp_inserts_value);
    RUN_TEST(test_addTimestamp_wraps_at_10);
    RUN_TEST(test_addTimestamp_circular_overwrites);

    // Edge cases
    RUN_TEST(test_calculate_rollover_simulation);
}
