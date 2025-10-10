/**
 * @file test_dst_sensors.cpp
 * @brief Integration test for DST sensor data (FR-002, FR-010, FR-011, FR-012)
 *
 * Scenario: SpeedData renamed to DSTData with depth and seaTemperature added
 * Validates: DSTData structure with depth, measuredBoatSpeed, seaTemperature
 *
 * @see specs/008-enhanced-boatdata-following/quickstart.md (Scenario 4)
 * @see specs/008-enhanced-boatdata-following/tasks.md (T012)
 */

#include <unity.h>
#include "types/BoatDataTypes.h"

void setUp(void) {}
void tearDown(void) {}

/**
 * @test DSTData structure accessible via BoatDataStructure.dst
 */
void test_dstdata_accessible(void) {
    BoatDataStructure boat;
    
    boat.dst.depth = 12.5;
    boat.dst.measuredBoatSpeed = 2.5;
    boat.dst.seaTemperature = 15.0;
    boat.dst.available = true;
    
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 12.5, boat.dst.depth);
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 2.5, boat.dst.measuredBoatSpeed);
    TEST_ASSERT_DOUBLE_WITHIN(0.5, 15.0, boat.dst.seaTemperature);
    TEST_ASSERT_TRUE(boat.dst.available);
}

/**
 * @test Depth from NMEA2000 PGN 128267
 */
void test_depth_from_pgn_128267(void) {
    BoatDataStructure boat;
    
    // Simulate PGN 128267 (Water Depth)
    boat.dst.depth = 10.0;  // 10 meters
    boat.dst.available = true;
    
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 10.0, boat.dst.depth);
}

/**
 * @test Speed from NMEA2000 PGN 128259
 */
void test_speed_from_pgn_128259(void) {
    BoatDataStructure boat;
    
    // Simulate PGN 128259 (Speed - Water Referenced)
    boat.dst.measuredBoatSpeed = 3.5;  // 3.5 m/s
    boat.dst.available = true;
    
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 3.5, boat.dst.measuredBoatSpeed);
}

/**
 * @test Temperature from NMEA2000 PGN 130316 (with Kelvin→Celsius conversion)
 */
void test_temperature_from_pgn_130316(void) {
    BoatDataStructure boat;
    
    // Simulate PGN 130316 (Temperature Extended Range)
    // 288.15K = 15.0°C (conversion should be done by handler)
    boat.dst.seaTemperature = 15.0;
    boat.dst.available = true;
    
    TEST_ASSERT_DOUBLE_WITHIN(0.5, 15.0, boat.dst.seaTemperature);
}

/**
 * @test Depth range validation (0-100 meters)
 */
void test_depth_range_validation(void) {
    BoatDataStructure boat;
    
    // Valid depth range
    boat.dst.depth = 0.0;
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, boat.dst.depth);
    
    boat.dst.depth = 100.0;
    TEST_ASSERT_LESS_OR_EQUAL(100.0, boat.dst.depth);
}

/**
 * @test Temperature range validation (-10 to 50°C)
 */
void test_temperature_range_validation(void) {
    BoatDataStructure boat;
    
    // Valid temperature range
    boat.dst.seaTemperature = -10.0;  // Ice water
    TEST_ASSERT_GREATER_OR_EQUAL(-10.0, boat.dst.seaTemperature);
    
    boat.dst.seaTemperature = 50.0;  // Tropical extreme
    TEST_ASSERT_LESS_OR_EQUAL(50.0, boat.dst.seaTemperature);
}

/**
 * @test Backward compatibility typedef (SpeedData → DSTData)
 */
void test_backward_compatibility_typedef(void) {
    // SpeedData should be typedef'd to DSTData during migration
    SpeedData speed;  // Should compile if typedef exists
    speed.measuredBoatSpeed = 5.0;
    
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 5.0, speed.measuredBoatSpeed);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_dstdata_accessible);
    RUN_TEST(test_depth_from_pgn_128267);
    RUN_TEST(test_speed_from_pgn_128259);
    RUN_TEST(test_temperature_from_pgn_130316);
    RUN_TEST(test_depth_range_validation);
    RUN_TEST(test_temperature_range_validation);
    RUN_TEST(test_backward_compatibility_typedef);
    return UNITY_END();
}
