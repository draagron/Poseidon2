/**
 * @file MockOneWireSensors.h
 * @brief Mock implementation of IOneWireSensors for unit/integration tests
 *
 * Provides test-controllable mock of 1-wire sensor adapter. Simulates sensor
 * readings with configurable values for testing without physical hardware.
 *
 * Usage in tests:
 * @code
 * MockOneWireSensors mockSensors;
 * mockSensors.setSaildriveEngaged(true);
 * SaildriveData data;
 * bool result = mockSensors.readSaildriveStatus(data);
 * TEST_ASSERT_TRUE(result);
 * TEST_ASSERT_TRUE(data.saildriveEngaged);
 * @endcode
 *
 * @version 2.0.0
 * @date 2025-10-10
 */

#ifndef MOCK_ONEWIRE_SENSORS_H
#define MOCK_ONEWIRE_SENSORS_H

#include "hal/interfaces/IOneWireSensors.h"

/**
 * @brief Mock 1-wire sensor adapter for testing
 *
 * Simulates sensor readings by returning pre-configured values.
 * No actual 1-wire communication occurs, enabling fast native platform tests.
 */
class MockOneWireSensors : public IOneWireSensors {
private:
    // Simulated sensor states
    bool _busHealthy;
    bool _initResult;
    bool _saildriveEngaged;
    BatteryMonitorData _batteryA;
    BatteryMonitorData _batteryB;
    ShorePowerData _shorePower;

    // Call tracking for verification
    bool _initCalled;
    int _saildriveReadCount;
    int _batteryAReadCount;
    int _batteryBReadCount;
    int _shorePowerReadCount;

public:
    /**
     * @brief Constructor - initialize mock state
     */
    MockOneWireSensors()
        : _busHealthy(true),
          _initResult(true),
          _saildriveEngaged(false),
          _initCalled(false),
          _saildriveReadCount(0),
          _batteryAReadCount(0),
          _batteryBReadCount(0),
          _shorePowerReadCount(0) {
        // Default battery A values
        _batteryA.voltage = 12.6;
        _batteryA.amperage = 0.0;
        _batteryA.stateOfCharge = 85.0;
        _batteryA.shoreChargerOn = false;
        _batteryA.engineChargerOn = false;
        _batteryA.available = true;

        // Default battery B values
        _batteryB.voltage = 12.4;
        _batteryB.amperage = 0.0;
        _batteryB.stateOfCharge = 80.0;
        _batteryB.shoreChargerOn = false;
        _batteryB.engineChargerOn = false;
        _batteryB.available = true;

        // Default shore power values
        _shorePower.shorePowerOn = false;
        _shorePower.power = 0.0;
        _shorePower.available = true;
        _shorePower.lastUpdate = 0;
    }

    /**
     * @brief Reset mock state
     *
     * Clears all tracked state and resets sensors to default values.
     */
    void reset() {
        _busHealthy = true;
        _initResult = true;
        _saildriveEngaged = false;
        _initCalled = false;
        _saildriveReadCount = 0;
        _batteryAReadCount = 0;
        _batteryBReadCount = 0;
        _shorePowerReadCount = 0;

        _batteryA.voltage = 12.6;
        _batteryA.amperage = 0.0;
        _batteryA.stateOfCharge = 85.0;
        _batteryA.shoreChargerOn = false;
        _batteryA.engineChargerOn = false;
        _batteryA.available = true;

        _batteryB.voltage = 12.4;
        _batteryB.amperage = 0.0;
        _batteryB.stateOfCharge = 80.0;
        _batteryB.shoreChargerOn = false;
        _batteryB.engineChargerOn = false;
        _batteryB.available = true;

        _shorePower.shorePowerOn = false;
        _shorePower.power = 0.0;
        _shorePower.available = true;
    }

    // Configuration methods for tests

    /**
     * @brief Set bus health status
     * @param healthy true = bus operational, false = bus failure
     */
    void setBusHealthy(bool healthy) {
        _busHealthy = healthy;
    }

    /**
     * @brief Set initialize() return value
     * @param result true = init succeeds, false = init fails
     */
    void setInitResult(bool result) {
        _initResult = result;
    }

    /**
     * @brief Set saildrive engagement status
     * @param engaged true = deployed/engaged, false = retracted/disengaged
     */
    void setSaildriveEngaged(bool engaged) {
        _saildriveEngaged = engaged;
    }

    /**
     * @brief Set Battery A monitor data
     * @param data Battery monitor data to return on next read
     */
    void setBatteryA(const BatteryMonitorData& data) {
        _batteryA = data;
    }

    /**
     * @brief Set Battery B monitor data
     * @param data Battery monitor data to return on next read
     */
    void setBatteryB(const BatteryMonitorData& data) {
        _batteryB = data;
    }

    /**
     * @brief Set shore power data
     * @param data Shore power data to return on next read
     */
    void setShorePower(const ShorePowerData& data) {
        _shorePower = data;
    }

    // Query methods for test verification

    /**
     * @brief Check if initialize() was called
     * @return true if initialize() was called
     */
    bool wasInitCalled() const {
        return _initCalled;
    }

    /**
     * @brief Get saildrive read count
     * @return Number of times readSaildriveStatus() was called
     */
    int getSaildriveReadCount() const {
        return _saildriveReadCount;
    }

    /**
     * @brief Get Battery A read count
     * @return Number of times readBatteryA() was called
     */
    int getBatteryAReadCount() const {
        return _batteryAReadCount;
    }

    /**
     * @brief Get Battery B read count
     * @return Number of times readBatteryB() was called
     */
    int getBatteryBReadCount() const {
        return _batteryBReadCount;
    }

    /**
     * @brief Get shore power read count
     * @return Number of times readShorePower() was called
     */
    int getShorePowerReadCount() const {
        return _shorePowerReadCount;
    }

    // IOneWireSensors interface implementation

    bool initialize() override {
        _initCalled = true;
        return _initResult;
    }

    bool readSaildriveStatus(SaildriveData& data) override {
        _saildriveReadCount++;
        if (!_busHealthy) {
            data.available = false;
            return false;
        }

        data.saildriveEngaged = _saildriveEngaged;
        data.available = true;
        data.lastUpdate = 0;  // millis() not available in native tests
        return true;
    }

    bool readBatteryA(BatteryMonitorData& data) override {
        _batteryAReadCount++;
        if (!_busHealthy) {
            data.available = false;
            return false;
        }

        data = _batteryA;
        return data.available;
    }

    bool readBatteryB(BatteryMonitorData& data) override {
        _batteryBReadCount++;
        if (!_busHealthy) {
            data.available = false;
            return false;
        }

        data = _batteryB;
        return data.available;
    }

    bool readShorePower(ShorePowerData& data) override {
        _shorePowerReadCount++;
        if (!_busHealthy) {
            data.available = false;
            return false;
        }

        data = _shorePower;
        return data.available;
    }

    bool isBusHealthy() override {
        return _busHealthy;
    }
};

#endif // MOCK_ONEWIRE_SENSORS_H
