/**
 * @file IBoatDataStore.h
 * @brief Abstract interface for boat data storage access
 *
 * This interface defines the contract for accessing all boat sensor data,
 * derived parameters, calibration, and diagnostics. It allows mocking of
 * BoatData for unit tests and provides a clean separation between data
 * storage and business logic.
 *
 * Usage:
 * - Read access: Components can retrieve current sensor data
 * - Write access: Calculation engine can update derived parameters
 * - Mocking: Tests can use MockBoatDataStore for isolated testing
 *
 * @see specs/003-boatdata-feature-as/contracts/README.md lines 11-29
 * @see test/contract/test_iboatdatastore_contract.cpp
 * @version 1.0.0
 * @date 2025-10-06
 */

#ifndef IBOAT_DATA_STORE_H
#define IBOAT_DATA_STORE_H

#include "../../types/BoatDataTypes.h"

/**
 * @brief Abstract interface for boat data storage
 *
 * Implementations:
 * - BoatData: Production implementation (src/components/BoatData.h)
 * - MockBoatDataStore: Test mock (src/mocks/MockBoatDataStore.h)
 */
class IBoatDataStore {
public:
    virtual ~IBoatDataStore() {}

    // =========================================================================
    // GPS DATA ACCESS
    // =========================================================================

    /**
     * @brief Get current GPS data
     * @return GPS data structure (latitude, longitude, COG, SOG)
     */
    virtual GPSData getGPSData() = 0;

    /**
     * @brief Set GPS data
     * @param data GPS data to store
     */
    virtual void setGPSData(const GPSData& data) = 0;

    // =========================================================================
    // COMPASS DATA ACCESS
    // =========================================================================

    /**
     * @brief Get current compass data
     * @return Compass data structure (true heading, magnetic heading, variation)
     */
    virtual CompassData getCompassData() = 0;

    /**
     * @brief Set compass data
     * @param data Compass data to store
     */
    virtual void setCompassData(const CompassData& data) = 0;

    // =========================================================================
    // WIND DATA ACCESS
    // =========================================================================

    /**
     * @brief Get current wind data
     * @return Wind data structure (apparent wind angle/speed)
     */
    virtual WindData getWindData() = 0;

    /**
     * @brief Set wind data
     * @param data Wind data to store
     */
    virtual void setWindData(const WindData& data) = 0;

    // =========================================================================
    // SPEED DATA ACCESS
    // =========================================================================

    /**
     * @brief Get current speed/heel data
     * @return Speed data structure (heel angle, measured boat speed)
     */
    virtual SpeedData getSpeedData() = 0;

    /**
     * @brief Set speed data
     * @param data Speed data to store
     */
    virtual void setSpeedData(const SpeedData& data) = 0;

    // =========================================================================
    // RUDDER DATA ACCESS
    // =========================================================================

    /**
     * @brief Get current rudder data
     * @return Rudder data structure (steering angle)
     */
    virtual RudderData getRudderData() = 0;

    /**
     * @brief Set rudder data
     * @param data Rudder data to store
     */
    virtual void setRudderData(const RudderData& data) = 0;

    // =========================================================================
    // DERIVED DATA ACCESS
    // =========================================================================

    /**
     * @brief Get current derived/calculated parameters
     * @return Derived data structure (TWS, TWA, VMG, etc.)
     */
    virtual DerivedData getDerivedData() = 0;

    /**
     * @brief Set derived data (typically called by CalculationEngine)
     * @param data Derived data to store
     */
    virtual void setDerivedData(const DerivedData& data) = 0;

    // =========================================================================
    // CALIBRATION DATA ACCESS
    // =========================================================================

    /**
     * @brief Get current calibration parameters
     * @return Calibration data structure (K factor, wind offset)
     */
    virtual CalibrationData getCalibration() = 0;

    /**
     * @brief Set calibration data
     * @param data Calibration data to store
     */
    virtual void setCalibration(const CalibrationData& data) = 0;
};

#endif // IBOAT_DATA_STORE_H
