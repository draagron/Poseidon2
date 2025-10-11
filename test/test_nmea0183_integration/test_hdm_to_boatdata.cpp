#include <unity.h>
#antml:parameter name="Arduino.h">
#include <NMEA0183.h>
#include "components/NMEA0183Handler.h"
#include "components/BoatData.h"
#include "mocks/MockSerialPort.h"
#include "mocks/MockDisplayAdapter.h"
#include "mocks/MockSystemMetrics.h"
#include "utils/WebSocketLogger.h"
#include "helpers/nmea0183_test_fixtures.h"

// T017: Integration Test - HDM Sentence to BoatData.CompassData
void test_hdm_to_boatdata() {
    // Setup
    MockSerialPort mockSerial;
    tNMEA0183 nmea0183;
    MockDisplayAdapter mockDisplay;
    MockSystemMetrics mockMetrics;
    WebSocketLogger logger(&mockDisplay, &mockMetrics);
    BoatData boatData;
    NMEA0183Handler handler(&nmea0183, &mockSerial, &boatData, &logger);

    // Load valid HDM sentence
    mockSerial.setMockData(VALID_APHDM);

    // Process
    handler.processSentences();

    // Verify
    CompassData compassData = boatData.getCompassData();
    TEST_ASSERT_TRUE(compassData.available);

    // Test: Magnetic heading converted to radians (45.5° = 0.7941 rad)
    TEST_ASSERT_FLOAT_WITHIN(0.001, 0.7941, compassData.magneticHeading);
}
