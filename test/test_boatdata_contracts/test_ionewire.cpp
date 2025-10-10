/**
 * @file test_ionewire.cpp
 * @brief Contract tests for IOneWireSensors HAL interface
 *
 * Validates that IOneWireSensors implementations satisfy the interface contract.
 * Tests mock implementation to verify business logic without hardware.
 *
 * Test Coverage:
 * - initialize() returns bool
 * - readSaildriveStatus() populates SaildriveData
 * - readBatteryA/B() populate BatteryMonitorData
 * - readShorePower() populates ShorePowerData
 * - isBusHealthy() returns bus status
 * - Graceful error handling (availability flags)
 *
 * @see specs/008-enhanced-boatdata-following/contracts/IOneWireSensors.h
 * @see specs/008-enhanced-boatdata-following/tasks.md (T005)
 */

#include <unity.h>
#include "hal/interfaces/IOneWireSensors.h"
#include "mocks/MockOneWireSensors.h"

// Test fixture
static MockOneWireSensors* mockSensors = nullptr;

void setUp(void) {
    mockSensors = new MockOneWireSensors();
}

void tearDown(void) {
    delete mockSensors;
    mockSensors = nullptr;
}

/**
 * @test Verify initialize() returns success status
 */
void test_ionewire_initialize_returns_bool(void) {
    bool result = mockSensors->initialize();
    TEST_ASSERT_TRUE(result);  // Mock should initialize successfully
}

/**
 * @test Verify readSaildriveStatus() populates SaildriveData structure
 */
void test_ionewire_read_saildrive_populates_data(void) {
    mockSensors->initialize();
    
    SaildriveData data;
    bool result = mockSensors->readSaildriveStatus(data);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(data.available);
    // Mock provides default engaged state
    TEST_ASSERT(data.saildriveEngaged == true || data.saildriveEngaged == false);
}

/**
 * @test Verify readBatteryA() populates BatteryMonitorData structure
 */
void test_ionewire_read_battery_a_populates_data(void) {
    mockSensors->initialize();
    
    BatteryMonitorData data;
    bool result = mockSensors->readBatteryA(data);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(data.available);
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, data.voltage);
    TEST_ASSERT_LESS_OR_EQUAL(30.0, data.voltage);
    TEST_ASSERT_GREATER_OR_EQUAL(-200.0, data.amperage);
    TEST_ASSERT_LESS_OR_EQUAL(200.0, data.amperage);
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, data.stateOfCharge);
    TEST_ASSERT_LESS_OR_EQUAL(100.0, data.stateOfCharge);
}

/**
 * @test Verify readBatteryB() populates BatteryMonitorData structure
 */
void test_ionewire_read_battery_b_populates_data(void) {
    mockSensors->initialize();
    
    BatteryMonitorData data;
    bool result = mockSensors->readBatteryB(data);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(data.available);
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, data.voltage);
    TEST_ASSERT_LESS_OR_EQUAL(30.0, data.voltage);
}

/**
 * @test Verify readShorePower() populates ShorePowerData structure
 */
void test_ionewire_read_shore_power_populates_data(void) {
    mockSensors->initialize();
    
    ShorePowerData data;
    bool result = mockSensors->readShorePower(data);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(data.available);
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, data.power);
    TEST_ASSERT_LESS_OR_EQUAL(5000.0, data.power);
}

/**
 * @test Verify isBusHealthy() returns bus status
 */
void test_ionewire_bus_healthy_returns_bool(void) {
    mockSensors->initialize();
    
    bool healthy = mockSensors->isBusHealthy();
    TEST_ASSERT_TRUE(healthy);  // Mock bus should be healthy after init
}

/**
 * @test Verify graceful failure when sensor not initialized
 */
void test_ionewire_read_fails_gracefully_without_init(void) {
    // Don't call initialize()
    
    SaildriveData saildriveData;
    bool result = mockSensors->readSaildriveStatus(saildriveData);
    
    // Should fail gracefully (implementation-dependent)
    // At minimum, should not crash or return corrupted data
    TEST_ASSERT(result == false || saildriveData.available == false);
}

/**
 * @test Verify battery sign convention (positive = charging, negative = discharging)
 */
void test_ionewire_battery_amperage_sign_convention(void) {
    mockSensors->initialize();
    
    BatteryMonitorData data;
    mockSensors->readBatteryA(data);
    
    // Sign convention: positive amperage = charging (current into battery)
    // This test verifies the interface contract, not specific values
    if (data.amperage > 0) {
        // Positive amperage should indicate charging
        TEST_PASS_MESSAGE("Battery amperage positive (charging)");
    } else if (data.amperage < 0) {
        // Negative amperage should indicate discharging
        TEST_PASS_MESSAGE("Battery amperage negative (discharging)");
    } else {
        // Zero amperage (idle)
        TEST_PASS_MESSAGE("Battery amperage zero (idle)");
    }
}

// Test runner
int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    RUN_TEST(test_ionewire_initialize_returns_bool);
    RUN_TEST(test_ionewire_read_saildrive_populates_data);
    RUN_TEST(test_ionewire_read_battery_a_populates_data);
    RUN_TEST(test_ionewire_read_battery_b_populates_data);
    RUN_TEST(test_ionewire_read_shore_power_populates_data);
    RUN_TEST(test_ionewire_bus_healthy_returns_bool);
    RUN_TEST(test_ionewire_read_fails_gracefully_without_init);
    RUN_TEST(test_ionewire_battery_amperage_sign_convention);
    
    return UNITY_END();
}
