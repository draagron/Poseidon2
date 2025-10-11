#include <unity.h>
#include <Arduino.h>
#include "utils/UnitConverter.h"

const double EPSILON = 0.0001;

// T011: Unit Test - UnitConverter Degree/Radian Conversion
void test_unit_converter_degrees_to_radians() {
    // Test 0 degrees
    TEST_ASSERT_FLOAT_WITHIN(EPSILON, 0.0, UnitConverter::degreesToRadians(0.0));

    // Test 90 degrees (π/2)
    TEST_ASSERT_FLOAT_WITHIN(EPSILON, M_PI / 2.0, UnitConverter::degreesToRadians(90.0));

    // Test 180 degrees (π)
    TEST_ASSERT_FLOAT_WITHIN(EPSILON, M_PI, UnitConverter::degreesToRadians(180.0));

    // Test 360 degrees (2π)
    TEST_ASSERT_FLOAT_WITHIN(EPSILON, 2.0 * M_PI, UnitConverter::degreesToRadians(360.0));

    // Test negative angle
    TEST_ASSERT_FLOAT_WITHIN(EPSILON, -M_PI / 2.0, UnitConverter::degreesToRadians(-90.0));
}

void test_unit_converter_normalize_angle() {
    // Test angle already in range
    TEST_ASSERT_FLOAT_WITHIN(EPSILON, M_PI, UnitConverter::normalizeAngle(M_PI));

    // Test 3π → π
    TEST_ASSERT_FLOAT_WITHIN(EPSILON, M_PI, UnitConverter::normalizeAngle(3.0 * M_PI));

    // Test -π/2 → 3π/2
    TEST_ASSERT_FLOAT_WITHIN(EPSILON, 3.0 * M_PI / 2.0, UnitConverter::normalizeAngle(-M_PI / 2.0));

    // Test negative angle wrap-around
    TEST_ASSERT_FLOAT_WITHIN(EPSILON, M_PI, UnitConverter::normalizeAngle(-M_PI));
}

// T012: Unit Test - UnitConverter NMEA Coordinate Conversion
void test_unit_converter_nmea_coordinate_conversion() {
    // Test North latitude: 52°30.5' = 52.508333°
    TEST_ASSERT_FLOAT_WITHIN(EPSILON, 52.508333, UnitConverter::nmeaCoordinateToDecimal(5230.5000, 'N'));

    // Test South latitude: 52°30.5' = -52.508333° (negative)
    TEST_ASSERT_FLOAT_WITHIN(EPSILON, -52.508333, UnitConverter::nmeaCoordinateToDecimal(5230.5000, 'S'));

    // Test East longitude: 5°7' = 5.116667°
    TEST_ASSERT_FLOAT_WITHIN(EPSILON, 5.116667, UnitConverter::nmeaCoordinateToDecimal(507.0000, 'E'));

    // Test West longitude: 5°7' = -5.116667° (negative)
    TEST_ASSERT_FLOAT_WITHIN(EPSILON, -5.116667, UnitConverter::nmeaCoordinateToDecimal(507.0000, 'W'));

    // Test boundary: 90°0' = 90.0° (max latitude)
    TEST_ASSERT_FLOAT_WITHIN(EPSILON, 90.0, UnitConverter::nmeaCoordinateToDecimal(9000.0000, 'N'));

    // Test 0°0' = 0.0°
    TEST_ASSERT_FLOAT_WITHIN(EPSILON, 0.0, UnitConverter::nmeaCoordinateToDecimal(0.0, 'N'));
}

// T013: Unit Test - UnitConverter Variation Calculation
void test_unit_converter_variation_calculation() {
    // Test West variation: true 54.7°, magnetic 57.9° → -3.2° (West)
    TEST_ASSERT_FLOAT_WITHIN(EPSILON, -3.2, UnitConverter::calculateVariation(54.7, 57.9));

    // Test East variation: true 100°, magnetic 95° → 5.0° (East)
    TEST_ASSERT_FLOAT_WITHIN(EPSILON, 5.0, UnitConverter::calculateVariation(100.0, 95.0));

    // Test wraparound case: true 10°, magnetic 350° → 20° (not -340°)
    TEST_ASSERT_FLOAT_WITHIN(EPSILON, 20.0, UnitConverter::calculateVariation(10.0, 350.0));

    // Test reverse wraparound: true 350°, magnetic 10° → -20°
    TEST_ASSERT_FLOAT_WITHIN(EPSILON, -20.0, UnitConverter::calculateVariation(350.0, 10.0));

    // Test no variation: true 100°, magnetic 100° → 0°
    TEST_ASSERT_FLOAT_WITHIN(EPSILON, 0.0, UnitConverter::calculateVariation(100.0, 100.0));
}
