/**
 * @file test_main.cpp
 * @brief Hardware timing tests for loop frequency measurement (ESP32 only)
 *
 * Tests actual loop frequency accuracy, measurement overhead, and timing precision
 * on real ESP32 hardware. These tests validate NFR-006 (< 1% overhead) and
 * NFR-007 (±5 Hz accuracy).
 *
 * REQUIRES: ESP32 hardware (run with: pio test -e esp32dev_test -f test_performance_hardware)
 */

#include <unity.h>
#include <Arduino.h>
#include "utils/LoopPerformanceMonitor.h"

// Test configuration
#define TEST_DURATION_MS 10000        // 10-second test duration
#define EXPECTED_MIN_FREQ_HZ 100      // Minimum expected frequency (ESP32 with ReactESP)
#define EXPECTED_MAX_FREQ_HZ 1000     // Maximum expected frequency
#define OVERHEAD_THRESHOLD_US 10      // Maximum acceptable overhead per loop (microseconds)
#define TIMING_ACCURACY_MS 100        // Acceptable timing variance (±100ms for 5-second window)

// Global monitor instance
LoopPerformanceMonitor monitor;

/**
 * HW-001: Test actual loop frequency accuracy on ESP32
 *
 * Validates:
 * - Loop frequency is within expected range (100-1000 Hz)
 * - Measurement updates every 5 seconds
 * - NFR-007: Accuracy within ±5 Hz
 */
void test_loop_frequency_accuracy() {
    Serial.println(F("\n=== HW-001: Loop Frequency Accuracy ==="));

    // Reset monitor
    LoopPerformanceMonitor testMonitor;

    // Run loops for 10 seconds, measuring actual iteration count
    uint32_t startTime = millis();
    uint32_t loopCount = 0;

    Serial.println(F("Running loops for 10 seconds..."));

    while (millis() - startTime < TEST_DURATION_MS) {
        testMonitor.endLoop();
        loopCount++;

        // Don't yield too often - we want to measure actual loop frequency
        if (loopCount % 100 == 0) {
            yield();  // Prevent watchdog timeout
        }
    }

    uint32_t actualDuration = millis() - startTime;
    uint32_t measuredFreq = testMonitor.getLoopFrequency();

    // Calculate expected frequency from actual loop count
    float expectedFreq = (float)loopCount / ((float)actualDuration / 1000.0);

    Serial.printf("Loop count: %lu\n", loopCount);
    Serial.printf("Actual duration: %lu ms\n", actualDuration);
    Serial.printf("Calculated frequency: %.1f Hz\n", expectedFreq);
    Serial.printf("Measured frequency: %lu Hz\n", measuredFreq);

    // Validate frequency is in expected range
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(EXPECTED_MIN_FREQ_HZ, measuredFreq);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(EXPECTED_MAX_FREQ_HZ, measuredFreq);

    // Validate measurement accuracy (±5 Hz per NFR-007)
    int32_t error = abs((int32_t)measuredFreq - (int32_t)expectedFreq);
    Serial.printf("Measurement error: %ld Hz\n", error);

    TEST_ASSERT_LESS_OR_EQUAL_INT32(5, error);

    Serial.println(F("✓ Loop frequency accuracy validated"));
}

/**
 * HW-002: Test measurement overhead
 *
 * Validates:
 * - instrumentLoop() adds < 10 µs per loop (NFR-006)
 * - Performance impact < 1% of loop time
 */
void test_measurement_overhead() {
    Serial.println(F("\n=== HW-002: Measurement Overhead ==="));

    const uint32_t iterations = 10000;

    // Measure baseline loop time (no instrumentation)
    uint32_t baselineStart = micros();
    for (uint32_t i = 0; i < iterations; i++) {
        // Empty loop
        yield();
    }
    uint32_t baselineDuration = micros() - baselineStart;

    // Measure instrumented loop time
    LoopPerformanceMonitor testMonitor;
    uint32_t instrumentedStart = micros();
    for (uint32_t i = 0; i < iterations; i++) {
        testMonitor.endLoop();
        yield();
    }
    uint32_t instrumentedDuration = micros() - instrumentedStart;

    // Calculate overhead
    uint32_t totalOverhead = instrumentedDuration - baselineDuration;
    float overheadPerLoop = (float)totalOverhead / (float)iterations;
    float overheadPercent = ((float)totalOverhead / (float)baselineDuration) * 100.0;

    Serial.printf("Baseline duration: %lu µs (%lu iterations)\n", baselineDuration, iterations);
    Serial.printf("Instrumented duration: %lu µs\n", instrumentedDuration);
    Serial.printf("Total overhead: %lu µs\n", totalOverhead);
    Serial.printf("Overhead per loop: %.2f µs\n", overheadPerLoop);
    Serial.printf("Overhead percentage: %.2f%%\n", overheadPercent);

    // Validate overhead is < 10 µs per loop
    TEST_ASSERT_LESS_OR_EQUAL_FLOAT(OVERHEAD_THRESHOLD_US, overheadPerLoop);

    // Validate overhead is < 1% (NFR-006)
    TEST_ASSERT_LESS_OR_EQUAL_FLOAT(1.0, overheadPercent);

    Serial.println(F("✓ Measurement overhead validated"));
}

