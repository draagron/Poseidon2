/**
 * @file IOneWireSensors.h
 * @brief HAL interface for 1-wire marine sensor devices
 *
 * Hardware abstraction for saildrive, battery monitor, and shore power sensors
 * connected via 1-wire bus on GPIO 4.
 *
 * Constitutional compliance: Principle I (Hardware Abstraction Layer)
 *
 * @see specs/008-enhanced-boatdata-following/data-model.md
 * @version 2.0.0
 * @date 2025-10-10
 */

#ifndef I_ONEWIRE_SENSORS_H
#define I_ONEWIRE_SENSORS_H

#include "types/BoatDataTypes.h"

/**
 * @brief Battery monitor data structure (internal to HAL)
 *
 * Aggregates single battery bank readings for transfer to BatteryData.
 */
struct BatteryMonitorData {
    double voltage;              ///< Volts, range [0, 30]
    double amperage;             ///< Amperes, range [-200, 200], positive = charging
    double stateOfCharge;        ///< Percent, range [0.0, 100.0]
    bool shoreChargerOn;         ///< true = shore charger active
    bool engineChargerOn;        ///< true = alternator charging
    bool available;              ///< true = valid read, false = sensor error
};

/**
 * @brief HAL interface for 1-wire sensor access
 *
 * Abstract interface for reading saildrive, battery, and shore power sensors.
 * Implementations:
 * - ESP32OneWireSensors: Hardware adapter using OneWire library
 * - MockOneWireSensors: Test mock with simulated sensor data
 *
 * Error Handling:
 * - CRC failures: Retry once, then set available=false
 * - Sensor not found: Log WARNING, set available=false
 * - Bus timeout: Skip cycle, set available=false
 *
 * Constitutional Compliance:
 * - Principle I: Hardware abstraction via interface
 * - Principle VII: Graceful degradation (available flags)
 */
class IOneWireSensors {
public:
    virtual ~IOneWireSensors() = default;

    /**
     * @brief Initialize 1-wire bus and enumerate devices
     *
     * Scans bus for expected device addresses and validates presence.
     *
     * @return true if bus initialized successfully, false on error
     */
    virtual bool initialize() = 0;

    /**
     * @brief Read saildrive engagement status
     *
     * Polls digital sensor for saildrive position.
     *
     * @param[out] data SaildriveData structure to populate
     * @return true if read successful, false on sensor error
     */
    virtual bool readSaildriveStatus(SaildriveData& data) = 0;

    /**
     * @brief Read Battery A (house bank) monitor data
     *
     * Polls analog battery monitor for voltage, current, and SOC.
     *
     * @param[out] data BatteryMonitorData structure to populate
     * @return true if read successful, false on sensor error
     */
    virtual bool readBatteryA(BatteryMonitorData& data) = 0;

    /**
     * @brief Read Battery B (starter bank) monitor data
     *
     * Polls analog battery monitor for voltage, current, and SOC.
     *
     * @param[out] data BatteryMonitorData structure to populate
     * @return true if read successful, false on sensor error
     */
    virtual bool readBatteryB(BatteryMonitorData& data) = 0;

    /**
     * @brief Read shore power connection and consumption
     *
     * Polls digital sensor (connection status) and analog sensor (power draw).
     *
     * @param[out] data ShorePowerData structure to populate
     * @return true if read successful, false on sensor error
     */
    virtual bool readShorePower(ShorePowerData& data) = 0;

    /**
     * @brief Check if 1-wire bus is operational
     *
     * Quick health check for bus communication.
     *
     * @return true if bus responding, false if bus failure
     */
    virtual bool isBusHealthy() = 0;
};

#endif // I_ONEWIRE_SENSORS_H
