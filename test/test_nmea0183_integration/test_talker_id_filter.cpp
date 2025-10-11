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

// T022: Integration Test - Talker ID Filtering
void test_talker_id_filter() {
    // Setup
    MockSerialPort mockSerial;
    tNMEA0183 nmea0183;
    MockDisplayAdapter mockDisplay;
    MockSystemMetrics mockMetrics;
    WebSocketLogger logger(&mockDisplay, &mockMetrics);
    BoatData boatData;
    NMEA0183Handler handler(&nmea0183, &mockSerial, &boatData, &logger);

    // Test: GGA sentence with "GP" talker ID (should be ignored, only "VH" accepted)
    mockSerial.setMockData(INVALID_GGA_TALKER);
    handler.processSentences();

    GPSData gpsData = boatData.getGPSData();
    TEST_ASSERT_FALSE(gpsData.available);  // No update

    // Test: RSA sentence with "VH" talker ID (should be ignored, only "AP" accepted)
    mockSerial.setMockData(INVALID_APRSA_TALKER);
    handler.processSentences();

    RudderData rudderData = boatData.getRudderData();
    TEST_ASSERT_FALSE(rudderData.available);  // No update

    // Test: Valid talker IDs ("AP" and "VH") are accepted
    mockSerial.setMockData(VALID_APRSA);
    handler.processSentences();
    TEST_ASSERT_TRUE(boatData.getRudderData().available);  // Updated

    mockSerial.setMockData(VALID_VHGGA);
    handler.processSentences();
    TEST_ASSERT_TRUE(boatData.getGPSData().available);  // Updated
}
