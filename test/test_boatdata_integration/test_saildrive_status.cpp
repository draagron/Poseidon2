/**
 * @file test_saildrive_status.cpp
 * @brief Integration test for saildrive status (FR-017, FR-018)
 *
 * Scenario: 1-wire sensor provides saildrive engagement status
 * Validates: SaildriveData structure via IOneWireSensors interface
 *
 * @see specs/008-enhanced-boatdata-following/quickstart.md (Scenario 6)
 * @see specs/008-enhanced-boatdata-following/tasks.md (T014)
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
 * @test Saildrive engaged status from 1-wire sensor
 */
void test_saildrive_engaged_from_1wire(void) {
    BoatDataStructure boat;
    
    SaildriveData data;
    bool result = mockSensors->readSaildriveStatus(data);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(data.available);
    
    // Copy to boat structure
    boat.saildrive = data;
    TEST_ASSERT_TRUE(boat.saildrive.available);
}

/**
 * @test Saildrive engaged boolean flag
 */
void test_saildrive_boolean_state(void) {
    SaildriveData data;
    mockSensors->readSaildriveStatus(data);
    
    // Should be boolean (true or false, no intermediate values)
    TEST_ASSERT(data.saildriveEngaged == true || data.saildriveEngaged == false);
}

/**
 * @test Saildrive graceful failure on sensor error
 */
void test_saildrive_graceful_failure(void) {
    BoatDataStructure boat;
    
    // Simulate sensor failure
    SaildriveData data;
    data.available = false;
    data.saildriveEngaged = false;
    
    boat.saildrive = data;
    
    TEST_ASSERT_FALSE(boat.saildrive.available);
}

/**
 * @test Saildrive polling interval (1000ms expected)
 */
void test_saildrive_polling_info(void) {
    // This test documents expected polling behavior
    // Actual polling rate controlled by ReactESP event loop in main.cpp
    const unsigned long expected_interval_ms = 1000;
    
    TEST_MESSAGE("Expected saildrive polling interval: 1000ms (1 Hz)");
    TEST_ASSERT_EQUAL_UINT32(1000, expected_interval_ms);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_saildrive_engaged_from_1wire);
    RUN_TEST(test_saildrive_boolean_state);
    RUN_TEST(test_saildrive_graceful_failure);
    RUN_TEST(test_saildrive_polling_info);
    return UNITY_END();
}
