/**
 * @file test_main.cpp
 * @brief WiFi Unit Tests - Main Entry Point
 *
 * This test suite validates WiFi component units:
 * - Config parser
 * - Connection state machine
 * - WiFi credentials
 * - WiFi config file
 * - WiFi manager logic
 *
 * @see user_requirements/R001 - foundation.md
 */

#ifdef UNIT_TEST

#include <unity.h>

// Forward declarations from individual test files
void test_config_parser();
void test_connection_state();
void test_state_machine();
void test_wifi_credentials();
void test_wifi_config_file();
void test_wifi_manager_logic();

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_config_parser);
    RUN_TEST(test_connection_state);
    RUN_TEST(test_state_machine);
    RUN_TEST(test_wifi_credentials);
    RUN_TEST(test_wifi_config_file);
    RUN_TEST(test_wifi_manager_logic);

    return UNITY_END();
}

#endif // UNIT_TEST
