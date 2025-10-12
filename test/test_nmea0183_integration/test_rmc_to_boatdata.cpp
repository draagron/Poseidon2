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

// T019: Integration Test - RMC Sentence to BoatData (GPS + Variation)
void test_rmc_to_boatdata() {
    // Setup
    MockSerialPort mockSerial;
    tNMEA0183 nmea0183;
    MockDisplayAdapter mockDisplay;
    MockSystemMetrics mockMetrics;
    WebSocketLogger logger(&mockDisplay, &mockMetrics);
    BoatData boatData;
    NMEA0183Handler handler(&nmea0183, &mockSerial, &boatData, &logger);

    // Load valid RMC sentence
    mockSerial.setMockData(VALID_VHRMC);

    // Process
    handler.processSentences();

    // Verify GPS data
    GPSData gpsData = boatData.getGPSData();
    TEST_ASSERT_TRUE(gpsData.available);
    TEST_ASSERT_FLOAT_WITHIN(0.0001, 52.508333, gpsData.latitude);
    TEST_ASSERT_FLOAT_WITHIN(0.0001, 5.116667, gpsData.longitude);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 0.9548, gpsData.cog);  // 54.7° in radians
    TEST_ASSERT_FLOAT_WITHIN(0.1, 5.5, gpsData.sog);  // knots

    // Verify variation (3.1°W = -0.0541 radians, negative)
    CompassData compassData = boatData.getCompassData();
    TEST_ASSERT_FLOAT_WITHIN(0.001, -0.0541, compassData.variation);
}
