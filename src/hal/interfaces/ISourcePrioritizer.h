/**
 * @file ISourcePrioritizer.h
 * @brief Abstract interface for multi-source data management
 *
 * This interface defines the contract for managing multiple data sources for
 * GPS and compass sensors. It handles automatic prioritization based on update
 * frequency, manual override, failover, and stale detection.
 *
 * Usage:
 * - Startup: registerSource() for each available source
 * - Data update: updateSourceTimestamp() on each sensor update
 * - Periodic: updatePriorities() to recalculate based on frequency
 * - Periodic: checkStale() to detect failed sources
 * - Query: getActiveSource() to determine which source to use
 * - Manual: setManualOverride() for user-specified source preference
 *
 * @see specs/003-boatdata-feature-as/contracts/README.md lines 56-66
 * @see test/contract/test_isourceprioritizer_contract.cpp
 * @version 1.0.0
 * @date 2025-10-06
 */

#ifndef ISOURCE_PRIORITIZER_H
#define ISOURCE_PRIORITIZER_H

#include "../../types/BoatDataTypes.h"

/**
 * @brief Abstract interface for source prioritization
 *
 * Implementations:
 * - SourcePrioritizer: Production implementation (src/components/SourcePrioritizer.h)
 * - MockSourcePrioritizer: Test mock (in contract test file)
 *
 * Priority algorithm:
 * 1. Manual override (if set) always wins
 * 2. Automatic: highest update frequency (Hz) wins
 * 3. Failover: switch to next-best available source when active becomes stale (>5s)
 */
class ISourcePrioritizer {
public:
    virtual ~ISourcePrioritizer() {}

    /**
     * @brief Register a new data source
     *
     * Registers a source for GPS or compass data. Call during system startup
     * for each available sensor. Maximum 5 sources per sensor type.
     *
     * @param sourceId Human-readable identifier (e.g., "GPS-NMEA2000", "GPS-NMEA0183")
     *                 Maximum 15 characters
     * @param type Sensor type (GPS or COMPASS)
     * @param protocol Protocol type (NMEA0183, NMEA2000, ONEWIRE)
     * @return Source index (0-4 for GPS, 0-4 for compass), or -1 if registration failed
     *
     * @example
     * int gpsA = prioritizer->registerSource("GPS-NMEA0183", SensorType::GPS, ProtocolType::NMEA0183);
     * int gpsB = prioritizer->registerSource("GPS-NMEA2000", SensorType::GPS, ProtocolType::NMEA2000);
     */
    virtual int registerSource(const char* sourceId, SensorType type, ProtocolType protocol) = 0;

    /**
     * @brief Update timestamp for a source
     *
     * Call on each data update from a source. Updates lastUpdateTime, increments
     * updateCount, and marks source as available. Used for frequency calculation
     * and stale detection.
     *
     * @param sourceIndex Index returned from registerSource()
     * @param timestamp Current time in milliseconds (millis())
     *
     * @example
     * // In NMEA GPS message handler
     * prioritizer->updateSourceTimestamp(gpsSourceIndex, millis());
     */
    virtual void updateSourceTimestamp(int sourceIndex, unsigned long timestamp) = 0;

    /**
     * @brief Get active source for a sensor type
     *
     * Returns the index of the currently active (highest priority) source for
     * GPS or compass. If manual override is set, returns the override source.
     * Otherwise, returns the highest-frequency available source.
     *
     * @param type Sensor type (GPS or COMPASS)
     * @return Source index of active source, or -1 if none available
     *
     * @example
     * int activeGPS = prioritizer->getActiveSource(SensorType::GPS);
     * if (activeGPS >= 0) {
     *     // Use data from active GPS source
     * }
     */
    virtual int getActiveSource(SensorType type) = 0;

    /**
     * @brief Set manual override (user forces specific source)
     *
     * Forces a specific source to be active, overriding automatic frequency-based
     * prioritization. Manual override is volatile (resets on reboot).
     *
     * @param sourceIndex Index of source to force active
     *
     * @example
     * // User selects GPS-A via web interface
     * prioritizer->setManualOverride(gpsAIndex);
     */
    virtual void setManualOverride(int sourceIndex) = 0;

    /**
     * @brief Clear manual override (restore automatic prioritization)
     *
     * Removes manual override for a sensor type, restoring automatic frequency-based
     * prioritization.
     *
     * @param type Sensor type to clear override for (GPS or COMPASS)
     *
     * @example
     * // User clicks "Auto" in web interface
     * prioritizer->clearManualOverride(SensorType::GPS);
     */
    virtual void clearManualOverride(SensorType type) = 0;

    /**
     * @brief Check if source is stale (no updates >5 seconds)
     *
     * Determines if a source has stopped sending data. Stale threshold is 5 seconds.
     *
     * @param sourceIndex Index of source to check
     * @param currentTime Current time in milliseconds (millis())
     * @return true if stale (last update >5000ms ago), false if fresh
     *
     * @example
     * if (prioritizer->isSourceStale(gpsAIndex, millis())) {
     *     logger->log("GPS-A is stale");
     * }
     */
    virtual bool isSourceStale(int sourceIndex, unsigned long currentTime) = 0;

    /**
     * @brief Get source information (for diagnostics/web display)
     *
     * Returns the SensorSource structure for a given index. Used for displaying
     * source status in web interface or diagnostics.
     *
     * @param sourceIndex Index of source
     * @return Source structure (copy)
     *
     * @example
     * SensorSource source = prioritizer->getSource(gpsAIndex);
     * Serial.printf("Source: %s, Freq: %.1f Hz, Active: %d\n",
     *               source.sourceId, source.updateFrequency, source.active);
     */
    virtual SensorSource getSource(int sourceIndex) = 0;

    /**
     * @brief Update priorities based on frequency
     *
     * Recalculates update frequencies for all sources and selects the highest-frequency
     * available source as active (unless manual override is set). Call periodically
     * (e.g., every 10 seconds) or after significant updates.
     *
     * Algorithm:
     * 1. Calculate frequency for each source: updateCount / elapsed time
     * 2. Sort by frequency (descending)
     * 3. Select highest-frequency available source (no manual override)
     *
     * @example
     * // In ReactESP event loop (every 10 seconds)
     * app.onRepeat(10000, []() {
     *     prioritizer->updatePriorities();
     * });
     */
    virtual void updatePriorities() = 0;

    /**
     * @brief Check for stale sources and trigger failover
     *
     * Checks all sources for staleness (>5 seconds since last update). Marks stale
     * sources as unavailable and triggers failover to next-best available source if
     * the active source becomes stale.
     *
     * Call periodically (e.g., every second) to detect sensor failures quickly.
     *
     * @param currentTime Current time in milliseconds (millis())
     *
     * @example
     * // In ReactESP event loop (every 1 second)
     * app.onRepeat(1000, []() {
     *     prioritizer->checkStale(millis());
     * });
     */
    virtual void checkStale(unsigned long currentTime) = 0;
};

#endif // ISOURCE_PRIORITIZER_H
