#ifndef UNITCONVERTER_H
#define UNITCONVERTER_H

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef DEG_TO_RAD
#define DEG_TO_RAD (M_PI / 180.0)
#endif

#ifndef RAD_TO_DEG
#define RAD_TO_DEG (180.0 / M_PI)
#endif

/**
 * @file UnitConverter.h
 * @brief Static utility functions for NMEA 0183 to BoatData unit conversions
 *
 * Provides conversion functions for marine data units:
 * - Degrees ↔ Radians
 * - NMEA coordinate format (DDMM.MMMM) → Decimal degrees
 * - Magnetic variation calculation from true/magnetic course difference
 * - Angle normalization to standard ranges
 *
 * All functions are static and header-only for performance (inline expansion).
 *
 * Usage:
 * @code
 * double radians = UnitConverter::degreesToRadians(45.5);
 * double decimalLat = UnitConverter::nmeaCoordinateToDecimal(5230.5000, 'N');
 * double variation = UnitConverter::calculateVariation(54.7, 57.9);
 * @endcode
 */
class UnitConverter {
public:
    /**
     * @brief Convert degrees to radians
     *
     * @param degrees Angle in degrees
     * @return Angle in radians
     */
    static inline double degreesToRadians(double degrees) {
        return degrees * DEG_TO_RAD;
    }

    /**
     * @brief Convert radians to degrees
     *
     * @param radians Angle in radians
     * @return Angle in degrees
     */
    static inline double radiansToDegrees(double radians) {
        return radians * RAD_TO_DEG;
    }

    /**
     * @brief Convert NMEA coordinate format to decimal degrees
     *
     * NMEA 0183 coordinates are in format DDMM.MMMM (latitude) or DDDMM.MMMM (longitude),
     * where DD/DDD are degrees and MM.MMMM are minutes. This function extracts degrees
     * and minutes, converts to decimal format, and applies hemisphere sign.
     *
     * Examples:
     * - 5230.5000, 'N' → 52.508333° (52°30.5' North)
     * - 5230.5000, 'S' → -52.508333° (52°30.5' South)
     * - 00507.0000, 'E' → 5.116667° (5°7' East)
     * - 00507.0000, 'W' → -5.116667° (5°7' West)
     *
     * @param degreesMinutes NMEA format coordinate (e.g., 5230.5000)
     * @param hemisphere 'N', 'S', 'E', or 'W'
     * @return Decimal degrees (positive = N/E, negative = S/W)
     */
    static inline double nmeaCoordinateToDecimal(double degreesMinutes, char hemisphere) {
        // Extract degrees (integer division by 100)
        int degrees = static_cast<int>(degreesMinutes / 100.0);

        // Extract minutes (remainder)
        double minutes = degreesMinutes - (degrees * 100.0);

        // Convert to decimal degrees
        double decimal = static_cast<double>(degrees) + (minutes / 60.0);

        // Apply hemisphere sign
        if (hemisphere == 'S' || hemisphere == 'W') {
            decimal = -decimal;
        }

        return decimal;
    }

    /**
     * @brief Calculate magnetic variation from true and magnetic course
     *
     * Magnetic variation (declination) is the difference between true north and
     * magnetic north. Calculated as: variation = true_course - magnetic_course
     *
     * - Positive variation = East (magnetic north is east of true north)
     * - Negative variation = West (magnetic north is west of true north)
     *
     * Result is normalized to [-180°, 180°] to handle wraparound cases.
     *
     * Examples:
     * - trueCOG=54.7°, magCOG=57.9° → -3.2° (3.2°W)
     * - trueCOG=100.0°, magCOG=95.0° → 5.0° (5.0°E)
     * - trueCOG=10.0°, magCOG=350.0° → 20.0° (wraparound case)
     *
     * @param trueCOG True course over ground (degrees)
     * @param magCOG Magnetic course over ground (degrees)
     * @return Magnetic variation in degrees (positive=East, negative=West)
     */
    static inline double calculateVariation(double trueCOG, double magCOG) {
        double variation = trueCOG - magCOG;

        // Normalize to [-180, 180] range
        while (variation > 180.0) {
            variation -= 360.0;
        }
        while (variation < -180.0) {
            variation += 360.0;
        }

        return variation;
    }

    /**
     * @brief Normalize angle to [0, 2π] range
     *
     * Wraps angle to standard range [0, 2π] radians by adding/subtracting 2π.
     *
     * Examples:
     * - 3π → π
     * - -π/2 → 3π/2
     * - 5π → π
     *
     * @param radians Angle in radians (any value)
     * @return Normalized angle in [0, 2π]
     */
    static inline double normalizeAngle(double radians) {
        while (radians >= (2.0 * M_PI)) {
            radians -= (2.0 * M_PI);
        }
        while (radians < 0.0) {
            radians += (2.0 * M_PI);
        }

        return radians;
    }

    /**
     * @brief Normalize angle to [-π, π] range
     *
     * Wraps angle to range [-π, π] radians by adding/subtracting 2π.
     * Useful for signed angles like apparent wind angle.
     *
     * @param radians Angle in radians (any value)
     * @return Normalized angle in [-π, π]
     */
    static inline double normalizeAngleSigned(double radians) {
        while (radians > M_PI) {
            radians -= 2.0 * M_PI;
        }
        while (radians < -M_PI) {
            radians += 2.0 * M_PI;
        }

        return radians;
    }
};

#endif // UNITCONVERTER_H
