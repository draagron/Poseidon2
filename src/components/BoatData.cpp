/**
 * @file BoatData.cpp
 * @brief Implementation of central boat data repository
 *
 * @see BoatData.h for detailed documentation
 */

#include "BoatData.h"

BoatData::BoatData(ISourcePrioritizer* prioritizer)
    : sourcePrioritizer(prioritizer) {
    // Initialize all data to zero/false
    memset(&data, 0, sizeof(BoatDataStructure));

    // Set calibration defaults
    data.calibration.leewayCalibrationFactor = DEFAULT_LEEWAY_K_FACTOR;
    data.calibration.windAngleOffset = DEFAULT_WIND_ANGLE_OFFSET;
    data.calibration.loaded = false;
}

// =============================================================================
// IBoatDataStore IMPLEMENTATION
// =============================================================================

GPSData BoatData::getGPSData() {
    return data.gps;
}

void BoatData::setGPSData(const GPSData& gpsData) {
    data.gps = gpsData;
}

CompassData BoatData::getCompassData() {
    return data.compass;
}

void BoatData::setCompassData(const CompassData& compassData) {
    data.compass = compassData;
}

WindData BoatData::getWindData() {
    return data.wind;
}

void BoatData::setWindData(const WindData& windData) {
    data.wind = windData;
}

SpeedData BoatData::getSpeedData() {
    return data.dst;  // Updated for v2.0.0: SpeedData renamed to DSTData
}

void BoatData::setSpeedData(const SpeedData& speedData) {
    data.dst = speedData;  // Updated for v2.0.0: SpeedData renamed to DSTData
}

RudderData BoatData::getRudderData() {
    return data.rudder;
}

void BoatData::setRudderData(const RudderData& rudderData) {
    data.rudder = rudderData;
}

DerivedData BoatData::getDerivedData() {
    return data.derived;
}

void BoatData::setDerivedData(const DerivedData& derivedData) {
    data.derived = derivedData;
}

CalibrationData BoatData::getCalibration() {
    return data.calibration;
}

void BoatData::setCalibration(const CalibrationData& calibrationData) {
    data.calibration = calibrationData;
}

// === NEW in v2.0.0 - Enhanced BoatData structures ===

EngineData BoatData::getEngineData() {
    return data.engine;
}

void BoatData::setEngineData(const EngineData& engineData) {
    data.engine = engineData;
}

SaildriveData BoatData::getSaildriveData() {
    return data.saildrive;
}

void BoatData::setSaildriveData(const SaildriveData& saildriveData) {
    data.saildrive = saildriveData;
}

BatteryData BoatData::getBatteryData() {
    return data.battery;
}

void BoatData::setBatteryData(const BatteryData& batteryData) {
    data.battery = batteryData;
}

ShorePowerData BoatData::getShorePowerData() {
    return data.shorePower;
}

void BoatData::setShorePowerData(const ShorePowerData& shorePowerData) {
    data.shorePower = shorePowerData;
}

// =============================================================================
// ISensorUpdate IMPLEMENTATION
// =============================================================================

bool BoatData::updateGPS(double lat, double lon, double cog, double sog, const char* sourceId) {
    // Validate and update
    if (!validateAndUpdateGPS(lat, lon, cog, sog)) {
        return false;
    }

    // Update source prioritizer timestamp (if provided)
    if (sourcePrioritizer != nullptr && sourceId != nullptr) {
        // Note: In full implementation, would lookup source index by sourceId
        // For now, simplified - assumes source already registered
        // sourcePrioritizer->updateSourceTimestamp(sourceIndex, millis());
    }

    return true;
}

bool BoatData::updateCompass(double trueHdg, double magHdg, double variation, const char* sourceId) {
    // Validate and update
    if (!validateAndUpdateCompass(trueHdg, magHdg, variation)) {
        return false;
    }

    // Update source prioritizer timestamp (if provided)
    if (sourcePrioritizer != nullptr && sourceId != nullptr) {
        // Note: Similar to updateGPS - would lookup source index
    }

    return true;
}

bool BoatData::updateWind(double awa, double aws, const char* sourceId) {
    // Range validation
    if (!DataValidator::isValidAWA(awa) || !DataValidator::isValidWindSpeed(aws)) {
        return false;
    }

    // Rate-of-change validation (if previous data exists)
    if (data.wind.available && data.wind.lastUpdate > 0) {
        unsigned long deltaTime = millis() - data.wind.lastUpdate;

        if (!DataValidator::isValidWindAngleRateOfChange(data.wind.apparentWindAngle, awa, deltaTime)) {
            return false;
        }

        if (!DataValidator::isValidWindSpeedRateOfChange(data.wind.apparentWindSpeed, aws, deltaTime)) {
            return false;
        }
    }

    // Accept and store
    data.wind.apparentWindAngle = awa;
    data.wind.apparentWindSpeed = aws;
    data.wind.available = true;
    data.wind.lastUpdate = millis();

    return true;
}

