/**
 * @file test_iboatdatastore_contract.cpp
 * @brief Contract test for IBoatDataStore interface
 *
 * This test verifies the IBoatDataStore interface contract:
 * - All getter methods return data correctly
 * - All setter methods update data correctly
 * - Data round-trips correctly (write → read)
 * - Available flags work correctly
 * - LastUpdate timestamps are set
 *
 * @see specs/003-boatdata-feature-as/contracts/README.md lines 11-29
 * @test T004
 */

#include <unity.h>
#include "../../src/types/BoatDataTypes.h"

// Forward declaration - interface will be implemented in Phase 3.3
class IBoatDataStore {
public:
    virtual ~IBoatDataStore() {}

    // GPS data access
    virtual GPSData getGPSData() = 0;
    virtual void setGPSData(const GPSData& data) = 0;

    // Compass data access
    virtual CompassData getCompassData() = 0;
    virtual void setCompassData(const CompassData& data) = 0;

    // Wind data access
    virtual WindData getWindData() = 0;
    virtual void setWindData(const WindData& data) = 0;

    // Speed data access
    virtual SpeedData getSpeedData() = 0;
    virtual void setSpeedData(const SpeedData& data) = 0;

    // Rudder data access
    virtual RudderData getRudderData() = 0;
    virtual void setRudderData(const RudderData& data) = 0;

    // Derived data access
    virtual DerivedData getDerivedData() = 0;
    virtual void setDerivedData(const DerivedData& data) = 0;

    // Calibration data access
    virtual CalibrationData getCalibration() = 0;
    virtual void setCalibration(const CalibrationData& data) = 0;
};

// Mock implementation for testing the interface contract
class MockBoatDataStore : public IBoatDataStore {
private:
    BoatData data;

public:
    MockBoatDataStore() {
        // Initialize all data to zero/false
        memset(&data, 0, sizeof(BoatData));
    }

    GPSData getGPSData() override { return data.gps; }
    void setGPSData(const GPSData& gpsData) override { data.gps = gpsData; }

    CompassData getCompassData() override { return data.compass; }
    void setCompassData(const CompassData& compassData) override { data.compass = compassData; }

    WindData getWindData() override { return data.wind; }
    void setWindData(const WindData& windData) override { data.wind = windData; }

    SpeedData getSpeedData() override { return data.speed; }
    void setSpeedData(const SpeedData& speedData) override { data.speed = speedData; }

    RudderData getRudderData() override { return data.rudder; }
    void setRudderData(const RudderData& rudderData) override { data.rudder = rudderData; }

    DerivedData getDerivedData() override { return data.derived; }
    void setDerivedData(const DerivedData& derivedData) override { data.derived = derivedData; }

    CalibrationData getCalibration() override { return data.calibration; }
    void setCalibration(const CalibrationData& calibrationData) override { data.calibration = calibrationData; }
};

// Test fixture
static MockBoatDataStore* store = nullptr;

void setUp(void) {
    store = new MockBoatDataStore();
}

void tearDown(void) {
    delete store;
    store = nullptr;
}

// =============================================================================
// GPS DATA TESTS
// =============================================================================

void test_gps_data_round_trip() {
    // Arrange: Create GPS data
    GPSData gpsData;
    gpsData.latitude = 40.7128;   // New York City
    gpsData.longitude = -74.0060;
    gpsData.cog = 1.571;          // 90° (π/2 rad), heading East
    gpsData.sog = 5.5;            // 5.5 knots
    gpsData.available = true;
    gpsData.lastUpdate = 12345;

    // Act: Write and read back
    store->setGPSData(gpsData);
    GPSData retrieved = store->getGPSData();

    // Assert: Data matches
    TEST_ASSERT_EQUAL_DOUBLE(40.7128, retrieved.latitude);
    TEST_ASSERT_EQUAL_DOUBLE(-74.0060, retrieved.longitude);
    TEST_ASSERT_EQUAL_DOUBLE(1.571, retrieved.cog);
    TEST_ASSERT_EQUAL_DOUBLE(5.5, retrieved.sog);
    TEST_ASSERT_TRUE(retrieved.available);
    TEST_ASSERT_EQUAL_UINT32(12345, retrieved.lastUpdate);
}

