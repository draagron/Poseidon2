/**
 * @file test_main.cpp
 * @brief BoatData Contract Tests - Main Entry Point
 *
 * This test suite validates all BoatData HAL interface contracts:
 * - IBoatDataStore: Data storage and retrieval interface
 * - ISensorUpdate: NMEA/1-Wire sensor update interface
 * - ICalibration: Calibration parameter management interface
 * - ISourcePrioritizer: Multi-source prioritization interface
 *
 * Each interface is tested to ensure:
 * 1. All methods are callable
 * 2. Data round-trips correctly (write → read)
 * 3. Validation rules are enforced
 * 4. Error conditions are handled properly
 *
 * @see specs/003-boatdata-feature-as/contracts/README.md
 * @see specs/003-boatdata-feature-as/tasks.md T004-T007
 */

#ifdef UNIT_TEST

#include <unity.h>
#include <cmath>

// Test files will be included here as they're migrated
// Each test file should provide test functions but no main()

// Forward declarations for test functions
// These will be implemented in separate .cpp files

// IBoatDataStore contract tests (T004)
void test_iboatdatastore_gps_getters_setters();
void test_iboatdatastore_compass_getters_setters();
void test_iboatdatastore_wind_getters_setters();
void test_iboatdatastore_speed_getters_setters();
void test_iboatdatastore_rudder_getters_setters();
void test_iboatdatastore_derived_getters_setters();
void test_iboatdatastore_calibration_getters_setters();
void test_iboatdatastore_diagnostics_getters();

// ISensorUpdate contract tests (T005)
void test_isensorupdate_gps_valid_data();
void test_isensorupdate_gps_invalid_data();
void test_isensorupdate_compass_valid_data();
void test_isensorupdate_compass_invalid_data();
void test_isensorupdate_wind_valid_data();
void test_isensorupdate_wind_invalid_data();
void test_isensorupdate_speed_valid_data();
void test_isensorupdate_speed_invalid_data();
void test_isensorupdate_rudder_valid_data();
void test_isensorupdate_rudder_invalid_data();

// ICalibration contract tests (T006)
void test_icalibration_get_set_roundtrip();
void test_icalibration_validation_leeway_factor();
void test_icalibration_validation_wind_offset();
void test_icalibration_load_from_flash();
void test_icalibration_save_to_flash();

// ISourcePrioritizer contract tests (T007)
void test_isourceprioritizer_register_source();
void test_isourceprioritizer_update_timestamp();
void test_isourceprioritizer_get_active_source();
void test_isourceprioritizer_manual_override();
void test_isourceprioritizer_clear_manual_override();
void test_isourceprioritizer_stale_detection();

void setUp(void) {
    // Runs before each test
}

void tearDown(void) {
    // Runs after each test
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // IBoatDataStore Contract Tests (T004)
    RUN_TEST(test_iboatdatastore_gps_getters_setters);
    RUN_TEST(test_iboatdatastore_compass_getters_setters);
    RUN_TEST(test_iboatdatastore_wind_getters_setters);
    RUN_TEST(test_iboatdatastore_speed_getters_setters);
    RUN_TEST(test_iboatdatastore_rudder_getters_setters);
    RUN_TEST(test_iboatdatastore_derived_getters_setters);
    RUN_TEST(test_iboatdatastore_calibration_getters_setters);
    RUN_TEST(test_iboatdatastore_diagnostics_getters);

    // ISensorUpdate Contract Tests (T005)
    RUN_TEST(test_isensorupdate_gps_valid_data);
    RUN_TEST(test_isensorupdate_gps_invalid_data);
    RUN_TEST(test_isensorupdate_compass_valid_data);
    RUN_TEST(test_isensorupdate_compass_invalid_data);
    RUN_TEST(test_isensorupdate_wind_valid_data);
    RUN_TEST(test_isensorupdate_wind_invalid_data);
    RUN_TEST(test_isensorupdate_speed_valid_data);
    RUN_TEST(test_isensorupdate_speed_invalid_data);
    RUN_TEST(test_isensorupdate_rudder_valid_data);
    RUN_TEST(test_isensorupdate_rudder_invalid_data);

    // ICalibration Contract Tests (T006)
    RUN_TEST(test_icalibration_get_set_roundtrip);
    RUN_TEST(test_icalibration_validation_leeway_factor);
    RUN_TEST(test_icalibration_validation_wind_offset);
    RUN_TEST(test_icalibration_load_from_flash);
    RUN_TEST(test_icalibration_save_to_flash);

    // ISourcePrioritizer Contract Tests (T007)
    RUN_TEST(test_isourceprioritizer_register_source);
    RUN_TEST(test_isourceprioritizer_update_timestamp);
    RUN_TEST(test_isourceprioritizer_get_active_source);
    RUN_TEST(test_isourceprioritizer_manual_override);
    RUN_TEST(test_isourceprioritizer_clear_manual_override);
    RUN_TEST(test_isourceprioritizer_stale_detection);

    return UNITY_END();
}

#endif // UNIT_TEST
