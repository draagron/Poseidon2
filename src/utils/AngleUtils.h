/**
 * @file AngleUtils.h
 * @brief Utility functions for angle normalization and wraparound handling
 *
 * This utility provides functions for normalizing angles to standard ranges
 * and handling angle arithmetic with wraparound (e.g., 359° + 5° = 4°, not 364°).
 *
 * All angles are in radians. Two standard ranges are supported:
 * - [0, 2π]: For headings/bearings (0 = North, clockwise)
 * - [-π, π]: For relative angles (positive = starboard, negative = port)
 *
 * @see specs/003-boatdata-feature-as/research.md line 263 (wraparound handling)
 * @see test/unit/test_angle_utils.cpp
 * @version 1.0.0
 * @date 2025-10-06
 */

#ifndef ANGLE_UTILS_H
#define ANGLE_UTILS_H

#include <Arduino.h>

/**
 * @brief Utility class for angle operations
 *
 * Static-only utility class (no instantiation required).
 */
class AngleUtils {
public:
    /**
     * @brief Normalize angle to [0, 2π] range
     *
     * Normalizes an angle in radians to the range [0, 2π]. This is the
     * standard range for headings and bearings (0 = North, clockwise).
     *
     * Examples:
     * - 7.0 rad → 0.717 rad (7.0 - 2π)
     * - -0.5 rad → 5.783 rad (-0.5 + 2π)
     * - 1.5 rad → 1.5 rad (already in range)
     *
     * @param angle Angle in radians (any value)
     * @return Normalized angle in radians [0, 2π]
     *
     * @example
     * double heading = 7.0;  // > 2π
     * double normalized = AngleUtils::normalizeToZeroTwoPi(heading);  // 0.717 rad
     */
    static double normalizeToZeroTwoPi(double angle) {
        // Reduce to [0, 2π] using fmod
        angle = fmod(angle, 2 * M_PI);

        // Handle negative angles
        if (angle < 0.0) {
            angle += 2 * M_PI;
        }

        return angle;
    }

    /**
     * @brief Normalize angle to [-π, π] range
     *
     * Normalizes an angle in radians to the range [-π, π]. This is the
     * standard range for relative angles (positive = starboard, negative = port).
     *
     * Examples:
     * - 4.0 rad → -2.283 rad (4.0 - 2π)
     * - -4.0 rad → 2.283 rad (-4.0 + 2π)
     * - 0.5 rad → 0.5 rad (already in range)
     *
     * @param angle Angle in radians (any value)
     * @return Normalized angle in radians [-π, π]
     *
     * @example
     * double awa = 4.0;  // > π
     * double normalized = AngleUtils::normalizeToPiMinusPi(awa);  // -2.283 rad
     */
    static double normalizeToPiMinusPi(double angle) {
        // First normalize to [0, 2π]
        angle = normalizeToZeroTwoPi(angle);

        // Then shift to [-π, π]
        if (angle > M_PI) {
            angle -= 2 * M_PI;
        }

        return angle;
    }

    /**
     * @brief Calculate angle difference with wraparound handling
     *
     * Calculates the signed difference between two angles (b - a) with proper
     * wraparound handling. The result is always in the range [-π, π].
     *
     * This is critical for heading changes that cross the 0°/360° boundary.
     *
     * Examples:
     * - angleDifference(6.1 rad (350°), 0.175 rad (10°)) → 0.261 rad (15°), NOT -6.0 rad
     * - angleDifference(0.175 rad (10°), 6.1 rad (350°)) → -0.261 rad (-15°)
     *
     * @param a First angle in radians
     * @param b Second angle in radians
     * @return Signed difference (b - a) in radians [-π, π]
     *
     * @example
     * double heading1 = 6.1;  // 350° (near North)
     * double heading2 = 0.175;  // 10°
     * double change = AngleUtils::angleDifference(heading1, heading2);  // 0.261 rad (15°)
     */
    static double angleDifference(double a, double b) {
        // Normalize both angles to [0, 2π]
        a = normalizeToZeroTwoPi(a);
        b = normalizeToZeroTwoPi(b);

        // Calculate difference
        double diff = b - a;

        // Normalize difference to [-π, π]
        if (diff > M_PI) {
            diff -= 2 * M_PI;
        } else if (diff < -M_PI) {
            diff += 2 * M_PI;
        }

        return diff;
    }

    /**
     * @brief Add two angles with wraparound
     *
     * Adds two angles and normalizes the result to [0, 2π].
     *
     * Example:
     * - angleAdd(6.1 rad (350°), 0.349 rad (20°)) → 0.366 rad (10°), NOT 6.449 rad
     *
     * @param a First angle in radians
     * @param b Second angle in radians
     * @return Sum of angles in radians [0, 2π]
     *
     * @example
     * double heading = 6.1;  // 350°
     * double turn = 0.349;   // 20°
     * double newHeading = AngleUtils::angleAdd(heading, turn);  // 0.366 rad (10°)
     */
    static double angleAdd(double a, double b) {
        return normalizeToZeroTwoPi(a + b);
    }

    /**
     * @brief Convert degrees to radians
     *
     * @param degrees Angle in degrees
     * @return Angle in radians
     */
    static double degreesToRadians(double degrees) {
        return degrees * M_PI / 180.0;
    }

    /**
     * @brief Convert radians to degrees
     *
     * @param radians Angle in radians
     * @return Angle in degrees
     */
    static double radiansToDegrees(double radians) {
        return radians * 180.0 / M_PI;
    }

private:
    // Static-only class - prevent instantiation
    AngleUtils() {}
};

#endif // ANGLE_UTILS_H
