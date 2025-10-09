/**
 * @file test_main.cpp
 * @brief Unity test runner for OLED contract tests
 *
 * Tests HAL interface contracts for IDisplayAdapter and ISystemMetrics.
 * These tests validate that mock implementations correctly implement
 * the interface contracts.
 *
 * Run: pio test -e native -f test_oled_contracts
 *
 * @version 1.0.0
 * @date 2025-10-09
 */

#include <unity.h>

// Forward declare all test functions
// IDisplayAdapter contract tests
void test_init_returns_true_on_success();
void test_init_returns_false_on_failure();
void test_isReady_false_before_init();
void test_isReady_true_after_init();
void test_clear_does_not_crash_before_init();
void test_setCursor_accepts_valid_coordinates();
void test_setTextSize_accepts_valid_sizes();
void test_print_renders_text();
void test_display_pushes_buffer();

// ISystemMetrics contract tests
void test_getFreeHeapBytes_returns_positive_value();
void test_getSketchSizeBytes_returns_positive_value();
void test_getFreeFlashBytes_returns_valid_value();
void test_getCpuIdlePercent_returns_0_to_100();
void test_getMillis_returns_increasing_value();

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

    // IDisplayAdapter contract tests
    RUN_TEST(test_init_returns_true_on_success);
    RUN_TEST(test_init_returns_false_on_failure);
    RUN_TEST(test_isReady_false_before_init);
    RUN_TEST(test_isReady_true_after_init);
    RUN_TEST(test_clear_does_not_crash_before_init);
    RUN_TEST(test_setCursor_accepts_valid_coordinates);
    RUN_TEST(test_setTextSize_accepts_valid_sizes);
    RUN_TEST(test_print_renders_text);
    RUN_TEST(test_display_pushes_buffer);

    // ISystemMetrics contract tests
    RUN_TEST(test_getFreeHeapBytes_returns_positive_value);
    RUN_TEST(test_getSketchSizeBytes_returns_positive_value);
    RUN_TEST(test_getFreeFlashBytes_returns_valid_value);
    RUN_TEST(test_getCpuIdlePercent_returns_0_to_100);
    RUN_TEST(test_getMillis_returns_increasing_value);

    return UNITY_END();
}
