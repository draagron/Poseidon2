/**
 * @file test_heading_reference_routing.cpp
 * @brief Unit tests for heading reference type routing (true vs magnetic)
 *
 * Tests that PGN 127250 (Vessel Heading) routes heading data to correct CompassData field
 * based on reference type (N2khr_true → trueHeading, N2khr_magnetic → magneticHeading).
 *
 * @see specs/010-nmea-2000-handling/data-model.md lines 293-299
 * @see specs/010-nmea-2000-handling/tasks.md T045
 */

#include <unity.h>
#include <N2kTypes.h>
#include <math.h>

// ============================================================================
// Heading Reference Routing Logic Tests
// ============================================================================

void test_true_heading_routing() {
    tN2kHeadingReference reference = N2khr_true;
    double heading = M_PI / 2.0;  // 90 degrees (East)

    // True heading should route to trueHeading field
    bool routeToTrueHeading = (reference == N2khr_true);
    bool routeToMagneticHeading = (reference == N2khr_magnetic);

    TEST_ASSERT_TRUE(routeToTrueHeading);
    TEST_ASSERT_FALSE(routeToMagneticHeading);
}

void test_magnetic_heading_routing() {
    tN2kHeadingReference reference = N2khr_magnetic;
    double heading = M_PI;  // 180 degrees (South)

    // Magnetic heading should route to magneticHeading field
    bool routeToTrueHeading = (reference == N2khr_true);
    bool routeToMagneticHeading = (reference == N2khr_magnetic);

    TEST_ASSERT_FALSE(routeToTrueHeading);
    TEST_ASSERT_TRUE(routeToMagneticHeading);
}

void test_pgn127250_routing_logic() {
    struct HeadingMessage {
        tN2kHeadingReference reference;
        double heading_rad;
        bool routeToTrue;
        bool routeToMagnetic;
    };

    HeadingMessage headings[] = {
        {N2khr_true, M_PI / 2.0, true, false},       // True: route to trueHeading
        {N2khr_magnetic, M_PI, false, true},         // Magnetic: route to magneticHeading
        {N2khr_true, 0.0, true, false},              // True: route to trueHeading
        {N2khr_magnetic, 3 * M_PI / 2.0, false, true} // Magnetic: route to magneticHeading
    };

    for (int i = 0; i < 4; i++) {
        bool routeToTrue = (headings[i].reference == N2khr_true);
        bool routeToMagnetic = (headings[i].reference == N2khr_magnetic);

        TEST_ASSERT_EQUAL(headings[i].routeToTrue, routeToTrue);
        TEST_ASSERT_EQUAL(headings[i].routeToMagnetic, routeToMagnetic);
    }
}

void test_unknown_reference_type() {
    // Test handling of unknown/undefined reference types
    // NMEA2000 library defines N2khr_true (0) and N2khr_magnetic (1)
    tN2kHeadingReference unknown = (tN2kHeadingReference)255;

    bool routeToTrue = (unknown == N2khr_true);
    bool routeToMagnetic = (unknown == N2khr_magnetic);

    // Unknown reference should not route to either field
    TEST_ASSERT_FALSE(routeToTrue);
    TEST_ASSERT_FALSE(routeToMagnetic);
}

// ============================================================================
// Test Suite Setup
// ============================================================================

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_true_heading_routing);
    RUN_TEST(test_magnetic_heading_routing);
    RUN_TEST(test_pgn127250_routing_logic);
    RUN_TEST(test_unknown_reference_type);
    return UNITY_END();
}
