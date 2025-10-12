/**
 * @file test_temperature_conversion.cpp
 * @brief Unit tests for temperature conversion (Kelvin → Celsius)
 *
 * Tests PGN 130316 (Temperature Extended Range) and PGN 127489 (Engine Parameters, Dynamic)
 * temperature conversion logic.
 *
 * @see src/utils/DataValidation.h
 * @see specs/010-nmea-2000-handling/tasks.md T041
 */

#include <unity.h>
#include "../../src/utils/DataValidation.h"

// ============================================================================
// Kelvin to Celsius Conversion Tests
// ============================================================================

void test_kelvin_to_celsius_basic() {
    // Basic conversion: K - 273.15 = °C

    // Absolute zero (0 K = -273.15 °C)
    TEST_ASSERT_EQUAL_DOUBLE(-273.15, DataValidation::kelvinToCelsius(0.0));

    // Freezing point of water (273.15 K = 0 °C)
    TEST_ASSERT_EQUAL_DOUBLE(0.0, DataValidation::kelvinToCelsius(273.15));

    // Boiling point of water (373.15 K = 100 °C)
    TEST_ASSERT_EQUAL_DOUBLE(100.0, DataValidation::kelvinToCelsius(373.15));

    // Room temperature (293.15 K = 20 °C)
    TEST_ASSERT_EQUAL_DOUBLE(20.0, DataValidation::kelvinToCelsius(293.15));
}

void test_kelvin_to_celsius_marine_temperatures() {
    // Typical marine temperature ranges

    // Cold seawater (276.15 K = 3 °C)
    TEST_ASSERT_EQUAL_DOUBLE(3.0, DataValidation::kelvinToCelsius(276.15));

    // Warm seawater (298.15 K = 25 °C)
    TEST_ASSERT_EQUAL_DOUBLE(25.0, DataValidation::kelvinToCelsius(298.15));

    // Tropical seawater (303.15 K = 30 °C)
    TEST_ASSERT_EQUAL_DOUBLE(30.0, DataValidation::kelvinToCelsius(303.15));

    // Engine oil temperature (373.15 K = 100 °C)
    TEST_ASSERT_EQUAL_DOUBLE(100.0, DataValidation::kelvinToCelsius(373.15));
}

void test_kelvin_to_celsius_precision() {
    // Test precision with fractional values

    // 283.45 K = 10.3 °C
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 10.3, DataValidation::kelvinToCelsius(283.45));

    // 288.65 K = 15.5 °C
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 15.5, DataValidation::kelvinToCelsius(288.65));

    // 293.85 K = 20.7 °C
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 20.7, DataValidation::kelvinToCelsius(293.85));

    // 363.15 K = 90.0 °C (engine oil)
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 90.0, DataValidation::kelvinToCelsius(363.15));
}

// ============================================================================
// Seawater Temperature Validation Tests
// ============================================================================

void test_seawater_temperature_valid_range() {
    // Valid seawater temperature range [-10, 50] °C

    TEST_ASSERT_TRUE(DataValidation::isValidWaterTemperature(-10.0));   // Ice-cold water
    TEST_ASSERT_TRUE(DataValidation::isValidWaterTemperature(0.0));     // Freezing point
    TEST_ASSERT_TRUE(DataValidation::isValidWaterTemperature(15.0));    // Temperate water
    TEST_ASSERT_TRUE(DataValidation::isValidWaterTemperature(25.0));    // Warm water
    TEST_ASSERT_TRUE(DataValidation::isValidWaterTemperature(30.0));    // Tropical water
    TEST_ASSERT_TRUE(DataValidation::isValidWaterTemperature(50.0));    // Maximum allowed
}

void test_seawater_temperature_out_of_range() {
    // Invalid seawater temperature values

    TEST_ASSERT_FALSE(DataValidation::isValidWaterTemperature(-11.0));  // Too cold
    TEST_ASSERT_FALSE(DataValidation::isValidWaterTemperature(-20.0));
    TEST_ASSERT_FALSE(DataValidation::isValidWaterTemperature(51.0));   // Too hot
    TEST_ASSERT_FALSE(DataValidation::isValidWaterTemperature(100.0));  // Boiling water
}

