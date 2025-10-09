/**
 * @file test_main.cpp
 * @brief Unity test runner for OLED integration tests
 *
 * Tests end-to-end scenarios for OLED display system with mocked hardware.
 * Validates DisplayManager orchestration with MockDisplayAdapter and
 * MockSystemMetrics.
 *
 * Run: pio test -e native -f test_oled_integration
 *
 * @version 1.0.0
 * @date 2025-10-09
 */

#include <unity.h>
#include "mocks/MockDisplayAdapter.h"
#include "mocks/MockSystemMetrics.h"

// Global mock instances for tests
MockDisplayAdapter* g_mockDisplay = nullptr;
MockSystemMetrics* g_mockMetrics = nullptr;

// Forward declare all test functions
// Integration scenario tests
void test_startup_sequence_displays_subsystem_status();
void test_status_updates_refresh_metrics();
void test_wifi_connected_displays_ssid_and_ip();
void test_wifi_disconnected_displays_message();
void test_wifi_connecting_displays_timer();
void test_animation_cycles_through_icons();
void test_graceful_degradation_when_display_fails();

/**
 * @brief Set up test environment before each test
 */
void setUp(void) {
    // Initialize fresh mocks for each test
    g_mockDisplay = new MockDisplayAdapter();
    g_mockMetrics = new MockSystemMetrics();
}

/**
 * @brief Tear down test environment after each test
 */
void tearDown(void) {
    // Clean up mocks after each test
    delete g_mockDisplay;
    delete g_mockMetrics;
    g_mockDisplay = nullptr;
    g_mockMetrics = nullptr;
}

/**
 * @brief Main test runner
 */
int main(int argc, char **argv) {
    UNITY_BEGIN();

    // Integration scenario tests
    RUN_TEST(test_startup_sequence_displays_subsystem_status);
    RUN_TEST(test_status_updates_refresh_metrics);
    RUN_TEST(test_wifi_connected_displays_ssid_and_ip);
    RUN_TEST(test_wifi_disconnected_displays_message);
    RUN_TEST(test_wifi_connecting_displays_timer);
    RUN_TEST(test_animation_cycles_through_icons);
    RUN_TEST(test_graceful_degradation_when_display_fails);

    return UNITY_END();
}