bool BoatData::updateSpeed(double heelAngle, double boatSpeed, const char* sourceId) {
    // Range validation
    if (!DataValidator::isValidHeelAngle(heelAngle) || !DataValidator::isValidBoatSpeed(boatSpeed)) {
        return false;
    }

    // Rate-of-change validation (if previous data exists)
    // NOTE v2.0.0: heelAngle moved to CompassData, but validation still uses old logic for backward compatibility
    if (data.dst.available && data.dst.lastUpdate > 0) {
        unsigned long deltaTime = millis() - data.dst.lastUpdate;

        if (!DataValidator::isValidHeelRateOfChange(data.compass.heelAngle, heelAngle, deltaTime)) {
            return false;
        }

        if (!DataValidator::isValidSpeedRateOfChange(data.dst.measuredBoatSpeed, boatSpeed, deltaTime, 5.0)) {
            return false;
        }
    }

    // Accept and store
    // NOTE v2.0.0: heelAngle now stored in CompassData, boatSpeed stored in DSTData
    data.compass.heelAngle = heelAngle;
    data.dst.measuredBoatSpeed = boatSpeed;
    data.dst.available = true;
    data.dst.lastUpdate = millis();

    return true;
}

bool BoatData::updateRudder(double angle, const char* sourceId) {
    // Range validation
    if (!DataValidator::isValidRudderAngle(angle)) {
        return false;
    }

    // Rate-of-change validation (if previous data exists)
    if (data.rudder.available && data.rudder.lastUpdate > 0) {
        unsigned long deltaTime = millis() - data.rudder.lastUpdate;

        if (!DataValidator::isValidRudderRateOfChange(data.rudder.steeringAngle, angle, deltaTime)) {
            return false;
        }
    }

    // Accept and store
    data.rudder.steeringAngle = angle;
    data.rudder.available = true;
    data.rudder.lastUpdate = millis();

    return true;
}

// =============================================================================
// ADDITIONAL PUBLIC METHODS
// =============================================================================

DiagnosticData BoatData::getDiagnostics() {
    return data.diagnostics;
}

void BoatData::incrementNMEA0183Count() {
    data.diagnostics.nmea0183MessageCount++;
}

void BoatData::incrementNMEA2000Count() {
    data.diagnostics.nmea2000MessageCount++;
}

void BoatData::incrementActisenseCount() {
    data.diagnostics.actisenseMessageCount++;
}

// =============================================================================
// PRIVATE HELPER METHODS
// =============================================================================

bool BoatData::validateAndUpdateGPS(double lat, double lon, double cog, double sog) {
    // Range validation
    if (!DataValidator::isValidLatitude(lat) ||
        !DataValidator::isValidLongitude(lon) ||
        !DataValidator::isValidCOG(cog) ||
        !DataValidator::isValidSOG(sog)) {
        return false;
    }

    // Rate-of-change validation (if previous data exists)
    if (data.gps.available && data.gps.lastUpdate > 0) {
        unsigned long deltaTime = millis() - data.gps.lastUpdate;

        // Skip rate check if data is stale (>5 seconds old)
        if (deltaTime < 5000) {
            if (!DataValidator::isValidGPSRateOfChange(
                    data.gps.latitude, lat,
                    data.gps.longitude, lon,
                    deltaTime)) {
                return false;
            }

            if (!DataValidator::isValidHeadingRateOfChange(data.gps.cog, cog, deltaTime)) {
                return false;
            }

            if (!DataValidator::isValidSpeedRateOfChange(data.gps.sog, sog, deltaTime, 10.0)) {
                return false;
            }
        }
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

bool BoatData::validateAndUpdateCompass(double trueHdg, double magHdg, double variation) {
    // Range validation
    if (!DataValidator::isValidHeading(trueHdg) || !DataValidator::isValidHeading(magHdg)) {
        return false;
    }

    // Rate-of-change validation (if previous data exists)
    if (data.compass.available && data.compass.lastUpdate > 0) {
        unsigned long deltaTime = millis() - data.compass.lastUpdate;

        // Skip rate check if data is stale (>5 seconds old)
        if (deltaTime < 5000) {
            if (!DataValidator::isValidHeadingRateOfChange(data.compass.trueHeading, trueHdg, deltaTime)) {
                return false;
            }

            if (!DataValidator::isValidHeadingRateOfChange(data.compass.magneticHeading, magHdg, deltaTime)) {
                return false;
            }
        }
    }

    // Accept and store
    data.compass.trueHeading = trueHdg;
    data.compass.magneticHeading = magHdg;
    // NOTE v2.0.0: variation moved to GPSData
    data.gps.variation = variation;
    data.compass.available = true;
    data.compass.lastUpdate = millis();

    return true;
}