/**
 * HW-003: Test 5-second window timing accuracy
 *
 * Validates:
 * - Frequency updates occur every 5 seconds (±100ms)
 * - First measurement appears after exactly 5 seconds
 * - Subsequent measurements at 5-second intervals
 */
void test_5_second_window_accuracy() {
    Serial.println(F("\n=== HW-003: 5-Second Window Accuracy ==="));

    LoopPerformanceMonitor testMonitor;

    // Verify initial state (no measurement yet)
    uint32_t freq = testMonitor.getLoopFrequency();
    TEST_ASSERT_EQUAL_UINT32(0, freq);
    Serial.println(F("✓ Initial state: frequency = 0"));

    // Run loops and measure time to first measurement
    uint32_t startTime = millis();
    uint32_t firstMeasurementTime = 0;
    bool firstMeasurementDetected = false;

    Serial.println(F("Waiting for first measurement..."));

    while (millis() - startTime < 6000) {  // Wait up to 6 seconds
        testMonitor.endLoop();

        uint32_t currentFreq = testMonitor.getLoopFrequency();
        if (!firstMeasurementDetected && currentFreq > 0) {
            firstMeasurementTime = millis() - startTime;
            firstMeasurementDetected = true;
            Serial.printf("First measurement at: %lu ms (frequency: %lu Hz)\n",
                         firstMeasurementTime, currentFreq);
        }

        yield();
    }

    TEST_ASSERT_TRUE(firstMeasurementDetected);

    // Validate first measurement occurred at ~5 seconds (±100ms)
    Serial.printf("Expected: 5000 ms, Actual: %lu ms, Error: %ld ms\n",
                 firstMeasurementTime, (int32_t)firstMeasurementTime - 5000);

    TEST_ASSERT_INT32_WITHIN(TIMING_ACCURACY_MS, 5000, firstMeasurementTime);

    Serial.println(F("✓ First measurement timing validated"));

    // Wait for second measurement to verify 5-second interval
    Serial.println(F("Waiting for second measurement..."));

    uint32_t prevFreq = testMonitor.getLoopFrequency();
    uint32_t secondMeasurementTime = 0;
    bool secondMeasurementDetected = false;

    uint32_t intervalStart = millis();
    while (millis() - intervalStart < 6000) {  // Wait up to 6 seconds
        testMonitor.endLoop();

        uint32_t currentFreq = testMonitor.getLoopFrequency();
        if (!secondMeasurementDetected && currentFreq != prevFreq) {
            secondMeasurementTime = millis() - intervalStart;
            secondMeasurementDetected = true;
            Serial.printf("Second measurement at: %lu ms (frequency: %lu Hz)\n",
                         secondMeasurementTime, currentFreq);
        }

        yield();
    }

    TEST_ASSERT_TRUE(secondMeasurementDetected);

    // Validate second measurement occurred at ~5 seconds after first (±100ms)
    Serial.printf("Expected interval: 5000 ms, Actual: %lu ms, Error: %ld ms\n",
                 secondMeasurementTime, (int32_t)secondMeasurementTime - 5000);

    TEST_ASSERT_INT32_WITHIN(TIMING_ACCURACY_MS, 5000, secondMeasurementTime);

    Serial.println(F("✓ 5-second interval validated"));
}

void setUp(void) {
    // Called before each test
    delay(100);  // Short delay between tests
}

void tearDown(void) {
    // Called after each test
}

void setup() {
    // Initialize serial for test output
    Serial.begin(115200);
    delay(2000);  // Wait for serial connection

    Serial.println(F("\n========================================"));
    Serial.println(F("Hardware Performance Tests (ESP32)"));
    Serial.println(F("========================================"));

    UNITY_BEGIN();

    RUN_TEST(test_loop_frequency_accuracy);
    RUN_TEST(test_measurement_overhead);
    RUN_TEST(test_5_second_window_accuracy);

    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}