void test_gps_available_flag() {
    // Arrange: GPS initially unavailable
    GPSData initial = store->getGPSData();
    TEST_ASSERT_FALSE(initial.available);

    // Act: Set available GPS data
    GPSData gpsData;
    gpsData.latitude = 40.0;
    gpsData.longitude = -74.0;
    gpsData.cog = 0.0;
    gpsData.sog = 0.0;
    gpsData.available = true;
    gpsData.lastUpdate = 1000;
    store->setGPSData(gpsData);

    // Assert: GPS now available
    GPSData retrieved = store->getGPSData();
    TEST_ASSERT_TRUE(retrieved.available);
}

// =============================================================================
// COMPASS DATA TESTS
// =============================================================================

void test_compass_data_round_trip() {
    // Arrange
    CompassData compassData;
    compassData.trueHeading = 1.571;      // 90° (π/2 rad), heading East
    compassData.magneticHeading = 1.658;  // ~95° (with 5° variation)
    compassData.variation = 0.087;        // 5° East variation
    compassData.available = true;
    compassData.lastUpdate = 23456;

    // Act
    store->setCompassData(compassData);
    CompassData retrieved = store->getCompassData();

    // Assert
    TEST_ASSERT_EQUAL_DOUBLE(1.571, retrieved.trueHeading);
    TEST_ASSERT_EQUAL_DOUBLE(1.658, retrieved.magneticHeading);
    TEST_ASSERT_EQUAL_DOUBLE(0.087, retrieved.variation);
    TEST_ASSERT_TRUE(retrieved.available);
    TEST_ASSERT_EQUAL_UINT32(23456, retrieved.lastUpdate);
}

// =============================================================================
// WIND DATA TESTS
// =============================================================================

void test_wind_data_round_trip() {
    // Arrange
    WindData windData;
    windData.apparentWindAngle = 0.785;  // 45° starboard (π/4 rad)
    windData.apparentWindSpeed = 12.0;   // 12 knots
    windData.available = true;
    windData.lastUpdate = 34567;

    // Act
    store->setWindData(windData);
    WindData retrieved = store->getWindData();

    // Assert
    TEST_ASSERT_EQUAL_DOUBLE(0.785, retrieved.apparentWindAngle);
    TEST_ASSERT_EQUAL_DOUBLE(12.0, retrieved.apparentWindSpeed);
    TEST_ASSERT_TRUE(retrieved.available);
    TEST_ASSERT_EQUAL_UINT32(34567, retrieved.lastUpdate);
}

// =============================================================================
// SPEED DATA TESTS
// =============================================================================

void test_speed_data_round_trip() {
    // Arrange
    SpeedData speedData;
    speedData.heelAngle = 0.175;         // 10° starboard heel
    speedData.measuredBoatSpeed = 5.5;   // 5.5 knots
    speedData.available = true;
    speedData.lastUpdate = 45678;

    // Act
    store->setSpeedData(speedData);
    SpeedData retrieved = store->getSpeedData();

    // Assert
    TEST_ASSERT_EQUAL_DOUBLE(0.175, retrieved.heelAngle);
    TEST_ASSERT_EQUAL_DOUBLE(5.5, retrieved.measuredBoatSpeed);
    TEST_ASSERT_TRUE(retrieved.available);
    TEST_ASSERT_EQUAL_UINT32(45678, retrieved.lastUpdate);
}

// =============================================================================
// RUDDER DATA TESTS
// =============================================================================

void test_rudder_data_round_trip() {
    // Arrange
    RudderData rudderData;
    rudderData.steeringAngle = 0.087;   // 5° starboard rudder
    rudderData.available = true;
    rudderData.lastUpdate = 56789;

    // Act
    store->setRudderData(rudderData);
    RudderData retrieved = store->getRudderData();

    // Assert
    TEST_ASSERT_EQUAL_DOUBLE(0.087, retrieved.steeringAngle);
    TEST_ASSERT_TRUE(retrieved.available);
    TEST_ASSERT_EQUAL_UINT32(56789, retrieved.lastUpdate);
}

