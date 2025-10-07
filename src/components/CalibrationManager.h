/**
 * @file CalibrationManager.h
 * @brief Calibration parameter management with LittleFS persistence
 *
 * This class implements the ICalibration interface, managing calibration
 * parameters (leeway K factor, wind angle offset) with persistence to flash
 * filesystem using LittleFS and ArduinoJson.
 *
 * Features:
 * - Load from /calibration.json on LittleFS
 * - Save to /calibration.json with validation
 * - Default values if file missing (K=1.0, offset=0.0)
 * - Validation: K > 0, wind offset [-2π, 2π]
 * - Atomic updates (via ReactESP deferred callback)
 *
 * @see specs/003-boatdata-feature-as/data-model.md lines 287-356
 * @see test/contract/test_icalibration_contract.cpp
 * @version 1.0.0
 * @date 2025-10-06
 */

#ifndef CALIBRATION_MANAGER_H
#define CALIBRATION_MANAGER_H

#include "../hal/interfaces/ICalibration.h"
#include "../types/BoatDataTypes.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

/**
 * @brief Calibration parameter manager with persistence
 *
 * Manages calibration parameters with JSON persistence to LittleFS.
 *
 * File format (/calibration.json):
 * {
 *   "version": 1,
 *   "leewayKFactor": 0.65,
 *   "windAngleOffset": 0.087,
 *   "lastModified": 1696608000
 * }
 *
 * Usage:
 * @code
 * CalibrationManager calibMgr;
 *
 * // Load from flash at startup
 * if (calibMgr.loadFromFlash()) {
 *     Serial.println("Calibration loaded");
 * } else {
 *     Serial.println("Using defaults");
 * }
 *
 * // Update calibration
 * CalibrationParameters params;
 * params.leewayCalibrationFactor = 0.75;
 * params.windAngleOffset = 0.087;
 * params.version = 1;
 * params.lastModified = time(nullptr);
 * params.valid = true;
 *
 * if (calibMgr.setCalibration(params)) {
 *     calibMgr.saveToFlash(params);
 * }
 * @endcode
 */
class CalibrationManager : public ICalibration {
public:
    /**
     * @brief Constructor
     */
    CalibrationManager();

    // =========================================================================
    // ICalibration IMPLEMENTATION
    // =========================================================================

    CalibrationParameters getCalibration() override;
    bool setCalibration(const CalibrationParameters& params) override;
    bool validateCalibration(const CalibrationParameters& params) override;
    bool loadFromFlash() override;
    bool saveToFlash(const CalibrationParameters& params) override;

private:
    // Current calibration parameters (in memory)
    CalibrationParameters currentParams;

    // Calibration file path on LittleFS
    static const char* CALIBRATION_FILE;
};

#endif // CALIBRATION_MANAGER_H
