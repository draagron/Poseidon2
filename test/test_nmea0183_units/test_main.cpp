#include <unity.h>

// Test functions from test_unit_conversions.cpp
void test_unit_converter_degrees_to_radians();
void test_unit_converter_normalize_angle();
void test_unit_converter_nmea_coordinate_conversion();
void test_unit_converter_variation_calculation();

// Test functions from test_parsers.cpp
void test_nmea0183_parse_rsa();

void setUp() {
    // Set up before each test
}

void tearDown() {
    // Clean up after each test
}

int main(int argc, char** argv) {
    UNITY_BEGIN();

    // UnitConverter tests
    RUN_TEST(test_unit_converter_degrees_to_radians);
    RUN_TEST(test_unit_converter_normalize_angle);
    RUN_TEST(test_unit_converter_nmea_coordinate_conversion);
    RUN_TEST(test_unit_converter_variation_calculation);

    // Parser tests
    RUN_TEST(test_nmea0183_parse_rsa);

    return UNITY_END();
}
