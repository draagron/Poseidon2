/**
 * @file DeviceInfoCollector.h
 * @brief NMEA2000 device discovery and metadata collection component
 *
 * Polls tN2kDeviceList every 5 seconds to extract device metadata and
 * enrich MessageSource entries in SourceRegistry.
 *
 * @see specs/013-r013-nmea2000-device/contracts/DeviceInfoCollectorContract.md
 * @version 1.0.0
 * @date 2025-10-13
 */

#ifndef DEVICE_INFO_COLLECTOR_H
#define DEVICE_INFO_COLLECTOR_H

#include <Arduino.h>
#include <ReactESP.h>
#include <N2kDeviceList.h>
#include "SourceRegistry.h"
#include "SourceStatistics.h"
#include "utils/WebSocketLogger.h"
#include "utils/ManufacturerLookup.h"

using namespace reactesp;

/**
 * @brief Device discovery and metadata collection component
 *
 * Responsibilities:
 * - Poll tN2kDeviceList every 5 seconds via ReactESP timer
 * - Extract device metadata from tDevice objects
 * - Correlate devices with MessageSource entries by SID
 * - Update SourceRegistry with device metadata
 * - Send WebSocket notifications for discovery events
 * - Handle discovery timeout (60 seconds) for non-compliant devices
 *
 * Performance: <10ms per poll cycle with 20 devices
 * Memory: ~500 bytes stack usage (no heap allocation)
 */
class DeviceInfoCollector {
public:
    /**
     * @brief Create DeviceInfoCollector with dependencies
     *
     * @param deviceList Pointer to tN2kDeviceList instance (must outlive this object)
     * @param registry Pointer to SourceRegistry instance (must outlive this object)
     * @param logger Optional WebSocketLogger for diagnostic events
     *
     * @pre deviceList != nullptr
     * @pre registry != nullptr
     * @post Collector ready to poll, but not started (call init() to start)
     */
    DeviceInfoCollector(tN2kDeviceList* deviceList,
                        SourceRegistry* registry,
                        WebSocketLogger* logger = nullptr);

    /**
     * @brief Initialize and start periodic polling
     *
     * Sets up ReactESP timer callback for 5-second polling interval.
     *
     * @param app ReactESP application instance
     *
     * @post ReactESP timer registered, polling active
     * @post First poll occurs after 5 seconds
     */
    void init(ReactESP& app);

    /**
     * @brief Manually trigger device list poll
     *
     * Checks for device list updates and processes discovered devices.
     * Intended for testing and forced updates.
     *
     * @return Number of sources updated with new device metadata
     *
     * @post SourceRegistry deviceInfo fields updated for discovered devices
     * @post WebSocket log messages sent for discoveries/updates
     */
    uint8_t pollDeviceList();

    /**
     * @brief Get total number of discovered devices
     *
     * @return Count of MessageSource entries with deviceInfo.hasInfo == true
     */
    uint8_t getDiscoveredCount() const { return discoveredCount_; }

    /**
     * @brief Get last poll timestamp
     *
     * @return millis() timestamp of last pollDeviceList() call
     */
    unsigned long getLastPollTime() const { return lastPollTime_; }

private:
    // === Dependencies ===
    tN2kDeviceList* deviceList_;        ///< NMEA2000 device list instance
    SourceRegistry* registry_;          ///< Source registry instance
    WebSocketLogger* logger_;           ///< Optional logger for diagnostics

    // === State ===
    uint8_t discoveredCount_;           ///< Count of discovered devices
    unsigned long lastPollTime_;        ///< millis() timestamp of last poll

    /**
     * @brief Extract device metadata from tDevice and update MessageSource
     *
     * @param device Pointer to tDevice from NMEA2000 library
     * @param sourceId Source identifier (e.g., "NMEA2000-42")
     *
     * @post DeviceInfo struct populated with metadata from tDevice
     * @post SourceRegistry updated via updateDeviceInfo()
     * @post WebSocket log sent for DEVICE_DISCOVERED or DEVICE_UPDATED
     */
    void extractAndUpdateDeviceMetadata(const tNMEA2000::tDevice* device, const char* sourceId);

    /**
     * @brief Check all NMEA2000 sources for discovery timeouts
     *
     * Iterates sources, marks those without device info after 60 seconds as "Unknown (timeout)".
     *
     * @post Sources timed out have deviceInfo updated with timeout status
     * @post WebSocket DEVICE_TIMEOUT logs sent
     */
    void checkDiscoveryTimeouts();
};

#endif // DEVICE_INFO_COLLECTOR_H
