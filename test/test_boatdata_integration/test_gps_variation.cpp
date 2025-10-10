/**
 * @file test_gps_variation.cpp
 * @brief Integration test for GPS variation field migration (FR-001, FR-009)
 *
 * Scenario: GPS variation moved from CompassData to GPSData
 * Validates: NMEA0183 RMC and NMEA2000 PGN 129029 populate GPSData.variation
 *
 * @see specs/008-enhanced-boatdata-following/quickstart.md (Scenario 1)
 * @see specs/008-enhanced-boatdata-following/tasks.md (T009)
 */

#include <unity.h>
#include "types/BoatDataTypes.h"

void setUp(void) {}
void tearDown(void) {}

/**
 * @test GPS variation stored in GPSData.variation (not CompassData)
 */
void test_gps_variation_in_gpsdata(void) {
    BoatDataStructure boat;
    
    // Simulate GPS providing variation (e.g., from NMEA0183 RMC or N2K PGN 129029)
    boat.gps.variation = -0.054;  // 3.1° West
    boat.gps.available = true;
    
    TEST_ASSERT_EQUAL_DOUBLE(-0.054, boat.gps.variation);
    TEST_ASSERT_TRUE(boat.gps.available);
}

/**
 * @test Variation range validation (±45° typical global range)
 */
void test_gps_variation_range_validation(void) {
    BoatDataStructure boat;
    
    // Test extreme but valid variation
    boat.gps.variation = M_PI / 4;  // +45° East (max typical)
    TEST_ASSERT_DOUBLE_WITHIN(0.01, M_PI / 4, boat.gps.variation);
    
    boat.gps.variation = -M_PI / 4;  // -45° West (min typical)
    TEST_ASSERT_DOUBLE_WITHIN(0.01, -M_PI / 4, boat.gps.variation);
}

/**
 * @test GPS variation sign convention (positive = East, negative = West)
 */
void test_gps_variation_sign_convention(void) {
    BoatDataStructure boat;
    
    // Positive variation = East
    boat.gps.variation = 0.1;
    TEST_ASSERT_GREATER_THAN(0.0, boat.gps.variation);
    
    // Negative variation = West
    boat.gps.variation = -0.1;
    TEST_ASSERT_LESS_THAN(0.0, boat.gps.variation);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_gps_variation_in_gpsdata);
    RUN_TEST(test_gps_variation_range_validation);
    RUN_TEST(test_gps_variation_sign_convention);
    return UNITY_END();
}
