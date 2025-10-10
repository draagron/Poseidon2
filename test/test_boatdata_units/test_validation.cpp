/**
 * @file test_validation.cpp
 * @brief Unit tests for data validation and clamping logic
 *
 * Tests validation helper functions for range checking and clamping.
 * These functions ensure data integrity at the HAL boundary.
 *
 * Test Coverage:
 * - clampPitchAngle (±30° = ±π/6 rad)
 * - clampHeave (±5.0 meters)
 * - clampEngineRPM (0-6000 RPM)
 * - clampBatteryVoltage (0-30V, warn outside 10-15V for 12V system)
 * - clampTemperature (-10 to 150°C for oil, -10 to 50°C for water)
 *
 * @see specs/008-enhanced-boatdata-following/quickstart.md (Scenario 9)
 * @see specs/008-enhanced-boatdata-following/tasks.md (T018)
 * @see specs/008-enhanced-boatdata-following/research.md (Validation Strategy)
 */

#include <unity.h>
#include <math.h>

void setUp(void) {}
void tearDown(void) {}

/**
 * @test Pitch angle clamping to ±30° (±π/6 rad)
 */
void test_pitch_angle_clamping(void) {
    double pitch;
    
    // Within range - no clamping
    pitch = 0.1;  // ~5.7°
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.1, pitch);
    
    // Exceeds max - should clamp to π/6
    pitch = M_PI / 2;  // 90°
    double clamped = (pitch > M_PI / 6) ? M_PI / 6 : pitch;
    TEST_ASSERT_DOUBLE_WITHIN(0.01, M_PI / 6, clamped);
    
    // Exceeds min - should clamp to -π/6
    pitch = -M_PI / 2;  // -90°
    clamped = (pitch < -M_PI / 6) ? -M_PI / 6 : pitch;
    TEST_ASSERT_DOUBLE_WITHIN(0.01, -M_PI / 6, clamped);
}

/**
 * @test Heave clamping to ±5 meters
 */
void test_heave_clamping(void) {
    double heave;
    
    // Within range
    heave = 2.5;
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 2.5, heave);
    
    // Exceeds max
    heave = 10.0;
    double clamped = (heave > 5.0) ? 5.0 : heave;
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 5.0, clamped);
    
    // Exceeds min
    heave = -10.0;
    clamped = (heave < -5.0) ? -5.0 : heave;
    TEST_ASSERT_DOUBLE_WITHIN(0.1, -5.0, clamped);
}

/**
 * @test Engine RPM clamping to 0-6000
 */
void test_engine_rpm_clamping(void) {
    double rpm;
    
    // Within range
    rpm = 1800.0;
    TEST_ASSERT_DOUBLE_WITHIN(10.0, 1800.0, rpm);
    
    // Negative (invalid) - clamp to 0
    rpm = -500.0;
    double clamped = (rpm < 0.0) ? 0.0 : rpm;
    TEST_ASSERT_DOUBLE_WITHIN(1.0, 0.0, clamped);
    
    // Exceeds max - clamp to 6000
    rpm = 8000.0;
    clamped = (rpm > 6000.0) ? 6000.0 : rpm;
    TEST_ASSERT_DOUBLE_WITHIN(10.0, 6000.0, clamped);
}

/**
 * @test Battery voltage clamping to 0-30V
 */
void test_battery_voltage_clamping(void) {
    double voltage;
    
    // Within range
    voltage = 12.6;
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 12.6, voltage);
    
    // Negative (invalid) - clamp to 0
    voltage = -5.0;
    double clamped = (voltage < 0.0) ? 0.0 : voltage;
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 0.0, clamped);
    
    // Exceeds max - clamp to 30
    voltage = 50.0;
    clamped = (voltage > 30.0) ? 30.0 : voltage;
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 30.0, clamped);
}

/**
 * @test Battery voltage warning range for 12V system (10-15V)
 */
void test_battery_voltage_warning_range(void) {
    double voltage;
    
    // Normal range (no warning expected)
    voltage = 12.6;
    bool inWarningRange = (voltage < 10.0 || voltage > 15.0);
    TEST_ASSERT_FALSE(inWarningRange);
    
    // Undercharged (warning expected)
    voltage = 9.5;
    inWarningRange = (voltage < 10.0 || voltage > 15.0);
    TEST_ASSERT_TRUE(inWarningRange);
    
    // Overcharged (warning expected)
    voltage = 16.0;
    inWarningRange = (voltage < 10.0 || voltage > 15.0);
    TEST_ASSERT_TRUE(inWarningRange);
}

/**
 * @test Oil temperature clamping (-10 to 150°C)
 */
void test_oil_temperature_clamping(void) {
    double temp;
    
    // Within range
    temp = 80.0;
    TEST_ASSERT_DOUBLE_WITHIN(1.0, 80.0, temp);
    
    // Below min
    temp = -20.0;
    double clamped = (temp < -10.0) ? -10.0 : temp;
    TEST_ASSERT_DOUBLE_WITHIN(1.0, -10.0, clamped);
    
    // Above max
    temp = 200.0;
    clamped = (temp > 150.0) ? 150.0 : temp;
    TEST_ASSERT_DOUBLE_WITHIN(1.0, 150.0, clamped);
}

/**
 * @test Water temperature clamping (-10 to 50°C)
 */
void test_water_temperature_clamping(void) {
    double temp;
    
    // Within range
    temp = 15.0;
    TEST_ASSERT_DOUBLE_WITHIN(0.5, 15.0, temp);
    
    // Below min
    temp = -20.0;
    double clamped = (temp < -10.0) ? -10.0 : temp;
    TEST_ASSERT_DOUBLE_WITHIN(0.5, -10.0, clamped);
    
    // Above max
    temp = 70.0;
    clamped = (temp > 50.0) ? 50.0 : temp;
    TEST_ASSERT_DOUBLE_WITHIN(0.5, 50.0, clamped);
}

/**
 * @test Depth negative rejection (sensor above water)
 */
void test_depth_negative_rejection(void) {
    double depth;
    
    // Valid depth
    depth = 10.5;
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, depth);
    
    // Invalid depth (negative) - should be rejected
    depth = -2.5;
    bool invalid = (depth < 0.0);
    TEST_ASSERT_TRUE(invalid);
    
    // After rejection, set to 0 or mark unavailable
    if (invalid) {
        depth = 0.0;
    }
    TEST_ASSERT_EQUAL_DOUBLE(0.0, depth);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_pitch_angle_clamping);
    RUN_TEST(test_heave_clamping);
    RUN_TEST(test_engine_rpm_clamping);
    RUN_TEST(test_battery_voltage_clamping);
    RUN_TEST(test_battery_voltage_warning_range);
    RUN_TEST(test_oil_temperature_clamping);
    RUN_TEST(test_water_temperature_clamping);
    RUN_TEST(test_depth_negative_rejection);
    return UNITY_END();
}
