/**
 * @file CalculationEngine.cpp
 * @brief Implementation of calculation engine for derived sailing parameters
 *
 * @see CalculationEngine.h for detailed documentation
 * @see specs/003-boatdata-feature-as/research.md lines 67-191 (validated formulas)
 */

#include "CalculationEngine.h"

void CalculationEngine::calculate(BoatDataStructure* boatData) {
    // Check if we have minimum required sensor data
    if (!boatData->gps.available ||
        !boatData->compass.available ||
        !boatData->wind.available ||
        !boatData->speed.available) {
        // Insufficient data - mark derived as unavailable
        boatData->derived.available = false;
        return;
    }

    // Get calibration parameters
    double K = boatData->calibration.leewayCalibrationFactor;
    double windOffset = boatData->calibration.windAngleOffset;

    // Extract sensor data
    double awa = boatData->wind.apparentWindAngle;
    double aws = boatData->wind.apparentWindSpeed;
    double heel = boatData->speed.heelAngle;
    double boatSpeed = boatData->speed.measuredBoatSpeed;
    double heading = boatData->compass.magneticHeading;
    double variation = boatData->compass.variation;
    double sog = boatData->gps.sog;
    double cog = boatData->gps.cog;

    // =========================================================================
    // STEP 1: AWA Offset Correction
    // =========================================================================
    double awaOffset = calculateAWAOffset(awa, windOffset);
    boatData->derived.awaOffset = awaOffset;

    // =========================================================================
    // STEP 2: AWA Heel Correction
    // =========================================================================
    double awaHeel = calculateAWAHeel(awaOffset, heel);
    boatData->derived.awaHeel = awaHeel;

    // =========================================================================
    // STEP 3: Leeway Calculation
    // =========================================================================
    double leeway = calculateLeeway(awaHeel, heel, boatSpeed, K);
    boatData->derived.leeway = leeway;

    // =========================================================================
    // STEP 4: Speed Through Water
    // =========================================================================
    double stw = calculateSTW(boatSpeed, leeway);
    boatData->derived.stw = stw;

    // =========================================================================
    // STEP 5 & 6: True Wind Speed and Angle
    // =========================================================================
    double tws = calculateTWS(aws, awaHeel, stw, boatSpeed, leeway);
    boatData->derived.tws = tws;

    // Calculate TWS vector components (needed for TWA)
    double cartesianAWA = AngleUtils::normalizeToZeroTwoPi(3 * M_PI / 2 - awaHeel);
    double aws_x = aws * cos(cartesianAWA);
    double aws_y = aws * sin(cartesianAWA);
    double lateral_speed = stw * sin(leeway);
    double tws_x = aws_x + lateral_speed;
    double tws_y = aws_y + boatSpeed;

    double twa = calculateTWA(tws_x, tws_y, awaHeel);
    boatData->derived.twa = twa;

    // =========================================================================
    // STEP 7: Wind Direction
    // =========================================================================
    double wdir = calculateWDIR(heading, twa);
    boatData->derived.wdir = wdir;

    // =========================================================================
    // STEP 8: Velocity Made Good
    // =========================================================================
    double vmg = calculateVMG(stw, twa, leeway);
    boatData->derived.vmg = vmg;

    // =========================================================================
    // STEP 9 & 10: Current Speed and Direction
    // =========================================================================
    double soc, doc;
    calculateCurrent(sog, cog, stw, heading, leeway, variation, soc, doc);
    boatData->derived.soc = soc;
    boatData->derived.doc = doc;

    // =========================================================================
    // Mark as available and update timestamp
    // =========================================================================
    boatData->derived.available = true;
    boatData->derived.lastUpdate = millis();

    // Update diagnostics
    boatData->diagnostics.calculationCount++;
}

// =============================================================================
// PRIVATE CALCULATION METHODS
// =============================================================================

double CalculationEngine::calculateAWAOffset(double awa, double offset) {
    // Add offset and normalize to [-π, π]
    double awaOffset = awa + offset;
    return AngleUtils::normalizeToPiMinusPi(awaOffset);
}

double CalculationEngine::calculateAWAHeel(double awaOffset, double heel) {
    // Calculate tan(AWA) - check for singularity
    double tan_awa = tan(awaOffset);

    if (isnan(tan_awa) || isinf(tan_awa)) {
        // Singularity: wind directly ahead (0°) or astern (±180°)
        // No heel correction needed
        return awaOffset;
    }

    // Calculate heel-corrected AWA
    double cos_heel = cos(heel);
    if (fabs(cos_heel) < 0.0001) {
        // Extreme heel (near 90°) - no correction
        return awaOffset;
    }

    double awaHeel = atan(tan_awa / cos_heel);

    // Quadrant correction (atan only returns [-π/2, π/2])
    if (awaOffset >= 0.0) {
        // Starboard tack
        if (awaOffset > M_PI / 2) {
            // Aft quadrant - add 180°
            awaHeel += M_PI;
        }
    } else {
        // Port tack
        if (awaOffset < -M_PI / 2) {
            // Aft quadrant - subtract 180°
            awaHeel -= M_PI;
        }
    }

    return AngleUtils::normalizeToPiMinusPi(awaHeel);
}

