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

// T023: Integration Test - Unsupported Message Type Filtering
void test_message_type_filter() {
    // Setup
    MockSerialPort mockSerial;
    tNMEA0183 nmea0183;
    MockDisplayAdapter mockDisplay;
    MockSystemMetrics mockMetrics;
    WebSocketLogger logger(&mockDisplay, &mockMetrics);
    BoatData boatData;
    NMEA0183Handler handler(&nmea0183, &mockSerial, &boatData, &logger);

    // Test: MWV sentence (wind data, not supported) should be ignored
    mockSerial.setMockData(UNSUPPORTED_APMWV);
    handler.processSentences();

    // Verify: No WindData update (feature doesn't support wind from NMEA 0183)
    WindData windData = boatData.getWindData();
    TEST_ASSERT_FALSE(windData.available);  // No update

    // Test: Only RSA, HDM, GGA, RMC, VTG are supported
    // All other message types should be silently ignored (FR-007)
}
