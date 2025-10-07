/**
 * @file MockBoatDataStore.h
 * @brief Mock implementation of IBoatDataStore for testing
 *
 * Simple in-memory mock that stores data without validation or processing.
 * Used for testing components that depend on IBoatDataStore interface.
 *
 * @see test/contract/test_iboatdatastore_contract.cpp
 * @version 1.0.0
 * @date 2025-10-07
 */

#ifndef MOCK_BOAT_DATA_STORE_H
#define MOCK_BOAT_DATA_STORE_H

#include "../hal/interfaces/IBoatDataStore.h"
#include "../types/BoatDataTypes.h"

/**
 * @brief Mock boat data store for testing
 *
 * Provides simple in-memory storage without validation, prioritization,
 * or side effects. Useful for testing components that read/write boat data.
 *
 * Usage:
 * @code
 * MockBoatDataStore mockStore;
 * GPSData gps = {.latitude = 40.7128, .longitude = -74.0060, .available = true};
 * mockStore.setGPSData(gps);
 * GPSData retrieved = mockStore.getGPSData();
 * @endcode
 */
class MockBoatDataStore : public IBoatDataStore {
public:
    MockBoatDataStore() {
        // Initialize all data to zero/false
        memset(&data, 0, sizeof(BoatDataStructure));
    }

    // =========================================================================
    // IBoatDataStore IMPLEMENTATION
    // =========================================================================

    GPSData getGPSData() override {
        return data.gps;
    }

    void setGPSData(const GPSData& gpsData) override {
        data.gps = gpsData;
    }

    CompassData getCompassData() override {
        return data.compass;
    }

    void setCompassData(const CompassData& compassData) override {
        data.compass = compassData;
    }

    WindData getWindData() override {
        return data.wind;
    }

    void setWindData(const WindData& windData) override {
        data.wind = windData;
    }

    SpeedData getSpeedData() override {
        return data.speed;
    }

    void setSpeedData(const SpeedData& speedData) override {
        data.speed = speedData;
    }

    RudderData getRudderData() override {
        return data.rudder;
    }

    void setRudderData(const RudderData& rudderData) override {
        data.rudder = rudderData;
    }

    DerivedData getDerivedData() override {
        return data.derived;
    }

    void setDerivedData(const DerivedData& derivedData) override {
        data.derived = derivedData;
    }

    CalibrationData getCalibration() override {
        return data.calibration;
    }

    void setCalibration(const CalibrationData& calibrationData) override {
        data.calibration = calibrationData;
    }

    // =========================================================================
    // TEST HELPERS
    // =========================================================================

    /**
     * @brief Reset all data to zero/false
     *
     * Useful for test setup/teardown
     */
    void reset() {
        memset(&data, 0, sizeof(BoatDataStructure));
    }

    /**
     * @brief Get direct access to internal data (for testing only)
     *
     * @return Pointer to internal data structure
     */
    BoatDataStructure* getInternalData() {
        return &data;
    }

private:
    BoatDataStructure data;
};

#endif // MOCK_BOAT_DATA_STORE_H
