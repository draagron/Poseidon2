/**
 * @file DataValidator.h
 * @brief Utility class for sensor data validation (range + rate-of-change)
 *
 * This utility provides validation for incoming sensor data using a hybrid
 * approach:
 * 1. Range checks: Validate values are within physically possible ranges
 * 2. Rate-of-change checks: Reject values that change too quickly
 *
 * Validation thresholds are based on marine sensor characteristics and boat
 * dynamics research.
 *
 * @see specs/003-boatdata-feature-as/research.md lines 236-274 (outlier detection)
 * @see specs/003-boatdata-feature-as/data-model.md lines 129-145 (validation rules)
 * @see test/unit/test_range_validation.cpp
 * @see test/unit/test_rate_validation.cpp
 * @version 1.0.0
 * @date 2025-10-06
 */

#ifndef DATA_VALIDATOR_H
#define DATA_VALIDATOR_H

#include <Arduino.h>
#include "AngleUtils.h"

/**
 * @brief Utility class for sensor data validation
 *
 * Implements hybrid range + rate-of-change validation to detect:
 * - Sensor errors (e.g., latitude = 200°N)
 * - Noise spikes (e.g., GPS jumps 100m instantly)
 * - Invalid readings while allowing valid rapid changes (e.g., tacking)
 *
 * Static-only utility class (no instantiation required).
 */
class DataValidator {
public:
    // =========================================================================
    // RANGE VALIDATION
    // =========================================================================

    /**
     * @brief Validate GPS latitude
     *
     * Valid range: [-90, 90] degrees
     *
     * @param lat Latitude in decimal degrees
     * @return true if valid, false if out of range
     */
    static bool isValidLatitude(double lat) {
        return lat >= -90.0 && lat <= 90.0;
    }

    /**
     * @brief Validate GPS longitude
     *
     * Valid range: [-180, 180] degrees
     *
     * @param lon Longitude in decimal degrees
     * @return true if valid, false if out of range
     */
    static bool isValidLongitude(double lon) {
        return lon >= -180.0 && lon <= 180.0;
    }

    /**
     * @brief Validate course over ground (COG)
     *
     * Valid range: [0, 2π] radians
     *
     * @param cog Course in radians
     * @return true if valid, false if out of range
     */
    static bool isValidCOG(double cog) {
        return cog >= 0.0 && cog <= 2 * M_PI;
    }

    /**
     * @brief Validate speed over ground (SOG)
     *
     * Valid range: [0, 100] knots
     *
     * @param sog Speed in knots
     * @return true if valid, false if negative or excessive
     */
    static bool isValidSOG(double sog) {
        return sog >= 0.0 && sog <= 100.0;
    }

    /**
     * @brief Validate heading (true or magnetic)
     *
     * Valid range: [0, 2π] radians
     *
     * @param heading Heading in radians
     * @return true if valid, false if out of range
     */
    static bool isValidHeading(double heading) {
        return heading >= 0.0 && heading <= 2 * M_PI;
    }

    /**
     * @brief Validate apparent wind angle (AWA)
     *
     * Valid range: [-π, π] radians
     *
     * @param awa Apparent wind angle in radians
     * @return true if valid, false if out of range
     */
    static bool isValidAWA(double awa) {
        return awa >= -M_PI && awa <= M_PI;
    }

    /**
     * @brief Validate wind speed (apparent or true)
     *
     * Valid range: [0, 100] knots
     *
     * @param speed Wind speed in knots
     * @return true if valid, false if negative or excessive
     */
    static bool isValidWindSpeed(double speed) {
        return speed >= 0.0 && speed <= 100.0;
    }

    /**
     * @brief Validate heel angle
     *
     * Valid range: [-π/2, π/2] radians (±90°)
     *
     * @param heel Heel angle in radians
     * @return true if valid, false if out of range
     */
    static bool isValidHeelAngle(double heel) {
        return heel >= -M_PI / 2 && heel <= M_PI / 2;
    }

    /**
     * @brief Validate boat speed
     *
     * Valid range: [0, 50] knots
     *
     * @param speed Boat speed in knots
     * @return true if valid, false if negative or excessive
     */
    static bool isValidBoatSpeed(double speed) {
        return speed >= 0.0 && speed <= 50.0;
    }

    /**
     * @brief Validate rudder angle
     *
     * Valid range: [-π/2, π/2] radians (±90°)
     *
     * @param angle Rudder angle in radians
     * @return true if valid, false if out of range
     */
    static bool isValidRudderAngle(double angle) {
        return angle >= -M_PI / 2 && angle <= M_PI / 2;
    }

    // =========================================================================
    // RATE-OF-CHANGE VALIDATION
    // =========================================================================

    /**
     * @brief Validate GPS position rate-of-change
     *
     * Maximum rate: 0.1°/second (~6nm at equator, ~360 knots max)
     *
     * @param prevLat Previous latitude (degrees)
     * @param newLat New latitude (degrees)
     * @param prevLon Previous longitude (degrees)
     * @param newLon New longitude (degrees)
     * @param deltaTime Time since last update (milliseconds)
     * @return true if change is reasonable, false if too fast
     */
    static bool isValidGPSRateOfChange(double prevLat, double newLat,
                                        double prevLon, double newLon,
                                        unsigned long deltaTime) {
        if (deltaTime == 0) return true;  // Avoid divide-by-zero

        double deltaSeconds = deltaTime / 1000.0;
        double latChange = fabs(newLat - prevLat);
        double lonChange = fabs(newLon - prevLon);

        // Maximum change per second: 0.1°
        const double MAX_CHANGE_PER_SEC = 0.1;

        double latRate = latChange / deltaSeconds;
        double lonRate = lonChange / deltaSeconds;

        return (latRate <= MAX_CHANGE_PER_SEC) && (lonRate <= MAX_CHANGE_PER_SEC);
    }

