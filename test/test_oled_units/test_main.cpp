/**
 * @file test_main.cpp
 * @brief Unity test runner for OLED unit tests
 *
 * Tests utility functions and component logic in isolation.
 * Validates DisplayLayout formatting, MetricsCollector logic,
 * and DisplayFormatter string operations.
 *
 * Run: pio test -e native -f test_oled_units
 *
 * @version 1.0.0
 * @date 2025-10-09
 */

#include <unity.h>

// Forward declare all test functions
// MetricsCollector unit tests
void test_collectMetrics_gathers_all_values();
void test_collectMetrics_updates_timestamp();
void test_collectMetrics_handles_zero_values();

// DisplayFormatter unit tests
void test_formatBytes_converts_to_KB();
void test_formatBytes_handles_small_values();
void test_formatBytes_handles_large_values();
void test_formatPercent_formats_correctly();
void test_formatPercent_handles_0_and_100();
void test_formatIPAddress_formats_correctly();
void test_formatIPAddress_handles_empty();

// DisplayLayout utility tests
void test_getLineY_returns_correct_positions();
void test_getLineY_handles_invalid_line();
void test_formatFlashUsage_formats_correctly();
void test_getAnimationIcon_cycles_correctly();

// Memory footprint validation tests
void test_static_allocation_under_target();
void test_no_dynamic_allocation_in_formatter();
void test_progmem_strings_used();
void test_efficient_data_types_used();

/**
 * @brief Set up test environment before each test
 */
void setUp(void) {
    // Called before each test
}

/**
 * @brief Tear down test environment after each test
 */
void tearDown(void) {
    // Called after each test
}

/**
 * @brief Main test runner
 */
int main(int argc, char **argv) {
    UNITY_BEGIN();

    // MetricsCollector unit tests
    RUN_TEST(test_collectMetrics_gathers_all_values);
    RUN_TEST(test_collectMetrics_updates_timestamp);
    RUN_TEST(test_collectMetrics_handles_zero_values);

    // DisplayFormatter unit tests
    RUN_TEST(test_formatBytes_converts_to_KB);
    RUN_TEST(test_formatBytes_handles_small_values);
    RUN_TEST(test_formatBytes_handles_large_values);
    RUN_TEST(test_formatPercent_formats_correctly);
    RUN_TEST(test_formatPercent_handles_0_and_100);
    RUN_TEST(test_formatIPAddress_formats_correctly);
    RUN_TEST(test_formatIPAddress_handles_empty);

    // DisplayLayout utility tests
    RUN_TEST(test_getLineY_returns_correct_positions);
    RUN_TEST(test_getLineY_handles_invalid_line);
    RUN_TEST(test_formatFlashUsage_formats_correctly);
    RUN_TEST(test_getAnimationIcon_cycles_correctly);

    // Memory footprint validation tests
    RUN_TEST(test_static_allocation_under_target);
    RUN_TEST(test_no_dynamic_allocation_in_formatter);
    RUN_TEST(test_progmem_strings_used);
    RUN_TEST(test_efficient_data_types_used);

    return UNITY_END();
}
