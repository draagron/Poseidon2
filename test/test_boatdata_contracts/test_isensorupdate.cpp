/**
 * @file test_isensorupdate_contract.cpp
 * @brief Contract test for ISensorUpdate interface
 *
 * This test verifies the ISensorUpdate interface contract:
 * - updateGPS() accepts valid GPS data
 * - updateCompass() accepts valid compass data
 * - updateWind() accepts valid wind data
 * - updateSpeed() accepts valid speed/heel data
 * - updateRudder() accepts valid rudder data
 * - Returns false for invalid/outlier data
 *
 * @see specs/003-boatdata-feature-as/contracts/README.md lines 31-42
 * @test T005
 */

#include <unity.h>
#include "../../src/types/BoatDataTypes.h"

// Forward declaration - interface will be implemented in Phase 3.3
class ISensorUpdate {
public:
    virtual ~ISensorUpdate() {}

    /**
     * @brief Update GPS data
     * @param lat Latitude in decimal degrees
     * @param lon Longitude in decimal degrees
     * @param cog Course over ground in radians
     * @param sog Speed over ground in knots
     * @param sourceId Source identifier (e.g., "GPS-NMEA0183")
     * @return true if update accepted (valid), false if rejected (outlier/invalid)
     */
    virtual bool updateGPS(double lat, double lon, double cog, double sog, const char* sourceId) = 0;

    /**
     * @brief Update compass data
     * @param trueHdg True heading in radians
     * @param magHdg Magnetic heading in radians
     * @param variation Magnetic variation in radians
     * @param sourceId Source identifier
     * @return true if accepted, false if rejected
     */
    virtual bool updateCompass(double trueHdg, double magHdg, double variation, const char* sourceId) = 0;

    /**
     * @brief Update wind data
     * @param awa Apparent wind angle in radians
     * @param aws Apparent wind speed in knots
     * @param sourceId Source identifier
     * @return true if accepted, false if rejected
     */
    virtual bool updateWind(double awa, double aws, const char* sourceId) = 0;

    /**
     * @brief Update speed and heel data
     * @param heelAngle Heel angle in radians
     * @param boatSpeed Boat speed in knots
     * @param sourceId Source identifier
     * @return true if accepted, false if rejected
     */
    virtual bool updateSpeed(double heelAngle, double boatSpeed, const char* sourceId) = 0;

    /**
     * @brief Update rudder data
     * @param angle Rudder angle in radians
     * @param sourceId Source identifier
     * @return true if accepted, false if rejected
     */
    virtual bool updateRudder(double angle, const char* sourceId) = 0;
};

// Mock implementation for testing the interface contract
class MockSensorUpdate : public ISensorUpdate {
private:
    BoatData data;

    // Simple range validation (detailed validation will be in DataValidator)
    bool isValidLatitude(double lat) { return lat >= -90.0 && lat <= 90.0; }
    bool isValidLongitude(double lon) { return lon >= -180.0 && lon <= 180.0; }
    bool isValidCOG(double cog) { return cog >= 0.0 && cog <= 2 * M_PI; }
    bool isValidSOG(double sog) { return sog >= 0.0 && sog <= 100.0; }
    bool isValidHeading(double hdg) { return hdg >= 0.0 && hdg <= 2 * M_PI; }
    bool isValidAWA(double awa) { return awa >= -M_PI && awa <= M_PI; }
    bool isValidAWS(double aws) { return aws >= 0.0 && aws <= 100.0; }
    bool isValidHeelAngle(double heel) { return heel >= -M_PI / 2 && heel <= M_PI / 2; }
    bool isValidBoatSpeed(double speed) { return speed >= 0.0 && speed <= 50.0; }
    bool isValidRudderAngle(double angle) { return angle >= -M_PI / 2 && angle <= M_PI / 2; }

public:
    MockSensorUpdate() {
        memset(&data, 0, sizeof(BoatData));
    }

    bool updateGPS(double lat, double lon, double cog, double sog, const char* sourceId) override {
        // Validate ranges
        if (!isValidLatitude(lat) || !isValidLongitude(lon) ||
            !isValidCOG(cog) || !isValidSOG(sog)) {
            return false;  // Reject invalid data
        }

        // Accept and store
        data.gps.latitude = lat;
        data.gps.longitude = lon;
        data.gps.cog = cog;
        data.gps.sog = sog;
        data.gps.available = true;
        data.gps.lastUpdate = millis();
        return true;
    }

    bool updateCompass(double trueHdg, double magHdg, double variation, const char* sourceId) override {
        if (!isValidHeading(trueHdg) || !isValidHeading(magHdg)) {
            return false;
        }

        data.compass.trueHeading = trueHdg;
        data.compass.magneticHeading = magHdg;
        data.compass.variation = variation;
        data.compass.available = true;
        data.compass.lastUpdate = millis();
        return true;
    }