double CalculationEngine::calculateLeeway(double awaHeel, double heel, double boatSpeed, double K) {
    // No leeway if boat is stopped (avoid divide-by-zero)
    if (boatSpeed < 0.1) {
        return 0.0;
    }

    // No leeway if wind and heel are on the same side (physical impossibility)
    // Wind on starboard (AWA > 0) with starboard heel (heel > 0) = no leeway
    // Wind on port (AWA < 0) with port heel (heel < 0) = no leeway
    if ((awaHeel > 0.0 && heel > 0.0) || (awaHeel < 0.0 && heel < 0.0)) {
        return 0.0;
    }

    // Calculate leeway: K * heel / (boat_speed^2)
    double leeway = K * heel / (boatSpeed * boatSpeed);

    // Clamp to ±45° (π/4 radians) for very low speeds
    if (leeway > M_PI / 4) {
        leeway = M_PI / 4;
    } else if (leeway < -M_PI / 4) {
        leeway = -M_PI / 4;
    }

    return leeway;
}

double CalculationEngine::calculateSTW(double boatSpeed, double leeway) {
    // Simplified: STW = measured boat speed
    // More complex implementations might adjust for leeway:
    // stw = boatSpeed / cos(leeway)
    return boatSpeed;
}

double CalculationEngine::calculateTWS(double aws, double awaHeel, double stw, double boatSpeed, double leeway) {
    // Convert AWA to Cartesian coordinates (0° = East, 90° = North)
    // Cartesian AWA = 270° - awaHeel
    double cartesianAWA = AngleUtils::normalizeToZeroTwoPi(3 * M_PI / 2 - awaHeel);

    // Apparent wind vector
    double aws_x = aws * cos(cartesianAWA);
    double aws_y = aws * sin(cartesianAWA);

    // Boat motion vector (forward + lateral from leeway)
    double lateral_speed = stw * sin(leeway);
    // double forward_speed = boatSpeed;  // Already in Y direction

    // True wind vector = Apparent wind + Boat motion
    double tws_x = aws_x + lateral_speed;
    double tws_y = aws_y + boatSpeed;

    // True wind speed = magnitude of true wind vector
    double tws = sqrt(tws_x * tws_x + tws_y * tws_y);

    return tws;
}

double CalculationEngine::calculateTWA(double tws_x, double tws_y, double awaHeel) {
    // Calculate TWA in Cartesian coordinates
    double twa_cartesian = atan2(tws_y, tws_x);

    // Check for singularity (zero wind)
    if (isnan(twa_cartesian)) {
        // Zero wind - default based on boat direction
        if (tws_y < 0.0) {
            return M_PI;  // 180° (wind astern)
        } else {
            return 0.0;   // 0° (wind ahead)
        }
    }

    // Convert from Cartesian to nautical
    // TWA = 270° - twa_cartesian
    double twa = AngleUtils::normalizeToZeroTwoPi(3 * M_PI / 2 - twa_cartesian);

    // Normalize to [-π, π] with proper sign based on AWA
    if (awaHeel >= 0.0) {
        // Starboard tack
        twa = fmod(twa, 2 * M_PI);
    } else {
        // Port tack
        twa -= 2 * M_PI;
    }

    // Final normalization to [-π, π]
    return AngleUtils::normalizeToPiMinusPi(twa);
}

double CalculationEngine::calculateWDIR(double heading, double twa) {
    // Wind direction = heading + true wind angle
    double wdir = heading + twa;

    // Normalize to [0, 2π]
    return AngleUtils::normalizeToZeroTwoPi(wdir);
}

double CalculationEngine::calculateVMG(double stw, double twa, double leeway) {
    // VMG = component of STW in the wind direction
    // VMG = STW * cos(-TWA + leeway)
    double vmg = stw * cos(-twa + leeway);

    return vmg;
}

void CalculationEngine::calculateCurrent(double sog, double cog, double stw, double heading,
                                          double leeway, double variation,
                                          double& outSOC, double& outDOC) {
    // Convert COG (true) to magnetic
    double cog_mag = cog + variation;
    cog_mag = AngleUtils::normalizeToZeroTwoPi(cog_mag);

    // Convert to Cartesian coordinates (0° = East, 90° = North)
    // Boat heading in Cartesian: alpha = 90° - (heading + leeway)
    double alpha = AngleUtils::normalizeToZeroTwoPi(M_PI / 2 - (heading + leeway));

    // GPS motion in Cartesian: gamma = 90° - cog_mag
    double gamma = AngleUtils::normalizeToZeroTwoPi(M_PI / 2 - cog_mag);

    // GPS velocity vector
    double sog_x = sog * cos(gamma);
    double sog_y = sog * sin(gamma);

    // Water velocity vector
    double stw_x = stw * cos(alpha);
    double stw_y = stw * sin(alpha);

    // Current vector = GPS velocity - Water velocity
    double curr_x = sog_x - stw_x;
    double curr_y = sog_y - stw_y;

    // Current speed (magnitude)
    outSOC = sqrt(curr_x * curr_x + curr_y * curr_y);

    // Current direction
    double doc_cartesian = atan2(curr_y, curr_x);

    if (isnan(doc_cartesian)) {
        // Singularity: zero current
        if (curr_y < 0.0) {
            outDOC = M_PI;  // 180°
        } else {
            outDOC = 0.0;   // 0°
        }
    } else {
        // Convert from Cartesian to nautical: DOC = 90° - doc_cartesian
        outDOC = AngleUtils::normalizeToZeroTwoPi(M_PI / 2 - doc_cartesian);
    }
}
