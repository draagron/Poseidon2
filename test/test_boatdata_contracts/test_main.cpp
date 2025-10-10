/**
 * @file test_main.cpp
 * @brief Unity test runner for BoatData contract tests
 *
 * Contract tests validate HAL interface specifications and data structure contracts.
 * These tests MUST pass before implementation begins (TDD approach).
 *
 * Test Group: test_boatdata_contracts
 * Platform: native (no hardware dependencies)
 *
 * @see specs/008-enhanced-boatdata-following/tasks.md (T004-T007)
 */

#include <unity.h>

// Unity test runner setup
void setUp(void) {
    // Called before each test
}

void tearDown(void) {
    // Called after each test
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    // Individual test files will register tests via RUN_TEST()
    return UNITY_END();
}
