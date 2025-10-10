/**
 * @file ESP32OneWireSensors.h
 * @brief ESP32 hardware implementation of 1-wire sensor HAL interface
 *
 * Hardware-specific adapter for 1-wire marine sensors (saildrive, battery, shore power)
 * using OneWire and DallasTemperature libraries on GPIO 4.
 *
 * Features:
 * - Sequential polling with 50ms spacing to avoid bus contention
 * - CRC validation with single retry on failure
 * - Device enumeration during initialization
 * - Graceful degradation on sensor failures
 *
 * Constitutional compliance: Principle I (HAL implementation)
 *
 * @see specs/008-enhanced-boatdata-following/research.md lines 208-259
 * @see src/hal/interfaces/IOneWireSensors.h
 * @version 2.0.0
 * @date 2025-10-10
 */

#ifndef ESP32_ONEWIRE_SENSORS_H
#define ESP32_ONEWIRE_SENSORS_H

#include "../interfaces/IOneWireSensors.h"
#include <OneWire.h>
#include <Arduino.h>

/**
 * @brief 1-wire device address structure
 *
 * 64-bit ROM address for DS2438/DS18B20 devices
 */
struct DeviceAddress {
    uint8_t addr[8];
};

/**
 * @brief ESP32 hardware implementation of 1-wire sensor interface
 *
 * Usage:
 * @code
 * ESP32OneWireSensors sensors;
 * if (sensors.initialize()) {
 *     SaildriveData saildrive;
 *     if (sensors.readSaildriveStatus(saildrive)) {
 *         Serial.printf("Saildrive: %s\n", saildrive.saildriveEngaged ? "engaged" : "disengaged");
 *     }
 * }
 * @endcode
 *
 * Memory footprint: ~200 bytes (OneWire bus + device addresses)
 */
class ESP32OneWireSensors : public IOneWireSensors {
public:
    /**
     * @brief Constructor
     *
     * @param pin GPIO pin for 1-wire bus (default: GPIO 4)
     */
    ESP32OneWireSensors(uint8_t pin = 4);

    /**
     * @brief Destructor
     */
    ~ESP32OneWireSensors() override;

    // IOneWireSensors interface implementation
    bool initialize() override;
    bool readSaildriveStatus(SaildriveData& data) override;
    bool readBatteryA(BatteryMonitorData& data) override;
    bool readBatteryB(BatteryMonitorData& data) override;
    bool readShorePower(ShorePowerData& data) override;
    bool isBusHealthy() override;

private:
    OneWire* oneWire;              ///< OneWire bus instance
    uint8_t busPin;                ///< GPIO pin number
    bool initialized;              ///< Initialization state flag

    // Device addresses (populated during initialization)
    DeviceAddress saildriveAddr;   ///< Saildrive sensor address
    DeviceAddress batteryAAddr;    ///< Battery A monitor address
    DeviceAddress batteryBAddr;    ///< Battery B monitor address
    DeviceAddress shorePowerAddr;  ///< Shore power sensor address

    // Device presence flags
    bool hasSaildriveSensor;       ///< true if saildrive sensor found
    bool hasBatteryASensor;        ///< true if battery A monitor found
    bool hasBatteryBSensor;        ///< true if battery B monitor found
    bool hasShorePowerSensor;      ///< true if shore power sensor found

    /**
     * @brief Enumerate all devices on the 1-wire bus
     *
     * Scans bus for expected device family codes and stores addresses.
     * Logs warnings for missing expected devices.
     *
     * @return Number of devices found (0-4 expected)
     */
    int enumerateDevices();

    /**
     * @brief Read digital sensor state (saildrive, shore power connection)
     *
     * Reads a single bit from a DS2438 device indicating on/off state.
     *
     * @param addr Device address
     * @param[out] state Digital state (true/false)
     * @return true if read successful (with valid CRC), false on error
     */
    bool readDigitalSensor(const DeviceAddress& addr, bool& state);

    /**
     * @brief Read analog battery monitor data from DS2438
     *
     * Reads voltage, current, and calculated SOC from battery monitor IC.
     * Implements single retry on CRC failure.
     *
     * @param addr Device address
     * @param[out] data Battery monitor data structure
     * @return true if read successful, false on sensor error
     */
    bool readBatteryMonitor(const DeviceAddress& addr, BatteryMonitorData& data);

    /**
     * @brief Read shore power consumption from analog sensor
     *
     * Reads power draw in watts from analog current sensor.
     *
     * @param addr Device address
     * @param[out] power Power consumption in watts
     * @return true if read successful, false on error
     */
    bool readPowerConsumption(const DeviceAddress& addr, double& power);

    /**
     * @brief Validate CRC for 1-wire data buffer
     *
     * @param data Data buffer
     * @param len Buffer length (including CRC byte)
     * @return true if CRC valid, false if CRC mismatch
     */
    bool validateCRC(const uint8_t* data, uint8_t len);

    /**
     * @brief Copy device address
     *
     * @param dest Destination address
     * @param src Source address
     */
    void copyAddress(DeviceAddress& dest, const uint8_t* src);

    /**
     * @brief Check if device address is valid (not all zeros)
     *
     * @param addr Device address to check
     * @return true if address is valid, false if uninitialized
     */
    bool isValidAddress(const DeviceAddress& addr);
};

#endif // ESP32_ONEWIRE_SENSORS_H
