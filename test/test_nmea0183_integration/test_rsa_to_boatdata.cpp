#include <unity.h>
#include <Arduino.h>
#include <NMEA0183.h>
#include "components/NMEA0183Handler.h"
#include "components/BoatData.h"
#include "mocks/MockSerialPort.h"
#include "mocks/MockDisplayAdapter.h"
#include "mocks/MockSystemMetrics.h"
#include "utils/WebSocketLogger.h"
#include "helpers/nmea0183_test_fixtures.h"

// T016: Integration Test - RSA Sentence to BoatData.RudderData
void test_rsa_to_boatdata() {
    // Setup mocked dependencies
    MockSerialPort mockSerial;
    tNMEA0183 nmea0183;
    MockDisplayAdapter mockDisplay;
    MockSystemMetrics mockMetrics;
    WebSocketLogger logger(&mockDisplay, &mockMetrics);
    BoatData boatData;

    // Create handler
    NMEA0183Handler handler(&nmea0183, &mockSerial, &boatData, &logger);

    // Load mock with valid RSA sentence
    mockSerial.setMockData(VALID_APRSA);

    // Process sentences
    handler.processSentences();

    // Verify BoatData updated
    RudderData rudderData = boatData.getRudderData();

    // Test: Rudder angle converted to radians (15° = 0.2618 rad)
    TEST_ASSERT_TRUE(rudderData.available);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 0.2618, rudderData.steeringAngle);

    // Test: Timestamp updated
    TEST_ASSERT_GREATER_THAN(0, rudderData.lastUpdate);
}
