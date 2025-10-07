/**
 * @file CalibrationManager.cpp
 * @brief Implementation of calibration parameter management
 *
 * @see CalibrationManager.h for detailed documentation
 */

#include "CalibrationManager.h"

const char* CalibrationManager::CALIBRATION_FILE = "/calibration.json";

CalibrationManager::CalibrationManager() {
    // Initialize with defaults
    currentParams.leewayCalibrationFactor = DEFAULT_LEEWAY_K_FACTOR;
    currentParams.windAngleOffset = DEFAULT_WIND_ANGLE_OFFSET;
    currentParams.version = 1;
    currentParams.lastModified = 0;
    currentParams.valid = false;
}

// =============================================================================
// ICalibration IMPLEMENTATION
// =============================================================================

CalibrationParameters CalibrationManager::getCalibration() {
    return currentParams;
}

bool CalibrationManager::setCalibration(const CalibrationParameters& params) {
    // Validate first
    if (!validateCalibration(params)) {
        return false;
    }

    // Accept and store in memory
    currentParams = params;
    currentParams.valid = true;

    return true;
}

bool CalibrationManager::validateCalibration(const CalibrationParameters& params) {
    // K factor must be positive
    if (params.leewayCalibrationFactor <= 0.0) {
        return false;
    }

    // Wind angle offset must be in range [-2π, 2π]
    if (params.windAngleOffset < -2 * M_PI || params.windAngleOffset > 2 * M_PI) {
        return false;
    }

    return true;
}

bool CalibrationManager::loadFromFlash() {
    // Check if file exists
    if (!LittleFS.exists(CALIBRATION_FILE)) {
        // File not found - keep defaults
        return false;
    }

    // Open file for reading
    File file = LittleFS.open(CALIBRATION_FILE, "r");
    if (!file) {
        return false;
    }

    // Parse JSON
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        // JSON parse error - keep defaults
        return false;
    }

    // Extract values
    CalibrationParameters loaded;
    loaded.version = doc["version"] | 1;
    loaded.leewayCalibrationFactor = doc["leewayKFactor"] | DEFAULT_LEEWAY_K_FACTOR;
    loaded.windAngleOffset = doc["windAngleOffset"] | DEFAULT_WIND_ANGLE_OFFSET;
    loaded.lastModified = doc["lastModified"] | 0;
    loaded.valid = true;

    // Validate loaded values
    if (!validateCalibration(loaded)) {
        // Invalid values - keep defaults
        return false;
    }

    // Accept loaded values
    currentParams = loaded;

    return true;
}

bool CalibrationManager::saveToFlash(const CalibrationParameters& params) {
    // Validate first
    if (!validateCalibration(params)) {
        return false;
    }

    // Create JSON document
    StaticJsonDocument<256> doc;
    doc["version"] = params.version;
    doc["leewayKFactor"] = params.leewayCalibrationFactor;
    doc["windAngleOffset"] = params.windAngleOffset;
    doc["lastModified"] = params.lastModified;

    // Open file for writing
    File file = LittleFS.open(CALIBRATION_FILE, "w");
    if (!file) {
        return false;
    }

    // Serialize JSON to file
    size_t bytesWritten = serializeJson(doc, file);
    file.close();

    if (bytesWritten == 0) {
        return false;
    }

    return true;
}
