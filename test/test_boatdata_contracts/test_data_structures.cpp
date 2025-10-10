/**
 * @file test_data_structures.cpp
 * @brief Contract tests for BoatDataTypes_v2 data structures
 *
 * Validates field presence, types, and memory layout of enhanced data structures.
 * These tests ensure the data model contract is satisfied.
 *
 * Test Coverage:
 * - GPSData has variation field
 * - CompassData has rateOfTurn, heelAngle, pitchAngle, heave
 * - CompassData does NOT have variation field
 * - DSTData structure exists (renamed from SpeedData)
 * - DSTData has depth, measuredBoatSpeed, seaTemperature
 * - EngineData structure exists with engineRev, oilTemperature, alternatorVoltage
 * - SaildriveData structure exists with saildriveEngaged
 * - BatteryData structure exists with dual bank fields
 * - ShorePowerData structure exists with shorePowerOn and power
 *
 * @see specs/008-enhanced-boatdata-following/contracts/BoatDataTypes_v2.h
 * @see specs/008-enhanced-boatdata-following/tasks.md (T006)
 */

#include <unity.h>
#include "types/BoatDataTypes.h"

void setUp(void) {}
void tearDown(void) {}

/**
 * @test Verify GPSData has variation field
 */
void test_gpsdata_has_variation_field(void) {
    GPSData gps;
    gps.variation = 0.054;  // 3.1° East
    
    TEST_ASSERT_EQUAL_DOUBLE(0.054, gps.variation);
}

/**
 * @test Verify CompassData has rateOfTurn field
 */
void test_compassdata_has_rate_of_turn_field(void) {
    CompassData compass;
    compass.rateOfTurn = 0.1;  // 0.1 rad/s
    
    TEST_ASSERT_EQUAL_DOUBLE(0.1, compass.rateOfTurn);
}

/**
 * @test Verify CompassData has heelAngle field
 */
void test_compassdata_has_heel_angle_field(void) {
    CompassData compass;
    compass.heelAngle = 0.05;  // ~2.9° starboard
    
    TEST_ASSERT_EQUAL_DOUBLE(0.05, compass.heelAngle);
}

/**
 * @test Verify CompassData has pitchAngle field
 */
void test_compassdata_has_pitch_angle_field(void) {
    CompassData compass;
    compass.pitchAngle = 0.15;  // ~8.6° bow up
    
    TEST_ASSERT_EQUAL_DOUBLE(0.15, compass.pitchAngle);
}

/**
 * @test Verify CompassData has heave field
 */
void test_compassdata_has_heave_field(void) {
    CompassData compass;
    compass.heave = 1.5;  // 1.5 meters upward
    
    TEST_ASSERT_EQUAL_DOUBLE(1.5, compass.heave);
}

/**
 * @test Verify DSTData structure exists (renamed from SpeedData)
 */
void test_dstdata_structure_exists(void) {
    DSTData dst;
    dst.depth = 12.5;
    dst.measuredBoatSpeed = 2.5;
    dst.seaTemperature = 15.0;
    dst.available = true;
    
    TEST_ASSERT_EQUAL_DOUBLE(12.5, dst.depth);
    TEST_ASSERT_EQUAL_DOUBLE(2.5, dst.measuredBoatSpeed);
    TEST_ASSERT_EQUAL_DOUBLE(15.0, dst.seaTemperature);
    TEST_ASSERT_TRUE(dst.available);
}

/**
 * @test Verify DSTData has depth field
 */
void test_dstdata_has_depth_field(void) {
    DSTData dst;
    dst.depth = 10.0;
    
    TEST_ASSERT_EQUAL_DOUBLE(10.0, dst.depth);
}

/**
 * @test Verify DSTData has seaTemperature field
 */
void test_dstdata_has_sea_temperature_field(void) {
    DSTData dst;
    dst.seaTemperature = 18.5;
    
    TEST_ASSERT_EQUAL_DOUBLE(18.5, dst.seaTemperature);
}

