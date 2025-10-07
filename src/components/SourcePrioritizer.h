/**
 * @file SourcePrioritizer.h
 * @brief Multi-source data prioritization and failover management
 *
 * This class implements the ISourcePrioritizer interface, managing multiple
 * data sources for GPS and compass sensors with automatic prioritization based
 * on update frequency, manual override capability, and failover on stale detection.
 *
 * Features:
 * - Automatic priority: highest update frequency wins
 * - Manual override: user can force specific source active (volatile, resets on reboot)
 * - Stale detection: 5-second threshold triggers failover
 * - Frequency calculation: tracks update count and timing
 * - Diagnostics: rejection counts, average update intervals
 *
 * @see specs/003-boatdata-feature-as/data-model.md lines 167-284
 * @see test/contract/test_isourceprioritizer_contract.cpp
 * @version 1.0.0
 * @date 2025-10-06
 */

#ifndef SOURCE_PRIORITIZER_H
#define SOURCE_PRIORITIZER_H

#include "../hal/interfaces/ISourcePrioritizer.h"
#include "../types/BoatDataTypes.h"

/**
 * @brief Source prioritization and failover manager
 *
 * Manages up to 5 GPS sources and 5 compass sources with automatic
 * frequency-based prioritization and failover.
 *
 * Usage:
 * @code
 * SourcePrioritizer prioritizer;
 *
 * // Register sources at startup
 * int gpsA = prioritizer.registerSource("GPS-NMEA0183", SensorType::GPS, ProtocolType::NMEA0183);
 * int gpsB = prioritizer.registerSource("GPS-NMEA2000", SensorType::GPS, ProtocolType::NMEA2000);
 *
 * // Update timestamps on each data reception
 * prioritizer.updateSourceTimestamp(gpsA, millis());
 *
 * // Periodic priority recalculation (every 10 seconds)
 * prioritizer.updatePriorities();
 *
 * // Periodic stale check (every 1 second)
 * prioritizer.checkStale(millis());
 *
 * // Get active source
 * int active = prioritizer.getActiveSource(SensorType::GPS);
 * @endcode
 */
class SourcePrioritizer : public ISourcePrioritizer {
public:
    /**
     * @brief Constructor
     */
    SourcePrioritizer();

    // =========================================================================
    // ISourcePrioritizer IMPLEMENTATION
    // =========================================================================

    int registerSource(const char* sourceId, SensorType type, ProtocolType protocol) override;
    void updateSourceTimestamp(int sourceIndex, unsigned long timestamp) override;
    int getActiveSource(SensorType type) override;
    void setManualOverride(int sourceIndex) override;
    void clearManualOverride(SensorType type) override;
    bool isSourceStale(int sourceIndex, unsigned long currentTime) override;
    SensorSource getSource(int sourceIndex) override;
    void updatePriorities() override;
    void checkStale(unsigned long currentTime) override;

private:
    // Source management data
    SourceManager manager;

    // Stale threshold: 5 seconds
    static const unsigned long STALE_THRESHOLD_MS = 5000;

    /**
     * @brief Get source array and metadata for a sensor type
     *
     * @param type Sensor type (GPS or COMPASS)
     * @param outCount Output: current source count
     * @param outActiveIndex Output: current active source index
     * @return Pointer to source array
     */
    SensorSource* getSourceArray(SensorType type, int& outCount, int& outActiveIndex);

    /**
     * @brief Set active source index for a sensor type
     *
     * @param type Sensor type
     * @param index Source index to set as active
     */
    void setActiveIndex(SensorType type, int index);

    /**
     * @brief Recalculate frequency for a source
     *
     * @param source Pointer to source structure
     * @param currentTime Current time (millis())
     */
    void calculateFrequency(SensorSource* source, unsigned long currentTime);

    /**
     * @brief Select highest priority available source for a sensor type
     *
     * @param type Sensor type
     */
    void selectBestSource(SensorType type);
};

#endif // SOURCE_PRIORITIZER_H
