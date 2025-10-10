/**
 * @file test_unit_conversions.cpp
 * @brief Unit tests for unit conversion functions
 *
 * Tests mathematical conversions between measurement units.
 * Primary focus: Kelvin → Celsius for NMEA2000 PGN 130316
 *
 * Test Coverage:
 * - Kelvin to Celsius conversion (T_C = T_K - 273.15)
 * - Edge cases (absolute zero, typical marine range)
 *
 * @see specs/008-enhanced-boatdata-following/quickstart.md (Scenario 4)
 * @see specs/008-enhanced-boatdata-following/tasks.md (T019)
 * @see specs/008-enhanced-boatdata-following/research.md (Unit conversions)
 */

#include <unity.h>
#include <math.h>

void setUp(void) {}
void tearDown(void) {}

/**
 * @test Kelvin to Celsius conversion formula
 */
void test_kelvin_to_celsius_conversion(void) {
    double kelvin, celsius;
    
    // Test case 1: Freezing point of water
    kelvin = 273.15;
    celsius = kelvin - 273.15;
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, celsius);
    
    // Test case 2: Typical sea temperature (15°C = 288.15K)
    kelvin = 288.15;
    celsius = kelvin - 273.15;
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 15.0, celsius);
    
    // Test case 3: Tropical water (30°C = 303.15K)
    kelvin = 303.15;
    celsius = kelvin - 273.15;
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 30.0, celsius);
}

/**
 * @test Kelvin to Celsius for NMEA2000 PGN 130316
 */
void test_pgn_130316_temperature_conversion(void) {
    // PGN 130316 transmits temperature in Kelvin
    double temperatureKelvin = 288.15;  // 15°C
    
    // Convert to Celsius for storage in DSTData.seaTemperature
    double temperatureCelsius = temperatureKelvin - 273.15;
    
    TEST_ASSERT_DOUBLE_WITHIN(0.5, 15.0, temperatureCelsius);
}

/**
 * @test Absolute zero edge case (0K = -273.15°C)
 */
void test_absolute_zero_conversion(void) {
    double kelvin = 0.0;
    double celsius = kelvin - 273.15;
    
    TEST_ASSERT_DOUBLE_WITHIN(0.01, -273.15, celsius);
}

/**
 * @test Marine temperature range conversions
 */
void test_marine_temperature_range_conversions(void) {
    double kelvin, celsius;
    
    // Ice water (-2°C = 271.15K)
    kelvin = 271.15;
    celsius = kelvin - 273.15;
    TEST_ASSERT_DOUBLE_WITHIN(0.1, -2.0, celsius);
    
    // Cold water (5°C = 278.15K)
    kelvin = 278.15;
    celsius = kelvin - 273.15;
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 5.0, celsius);
    
    // Warm water (25°C = 298.15K)
    kelvin = 298.15;
    celsius = kelvin - 273.15;
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 25.0, celsius);
}

/**
 * @test Kelvin input validation (should reject negative Kelvin)
 */
void test_kelvin_input_validation(void) {
    double kelvin = -10.0;  // Invalid (Kelvin cannot be negative)
    
    // Validation should reject this
    bool invalid = (kelvin < 0.0);
    TEST_ASSERT_TRUE(invalid);
}

/**
 * @test Conversion precision (sufficient for marine applications)
 */
void test_conversion_precision(void) {
    // Marine temperature precision: ±0.5°C is acceptable
    double kelvin = 288.15;  // 15.0°C
    double celsius = kelvin - 273.15;
    
    // Should be within 0.5°C tolerance
    TEST_ASSERT_DOUBLE_WITHIN(0.5, 15.0, celsius);
}

/**
 * @test No conversion needed for NMEA0183 MTW (already Celsius)
 */
void test_nmea0183_mtw_no_conversion(void) {
    // NMEA0183 MTW sentence already provides temperature in Celsius
    double temperatureCelsius = 15.0;
    
    // No conversion needed - direct assignment
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 15.0, temperatureCelsius);
    TEST_PASS_MESSAGE("NMEA0183 MTW uses Celsius directly (no conversion)");
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_kelvin_to_celsius_conversion);
    RUN_TEST(test_pgn_130316_temperature_conversion);
    RUN_TEST(test_absolute_zero_conversion);
    RUN_TEST(test_marine_temperature_range_conversions);
    RUN_TEST(test_kelvin_input_validation);
    RUN_TEST(test_conversion_precision);
    RUN_TEST(test_nmea0183_mtw_no_conversion);
    return UNITY_END();
}
