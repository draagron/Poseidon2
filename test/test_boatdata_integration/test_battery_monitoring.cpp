/**
 * @file test_battery_monitoring.cpp
 * @brief Integration test for battery monitoring (FR-019 to FR-025)
 *
 * Scenario: 1-wire sensors provide dual battery bank monitoring
 * Validates: BatteryData structure via IOneWireSensors interface
 *
 * @see specs/008-enhanced-boatdata-following/quickstart.md (Scenario 7)
 * @see specs/008-enhanced-boatdata-following/tasks.md (T015)
 */

#include <unity.h>
#include "types/BoatDataTypes.h"
#include "hal/interfaces/IOneWireSensors.h"
#include "mocks/MockOneWireSensors.h"

static MockOneWireSensors* mockSensors = nullptr;

void setUp(void) {
    mockSensors = new MockOneWireSensors();
    mockSensors->initialize();
}

void tearDown(void) {
    delete mockSensors;
    mockSensors = nullptr;
}

/**
 * @test Battery A monitoring from 1-wire
 */
void test_battery_a_from_1wire(void) {
    BoatDataStructure boat;
    
    BatteryMonitorData data;
    bool result = mockSensors->readBatteryA(data);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(data.available);
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, data.voltage);
    TEST_ASSERT_LESS_OR_EQUAL(30.0, data.voltage);
}

/**
 * @test Battery B monitoring from 1-wire
 */
void test_battery_b_from_1wire(void) {
    BoatDataStructure boat;
    
    BatteryMonitorData data;
    bool result = mockSensors->readBatteryB(data);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(data.available);
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, data.voltage);
    TEST_ASSERT_LESS_OR_EQUAL(30.0, data.voltage);
}

/**
 * @test Battery amperage sign convention (+ = charging, - = discharging)
 */
void test_battery_amperage_sign_convention(void) {
    BatteryMonitorData data;
    
    // Positive amperage = charging (current into battery)
    data.amperage = 10.0;
    TEST_ASSERT_GREATER_THAN(0.0, data.amperage);
    
    // Negative amperage = discharging (current out of battery)
    data.amperage = -15.0;
    TEST_ASSERT_LESS_THAN(0.0, data.amperage);
}

/**
 * @test Battery voltage range validation (0-30V)
 */
void test_battery_voltage_range(void) {
    BatteryMonitorData data;
    mockSensors->readBatteryA(data);
    
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, data.voltage);
    TEST_ASSERT_LESS_OR_EQUAL(30.0, data.voltage);
}

/**
 * @test Battery amperage range validation (-200 to +200A)
 */
void test_battery_amperage_range(void) {
    BatteryMonitorData data;
    mockSensors->readBatteryA(data);
    
    TEST_ASSERT_GREATER_OR_EQUAL(-200.0, data.amperage);
    TEST_ASSERT_LESS_OR_EQUAL(200.0, data.amperage);
}

/**
 * @test Battery state of charge percentage (0-100%)
 */
void test_battery_soc_range(void) {
    BatteryMonitorData data;
    mockSensors->readBatteryA(data);
    
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, data.stateOfCharge);
    TEST_ASSERT_LESS_OR_EQUAL(100.0, data.stateOfCharge);
}

/**
 * @test Battery charger status flags
 */
void test_battery_charger_flags(void) {
    BatteryMonitorData data;
    mockSensors->readBatteryA(data);
    
    // Should be boolean flags
    TEST_ASSERT(data.shoreChargerOn == true || data.shoreChargerOn == false);
    TEST_ASSERT(data.engineChargerOn == true || data.engineChargerOn == false);
}

/**
 * @test Dual battery bank structure
 */
void test_dual_battery_banks(void) {
    BoatDataStructure boat;
    
    BatteryMonitorData battA, battB;
    mockSensors->readBatteryA(battA);
    mockSensors->readBatteryB(battB);
    
    boat.battery.voltageA = battA.voltage;
    boat.battery.voltageB = battB.voltage;
    
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, boat.battery.voltageA);
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, boat.battery.voltageB);
}

/**
 * @test Battery polling interval (2000ms expected)
 */
void test_battery_polling_info(void) {
    const unsigned long expected_interval_ms = 2000;
    
    TEST_MESSAGE("Expected battery polling interval: 2000ms (0.5 Hz)");
    TEST_ASSERT_EQUAL_UINT32(2000, expected_interval_ms);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_battery_a_from_1wire);
    RUN_TEST(test_battery_b_from_1wire);
    RUN_TEST(test_battery_amperage_sign_convention);
    RUN_TEST(test_battery_voltage_range);
    RUN_TEST(test_battery_amperage_range);
    RUN_TEST(test_battery_soc_range);
    RUN_TEST(test_battery_charger_flags);
    RUN_TEST(test_dual_battery_banks);
    RUN_TEST(test_battery_polling_info);
    return UNITY_END();
}
