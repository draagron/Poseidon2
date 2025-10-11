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

// T021: Integration Test - Multi-Source Priority (NMEA 2000 Wins)
void test_multi_source_priority() {
    // Setup
    MockSerialPort mockSerial;
    tNMEA0183 nmea0183;
    MockDisplayAdapter mockDisplay;
    MockSystemMetrics mockMetrics;
    WebSocketLogger logger(&mockDisplay, &mockMetrics);
    BoatData boatData;
    NMEA0183Handler handler(&nmea0183, &mockSerial, &boatData, &logger);

    // Register higher-priority NMEA 2000 GPS source (10 Hz)
    boatData.updateGPS(50.0, 4.0, 0.0, 0.0, "NMEA2000-GPS");

    // Simulate multiple updates to establish priority
    for (int i = 0; i < 10; i++) {
        boatData.updateGPS(50.0 + i * 0.001, 4.0 + i * 0.001, 0.0, 0.0, "NMEA2000-GPS");
        delay(100);  // 10 Hz frequency
    }

    // Load NMEA 0183 GGA sentence (1 Hz, lower frequency)
    mockSerial.setMockData(VALID_VHGGA);
    handler.processSentences();

    // Verify: NMEA 0183 data rejected (NMEA 2000 source has higher priority)
    GPSData gpsData = boatData.getGPSData();

    // GPS data should still be from NMEA 2000 source (not NMEA 0183)
    TEST_ASSERT_FLOAT_WITHIN(0.01, 50.009, gpsData.latitude);  // Last NMEA 2000 update
    TEST_ASSERT_NOT_EQUAL_FLOAT(52.508333, gpsData.latitude);  // Not NMEA 0183 data
}
