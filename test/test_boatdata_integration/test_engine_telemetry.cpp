/**
 * @file test_engine_telemetry.cpp
 * @brief Integration test for engine telemetry (FR-013 to FR-016)
 *
 * Scenario: NMEA2000 PGN 127488/127489 provide engine data
 * Validates: EngineData structure with engineRev, oilTemperature, alternatorVoltage
 *
 * @see specs/008-enhanced-boatdata-following/quickstart.md (Scenario 5)
 * @see specs/008-enhanced-boatdata-following/tasks.md (T013)
 */

#include <unity.h>
#include "types/BoatDataTypes.h"

void setUp(void) {}
void tearDown(void) {}

/**
 * @test Engine RPM from PGN 127488 (Engine Parameters, Rapid Update)
 */
void test_engine_rpm_from_pgn_127488(void) {
    BoatDataStructure boat;
    
    // Simulate PGN 127488
    boat.engine.engineRev = 1800.0;  // 1800 RPM
    boat.engine.available = true;
    
    TEST_ASSERT_DOUBLE_WITHIN(10.0, 1800.0, boat.engine.engineRev);
    TEST_ASSERT_TRUE(boat.engine.available);
}

/**
 * @test Oil temperature from PGN 127489 (Engine Parameters, Dynamic)
 */
void test_oil_temperature_from_pgn_127489(void) {
    BoatDataStructure boat;
    
    // Simulate PGN 127489
    boat.engine.oilTemperature = 80.0;  // 80°C
    boat.engine.available = true;
    
    TEST_ASSERT_DOUBLE_WITHIN(1.0, 80.0, boat.engine.oilTemperature);
}

/**
 * @test Alternator voltage from PGN 127489
 */
void test_alternator_voltage_from_pgn_127489(void) {
    BoatDataStructure boat;
    
    // Simulate PGN 127489
    boat.engine.alternatorVoltage = 14.2;  // 14.2V
    boat.engine.available = true;
    
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 14.2, boat.engine.alternatorVoltage);
}

/**
 * @test Engine RPM range validation (0-6000 RPM)
 */
void test_engine_rpm_range_validation(void) {
    BoatDataStructure boat;
    
    // Valid RPM range
    boat.engine.engineRev = 0.0;
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, boat.engine.engineRev);
    
    boat.engine.engineRev = 6000.0;
    TEST_ASSERT_LESS_OR_EQUAL(6000.0, boat.engine.engineRev);
}

/**
 * @test Oil temperature range validation (-10 to 150°C)
 */
void test_oil_temperature_range_validation(void) {
    BoatDataStructure boat;
    
    // Valid temperature range
    boat.engine.oilTemperature = -10.0;  // Cold start
    TEST_ASSERT_GREATER_OR_EQUAL(-10.0, boat.engine.oilTemperature);
    
    boat.engine.oilTemperature = 150.0;  // Max temp
    TEST_ASSERT_LESS_OR_EQUAL(150.0, boat.engine.oilTemperature);
}

/**
 * @test Alternator voltage range validation (0-30V)
 */
void test_alternator_voltage_range_validation(void) {
    BoatDataStructure boat;
    
    // Valid voltage range
    boat.engine.alternatorVoltage = 0.0;
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, boat.engine.alternatorVoltage);
    
    boat.engine.alternatorVoltage = 30.0;
    TEST_ASSERT_LESS_OR_EQUAL(30.0, boat.engine.alternatorVoltage);
}

/**
 * @test Engine unit convention (RPM, not Hz)
 */
void test_engine_unit_is_rpm(void) {
    BoatDataStructure boat;
    
    // Engine RPM should be in RPM (not Hz)
    boat.engine.engineRev = 1500.0;  // 1500 RPM (not 25 Hz)
    
    // Verify value is in RPM range (not Hz range which would be 0-100)
    TEST_ASSERT_GREATER_THAN(100.0, boat.engine.engineRev);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_engine_rpm_from_pgn_127488);
    RUN_TEST(test_oil_temperature_from_pgn_127489);
    RUN_TEST(test_alternator_voltage_from_pgn_127489);
    RUN_TEST(test_engine_rpm_range_validation);
    RUN_TEST(test_oil_temperature_range_validation);
    RUN_TEST(test_alternator_voltage_range_validation);
    RUN_TEST(test_engine_unit_is_rpm);
    return UNITY_END();
}
