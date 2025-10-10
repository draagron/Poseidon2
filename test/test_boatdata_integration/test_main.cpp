/**
 * @file test_main.cpp
 * @brief Unity test runner for BoatData integration tests
 *
 * Integration tests validate end-to-end scenarios from quickstart.md.
 * These tests exercise complete data flow from sensors to data structures.
 *
 * Test Group: test_boatdata_integration
 * Platform: native (with mocked hardware via HAL)
 *
 * @see specs/008-enhanced-boatdata-following/tasks.md (T008-T016)
 * @see specs/008-enhanced-boatdata-following/quickstart.md
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