    bool updateWind(double awa, double aws, const char* sourceId) override {
        if (!isValidAWA(awa) || !isValidAWS(aws)) {
            return false;
        }

        data.wind.apparentWindAngle = awa;
        data.wind.apparentWindSpeed = aws;
        data.wind.available = true;
        data.wind.lastUpdate = millis();
        return true;
    }

    bool updateSpeed(double heelAngle, double boatSpeed, const char* sourceId) override {
        if (!isValidHeelAngle(heelAngle) || !isValidBoatSpeed(boatSpeed)) {
            return false;
        }

        data.speed.heelAngle = heelAngle;
        data.speed.measuredBoatSpeed = boatSpeed;
        data.speed.available = true;
        data.speed.lastUpdate = millis();
        return true;
    }

    bool updateRudder(double angle, const char* sourceId) override {
        if (!isValidRudderAngle(angle)) {
            return false;
        }

        data.rudder.steeringAngle = angle;
        data.rudder.available = true;
        data.rudder.lastUpdate = millis();
        return true;
    }

    // Accessor for testing
    BoatData getData() { return data; }
};

// Test fixture
static MockSensorUpdate* sensorUpdate = nullptr;

void setUp(void) {
    sensorUpdate = new MockSensorUpdate();
}

void tearDown(void) {
    delete sensorUpdate;
    sensorUpdate = nullptr;
}

// =============================================================================
// GPS UPDATE TESTS
// =============================================================================

void test_updateGPS_accepts_valid_data() {
    // Act: Update with valid GPS data (New York City)
    bool accepted = sensorUpdate->updateGPS(
        40.7128,   // latitude
        -74.0060,  // longitude
        1.571,     // COG: 90° (π/2 rad), heading East
        5.5,       // SOG: 5.5 knots
        "GPS-NMEA0183"
    );

    // Assert: Update accepted
    TEST_ASSERT_TRUE(accepted);

    // Assert: Data stored correctly
    BoatData data = sensorUpdate->getData();
    TEST_ASSERT_EQUAL_DOUBLE(40.7128, data.gps.latitude);
    TEST_ASSERT_EQUAL_DOUBLE(-74.0060, data.gps.longitude);
    TEST_ASSERT_EQUAL_DOUBLE(1.571, data.gps.cog);
    TEST_ASSERT_EQUAL_DOUBLE(5.5, data.gps.sog);
    TEST_ASSERT_TRUE(data.gps.available);
}

void test_updateGPS_rejects_invalid_latitude() {
    // Act: Try to update with invalid latitude (200° > 90° max)
    bool accepted = sensorUpdate->updateGPS(
        200.0,     // INVALID: outside [-90, 90] range
        -74.0060,
        1.571,
        5.5,
        "GPS-NMEA0183"
    );

    // Assert: Update rejected
    TEST_ASSERT_FALSE(accepted);
}

void test_updateGPS_rejects_negative_speed() {
    // Act: Try to update with negative SOG
    bool accepted = sensorUpdate->updateGPS(
        40.7128,
        -74.0060,
        1.571,
        -5.5,      // INVALID: negative speed
        "GPS-NMEA0183"
    );

    // Assert: Update rejected
    TEST_ASSERT_FALSE(accepted);
}

// =============================================================================
// COMPASS UPDATE TESTS
// =============================================================================

void test_updateCompass_accepts_valid_data() {
    // Act: Update with valid compass data
    bool accepted = sensorUpdate->updateCompass(
        1.571,  // true heading: 90° (East)
        1.658,  // magnetic heading: ~95° (with variation)
        0.087,  // variation: 5° East
        "Compass-NMEA2000"
    );

    // Assert: Update accepted
    TEST_ASSERT_TRUE(accepted);

    // Assert: Data stored
    BoatData data = sensorUpdate->getData();
    TEST_ASSERT_EQUAL_DOUBLE(1.571, data.compass.trueHeading);
    TEST_ASSERT_EQUAL_DOUBLE(1.658, data.compass.magneticHeading);
    TEST_ASSERT_EQUAL_DOUBLE(0.087, data.compass.variation);
    TEST_ASSERT_TRUE(data.compass.available);
}

void test_updateCompass_rejects_invalid_heading() {
    // Act: Try to update with heading > 2π
    bool accepted = sensorUpdate->updateCompass(
        7.0,    // INVALID: > 2π (6.283)
        1.658,
        0.087,
        "Compass-NMEA2000"
    );

    // Assert: Update rejected
    TEST_ASSERT_FALSE(accepted);
}

// =============================================================================
// WIND UPDATE TESTS
// =============================================================================