/**
 * @test Verify EngineData structure exists
 */
void test_enginedata_structure_exists(void) {
    EngineData engine;
    engine.engineRev = 1800.0;
    engine.oilTemperature = 80.0;
    engine.alternatorVoltage = 14.2;
    engine.available = true;
    
    TEST_ASSERT_EQUAL_DOUBLE(1800.0, engine.engineRev);
    TEST_ASSERT_EQUAL_DOUBLE(80.0, engine.oilTemperature);
    TEST_ASSERT_EQUAL_DOUBLE(14.2, engine.alternatorVoltage);
    TEST_ASSERT_TRUE(engine.available);
}

/**
 * @test Verify SaildriveData structure exists
 */
void test_saildrivedata_structure_exists(void) {
    SaildriveData saildrive;
    saildrive.saildriveEngaged = true;
    saildrive.available = true;
    
    TEST_ASSERT_TRUE(saildrive.saildriveEngaged);
    TEST_ASSERT_TRUE(saildrive.available);
}

/**
 * @test Verify BatteryData structure exists with dual banks
 */
void test_batterydata_structure_exists(void) {
    BatteryData battery;
    battery.voltageA = 12.6;
    battery.amperageA = 5.2;
    battery.stateOfChargeA = 85.0;
    battery.shoreChargerOnA = false;
    battery.engineChargerOnA = true;
    
    battery.voltageB = 12.8;
    battery.amperageB = -2.1;
    battery.stateOfChargeB = 92.0;
    battery.shoreChargerOnB = false;
    battery.engineChargerOnB = false;
    
    battery.available = true;
    
    TEST_ASSERT_EQUAL_DOUBLE(12.6, battery.voltageA);
    TEST_ASSERT_EQUAL_DOUBLE(5.2, battery.amperageA);
    TEST_ASSERT_EQUAL_DOUBLE(85.0, battery.stateOfChargeA);
    TEST_ASSERT_FALSE(battery.shoreChargerOnA);
    TEST_ASSERT_TRUE(battery.engineChargerOnA);
    
    TEST_ASSERT_EQUAL_DOUBLE(12.8, battery.voltageB);
    TEST_ASSERT_EQUAL_DOUBLE(-2.1, battery.amperageB);
    TEST_ASSERT_EQUAL_DOUBLE(92.0, battery.stateOfChargeB);
    TEST_ASSERT_TRUE(battery.available);
}

/**
 * @test Verify ShorePowerData structure exists
 */
void test_shorepowerdata_structure_exists(void) {
    ShorePowerData shorePower;
    shorePower.shorePowerOn = true;
    shorePower.power = 1200.0;
    shorePower.available = true;
    
    TEST_ASSERT_TRUE(shorePower.shorePowerOn);
    TEST_ASSERT_EQUAL_DOUBLE(1200.0, shorePower.power);
    TEST_ASSERT_TRUE(shorePower.available);
}

/**
 * @test Verify BoatDataStructure has dst field (renamed from speed)
 */
void test_boatdatastructure_has_dst_field(void) {
    BoatDataStructure boat;
    boat.dst.depth = 15.0;
    
    TEST_ASSERT_EQUAL_DOUBLE(15.0, boat.dst.depth);
}

/**
 * @test Verify BoatDataStructure has engine field
 */
void test_boatdatastructure_has_engine_field(void) {
    BoatDataStructure boat;
    boat.engine.engineRev = 2000.0;
    
    TEST_ASSERT_EQUAL_DOUBLE(2000.0, boat.engine.engineRev);
}

/**
 * @test Verify BoatDataStructure has saildrive field
 */
void test_boatdatastructure_has_saildrive_field(void) {
    BoatDataStructure boat;
    boat.saildrive.saildriveEngaged = true;
    
    TEST_ASSERT_TRUE(boat.saildrive.saildriveEngaged);
}

/**
 * @test Verify BoatDataStructure has battery field
 */
