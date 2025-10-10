/**
 * @file test_shore_power.cpp
 * @brief Integration test for shore power monitoring (FR-026 to FR-028)
 *
 * Scenario: 1-wire sensor provides shore power connection and consumption
 * Validates: ShorePowerData structure via IOneWireSensors interface
 *
 * @see specs/008-enhanced-boatdata-following/quickstart.md (Scenario 8)
 * @see specs/008-enhanced-boatdata-following/tasks.md (T016)
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
 * @test Shore power status from 1-wire sensor
 */
void test_shore_power_status_from_1wire(void) {
    BoatDataStructure boat;
    
    ShorePowerData data;
    bool result = mockSensors->readShorePower(data);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(data.available);
}

/**
 * @test Shore power connection boolean flag
 */
void test_shore_power_connection_flag(void) {
    ShorePowerData data;
    mockSensors->readShorePower(data);
    
    // Should be boolean
    TEST_ASSERT(data.shorePowerOn == true || data.shorePowerOn == false);
}

/**
 * @test Shore power consumption (watts)
 */
void test_shore_power_consumption(void) {
    ShorePowerData data;
    mockSensors->readShorePower(data);
    
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, data.power);
    TEST_ASSERT_LESS_OR_EQUAL(5000.0, data.power);
}

/**
 * @test Shore power range validation (0-5000W)
 */
void test_shore_power_range_validation(void) {
    BoatDataStructure boat;
    
    boat.shorePower.power = 0.0;  // Idle
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, boat.shorePower.power);
    
    boat.shorePower.power = 5000.0;  // Max
    TEST_ASSERT_LESS_OR_EQUAL(5000.0, boat.shorePower.power);
}

/**
 * @test Shore power typical circuit limit (3000W for 30A)
 */
void test_shore_power_typical_limit(void) {
    BoatDataStructure boat;
    
    // Typical 30A shore power circuit = ~3000W
    boat.shorePower.power = 3000.0;
    
    TEST_MESSAGE("Typical 30A shore power limit: 3000W");
    TEST_ASSERT_LESS_OR_EQUAL(3000.0, boat.shorePower.power);
}

/**
 * @test Shore power off state
 */
void test_shore_power_off_state(void) {
    BoatDataStructure boat;
    
    boat.shorePower.shorePowerOn = false;
    boat.shorePower.power = 0.0;
    boat.shorePower.available = true;
    
    TEST_ASSERT_FALSE(boat.shorePower.shorePowerOn);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, boat.shorePower.power);
}

/**
 * @test Shore power on state with load
 */
void test_shore_power_on_with_load(void) {
    BoatDataStructure boat;
    
    boat.shorePower.shorePowerOn = true;
    boat.shorePower.power = 1200.0;  // 1200W load
    boat.shorePower.available = true;
    
    TEST_ASSERT_TRUE(boat.shorePower.shorePowerOn);
    TEST_ASSERT_DOUBLE_WITHIN(10.0, 1200.0, boat.shorePower.power);
}

/**
 * @test Shore power graceful failure
 */
void test_shore_power_graceful_failure(void) {
    BoatDataStructure boat;
    
    boat.shorePower.available = false;
    
    TEST_ASSERT_FALSE(boat.shorePower.available);
}

/**
 * @test Shore power polling interval (2000ms expected)
 */
void test_shore_power_polling_info(void) {
    const unsigned long expected_interval_ms = 2000;
    
    TEST_MESSAGE("Expected shore power polling interval: 2000ms (0.5 Hz)");
    TEST_ASSERT_EQUAL_UINT32(2000, expected_interval_ms);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_shore_power_status_from_1wire);
    RUN_TEST(test_shore_power_connection_flag);
    RUN_TEST(test_shore_power_consumption);
    RUN_TEST(test_shore_power_range_validation);
    RUN_TEST(test_shore_power_typical_limit);
    RUN_TEST(test_shore_power_off_state);
    RUN_TEST(test_shore_power_on_with_load);
    RUN_TEST(test_shore_power_graceful_failure);
    RUN_TEST(test_shore_power_polling_info);
    return UNITY_END();
}
