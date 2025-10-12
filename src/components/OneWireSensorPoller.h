/**
 * @file OneWireSensorPoller.h
 * @brief 1-Wire sensor polling logic for marine sensors
 *
 * Encapsulates periodic polling of:
 * - Saildrive engagement status
 * - Dual battery bank monitoring (A/B)
 * - Shore power connection and draw
 *
 * Designed to be called from ReactESP event loops.
 */

#ifndef ONEWIRE_SENSOR_POLLER_H
#define ONEWIRE_SENSOR_POLLER_H

#include "hal/implementations/ESP32OneWireSensors.h"
#include "components/BoatData.h"
#include "utils/WebSocketLogger.h"

/**
 * @class OneWireSensorPoller
 * @brief Handles periodic polling of 1-Wire marine sensors
 *
 * Usage:
 * @code
 * OneWireSensorPoller* poller = new OneWireSensorPoller(sensors, boatData, &logger);
 * app.onRepeat(1000, [&]() { poller->pollSaildriveData(); });
 * app.onRepeat(2000, [&]() { poller->pollBatteryData(); });
 * app.onRepeat(2000, [&]() { poller->pollShorePowerData(); });
 * @endcode
 */
class OneWireSensorPoller {
public:
    /**
     * @brief Constructor
     * @param sensors ESP32OneWireSensors instance for hardware access
     * @param data BoatData instance for storing sensor readings
     * @param log WebSocketLogger instance for diagnostics
     */
    OneWireSensorPoller(ESP32OneWireSensors* sensors, BoatData* data, WebSocketLogger* log);

    /**
     * @brief Poll saildrive engagement status (1 Hz recommended)
     *
     * Reads saildrive sensor and updates BoatData.
     * Logs DEBUG on success, WARN on failure.
     */
    void pollSaildriveData();

    /**
     * @brief Poll dual battery bank monitoring (0.5 Hz recommended)
     *
     * Reads battery A (house) and battery B (starter) sensors.
     * Updates BoatData with voltage, amperage, SOC, and charger status.
     * Logs DEBUG on success, WARN on failure.
     */
    void pollBatteryData();

    /**
     * @brief Poll shore power connection status (0.5 Hz recommended)
     *
     * Reads shore power sensor and updates BoatData.
     * Logs DEBUG on success, WARN on failure.
     */
    void pollShorePowerData();

private:
    ESP32OneWireSensors* oneWireSensors;
    BoatData* boatData;
    WebSocketLogger* logger;
};

#endif // ONEWIRE_SENSOR_POLLER_H
