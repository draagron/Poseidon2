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

// T018: Integration Test - GGA Sentence to BoatData.GPSData
void test_gga_to_boatdata() {
    // Setup
    MockSerialPort mockSerial;
    tNMEA0183 nmea0183;
    MockDisplayAdapter mockDisplay;
    MockSystemMetrics mockMetrics;
    WebSocketLogger logger(&mockDisplay, &mockMetrics);
    BoatData boatData;
    NMEA0183Handler handler(&nmea0183, &mockSerial, &boatData, &logger);

    // Load valid GGA sentence
    mockSerial.setMockData(VALID_VHGGA);

    // Process
    handler.processSentences();

    // Verify
    GPSData gpsData = boatData.getGPSData();
    TEST_ASSERT_TRUE(gpsData.available);

    // Test: Latitude and longitude converted from DDMM.MMMM to decimal degrees
    TEST_ASSERT_FLOAT_WITHIN(0.0001, 52.508333, gpsData.latitude);
    TEST_ASSERT_FLOAT_WITHIN(0.0001, 5.116667, gpsData.longitude);
}
