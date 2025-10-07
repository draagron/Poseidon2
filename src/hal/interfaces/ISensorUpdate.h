/**
 * @file ISensorUpdate.h
 * @brief Abstract interface for sensor data updates
 *
 * This interface defines the contract for NMEA0183, NMEA2000, and 1-Wire
 * message handlers to update boat sensor data. It provides validation and
 * multi-source management without coupling to internal storage details.
 *
 * Usage:
 * - NMEA0183 handlers call updateGPS(), updateCompass(), etc.
 * - NMEA2000 handlers call the same methods
 * - 1-Wire handlers call updateSpeed(), updateRudder()
 * - Returns true if data accepted, false if rejected (outlier/invalid)
 *
 * @see specs/003-boatdata-feature-as/contracts/README.md lines 31-42
 * @see test/contract/test_isensorupdate_contract.cpp
 * @version 1.0.0
 * @date 2025-10-06
 */

#ifndef ISENSOR_UPDATE_H
#define ISENSOR_UPDATE_H

/**
 * @brief Abstract interface for sensor updates
 *
 * All update methods return a boolean indicating acceptance:
 * - true: Data valid and accepted
 * - false: Data invalid/outlier, rejected and not stored
 *
 * Implementations:
 * - BoatData: Production implementation (src/components/BoatData.h)
 * - MockSensorUpdate: Test mock (in contract test file)
 */
class ISensorUpdate {
public:
    virtual ~ISensorUpdate() {}

    /**
     * @brief Update GPS data from NMEA message
     *
     * Validates and stores GPS position and speed data. Rejects:
     * - Latitude outside [-90, 90] degrees
     * - Longitude outside [-180, 180] degrees
     * - COG outside [0, 2π] radians
     * - SOG < 0 or > 100 knots
     * - Rate-of-change exceeding limits (see research.md)
     *
     * @param lat Latitude in decimal degrees (positive = North)
     * @param lon Longitude in decimal degrees (positive = East)
     * @param cog Course over ground in radians [0, 2π], true
     * @param sog Speed over ground in knots [0, 100]
     * @param sourceId Source identifier (e.g., "GPS-NMEA0183", "GPS-NMEA2000")
     * @return true if accepted, false if rejected
     *
     * @example
     * bool accepted = boatData->updateGPS(40.7128, -74.0060, 1.571, 5.5, "GPS-NMEA0183");
     */
    virtual bool updateGPS(double lat, double lon, double cog, double sog, const char* sourceId) = 0;

    /**
     * @brief Update compass data from NMEA message
     *
     * Validates and stores heading and magnetic variation. Rejects:
     * - Headings outside [0, 2π] radians
     * - Rate-of-change exceeding 180°/sec
     *
     * @param trueHdg True heading in radians [0, 2π]
     * @param magHdg Magnetic heading in radians [0, 2π]
     * @param variation Magnetic variation in radians (positive = East, negative = West)
     * @param sourceId Source identifier (e.g., "Compass-NMEA2000")
     * @return true if accepted, false if rejected
     *
     * @example
     * bool accepted = boatData->updateCompass(1.571, 1.658, 0.087, "Compass-NMEA2000");
     */
    virtual bool updateCompass(double trueHdg, double magHdg, double variation, const char* sourceId) = 0;

    /**
     * @brief Update wind vane data from NMEA message
     *
     * Validates and stores apparent wind angle and speed. Rejects:
     * - AWA outside [-π, π] radians
     * - AWS < 0 or > 100 knots
     * - Rate-of-change exceeding limits
     *
     * @param awa Apparent wind angle in radians [-π, π] (positive = starboard, negative = port)
     * @param aws Apparent wind speed in knots [0, 100]
     * @param sourceId Source identifier (e.g., "Wind-NMEA0183")
     * @return true if accepted, false if rejected
     *
     * @example
     * bool accepted = boatData->updateWind(0.785, 12.0, "Wind-NMEA0183");
     */
    virtual bool updateWind(double awa, double aws, const char* sourceId) = 0;

    /**
     * @brief Update speed and heel data from paddle wheel/1-Wire sensors
     *
     * Validates and stores boat speed and heel angle. Rejects:
     * - Heel angle outside [-π/2, π/2] radians (±90°)
     * - Boat speed < 0 or > 50 knots
     * - Rate-of-change exceeding limits
     *
     * @param heelAngle Heel angle in radians [-π/2, π/2] (positive = starboard, negative = port)
     * @param boatSpeed Measured boat speed in knots [0, 50]
     * @param sourceId Source identifier (e.g., "Speed-1Wire")
     * @return true if accepted, false if rejected
     *
     * @example
     * bool accepted = boatData->updateSpeed(0.175, 5.5, "Speed-1Wire");
     */
    virtual bool updateSpeed(double heelAngle, double boatSpeed, const char* sourceId) = 0;

    /**
     * @brief Update rudder angle from 1-Wire sensor
     *
     * Validates and stores rudder position. Rejects:
     * - Rudder angle outside [-π/2, π/2] radians (±90°)
     * - Rate-of-change exceeding 60°/sec
     *
     * @param angle Rudder angle in radians [-π/2, π/2] (positive = starboard, negative = port)
     * @param sourceId Source identifier (e.g., "Rudder-1Wire")
     * @return true if accepted, false if rejected
     *
     * @example
     * bool accepted = boatData->updateRudder(0.087, "Rudder-1Wire");
     */
    virtual bool updateRudder(double angle, const char* sourceId) = 0;
};

#endif // ISENSOR_UPDATE_H