void test_seawater_temperature_clamping() {
    // Clamping out-of-range seawater temperatures

    TEST_ASSERT_EQUAL_DOUBLE(-10.0, DataValidation::clampWaterTemperature(-11.0));
    TEST_ASSERT_EQUAL_DOUBLE(-10.0, DataValidation::clampWaterTemperature(-20.0));
    TEST_ASSERT_EQUAL_DOUBLE(50.0, DataValidation::clampWaterTemperature(51.0));
    TEST_ASSERT_EQUAL_DOUBLE(50.0, DataValidation::clampWaterTemperature(100.0));

    // Valid temperatures should not be clamped
    TEST_ASSERT_EQUAL_DOUBLE(15.0, DataValidation::clampWaterTemperature(15.0));
    TEST_ASSERT_EQUAL_DOUBLE(25.0, DataValidation::clampWaterTemperature(25.0));
}

// ============================================================================
// Engine Oil Temperature Validation Tests
// ============================================================================

void test_oil_temperature_valid_range() {
    // Valid oil temperature range [-10, 150] °C

    TEST_ASSERT_TRUE(DataValidation::isValidOilTemperature(-10.0));     // Cold start
    TEST_ASSERT_TRUE(DataValidation::isValidOilTemperature(0.0));       // Freezing
    TEST_ASSERT_TRUE(DataValidation::isValidOilTemperature(60.0));      // Warm oil
    TEST_ASSERT_TRUE(DataValidation::isValidOilTemperature(90.0));      // Normal operating temp
    TEST_ASSERT_TRUE(DataValidation::isValidOilTemperature(120.0));     // High operating temp
    TEST_ASSERT_TRUE(DataValidation::isValidOilTemperature(150.0));     // Maximum allowed
}

void test_oil_temperature_out_of_range() {
    // Invalid oil temperature values

    TEST_ASSERT_FALSE(DataValidation::isValidOilTemperature(-11.0));    // Too cold
    TEST_ASSERT_FALSE(DataValidation::isValidOilTemperature(-20.0));
    TEST_ASSERT_FALSE(DataValidation::isValidOilTemperature(151.0));    // Too hot
    TEST_ASSERT_FALSE(DataValidation::isValidOilTemperature(200.0));    // Dangerously hot
}

void test_oil_temperature_clamping() {
    // Clamping out-of-range oil temperatures

    TEST_ASSERT_EQUAL_DOUBLE(-10.0, DataValidation::clampOilTemperature(-11.0));
    TEST_ASSERT_EQUAL_DOUBLE(-10.0, DataValidation::clampOilTemperature(-20.0));
    TEST_ASSERT_EQUAL_DOUBLE(150.0, DataValidation::clampOilTemperature(151.0));
    TEST_ASSERT_EQUAL_DOUBLE(150.0, DataValidation::clampOilTemperature(200.0));

    // Valid temperatures should not be clamped
    TEST_ASSERT_EQUAL_DOUBLE(60.0, DataValidation::clampOilTemperature(60.0));
    TEST_ASSERT_EQUAL_DOUBLE(90.0, DataValidation::clampOilTemperature(90.0));
    TEST_ASSERT_EQUAL_DOUBLE(120.0, DataValidation::clampOilTemperature(120.0));
}

// ============================================================================
// Integration Tests (Kelvin → Celsius → Validation)
// ============================================================================

