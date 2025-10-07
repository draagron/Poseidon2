/**
 * @file test_main.cpp
 * @brief BoatData Unit Tests - Main Entry Point
 *
 * This test suite validates individual calculation functions and utilities:
 * - AngleUtils: Normalization, wraparound, conversions
 * - DataValidator: Range checks, rate-of-change validation
 * - Calculation formulas: AWA correction, leeway, true wind, VMG, current
 *
 * Each test validates a single function with known inputs/outputs
 * and edge cases (singularities, wraparound, zero values).
 *
 * @see specs/003-boatdata-feature-as/research.md (Decision 2: Mathematical formulas)
 * @see specs/003-boatdata-feature-as/tasks.md T015-T024
 */

#ifdef UNIT_TEST

#include <unity.h>
#include <cmath>

// Forward declarations for test functions

// AngleUtils tests (T015-T016)
void test_angle_normalize_to_pi();
void test_angle_normalize_to_two_pi();
void test_angle_wraparound_positive();
void test_angle_wraparound_negative();
void test_angle_difference();
void test_deg_to_rad_conversion();

// DataValidator tests (T017-T018)
void test_validator_gps_range_checks();
void test_validator_gps_rate_of_change();
void test_validator_compass_range_checks();
void test_validator_compass_rate_of_change();
void test_validator_wind_range_checks();
void test_validator_wind_rate_of_change();
void test_validator_speed_range_checks();
void test_validator_speed_rate_of_change();

// Calculation formula tests (T019-T024)
void test_awa_offset_correction();
void test_awa_heel_correction_normal();
void test_awa_heel_correction_singularity();
void test_leeway_calculation_normal();
void test_leeway_calculation_zero_speed();
void test_leeway_calculation_same_side();
void test_true_wind_speed_calculation();
void test_true_wind_angle_calculation();
void test_true_wind_angle_singularity();
void test_vmg_calculation();
void test_wind_direction_calculation();
void test_current_speed_calculation();
void test_current_direction_calculation();
void test_current_singularity();

void setUp(void) {
    // Runs before each test
}

void tearDown(void) {
    // Runs after each test
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // AngleUtils Tests (T015-T016)
    RUN_TEST(test_angle_normalize_to_pi);
    RUN_TEST(test_angle_normalize_to_two_pi);
    RUN_TEST(test_angle_wraparound_positive);
    RUN_TEST(test_angle_wraparound_negative);
    RUN_TEST(test_angle_difference);
    RUN_TEST(test_deg_to_rad_conversion);

    // DataValidator Tests (T017-T018)
    RUN_TEST(test_validator_gps_range_checks);
    RUN_TEST(test_validator_gps_rate_of_change);
    RUN_TEST(test_validator_compass_range_checks);
    RUN_TEST(test_validator_compass_rate_of_change);
    RUN_TEST(test_validator_wind_range_checks);
    RUN_TEST(test_validator_wind_rate_of_change);
    RUN_TEST(test_validator_speed_range_checks);
    RUN_TEST(test_validator_speed_rate_of_change);

    // Calculation Formula Tests (T019-T024)
    RUN_TEST(test_awa_offset_correction);
    RUN_TEST(test_awa_heel_correction_normal);
    RUN_TEST(test_awa_heel_correction_singularity);
    RUN_TEST(test_leeway_calculation_normal);
    RUN_TEST(test_leeway_calculation_zero_speed);
    RUN_TEST(test_leeway_calculation_same_side);
    RUN_TEST(test_true_wind_speed_calculation);
    RUN_TEST(test_true_wind_angle_calculation);
    RUN_TEST(test_true_wind_angle_singularity);
    RUN_TEST(test_vmg_calculation);
    RUN_TEST(test_wind_direction_calculation);
    RUN_TEST(test_current_speed_calculation);
    RUN_TEST(test_current_direction_calculation);
    RUN_TEST(test_current_singularity);

    return UNITY_END();
}

#endif // UNIT_TEST
