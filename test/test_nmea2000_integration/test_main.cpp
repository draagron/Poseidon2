/**
 * @file test_main.cpp
 * @brief Unity test runner for NMEA2000 integration tests
 *
 * Integration tests validate end-to-end scenarios for NMEA2000 PGN handlers.
 * These tests exercise complete data flow from NMEA2000 messages to BoatData structures.
 *
 * Test Group: test_nmea2000_integration
 * Platform: native (with mocked NMEA2000 messages)
 *
 * Test Coverage:
 * - GPS data flow (PGNs 129025, 129026, 129029, 127258)
 * - Compass data flow (PGNs 127250, 127251, 127252, 127257)
 * - DST data flow (PGNs 128267, 128259, 130316)
 * - Engine data flow (PGNs 127488, 127489)
 * - Wind data flow (PGN 130306)
 * - Multi-source prioritization (NMEA2000 vs NMEA0183)
 * - Source failover scenarios
 *
 * @see specs/010-nmea-2000-handling/tasks.md (Phase 6: T032-T038)
 * @see specs/010-nmea2000-handling/quickstart.md
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
    // Tests are run independently in their own executables

    return UNITY_END();
}