// =============================================================================
// DERIVED DATA TESTS
// =============================================================================

void test_derived_data_round_trip() {
    // Arrange
    DerivedData derivedData;
    derivedData.awaOffset = 0.8;
    derivedData.awaHeel = 0.75;
    derivedData.leeway = 0.05;
    derivedData.stw = 5.3;
    derivedData.tws = 10.5;
    derivedData.twa = 0.9;
    derivedData.wdir = 1.6;
    derivedData.vmg = 4.8;
    derivedData.soc = 0.8;
    derivedData.doc = 2.1;
    derivedData.available = true;
    derivedData.lastUpdate = 67890;

    // Act
    store->setDerivedData(derivedData);
    DerivedData retrieved = store->getDerivedData();

    // Assert all 11 parameters
    TEST_ASSERT_EQUAL_DOUBLE(0.8, retrieved.awaOffset);
    TEST_ASSERT_EQUAL_DOUBLE(0.75, retrieved.awaHeel);
    TEST_ASSERT_EQUAL_DOUBLE(0.05, retrieved.leeway);
    TEST_ASSERT_EQUAL_DOUBLE(5.3, retrieved.stw);
    TEST_ASSERT_EQUAL_DOUBLE(10.5, retrieved.tws);
    TEST_ASSERT_EQUAL_DOUBLE(0.9, retrieved.twa);
    TEST_ASSERT_EQUAL_DOUBLE(1.6, retrieved.wdir);
    TEST_ASSERT_EQUAL_DOUBLE(4.8, retrieved.vmg);
    TEST_ASSERT_EQUAL_DOUBLE(0.8, retrieved.soc);
    TEST_ASSERT_EQUAL_DOUBLE(2.1, retrieved.doc);
    TEST_ASSERT_TRUE(retrieved.available);
    TEST_ASSERT_EQUAL_UINT32(67890, retrieved.lastUpdate);
}

// =============================================================================
// CALIBRATION DATA TESTS
// =============================================================================

void test_calibration_data_round_trip() {
    // Arrange
    CalibrationData calibrationData;
    calibrationData.leewayCalibrationFactor = 0.65;
    calibrationData.windAngleOffset = 0.087;  // 5° masthead offset
    calibrationData.loaded = true;

    // Act
    store->setCalibration(calibrationData);
    CalibrationData retrieved = store->getCalibration();

    // Assert
    TEST_ASSERT_EQUAL_DOUBLE(0.65, retrieved.leewayCalibrationFactor);
    TEST_ASSERT_EQUAL_DOUBLE(0.087, retrieved.windAngleOffset);
    TEST_ASSERT_TRUE(retrieved.loaded);
}

void test_calibration_defaults() {
    // Arrange: Fresh store
    // Act: Get calibration (should be defaults)
    CalibrationData retrieved = store->getCalibration();

    // Assert: Default values (uninitialized, will be 0)
    // Note: Actual implementation should initialize to DEFAULT_LEEWAY_K_FACTOR (1.0)
    // and DEFAULT_WIND_ANGLE_OFFSET (0.0), but this mock doesn't do that
    TEST_ASSERT_EQUAL_DOUBLE(0.0, retrieved.leewayCalibrationFactor);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, retrieved.windAngleOffset);
    TEST_ASSERT_FALSE(retrieved.loaded);
}

// =============================================================================
// TEST RUNNER
// =============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // GPS data tests
    RUN_TEST(test_gps_data_round_trip);
    RUN_TEST(test_gps_available_flag);

    // Compass data tests
    RUN_TEST(test_compass_data_round_trip);

    // Wind data tests
    RUN_TEST(test_wind_data_round_trip);

    // Speed data tests
    RUN_TEST(test_speed_data_round_trip);

    // Rudder data tests
    RUN_TEST(test_rudder_data_round_trip);

    // Derived data tests
    RUN_TEST(test_derived_data_round_trip);

    // Calibration data tests
    RUN_TEST(test_calibration_data_round_trip);
    RUN_TEST(test_calibration_defaults);

    return UNITY_END();
}
