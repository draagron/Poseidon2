/**
 * @file ICalibration.h
 * @brief Abstract interface for calibration parameter management
 *
 * This interface defines the contract for accessing and persisting calibration
 * parameters (leeway K factor, wind angle offset). It allows the calibration
 * web server to update parameters and the calculation engine to read them.
 *
 * Usage:
 * - CalibrationWebServer: setCalibration(), saveToFlash()
 * - CalculationEngine: getCalibration()
 * - System startup: loadFromFlash()
 *
 * @see specs/003-boatdata-feature-as/contracts/README.md lines 44-54
 * @see test/contract/test_icalibration_contract.cpp
 * @version 1.0.0
 * @date 2025-10-06
 */

#ifndef ICALIBRATION_H
#define ICALIBRATION_H

#include "../../types/BoatDataTypes.h"

/**
 * @brief Abstract interface for calibration management
 *
 * Implementations:
 * - CalibrationManager: Production implementation with LittleFS persistence (src/components/CalibrationManager.h)
 * - MockCalibrationManager: Test mock (in contract test file)
 *
 * Persistence format: /calibration.json on LittleFS filesystem
 * {"leewayKFactor":0.65,"windAngleOffset":0.087}
 */
class ICalibration {
public:
    virtual ~ICalibration() {}

    /**
     * @brief Get current calibration parameters
     *
     * Returns the active calibration parameters used by the calculation engine.
     * If not loaded from flash, returns default values (K=1.0, offset=0.0).
     *
     * @return Current calibration parameters
     *
     * @example
     * CalibrationParameters params = calibMgr->getCalibration();
     * double K = params.leewayCalibrationFactor;
     */
    virtual CalibrationParameters getCalibration() = 0;

    /**
     * @brief Set calibration parameters (in memory)
     *
     * Updates calibration parameters in memory. Does NOT persist to flash
     * (use saveToFlash() for persistence). Validates parameters before
     * accepting.
     *
     * Validation rules:
     * - K factor must be > 0
     * - Wind angle offset must be in range [-2π, 2π]
     *
     * @param params New calibration parameters
     * @return true if valid and set, false if validation failed
     *
     * @example
     * CalibrationParameters params;
     * params.leewayCalibrationFactor = 0.65;
     * params.windAngleOffset = 0.087;  // 5° masthead offset
     * if (calibMgr->setCalibration(params)) {
     *     // Calibration updated successfully
     * }
     */
    virtual bool setCalibration(const CalibrationParameters& params) = 0;

    /**
     * @brief Validate calibration parameters
     *
     * Checks if calibration parameters are within acceptable ranges.
     * Called internally by setCalibration() and saveToFlash().
     *
     * Validation rules:
     * - leewayCalibrationFactor > 0.0 (must be positive)
     * - windAngleOffset in range [-2π, 2π]
     *
     * @param params Parameters to validate
     * @return true if valid, false if invalid
     *
     * @example
     * CalibrationParameters params;
     * params.leewayCalibrationFactor = -0.5;  // Invalid: K must be > 0
     * params.windAngleOffset = 0.087;
     * bool valid = calibMgr->validateCalibration(params);  // Returns false
     */
    virtual bool validateCalibration(const CalibrationParameters& params) = 0;

    /**
     * @brief Load calibration from flash filesystem
     *
     * Attempts to load calibration from /calibration.json on LittleFS.
     * If file missing or invalid, keeps current values (defaults).
     *
     * Should be called during system startup after LittleFS is mounted.
     *
     * @return true if loaded successfully, false if file missing or invalid
     *
     * @example
     * // During system startup
     * LittleFS.begin();
     * if (calibMgr->loadFromFlash()) {
     *     logger->log("Calibration loaded from flash");
     * } else {
     *     logger->log("Using default calibration");
     * }
     */
    virtual bool loadFromFlash() = 0;

    /**
     * @brief Save calibration to flash filesystem
     *
     * Persists calibration parameters to /calibration.json on LittleFS.
     * Validates parameters before saving. Calibration survives system reboots.
     *
     * @param params Parameters to persist
     * @return true if saved successfully, false if validation failed or filesystem error
     *
     * @example
     * // After user updates calibration via web interface
     * CalibrationParameters params = calibMgr->getCalibration();
     * params.leewayCalibrationFactor = 0.75;
     * if (calibMgr->saveToFlash(params)) {
     *     logger->log("Calibration saved");
     * }
     */
    virtual bool saveToFlash(const CalibrationParameters& params) = 0;
};

#endif // ICALIBRATION_H