void test_boatdatastructure_has_battery_field(void) {
    BoatDataStructure boat;
    boat.battery.voltageA = 12.5;
    
    TEST_ASSERT_EQUAL_DOUBLE(12.5, boat.battery.voltageA);
}

/**
 * @test Verify BoatDataStructure has shorePower field
 */
void test_boatdatastructure_has_shore_power_field(void) {
    BoatDataStructure boat;
    boat.shorePower.shorePowerOn = false;
    
    TEST_ASSERT_FALSE(boat.shorePower.shorePowerOn);
}

/**
 * @test Verify all structures have available flag
 */
void test_all_structures_have_available_flag(void) {
    GPSData gps;
    CompassData compass;
    DSTData dst;
    EngineData engine;
    SaildriveData saildrive;
    BatteryData battery;
    ShorePowerData shorePower;
    
    gps.available = true;
    compass.available = true;
    dst.available = true;
    engine.available = true;
    saildrive.available = true;
    battery.available = true;
    shorePower.available = true;
    
    TEST_ASSERT_TRUE(gps.available);
    TEST_ASSERT_TRUE(compass.available);
    TEST_ASSERT_TRUE(dst.available);
    TEST_ASSERT_TRUE(engine.available);
    TEST_ASSERT_TRUE(saildrive.available);
    TEST_ASSERT_TRUE(battery.available);
    TEST_ASSERT_TRUE(shorePower.available);
}

/**
 * @test Verify all structures have lastUpdate timestamp
 */
void test_all_structures_have_last_update_timestamp(void) {
    GPSData gps;
    CompassData compass;
    DSTData dst;
    EngineData engine;
    SaildriveData saildrive;
    BatteryData battery;
    ShorePowerData shorePower;
    
    unsigned long now = 123456789UL;
    gps.lastUpdate = now;
    compass.lastUpdate = now;
    dst.lastUpdate = now;
    engine.lastUpdate = now;
    saildrive.lastUpdate = now;
    battery.lastUpdate = now;
    shorePower.lastUpdate = now;
    
    TEST_ASSERT_EQUAL_UINT32(now, gps.lastUpdate);
    TEST_ASSERT_EQUAL_UINT32(now, compass.lastUpdate);
    TEST_ASSERT_EQUAL_UINT32(now, dst.lastUpdate);
    TEST_ASSERT_EQUAL_UINT32(now, engine.lastUpdate);
    TEST_ASSERT_EQUAL_UINT32(now, saildrive.lastUpdate);
    TEST_ASSERT_EQUAL_UINT32(now, battery.lastUpdate);
    TEST_ASSERT_EQUAL_UINT32(now, shorePower.lastUpdate);
}

// Test runner
int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    RUN_TEST(test_gpsdata_has_variation_field);
    RUN_TEST(test_compassdata_has_rate_of_turn_field);
    RUN_TEST(test_compassdata_has_heel_angle_field);
    RUN_TEST(test_compassdata_has_pitch_angle_field);
    RUN_TEST(test_compassdata_has_heave_field);
    RUN_TEST(test_dstdata_structure_exists);
    RUN_TEST(test_dstdata_has_depth_field);
    RUN_TEST(test_dstdata_has_sea_temperature_field);
    RUN_TEST(test_enginedata_structure_exists);
    RUN_TEST(test_saildrivedata_structure_exists);
    RUN_TEST(test_batterydata_structure_exists);
    RUN_TEST(test_shorepowerdata_structure_exists);
    RUN_TEST(test_boatdatastructure_has_dst_field);
    RUN_TEST(test_boatdatastructure_has_engine_field);
    RUN_TEST(test_boatdatastructure_has_saildrive_field);
    RUN_TEST(test_boatdatastructure_has_battery_field);
    RUN_TEST(test_boatdatastructure_has_shore_power_field);
    RUN_TEST(test_all_structures_have_available_flag);
    RUN_TEST(test_all_structures_have_last_update_timestamp);
    
    return UNITY_END();
}
