/**
 * @file MockCalibration.h
 * @brief Mock implementation of ICalibration for testing
 *
 * Simple in-memory mock that stores calibration without flash persistence.
 * Used for testing components that depend on ICalibration interface.
 *
 * @see test/contract/test_icalibration_contract.cpp
 * @version 1.0.0
 * @date 2025-10-07
 */

#ifndef MOCK_CALIBRATION_H
#define MOCK_CALIBRATION_H

#include "../hal/interfaces/ICalibration.h"
#include "../types/BoatDataTypes.h"

/**
 * @brief Mock calibration manager for testing
 *
 * Provides in-memory calibration storage with validation but no flash persistence.
 * Useful for testing components that read/write calibration parameters.
 *
 * Usage:
 * @code
 * MockCalibration mockCal;
 * CalibrationParameters params = {
 *     .version = 1,
 *     .leewayCalibrationFactor = 1.2,
 *     .windAngleOffset = 0.05
 * };
 * bool success = mockCal.setCalibration(params);
 * CalibrationParameters retrieved = mockCal.getCalibration();
 * @endcode
 */
class MockCalibration : public ICalibration {
public:
    MockCalibration() {
        // Initialize to defaults
        currentParams.version = 1;
        currentParams.leewayCalibrationFactor = DEFAULT_LEEWAY_K_FACTOR;
        currentParams.windAngleOffset = DEFAULT_WIND_ANGLE_OFFSET;

        flashLoadSuccess = true;
        flashSaveSuccess = true;
        flashLoadCallCount = 0;
        flashSaveCallCount = 0;
    }

    // =========================================================================
    // ICalibration IMPLEMENTATION
    // =========================================================================

    CalibrationParameters getCalibration() override {
        return currentParams;
    }

    bool setCalibration(const CalibrationParameters& params) override {
        if (!validateCalibration(params)) {
            return false;
        }
        currentParams = params;
        return true;
    }

    bool validateCalibration(const CalibrationParameters& params) override {
        // K factor must be positive
        if (params.leewayCalibrationFactor <= 0.0) {
            return false;
        }

        // Wind angle offset must be within [-2π, 2π]
        if (params.windAngleOffset < -2 * M_PI || params.windAngleOffset > 2 * M_PI) {
            return false;
        }

        return true;
    }

    bool loadFromFlash() override {
        flashLoadCallCount++;
        return flashLoadSuccess;
    }

    bool saveToFlash(const CalibrationParameters& params) override {
        flashSaveCallCount++;

        if (!flashSaveSuccess) {
            return false;
        }

        if (!validateCalibration(params)) {
            return false;
        }

        currentParams = params;
        return true;
    }

    // =========================================================================
    // TEST HELPERS
    // =========================================================================

    /**
     * @brief Reset calibration to defaults
     */
    void reset() {
        currentParams.version = 1;
        currentParams.leewayCalibrationFactor = DEFAULT_LEEWAY_K_FACTOR;
        currentParams.windAngleOffset = DEFAULT_WIND_ANGLE_OFFSET;
        flashLoadCallCount = 0;
        flashSaveCallCount = 0;
        flashLoadSuccess = true;
        flashSaveSuccess = true;
    }

    /**
     * @brief Simulate flash load failure
     *
     * @param shouldFail true to make loadFromFlash() return false
     */
    void setFlashLoadFailure(bool shouldFail) {
        flashLoadSuccess = !shouldFail;
    }

    /**
     * @brief Simulate flash save failure
     *
     * @param shouldFail true to make saveToFlash() return false
     */
    void setFlashSaveFailure(bool shouldFail) {
        flashSaveSuccess = !shouldFail;
    }

    /**
     * @brief Get number of times loadFromFlash() was called
     *
     * @return Call count
     */
    int getFlashLoadCallCount() const {
        return flashLoadCallCount;
    }

    /**
     * @brief Get number of times saveToFlash() was called
     *
     * @return Call count
     */
    int getFlashSaveCallCount() const {
        return flashSaveCallCount;
    }

private:
    CalibrationParameters currentParams;
    bool flashLoadSuccess;
    bool flashSaveSuccess;
    int flashLoadCallCount;
    int flashSaveCallCount;
};

#endif // MOCK_CALIBRATION_H