void test_pgn130316_seawater_temperature_flow() {
    // Simulate PGN 130316 data flow: Kelvin → Celsius → Validation → Clamping

    // Valid seawater temperature (288.15 K = 15 °C)
    double kelvin = 288.15;
    double celsius = DataValidation::kelvinToCelsius(kelvin);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 15.0, celsius);
    TEST_ASSERT_TRUE(DataValidation::isValidWaterTemperature(celsius));

    // Out-of-range seawater temperature (333.15 K = 60 °C, too hot)
    kelvin = 333.15;
    celsius = DataValidation::kelvinToCelsius(kelvin);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 60.0, celsius);
    TEST_ASSERT_FALSE(DataValidation::isValidWaterTemperature(celsius));
    celsius = DataValidation::clampWaterTemperature(celsius);
    TEST_ASSERT_EQUAL_DOUBLE(50.0, celsius);  // Clamped to max

    // Out-of-range seawater temperature (263.15 K = -10 °C, at lower limit)
    kelvin = 263.15;
    celsius = DataValidation::kelvinToCelsius(kelvin);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, -10.0, celsius);
    TEST_ASSERT_TRUE(DataValidation::isValidWaterTemperature(celsius));

    // Out-of-range seawater temperature (253.15 K = -20 °C, too cold)
    kelvin = 253.15;
    celsius = DataValidation::kelvinToCelsius(kelvin);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, -20.0, celsius);
    TEST_ASSERT_FALSE(DataValidation::isValidWaterTemperature(celsius));
    celsius = DataValidation::clampWaterTemperature(celsius);
    TEST_ASSERT_EQUAL_DOUBLE(-10.0, celsius);  // Clamped to min
}

void test_pgn127489_oil_temperature_flow() {
    // Simulate PGN 127489 data flow: Kelvin → Celsius → Validation → Clamping

    // Valid oil temperature (363.15 K = 90 °C, normal operating temp)
    double kelvin = 363.15;
    double celsius = DataValidation::kelvinToCelsius(kelvin);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 90.0, celsius);
    TEST_ASSERT_TRUE(DataValidation::isValidOilTemperature(celsius));

    // High oil temperature (393.15 K = 120 °C, acceptable but high)
    kelvin = 393.15;
    celsius = DataValidation::kelvinToCelsius(kelvin);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 120.0, celsius);
    TEST_ASSERT_TRUE(DataValidation::isValidOilTemperature(celsius));

    // Out-of-range oil temperature (433.15 K = 160 °C, too hot)
    kelvin = 433.15;
    celsius = DataValidation::kelvinToCelsius(kelvin);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 160.0, celsius);
    TEST_ASSERT_FALSE(DataValidation::isValidOilTemperature(celsius));
    celsius = DataValidation::clampOilTemperature(celsius);
    TEST_ASSERT_EQUAL_DOUBLE(150.0, celsius);  // Clamped to max

    // Cold start temperature (263.15 K = -10 °C, at lower limit)
    kelvin = 263.15;
    celsius = DataValidation::kelvinToCelsius(kelvin);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, -10.0, celsius);
    TEST_ASSERT_TRUE(DataValidation::isValidOilTemperature(celsius));
}

// ============================================================================
// Edge Cases
// ============================================================================

void test_temperature_conversion_edge_cases() {
    // Absolute zero (0 K = -273.15 °C)
    double celsius = DataValidation::kelvinToCelsius(0.0);
    TEST_ASSERT_EQUAL_DOUBLE(-273.15, celsius);

    // Very small positive Kelvin value
    celsius = DataValidation::kelvinToCelsius(0.01);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, -273.14, celsius);

    // Large Kelvin value (473.15 K = 200 °C)
    celsius = DataValidation::kelvinToCelsius(473.15);
    TEST_ASSERT_EQUAL_DOUBLE(200.0, celsius);
}

// ============================================================================
// Test Suite Setup
// ============================================================================

void setUp(void) {
    // Set up before each test (if needed)
}

void tearDown(void) {
    // Clean up after each test (if needed)
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // Kelvin to Celsius conversion tests
    RUN_TEST(test_kelvin_to_celsius_basic);
    RUN_TEST(test_kelvin_to_celsius_marine_temperatures);
    RUN_TEST(test_kelvin_to_celsius_precision);

    // Seawater temperature validation tests
    RUN_TEST(test_seawater_temperature_valid_range);
    RUN_TEST(test_seawater_temperature_out_of_range);
    RUN_TEST(test_seawater_temperature_clamping);

    // Oil temperature validation tests
    RUN_TEST(test_oil_temperature_valid_range);
    RUN_TEST(test_oil_temperature_out_of_range);
    RUN_TEST(test_oil_temperature_clamping);

    // Integration tests
    RUN_TEST(test_pgn130316_seawater_temperature_flow);
    RUN_TEST(test_pgn127489_oil_temperature_flow);

    // Edge cases
    RUN_TEST(test_temperature_conversion_edge_cases);

    return UNITY_END();
}
