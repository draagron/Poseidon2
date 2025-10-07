/**
 * @file MockSourcePrioritizer.h
 * @brief Mock implementation of ISourcePrioritizer for testing
 *
 * Simple in-memory mock that stores source information without complex prioritization.
 * Used for testing components that depend on ISourcePrioritizer interface.
 *
 * @see test/contract/test_isourceprioritizer_contract.cpp
 * @version 1.0.0
 * @date 2025-10-07
 */

#ifndef MOCK_SOURCE_PRIORITIZER_H
#define MOCK_SOURCE_PRIORITIZER_H

#include "../hal/interfaces/ISourcePrioritizer.h"
#include "../types/BoatDataTypes.h"
#include <cstring>

/**
 * @brief Mock source prioritizer for testing
 *
 * Provides simplified source management without frequency calculations.
 * Useful for testing components that register/query sources.
 *
 * Behavior:
 * - registerSource() assigns sequential indices
 * - getActiveSource() returns first registered source (simplified)
 * - Manual overrides are respected
 * - Stale detection uses 5-second threshold
 *
 * Usage:
 * @code
 * MockSourcePrioritizer mockPrio;
 * int gpsIndex = mockPrio.registerSource("GPS-NMEA0183", SensorType::GPS, ProtocolType::NMEA0183);
 * mockPrio.updateSourceTimestamp(gpsIndex, millis());
 * int active = mockPrio.getActiveSource(SensorType::GPS);
 * @endcode
 */
class MockSourcePrioritizer : public ISourcePrioritizer {
public:
    MockSourcePrioritizer() {
        reset();
    }

    // =========================================================================
    // ISourcePrioritizer IMPLEMENTATION
    // =========================================================================

    int registerSource(const char* sourceId, SensorType type, ProtocolType protocol) override {
        if (sourceCount >= MAX_SOURCES) {
            return -1;  // No space
        }

        int index = sourceCount;
        sources[index].type = type;
        sources[index].protocol = protocol;
        strncpy(sources[index].sourceId, sourceId, sizeof(sources[index].sourceId) - 1);
        sources[index].sourceId[sizeof(sources[index].sourceId) - 1] = '\0';
        sources[index].available = false;  // Available after first timestamp update
        sources[index].lastUpdate = 0;
        sources[index].manualOverride = false;

        sourceCount++;
        return index;
    }

    void updateSourceTimestamp(int sourceIndex, unsigned long timestamp) override {
        if (sourceIndex < 0 || sourceIndex >= sourceCount) {
            return;  // Invalid index
        }

        sources[sourceIndex].lastUpdate = timestamp;
        sources[sourceIndex].available = true;

        // Update active source (simplified: first available or manual override)
        updateActiveSource(sources[sourceIndex].type);
    }

    int getActiveSource(SensorType type) override {
        // Check for manual override first
        for (int i = 0; i < sourceCount; i++) {
            if (sources[i].type == type && sources[i].manualOverride && sources[i].available) {
                return i;
            }
        }

        // Return first available source of this type (simplified)
        for (int i = 0; i < sourceCount; i++) {
            if (sources[i].type == type && sources[i].available) {
                return i;
            }
        }

        return -1;  // No active source
    }

    void setManualOverride(int sourceIndex) override {
        if (sourceIndex < 0 || sourceIndex >= sourceCount) {
            return;  // Invalid index
        }

        // Clear all manual overrides for this sensor type
        SensorType type = sources[sourceIndex].type;
        for (int i = 0; i < sourceCount; i++) {
            if (sources[i].type == type) {
                sources[i].manualOverride = false;
            }
        }

        // Set new manual override
        sources[sourceIndex].manualOverride = true;
        updateActiveSource(type);
    }

    void clearManualOverride(SensorType type) override {
        for (int i = 0; i < sourceCount; i++) {
            if (sources[i].type == type) {
                sources[i].manualOverride = false;
            }
        }
        updateActiveSource(type);
    }

    bool isSourceStale(int sourceIndex, unsigned long currentTime) override {
        if (sourceIndex < 0 || sourceIndex >= sourceCount) {
            return true;  // Invalid index is considered stale
        }

        if (sources[sourceIndex].lastUpdate == 0) {
            return true;  // Never updated
        }

        unsigned long age = currentTime - sources[sourceIndex].lastUpdate;
        return age > STALE_THRESHOLD_MS;
    }

    void checkStale(unsigned long currentTime) override {
        for (int i = 0; i < sourceCount; i++) {
            if (isSourceStale(i, currentTime)) {
                if (sources[i].available) {
                    sources[i].available = false;
                    updateActiveSource(sources[i].type);  // Trigger failover
                }
            }
        }
    }

    void updatePriorities() override {
        // Simplified: no frequency calculation in mock
        // Just update active sources based on availability
        for (int type = 0; type < 2; type++) {  // GPS and Compass only
            updateActiveSource(static_cast<SensorType>(type));
        }
    }

    // =========================================================================
    // TEST HELPERS
    // =========================================================================

    /**
     * @brief Reset all source data
     */
    void reset() {
        memset(sources, 0, sizeof(sources));
        sourceCount = 0;
    }

    /**
     * @brief Get number of registered sources
     *
     * @return Source count
     */
    int getSourceCount() const {
        return sourceCount;
    }

    /**
     * @brief Get source information by index
     *
     * @param index Source index
     * @return Source info (or empty if invalid index)
     */
    const char* getSourceId(int index) const {
        if (index < 0 || index >= sourceCount) {
            return "";
        }
        return sources[index].sourceId;
    }

    /**
     * @brief Check if source is available
     *
     * @param index Source index
     * @return true if available, false otherwise
     */
    bool isSourceAvailable(int index) const {
        if (index < 0 || index >= sourceCount) {
            return false;
        }
        return sources[index].available;
    }

    /**
     * @brief Check if source has manual override
     *
     * @param index Source index
     * @return true if manual override active, false otherwise
     */
    bool hasManualOverride(int index) const {
        if (index < 0 || index >= sourceCount) {
            return false;
        }
        return sources[index].manualOverride;
    }

private:
    static const int MAX_SOURCES = 16;
    static const unsigned long STALE_THRESHOLD_MS = 5000;

    struct SourceInfo {
        SensorType type;
        ProtocolType protocol;
        char sourceId[32];
        unsigned long lastUpdate;
        bool available;
        bool manualOverride;
    };

    SourceInfo sources[MAX_SOURCES];
    int sourceCount;

    /**
     * @brief Update active source for a sensor type
     *
     * @param type Sensor type to update
     */
    void updateActiveSource(SensorType type) {
        // Simplified: just ensures consistency
        // Real implementation would update manager.activeGpsSourceIndex etc.
        getActiveSource(type);  // Calls prioritization logic
    }
};

#endif // MOCK_SOURCE_PRIORITIZER_H
