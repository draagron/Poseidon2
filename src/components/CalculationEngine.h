/**
 * @file CalculationEngine.h
 * @brief Calculation engine for derived sailing parameters
 *
 * This class implements all 11 derived parameter calculations from raw sensor data:
 * 1. AWA Offset - Apparent wind angle corrected for masthead offset
 * 2. AWA Heel - AWA corrected for heel angle
 * 3. Leeway - Leeway angle from heel and boat speed
 * 4. STW - Speed through water (corrected for leeway)
 * 5. TWS - True wind speed
 * 6. TWA - True wind angle (relative to boat)
 * 7. WDIR - Wind direction (magnetic)
 * 8. VMG - Velocity made good
 * 9. SOC - Speed of current
 * 10. DOC - Direction of current
 *
 * Formulas validated from examples/Calculations/calc.cpp and referenced blog posts.
 * All singularities (divide-by-zero, atan2 NaN, tan(±90°)) are handled gracefully.
 *
 * @see specs/003-boatdata-feature-as/research.md lines 67-191
 * @see test/integration/test_derived_calculation.cpp
 * @version 1.0.0
 * @date 2025-10-06
 */

#ifndef CALCULATION_ENGINE_H
#define CALCULATION_ENGINE_H

#include "../types/BoatDataTypes.h"
#include "../utils/AngleUtils.h"
#include <Arduino.h>

/**
 * @brief Calculation engine for derived parameters
 *
 * Stateless calculation engine - all calculations are pure functions of input data.
 * Call calculate() every 200ms to update derived parameters.
 *
 * Usage:
 * @code
 * CalculationEngine engine;
 * BoatData* boatData = ...;
 * engine.calculate(boatData);  // Updates boatData->derived
 * @endcode
 */
class CalculationEngine {
public:
    /**
     * @brief Calculate all derived parameters
     *
     * Calculates all 11 derived parameters from raw sensor data and calibration.
     * Updates boatData->derived structure. Marks derived.available = false if
     * insufficient sensor data is available.
     *
     * Required sensor data:
     * - GPS: latitude, longitude, COG, SOG (for current calculations)
     * - Compass: true heading, magnetic heading, variation
     * - Wind: apparent wind angle, apparent wind speed
     * - Speed: heel angle, measured boat speed
     * - Calibration: leeway K factor, wind angle offset
     *
     * @param boatData Pointer to BoatDataStructure (modified in place)
     */
    void calculate(BoatDataStructure* boatData);

private:
    /**
     * @brief Calculate AWA offset correction
     *
     * Corrects apparent wind angle for masthead sensor offset (misalignment).
     *
     * Formula: awa_offset = awa_measured + offset
     * Normalization: [-π, π]
     *
     * @param awa Measured apparent wind angle (radians)
     * @param offset Masthead offset from calibration (radians)
     * @return Corrected AWA (radians, [-π, π])
     */
    double calculateAWAOffset(double awa, double offset);

    /**
     * @brief Calculate AWA heel correction
     *
     * Corrects apparent wind angle for heel angle effect. When the boat heels,
     * the masthead sensor tilts, causing an apparent wind angle shift.
     *
     * Formula: awa_heel = atan(tan(awa_offset) / cos(heel))
     * Singularities: tan(±90°) → ±∞ (wind directly ahead/astern)
     * Quadrant correction: Required to resolve atan ambiguity
     *
     * @param awaOffset AWA corrected for offset (radians)
     * @param heel Heel angle (radians, positive = starboard)
     * @return Corrected AWA (radians, [-π, π])
     */
    double calculateAWAHeel(double awaOffset, double heel);

    /**
     * @brief Calculate leeway angle
     *
     * Calculates leeway (sideways drift) from heel angle and boat speed.
     * Leeway occurs when wind pressure on the hull causes the boat to slip
     * sideways through the water.
     *
     * Formula: leeway = K * heel / (boat_speed^2)
     * Conditions:
     * - No leeway if boat speed = 0 (avoid divide-by-zero)
     * - No leeway if wind and heel on same side (physical impossibility)
     * - Clamped to ±45° for very low speeds
     *
     * @param awaHeel AWA corrected for heel (radians)
     * @param heel Heel angle (radians)
     * @param boatSpeed Measured boat speed (knots)
     * @param K Leeway calibration factor
     * @return Leeway angle (radians, [-π/4, π/4])
     */
    double calculateLeeway(double awaHeel, double heel, double boatSpeed, double K);