void test_updateWind_accepts_valid_data() {
    // Act: Update with valid wind data
    bool accepted = sensorUpdate->updateWind(
        0.785,  // AWA: 45° starboard (π/4 rad)
        12.0,   // AWS: 12 knots
        "Wind-NMEA0183"
    );

    // Assert: Update accepted
    TEST_ASSERT_TRUE(accepted);

    // Assert: Data stored
    BoatData data = sensorUpdate->getData();
    TEST_ASSERT_EQUAL_DOUBLE(0.785, data.wind.apparentWindAngle);
    TEST_ASSERT_EQUAL_DOUBLE(12.0, data.wind.apparentWindSpeed);
    TEST_ASSERT_TRUE(data.wind.available);
}

void test_updateWind_rejects_invalid_AWA() {
    // Act: Try to update with AWA > π (outside [-π, π] range)
    bool accepted = sensorUpdate->updateWind(
        4.0,    // INVALID: > π (3.14159)
        12.0,
        "Wind-NMEA0183"
    );

    // Assert: Update rejected
    TEST_ASSERT_FALSE(accepted);
}

// =============================================================================
// SPEED UPDATE TESTS
// =============================================================================

void test_updateSpeed_accepts_valid_data() {
    // Act: Update with valid speed/heel data
    bool accepted = sensorUpdate->updateSpeed(
        0.175,  // heel angle: 10° starboard
        5.5,    // boat speed: 5.5 knots
        "Speed-1Wire"
    );

    // Assert: Update accepted
    TEST_ASSERT_TRUE(accepted);

    // Assert: Data stored
    BoatData data = sensorUpdate->getData();
    TEST_ASSERT_EQUAL_DOUBLE(0.175, data.speed.heelAngle);
    TEST_ASSERT_EQUAL_DOUBLE(5.5, data.speed.measuredBoatSpeed);
    TEST_ASSERT_TRUE(data.speed.available);
}

void test_updateSpeed_rejects_invalid_heel_angle() {
    // Act: Try to update with heel angle > π/2 (90°)
    bool accepted = sensorUpdate->updateSpeed(
        2.0,    // INVALID: > π/2 (1.5708)
        5.5,
        "Speed-1Wire"
    );

    // Assert: Update rejected
    TEST_ASSERT_FALSE(accepted);
}

// =============================================================================
// RUDDER UPDATE TESTS
// =============================================================================

void test_updateRudder_accepts_valid_data() {
    // Act: Update with valid rudder data
    bool accepted = sensorUpdate->updateRudder(
        0.087,  // rudder angle: 5° starboard
        "Rudder-1Wire"
    );

    // Assert: Update accepted
    TEST_ASSERT_TRUE(accepted);

    // Assert: Data stored
    BoatData data = sensorUpdate->getData();
    TEST_ASSERT_EQUAL_DOUBLE(0.087, data.rudder.steeringAngle);
    TEST_ASSERT_TRUE(data.rudder.available);
}

void test_updateRudder_rejects_invalid_angle() {
    // Act: Try to update with rudder angle > π/2 (90°)
    bool accepted = sensorUpdate->updateRudder(
        2.0,    // INVALID: > π/2 (1.5708)
        "Rudder-1Wire"
    );

    // Assert: Update rejected
    TEST_ASSERT_FALSE(accepted);
}

// =============================================================================
// SOURCE ID TRACKING TESTS
// =============================================================================

void test_updateGPS_tracks_source_id() {
    // Act: Update from different sources
    sensorUpdate->updateGPS(40.0, -74.0, 1.5, 5.0, "GPS-NMEA0183");
    sensorUpdate->updateGPS(40.1, -74.1, 1.6, 5.5, "GPS-NMEA2000");

    // Assert: Last update wins (this is simplified; real implementation will use SourcePrioritizer)
    BoatData data = sensorUpdate->getData();
    TEST_ASSERT_EQUAL_DOUBLE(40.1, data.gps.latitude);
}

// =============================================================================
// TEST RUNNER
// =============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // GPS update tests
    RUN_TEST(test_updateGPS_accepts_valid_data);
    RUN_TEST(test_updateGPS_rejects_invalid_latitude);
    RUN_TEST(test_updateGPS_rejects_negative_speed);

    // Compass update tests
    RUN_TEST(test_updateCompass_accepts_valid_data);
    RUN_TEST(test_updateCompass_rejects_invalid_heading);

    // Wind update tests
    RUN_TEST(test_updateWind_accepts_valid_data);
    RUN_TEST(test_updateWind_rejects_invalid_AWA);

    // Speed update tests
    RUN_TEST(test_updateSpeed_accepts_valid_data);
    RUN_TEST(test_updateSpeed_rejects_invalid_heel_angle);

    // Rudder update tests
    RUN_TEST(test_updateRudder_accepts_valid_data);
    RUN_TEST(test_updateRudder_rejects_invalid_angle);

    // Source tracking tests
    RUN_TEST(test_updateGPS_tracks_source_id);

    return UNITY_END();
}
