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

// T024: Integration Test - Invalid Sentence Handling (Malformed/Out-of-Range)
void test_invalid_sentences() {
    // Setup
    MockSerialPort mockSerial;
    tNMEA0183 nmea0183;
    MockDisplayAdapter mockDisplay;
    MockSystemMetrics mockMetrics;
    WebSocketLogger logger(&mockDisplay, &mockMetrics);
    BoatData boatData;
    NMEA0183Handler handler(&nmea0183, &mockSerial, &boatData, &logger);

    // Test 1: Bad checksum - silent discard (FR-024)
    mockSerial.setMockData(INVALID_APRSA_CHECKSUM);
    handler.processSentences();
    TEST_ASSERT_FALSE(boatData.getRudderData().available);

    // Test 2: Out-of-range rudder angle (120° exceeds ±90°) - rejected (FR-026)
    mockSerial.setMockData(INVALID_APRSA_RANGE);
    handler.processSentences();
    TEST_ASSERT_FALSE(boatData.getRudderData().available);

    // Test 3: Out-of-range variation (35.3° exceeds ±30°) - rejected (FR-026)
    mockSerial.setMockData(INVALID_VTG_VARIATION);
    handler.processSentences();
    CompassData compassData = boatData.getCompassData();
    TEST_ASSERT_FALSE(compassData.available);  // Variation not updated

    // Test 4: Malformed sentence (non-numeric angle) - silent discard
    mockSerial.setMockData(MALFORMED_APRSA);
    handler.processSentences();
    TEST_ASSERT_FALSE(boatData.getRudderData().available);

    // Test 5: Invalid status field ('V' = invalid) - silent discard
    mockSerial.setMockData(INVALID_APRSA_STATUS);
    handler.processSentences();
    TEST_ASSERT_FALSE(boatData.getRudderData().available);

    // All invalid sentences should be silently discarded with no error logs (FR-024/FR-025)
}
