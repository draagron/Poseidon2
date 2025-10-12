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

// T020: Integration Test - VTG Sentence to BoatData (COG/SOG + Calculated Variation)
void test_vtg_to_boatdata() {
    // Setup
    MockSerialPort mockSerial;
    tNMEA0183 nmea0183;
    MockDisplayAdapter mockDisplay;
    MockSystemMetrics mockMetrics;
    WebSocketLogger logger(&mockDisplay, &mockMetrics);
    BoatData boatData;
    NMEA0183Handler handler(&nmea0183, &mockSerial, &boatData, &logger);

    // Load valid VTG sentence
    mockSerial.setMockData(VALID_VHVTG);

    // Process
    handler.processSentences();

    // Verify GPS data (COG/SOG)
    GPSData gpsData = boatData.getGPSData();
    TEST_ASSERT_FLOAT_WITHIN(0.001, 0.9548, gpsData.cog);  // 54.7° true COG in radians
    TEST_ASSERT_FLOAT_WITHIN(0.1, 5.5, gpsData.sog);  // knots

    // Verify calculated variation (54.7 - 57.9 = -3.2° = -0.0558 radians)
    CompassData compassData = boatData.getCompassData();
    TEST_ASSERT_FLOAT_WITHIN(0.001, -0.0558, compassData.variation);
}
