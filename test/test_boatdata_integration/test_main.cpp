/**
 * @file test_main.cpp
 * @brief BoatData Integration Tests - Main Entry Point
 *
 * This test suite validates complete BoatData integration scenarios:
 * - Scenario 1: Single GPS source data flow
 * - Scenario 2: Multi-source GPS with automatic prioritization
 * - Scenario 3: Source failover on data loss
 * - Scenario 4: Manual priority override via API
 * - Scenario 5: Derived parameter calculations (11 parameters)
 * - Scenario 6: Calibration parameter updates and persistence
 * - Scenario 7: Outlier detection and rejection
 *
 * Each scenario tests the full stack integration with mocked hardware.
 *
 * @see specs/003-boatdata-feature-as/quickstart.md
 * @see specs/003-boatdata-feature-as/tasks.md T008-T014
 */

#ifdef UNIT_TEST

#include <unity.h>
#include <cmath>

// Forward declarations for test functions from individual test files

// Scenario 1: Single GPS source (T008)
void test_single_gps_source_update_and_retrieve();
void test_single_gps_source_availability_flag();
void test_single_gps_source_timestamp_tracking();

// Scenario 2: Multi-source priority (T009)
void test_multi_source_automatic_priority();
void test_frequency_calculation();
void test_lower_frequency_not_active();
void test_priority_recalculation();

// Scenario 3: Source failover (T010)
void test_failover_on_stale_data();
void test_fallback_source_activated();
void test_gps_data_still_available_after_failover();

// Scenario 4: Manual override (T011)
void test_manual_override_forces_source();
void test_manual_override_flag_set();
void test_other_source_remains_available();

// Scenario 5: Derived calculations (T012)
void test_all_derived_parameters_calculated();
void test_awa_corrections();
void test_leeway_and_stw();
void test_true_wind_calculation();
void test_vmg_calculation();
void test_current_calculation();

// Scenario 6: Calibration update (T013)
void test_calibration_load_defaults();
void test_calibration_update_via_api();
void test_calibration_persistence();
void test_calibration_used_in_calculation();

// Scenario 7: Outlier rejection (T014)
void test_outlier_range_check_rejection();
void test_outlier_rate_of_change_rejection();
void test_valid_data_retained_after_outlier();
void test_rejection_logged();

void setUp(void) {
    // Runs before each test
}

void tearDown(void) {
    // Runs after each test
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // Scenario 1: Single GPS Source (T008)
    RUN_TEST(test_single_gps_source_update_and_retrieve);
    RUN_TEST(test_single_gps_source_availability_flag);
    RUN_TEST(test_single_gps_source_timestamp_tracking);

    // Scenario 2: Multi-Source Priority (T009)
    RUN_TEST(test_multi_source_automatic_priority);
    RUN_TEST(test_frequency_calculation);
    RUN_TEST(test_lower_frequency_not_active);
    RUN_TEST(test_priority_recalculation);

    // Scenario 3: Source Failover (T010)
    RUN_TEST(test_failover_on_stale_data);
    RUN_TEST(test_fallback_source_activated);
    RUN_TEST(test_gps_data_still_available_after_failover);

    // Scenario 4: Manual Override (T011)
    RUN_TEST(test_manual_override_forces_source);
    RUN_TEST(test_manual_override_flag_set);
    RUN_TEST(test_other_source_remains_available);

    // Scenario 5: Derived Calculations (T012)
    RUN_TEST(test_all_derived_parameters_calculated);
    RUN_TEST(test_awa_corrections);
    RUN_TEST(test_leeway_and_stw);
    RUN_TEST(test_true_wind_calculation);
    RUN_TEST(test_vmg_calculation);
    RUN_TEST(test_current_calculation);

    // Scenario 6: Calibration Update (T013)
    RUN_TEST(test_calibration_load_defaults);
    RUN_TEST(test_calibration_update_via_api);
    RUN_TEST(test_calibration_persistence);
    RUN_TEST(test_calibration_used_in_calculation);

    // Scenario 7: Outlier Rejection (T014)
    RUN_TEST(test_outlier_range_check_rejection);
    RUN_TEST(test_outlier_rate_of_change_rejection);
    RUN_TEST(test_valid_data_retained_after_outlier);
    RUN_TEST(test_rejection_logged);

    return UNITY_END();
}

#endif // UNIT_TEST
