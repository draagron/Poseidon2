/**
 * @file test_main.cpp
 * @brief Main entry point for source statistics unit tests
 *
 * This file provides the Unity test runner entry point.
 * All test files in this directory are discovered and executed.
 */

#include <unity.h>

// Arduino mock functions for native testing
extern "C" {
    unsigned long millis() {
        return 0;  // Mock implementation - not used in unit tests
    }

    void delay(unsigned long ms) {
        // Mock implementation - no-op for native tests
    }

    // External test registration functions
    void run_frequency_calculator_tests();
}

// Test setup/teardown hooks (defined in individual test files)
void setUp(void) {
    // Setup code runs before each test
}

void tearDown(void) {
    // Cleanup code runs after each test
}

void setup() {
    // Required by Arduino framework - not used
    UNITY_BEGIN();
}

void loop() {
    // Required by Arduino framework - tests run once
    UNITY_END();
}

// Native platform main entry point
int main(int argc, char **argv) {
    UNITY_BEGIN();

    // Run all registered test suites
    run_frequency_calculator_tests();

    UNITY_END();
    return 0;
}