    /**
     * @brief Validate heading rate-of-change
     *
     * Maximum rate: 180°/second (π rad/sec) - max turn rate
     *
     * @param prevHeading Previous heading (radians)
     * @param newHeading New heading (radians)
     * @param deltaTime Time since last update (milliseconds)
     * @return true if change is reasonable, false if too fast
     */
    static bool isValidHeadingRateOfChange(double prevHeading, double newHeading,
                                            unsigned long deltaTime) {
        if (deltaTime == 0) return true;

        double deltaSeconds = deltaTime / 1000.0;

        // Calculate heading change with wraparound handling
        double change = fabs(AngleUtils::angleDifference(prevHeading, newHeading));

        // Maximum change per second: π radians (180°)
        const double MAX_CHANGE_PER_SEC = M_PI;

        double rate = change / deltaSeconds;

        return rate <= MAX_CHANGE_PER_SEC;
    }

    /**
     * @brief Validate speed (SOG or boat speed) rate-of-change
     *
     * Maximum rate for SOG: 10 knots/second (max acceleration)
     * Maximum rate for boat speed: 5 knots/second
     *
     * @param prevSpeed Previous speed (knots)
     * @param newSpeed New speed (knots)
     * @param deltaTime Time since last update (milliseconds)
     * @param maxRate Maximum rate of change (knots/second), default 10.0
     * @return true if change is reasonable, false if too fast
     */
    static bool isValidSpeedRateOfChange(double prevSpeed, double newSpeed,
                                          unsigned long deltaTime,
                                          double maxRate = 10.0) {
        if (deltaTime == 0) return true;

        double deltaSeconds = deltaTime / 1000.0;
        double change = fabs(newSpeed - prevSpeed);
        double rate = change / deltaSeconds;

        return rate <= maxRate;
    }

    /**
     * @brief Validate wind angle rate-of-change
     *
     * Maximum rate: 360°/second (2π rad/sec) - allows instant tacking
     *
     * @param prevAWA Previous AWA (radians)
     * @param newAWA New AWA (radians)
     * @param deltaTime Time since last update (milliseconds)
     * @return true if change is reasonable, false if too fast
     */
    static bool isValidWindAngleRateOfChange(double prevAWA, double newAWA,
                                              unsigned long deltaTime) {
        if (deltaTime == 0) return true;

        double deltaSeconds = deltaTime / 1000.0;

        // Calculate angle change with wraparound
        double change = fabs(AngleUtils::angleDifference(prevAWA, newAWA));

        // Maximum change per second: 2π radians (360°)
        const double MAX_CHANGE_PER_SEC = 2 * M_PI;

        double rate = change / deltaSeconds;

        return rate <= MAX_CHANGE_PER_SEC;
    }

    /**
     * @brief Validate wind speed rate-of-change
     *
     * Maximum rate: 30 knots/second (allows gusts)
     *
     * @param prevAWS Previous AWS (knots)
     * @param newAWS New AWS (knots)
     * @param deltaTime Time since last update (milliseconds)
     * @return true if change is reasonable, false if too fast
     */
    static bool isValidWindSpeedRateOfChange(double prevAWS, double newAWS,
                                              unsigned long deltaTime) {
        return isValidSpeedRateOfChange(prevAWS, newAWS, deltaTime, 30.0);
    }

    /**
     * @brief Validate heel angle rate-of-change
     *
     * Maximum rate: 45°/second (π/4 rad/sec) - max roll rate
     *
     * @param prevHeel Previous heel (radians)
     * @param newHeel New heel (radians)
     * @param deltaTime Time since last update (milliseconds)
     * @return true if change is reasonable, false if too fast
     */
    static bool isValidHeelRateOfChange(double prevHeel, double newHeel,
                                         unsigned long deltaTime) {
        if (deltaTime == 0) return true;

        double deltaSeconds = deltaTime / 1000.0;
        double change = fabs(newHeel - prevHeel);

        // Maximum change per second: π/4 radians (45°)
        const double MAX_CHANGE_PER_SEC = M_PI / 4;

        double rate = change / deltaSeconds;

        return rate <= MAX_CHANGE_PER_SEC;
    }

    /**
     * @brief Validate rudder angle rate-of-change
     *
     * Maximum rate: 60°/second (π/3 rad/sec) - max helm rate
     *
     * @param prevAngle Previous rudder angle (radians)
     * @param newAngle New rudder angle (radians)
     * @param deltaTime Time since last update (milliseconds)
     * @return true if change is reasonable, false if too fast
     */
    static bool isValidRudderRateOfChange(double prevAngle, double newAngle,
                                           unsigned long deltaTime) {
        if (deltaTime == 0) return true;

        double deltaSeconds = deltaTime / 1000.0;
        double change = fabs(newAngle - prevAngle);

        // Maximum change per second: π/3 radians (60°)
        const double MAX_CHANGE_PER_SEC = M_PI / 3;

        double rate = change / deltaSeconds;

        return rate <= MAX_CHANGE_PER_SEC;
    }

private:
    // Static-only class - prevent instantiation
    DataValidator() {}
};

#endif // DATA_VALIDATOR_H
