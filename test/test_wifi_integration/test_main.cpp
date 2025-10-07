/**
 * @file test_main.cpp
 * @brief WiFi Integration Tests - Main Entry Point
 *
 * This test suite validates WiFi connection scenarios:
 * - First-time configuration
 * - Network priority and failover
 * - Connection loss recovery
 * - Services running independently of WiFi
 * - All networks unavailable handling
 *
 * @see user_requirements/R001 - foundation.md
 */

#ifdef UNIT_TEST

#include <unity.h>

// Forward declarations from individual test files
void test_first_time_config();
void test_network_failover();
void test_connection_loss_recovery();
void test_all_networks_unavailable();
void test_services_run_independently();

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_first_time_config);
    RUN_TEST(test_network_failover);
    RUN_TEST(test_connection_loss_recovery);
    RUN_TEST(test_all_networks_unavailable);
    RUN_TEST(test_services_run_independently);

    return UNITY_END();
}

#endif // UNIT_TEST
