/**
 * @file DataValidation.h
 * @brief Validation helper functions for Enhanced BoatData v2.0.0
 *
 * Provides range validation and clamping functions for new data fields
 * introduced in Enhanced BoatData feature (R005).
 *
 * Validation Rules:
 * - Pitch: [-π/6, π/6] (±30°), warn if exceeded
 * - Heave: [-5.0, 5.0] meters, warn if exceeded
 * - Engine RPM: [0, 6000], warn if exceeded
 * - Battery voltage: [0, 30] volts, warn if outside [10, 15] for 12V system
 * - Temperature: [-10, 150] Celsius for oil, [-10, 50] for water
 *
 * @see specs/008-enhanced-boatdata-following/data-model.md
 * @see specs/008-enhanced-boatdata-following/research.md lines 20-86
 * @version 2.0.0
 * @date 2025-10-10
 */

#ifndef DATA_VALIDATION_H
#define DATA_VALIDATION_H

#include <Arduino.h>
#include <math.h>

namespace DataValidation {

// Validation constants (from research.md)
constexpr double MAX_PITCH_ANGLE = M_PI / 6.0;    // ±30° (π/6 radians)
constexpr double MAX_HEAVE = 5.0;                  // ±5 meters
constexpr double MAX_ENGINE_RPM = 6000.0;          // 0-6000 RPM
constexpr double MAX_BATTERY_VOLTAGE = 30.0;      // 0-30 volts
constexpr double MIN_12V_VOLTAGE = 10.0;          // 12V system low threshold
constexpr double MAX_12V_VOLTAGE = 15.0;          // 12V system high threshold
constexpr double MIN_TEMPERATURE = -10.0;         // Celsius
constexpr double MAX_OIL_TEMP = 150.0;            // Celsius
constexpr double MAX_WATER_TEMP = 50.0;           // Celsius
constexpr double MAX_DEPTH = 100.0;                // meters
constexpr double MAX_BOAT_SPEED = 25.0;           // m/s (~50 knots)
constexpr double MAX_SHORE_POWER = 5000.0;        // watts
constexpr double MAX_BATTERY_AMPERAGE = 200.0;    // amperes

/**
 * @brief Clamp pitch angle to valid range [-π/6, π/6]
 *
 * @param pitch Pitch angle in radians
 * @return Clamped pitch angle
 */
inline double clampPitchAngle(double pitch) {
    return constrain(pitch, -MAX_PITCH_ANGLE, MAX_PITCH_ANGLE);
}

/**
 * @brief Validate pitch angle is within normal range
 *
 * @param pitch Pitch angle in radians
 * @return true if within [-π/6, π/6], false otherwise
 */
inline bool isValidPitchAngle(double pitch) {
    return (fabs(pitch) <= MAX_PITCH_ANGLE);
}

/**
 * @brief Clamp heave to valid range [-5.0, 5.0] meters
 *
 * @param heave Heave in meters
 * @return Clamped heave value
 */
inline double clampHeave(double heave) {
    return constrain(heave, -MAX_HEAVE, MAX_HEAVE);
}

/**
 * @brief Validate heave is within normal range
 *
 * @param heave Heave in meters
 * @return true if within [-5.0, 5.0], false otherwise
 */
inline bool isValidHeave(double heave) {
    return (fabs(heave) <= MAX_HEAVE);
}

/**
 * @brief Clamp engine RPM to valid range [0, 6000]
 *
 * @param rpm Engine RPM
 * @return Clamped RPM value
 */
inline double clampEngineRPM(double rpm) {
    return constrain(rpm, 0.0, MAX_ENGINE_RPM);
}

/**
 * @brief Validate engine RPM is within normal range
 *
 * @param rpm Engine RPM
 * @return true if within [0, 6000], false otherwise
 */
inline bool isValidEngineRPM(double rpm) {
    return (rpm >= 0.0 && rpm <= MAX_ENGINE_RPM);
}

/**
 * @brief Clamp battery voltage to valid range [0, 30] volts
 *
 * @param voltage Battery voltage in volts
 * @return Clamped voltage value
 */
inline double clampBatteryVoltage(double voltage) {
    return constrain(voltage, 0.0, MAX_BATTERY_VOLTAGE);
}

/**
 * @brief Validate battery voltage is within normal range for 12V system
 *
 * @param voltage Battery voltage in volts
 * @return true if within [10, 15] volts, false if outside (but still [0, 30])
 */
inline bool isValidBatteryVoltage(double voltage) {
    return (voltage >= MIN_12V_VOLTAGE && voltage <= MAX_12V_VOLTAGE);
}

/**
 * @brief Check if battery voltage is within absolute bounds [0, 30]
 *
 * @param voltage Battery voltage in volts
 * @return true if within [0, 30], false otherwise
 */
inline bool isWithinVoltageRange(double voltage) {
    return (voltage >= 0.0 && voltage <= MAX_BATTERY_VOLTAGE);
}

/**
 * @brief Clamp temperature to valid range for oil temperature
 *
 * @param temp Temperature in Celsius
 * @return Clamped temperature value [-10, 150]
 */
inline double clampOilTemperature(double temp) {
    return constrain(temp, MIN_TEMPERATURE, MAX_OIL_TEMP);
}

/**
 * @brief Validate oil temperature is within normal range
 *
 * @param temp Temperature in Celsius
 * @return true if within [-10, 150], false otherwise
 */
inline bool isValidOilTemperature(double temp) {
    return (temp >= MIN_TEMPERATURE && temp <= MAX_OIL_TEMP);
}

/**
 * @brief Clamp temperature to valid range for water temperature
 *
 * @param temp Temperature in Celsius
 * @return Clamped temperature value [-10, 50]
 */
inline double clampWaterTemperature(double temp) {
    return constrain(temp, MIN_TEMPERATURE, MAX_WATER_TEMP);
}

/**
 * @brief Validate water temperature is within normal range
 *
 * @param temp Temperature in Celsius
 * @return true if within [-10, 50], false otherwise
 */
inline bool isValidWaterTemperature(double temp) {
    return (temp >= MIN_TEMPERATURE && temp <= MAX_WATER_TEMP);
}

/**
 * @brief Convert Kelvin to Celsius
 *
 * Used for NMEA2000 PGN 130316 (Temperature Extended Range)
 *
 * @param kelvin Temperature in Kelvin
 * @return Temperature in Celsius
 */
inline double kelvinToCelsius(double kelvin) {
    return kelvin - 273.15;
}

/**
 * @brief Convert meters per second to knots
 *
 * Used for NMEA2000 speed conversions (SOG, wind speed, boat speed)
 *
 * @param mps Speed in meters per second
 * @return Speed in knots
 */
inline double mpsToKnots(double mps) {
    return mps * 1.9438444924406047516198704103672;
}

/**
 * @brief Clamp latitude to valid range [-90, 90] degrees
 *
 * @param latitude Latitude in decimal degrees
 * @return Clamped latitude value
 */
inline double clampLatitude(double latitude) {
    return constrain(latitude, -90.0, 90.0);
}

/**
 * @brief Validate latitude is within valid range
 *
 * @param latitude Latitude in decimal degrees
 * @return true if within [-90, 90], false otherwise
 */
inline bool isValidLatitude(double latitude) {
    return (latitude >= -90.0 && latitude <= 90.0);
}

/**
 * @brief Clamp longitude to valid range [-180, 180] degrees
 *
 * @param longitude Longitude in decimal degrees
 * @return Clamped longitude value
 */
inline double clampLongitude(double longitude) {
    return constrain(longitude, -180.0, 180.0);
}

/**
 * @brief Validate longitude is within valid range
 *
 * @param longitude Longitude in decimal degrees
 * @return true if within [-180, 180], false otherwise
 */
inline bool isValidLongitude(double longitude) {
    return (longitude >= -180.0 && longitude <= 180.0);
}

/**
 * @brief Wrap angle to range [0, 2π] radians
 *
 * @param angle Angle in radians
 * @return Wrapped angle in [0, 2π]
 */
inline double wrapAngle2Pi(double angle) {
    while (angle < 0) angle += 2 * M_PI;
    while (angle >= 2 * M_PI) angle -= 2 * M_PI;
    return angle;
}

/**
 * @brief Validate COG (Course Over Ground) is within range
 *
 * @param cog Course in radians
 * @return true if within [0, 2π], false otherwise
 */
inline bool isValidCOG(double cog) {
    return (cog >= 0.0 && cog <= 2 * M_PI);
}

/**
 * @brief Clamp SOG (Speed Over Ground) to valid range [0, 100] knots
 *
 * @param sog Speed in knots
 * @return Clamped SOG value
 */
inline double clampSOG(double sog) {
    return constrain(sog, 0.0, 100.0);
}

/**
 * @brief Validate SOG is within normal range
 *
 * @param sog Speed in knots
 * @return true if within [0, 100], false otherwise
 */
inline bool isValidSOG(double sog) {
    return (sog >= 0.0 && sog <= 100.0);
}

/**
 * @brief Clamp magnetic variation to valid range [-30°, 30°]
 *
 * @param variation Variation in radians
 * @return Clamped variation value
 */
inline double clampVariation(double variation) {
    constexpr double MAX_VARIATION = 30.0 * M_PI / 180.0;  // 30° in radians
    return constrain(variation, -MAX_VARIATION, MAX_VARIATION);
}

/**
 * @brief Validate magnetic variation is within normal range
 *
 * @param variation Variation in radians
 * @return true if within [-30°, 30°], false otherwise
 */
inline bool isValidVariation(double variation) {
    constexpr double MAX_VARIATION = 30.0 * M_PI / 180.0;  // 30° in radians
    return (fabs(variation) <= MAX_VARIATION);
}

/**
 * @brief Wrap wind angle to range [-π, π] radians
 *
 * @param angle Wind angle in radians
 * @return Wrapped angle in [-π, π]
 */
inline double wrapWindAngle(double angle) {
    while (angle < -M_PI) angle += 2 * M_PI;
    while (angle > M_PI) angle -= 2 * M_PI;
    return angle;
}

/**
 * @brief Clamp wind speed to valid range [0, 100] knots
 *
 * @param speed Wind speed in knots
 * @return Clamped wind speed value
 */
inline double clampWindSpeed(double speed) {
    return constrain(speed, 0.0, 100.0);
}

/**
 * @brief Validate wind speed is within normal range
 *
 * @param speed Wind speed in knots
 * @return true if within [0, 100], false otherwise
 */
inline bool isValidWindSpeed(double speed) {
    return (speed >= 0.0 && speed <= 100.0);
}

/**
 * @brief Validate heading angle is within range [0, 2π]
 *
 * @param heading Heading in radians
 * @return true if within [0, 2π], false otherwise
 */
inline bool isValidHeading(double heading) {
    return (heading >= 0.0 && heading <= 2 * M_PI);
}

/**
 * @brief Clamp depth to valid range [0, 100] meters
 *
 * @param depth Depth in meters
 * @return Clamped depth value (negative values set to 0)
 */
inline double clampDepth(double depth) {
    return constrain(depth, 0.0, MAX_DEPTH);
}

/**
 * @brief Validate depth is within normal range
 *
 * @param depth Depth in meters
 * @return true if within [0, 100], false if negative or excessive
 */
inline bool isValidDepth(double depth) {
    return (depth >= 0.0 && depth <= MAX_DEPTH);
}

/**
 * @brief Clamp boat speed to valid range [0, 25] m/s
 *
 * @param speed Boat speed in m/s
 * @return Clamped speed value
 */
inline double clampBoatSpeed(double speed) {
    return constrain(speed, 0.0, MAX_BOAT_SPEED);
}

/**
 * @brief Validate boat speed is within normal range
 *
 * @param speed Boat speed in m/s
 * @return true if within [0, 25], false otherwise
 */
inline bool isValidBoatSpeed(double speed) {
    return (speed >= 0.0 && speed <= MAX_BOAT_SPEED);
}

/**
 * @brief Clamp battery amperage to valid range [-200, 200] amperes
 *
 * Sign convention: positive = charging, negative = discharging
 *
 * @param amperage Battery current in amperes
 * @return Clamped amperage value
 */
inline double clampBatteryAmperage(double amperage) {
    return constrain(amperage, -MAX_BATTERY_AMPERAGE, MAX_BATTERY_AMPERAGE);
}

/**
 * @brief Validate battery amperage is within normal range
 *
 * @param amperage Battery current in amperes
 * @return true if within [-200, 200], false otherwise
 */
inline bool isValidBatteryAmperage(double amperage) {
    return (fabs(amperage) <= MAX_BATTERY_AMPERAGE);
}

/**
 * @brief Clamp shore power consumption to valid range [0, 5000] watts
 *
 * @param power Power consumption in watts
 * @return Clamped power value
 */
inline double clampShorePower(double power) {
    return constrain(power, 0.0, MAX_SHORE_POWER);
}

/**
 * @brief Validate shore power is within normal range
 *
 * Warns if exceeds 3000W (typical 30A circuit limit)
 *
 * @param power Power consumption in watts
 * @return true if within [0, 5000], false otherwise
 */
inline bool isValidShorePower(double power) {
    return (power >= 0.0 && power <= MAX_SHORE_POWER);
}

/**
 * @brief Check if shore power exceeds typical circuit limit
 *
 * @param power Power consumption in watts
 * @return true if exceeds 3000W (warning threshold)
 */
inline bool exceedsShorePowerWarningThreshold(double power) {
    return (power > 3000.0);
}

/**
 * @brief Clamp heel angle to valid range [-π/2, π/2]
 *
 * @param heel Heel angle in radians
 * @return Clamped heel angle
 */
inline double clampHeelAngle(double heel) {
    return constrain(heel, -M_PI / 2.0, M_PI / 2.0);
}

/**
 * @brief Validate heel angle is within normal range (warn if > ±45°)
 *
 * @param heel Heel angle in radians
 * @return true if within [-π/4, π/4] (±45°), false if excessive
 */
inline bool isValidHeelAngle(double heel) {
    return (fabs(heel) <= M_PI / 4.0);
}

/**
 * @brief Clamp rate of turn to valid range [-π, π] rad/s
 *
 * @param rot Rate of turn in radians/second
 * @return Clamped rate of turn
 */
inline double clampRateOfTurn(double rot) {
    return constrain(rot, -M_PI, M_PI);
}

/**
 * @brief Validate rate of turn is within normal range
 *
 * @param rot Rate of turn in radians/second
 * @return true if within [-π, π], false otherwise
 */
inline bool isValidRateOfTurn(double rot) {
    return (fabs(rot) <= M_PI);
}

/**
 * @brief Clamp state of charge to valid range [0, 100] percent
 *
 * @param soc State of charge percentage
 * @return Clamped SOC value
 */
inline double clampStateOfCharge(double soc) {
    return constrain(soc, 0.0, 100.0);
}

/**
 * @brief Validate state of charge is within valid range
 *
 * @param soc State of charge percentage
 * @return true if within [0, 100], false otherwise
 */
inline bool isValidStateOfCharge(double soc) {
    return (soc >= 0.0 && soc <= 100.0);
}

} // namespace DataValidation

#endif // DATA_VALIDATION_H