    /**
     * @brief Calculate speed through water
     *
     * Calculates boat speed through water, accounting for leeway.
     *
     * Formula: stw = boat_speed (for now - simplified)
     * Note: More complex implementations might adjust for leeway correction
     *
     * @param boatSpeed Measured boat speed (knots)
     * @param leeway Leeway angle (radians)
     * @return Speed through water (knots)
     */
    double calculateSTW(double boatSpeed, double leeway);

    /**
     * @brief Calculate true wind speed
     *
     * Calculates true wind speed from apparent wind and boat motion.
     * Uses vector addition in Cartesian coordinates.
     *
     * Formula:
     * - Convert AWA to Cartesian: awa_cartesian = 270° - awa_heel
     * - AWS vector: (aws_x, aws_y) = (AWS * cos(awa_cart), AWS * sin(awa_cart))
     * - Boat motion: lateral_speed = STW * sin(leeway)
     * - True wind: TWS = sqrt((aws_x + lateral_speed)^2 + (aws_y + boat_speed)^2)
     *
     * @param aws Apparent wind speed (knots)
     * @param awaHeel AWA corrected for heel (radians)
     * @param stw Speed through water (knots)
     * @param boatSpeed Measured boat speed (knots)
     * @param leeway Leeway angle (radians)
     * @return True wind speed (knots)
     */
    double calculateTWS(double aws, double awaHeel, double stw, double boatSpeed, double leeway);

    /**
     * @brief Calculate true wind angle
     *
     * Calculates true wind angle relative to boat heading from apparent wind
     * and boat motion. Uses atan2 for quadrant-correct results.
     *
     * Formula:
     * - Calculate TWS vectors (tws_x, tws_y) from calculateTWS
     * - TWA = 270° - atan2(tws_y, tws_x)
     * - Normalize to [-π, π]
     *
     * Singularity: Zero wind → TWA = 0° or 180° based on boat motion direction
     *
     * @param tws_x True wind speed X component (knots)
     * @param tws_y True wind speed Y component (knots)
     * @param awaHeel AWA corrected for heel (radians, for normalization)
     * @return True wind angle (radians, [-π, π])
     */
    double calculateTWA(double tws_x, double tws_y, double awaHeel);

    /**
     * @brief Calculate wind direction
     *
     * Calculates absolute magnetic wind direction from true wind angle and heading.
     *
     * Formula: wdir = heading + twa
     * Normalization: [0, 2π]
     *
     * @param heading Magnetic heading (radians)
     * @param twa True wind angle (radians)
     * @return Wind direction (radians, [0, 2π], magnetic)
     */
    double calculateWDIR(double heading, double twa);

    /**
     * @brief Calculate velocity made good
     *
     * Calculates velocity made good (VMG) - the component of boat speed in
     * the direction of the wind.
     *
     * Formula: vmg = STW * cos(-TWA + leeway)
     *
     * @param stw Speed through water (knots)
     * @param twa True wind angle (radians)
     * @param leeway Leeway angle (radians)
     * @return VMG (knots, signed: positive = toward wind, negative = away)
     */
    double calculateVMG(double stw, double twa, double leeway);

    /**
     * @brief Calculate current speed and direction
     *
     * Calculates ocean/tidal current speed and direction by comparing GPS
     * motion (COG/SOG) with water motion (heading+leeway, STW).
     *
     * Formula:
     * - Convert COG to magnetic: cog_mag = COG + variation
     * - Boat heading in Cartesian: alpha = 90° - (heading + leeway)
     * - GPS motion in Cartesian: gamma = 90° - cog_mag
     * - Current vector: (curr_x, curr_y) = (SOG*cos(gamma), SOG*sin(gamma)) - (STW*cos(alpha), STW*sin(alpha))
     * - SOC = sqrt(curr_x^2 + curr_y^2)
     * - DOC = 90° - atan2(curr_y, curr_x)
     *
     * Singularity: SOG = STW and COG = heading → SOC = 0, DOC undefined but handled
     *
     * @param sog Speed over ground (knots)
     * @param cog Course over ground (radians, true)
     * @param stw Speed through water (knots)
     * @param heading Magnetic heading (radians)
     * @param leeway Leeway angle (radians)
     * @param variation Magnetic variation (radians)
     * @param outSOC Output: Speed of current (knots)
     * @param outDOC Output: Direction of current (radians, [0, 2π], magnetic)
     */
    void calculateCurrent(double sog, double cog, double stw, double heading,
                          double leeway, double variation,
                          double& outSOC, double& outDOC);
};

#endif // CALCULATION_ENGINE_H
