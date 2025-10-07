/**
 * @file SourcePrioritizer.cpp
 * @brief Implementation of multi-source prioritization
 *
 * @see SourcePrioritizer.h for detailed documentation
 */

#include "SourcePrioritizer.h"

SourcePrioritizer::SourcePrioritizer() {
    // Initialize manager structure
    memset(&manager, 0, sizeof(SourceManager));
    manager.activeGpsSourceIndex = -1;
    manager.activeCompassSourceIndex = -1;
    manager.gpsSourceCount = 0;
    manager.compassSourceCount = 0;
    manager.lastPriorityUpdate = 0;
}

// =============================================================================
// ISourcePrioritizer IMPLEMENTATION
// =============================================================================

int SourcePrioritizer::registerSource(const char* sourceId, SensorType type, ProtocolType protocol) {
    int count, activeIndex;
    SensorSource* sources = getSourceArray(type, count, activeIndex);

    // Check if array is full
    if (count >= MAX_GPS_SOURCES) {
        return -1;  // Registration failed
    }

    // Get next available index
    int index = count;

    // Initialize source
    strncpy(sources[index].sourceId, sourceId, 15);
    sources[index].sourceId[15] = '\0';
    sources[index].sensorType = type;
    sources[index].protocolType = protocol;
    sources[index].updateFrequency = 0.0;
    sources[index].priority = -1;  // Automatic prioritization
    sources[index].manualOverride = false;
    sources[index].active = false;
    sources[index].available = false;
    sources[index].lastUpdateTime = 0;
    sources[index].updateCount = 0;
    sources[index].rejectedCount = 0;
    sources[index].avgUpdateInterval = 0.0;

    // Increment count
    if (type == SensorType::GPS) {
        manager.gpsSourceCount++;
    } else {
        manager.compassSourceCount++;
    }

    return index;
}

void SourcePrioritizer::updateSourceTimestamp(int sourceIndex, unsigned long timestamp) {
    // Determine which array this source is in
    if (sourceIndex < manager.gpsSourceCount) {
        // GPS source
        manager.gpsSources[sourceIndex].lastUpdateTime = timestamp;
        manager.gpsSources[sourceIndex].updateCount++;
        manager.gpsSources[sourceIndex].available = true;
    } else if (sourceIndex < manager.gpsSourceCount + manager.compassSourceCount) {
        // Compass source (offset by GPS count)
        int compassIndex = sourceIndex - manager.gpsSourceCount;
        manager.compassSources[compassIndex].lastUpdateTime = timestamp;
        manager.compassSources[compassIndex].updateCount++;
        manager.compassSources[compassIndex].available = true;
    }
}

int SourcePrioritizer::getActiveSource(SensorType type) {
    if (type == SensorType::GPS) {
        return manager.activeGpsSourceIndex;
    } else {
        return manager.activeCompassSourceIndex;
    }
}

void SourcePrioritizer::setManualOverride(int sourceIndex) {
    // Determine sensor type from source index
    if (sourceIndex < manager.gpsSourceCount) {
        // GPS source
        manager.gpsSources[sourceIndex].manualOverride = true;
        manager.gpsSources[sourceIndex].active = true;
        manager.activeGpsSourceIndex = sourceIndex;

        // Deactivate other GPS sources
        for (int i = 0; i < manager.gpsSourceCount; i++) {
            if (i != sourceIndex) {
                manager.gpsSources[i].active = false;
            }
        }
    } else if (sourceIndex < manager.gpsSourceCount + manager.compassSourceCount) {
        // Compass source
        int compassIndex = sourceIndex - manager.gpsSourceCount;
        manager.compassSources[compassIndex].manualOverride = true;
        manager.compassSources[compassIndex].active = true;
        manager.activeCompassSourceIndex = compassIndex;

        // Deactivate other compass sources
        for (int i = 0; i < manager.compassSourceCount; i++) {
            if (i != compassIndex) {
                manager.compassSources[i].active = false;
            }
        }
    }
}

void SourcePrioritizer::clearManualOverride(SensorType type) {
    int count, activeIndex;
    SensorSource* sources = getSourceArray(type, count, activeIndex);

    // Clear manual override flag for all sources of this type
    for (int i = 0; i < count; i++) {
        sources[i].manualOverride = false;
    }
}

bool SourcePrioritizer::isSourceStale(int sourceIndex, unsigned long currentTime) {
    if (sourceIndex < manager.gpsSourceCount) {
        // GPS source
        unsigned long lastUpdate = manager.gpsSources[sourceIndex].lastUpdateTime;
        if (lastUpdate == 0) return true;  // Never updated
        return (currentTime - lastUpdate) > STALE_THRESHOLD_MS;
    } else if (sourceIndex < manager.gpsSourceCount + manager.compassSourceCount) {
        // Compass source
        int compassIndex = sourceIndex - manager.gpsSourceCount;
        unsigned long lastUpdate = manager.compassSources[compassIndex].lastUpdateTime;
        if (lastUpdate == 0) return true;  // Never updated
        return (currentTime - lastUpdate) > STALE_THRESHOLD_MS;
    }

    return true;  // Unknown source is stale
}

SensorSource SourcePrioritizer::getSource(int sourceIndex) {
    if (sourceIndex < manager.gpsSourceCount) {
        return manager.gpsSources[sourceIndex];
    } else if (sourceIndex < manager.gpsSourceCount + manager.compassSourceCount) {
        int compassIndex = sourceIndex - manager.gpsSourceCount;
        return manager.compassSources[compassIndex];
    }

    // Return empty source if invalid index
    SensorSource empty;
    memset(&empty, 0, sizeof(SensorSource));
    return empty;
}

void SourcePrioritizer::updatePriorities() {
    unsigned long currentTime = millis();

    // Update GPS priorities
    selectBestSource(SensorType::GPS);

    // Update compass priorities
    selectBestSource(SensorType::COMPASS);

    manager.lastPriorityUpdate = currentTime;
}

void SourcePrioritizer::checkStale(unsigned long currentTime) {
    // Check GPS sources
    for (int i = 0; i < manager.gpsSourceCount; i++) {
        if (isSourceStale(i, currentTime)) {
            manager.gpsSources[i].available = false;

            // If this was the active source, trigger failover
            if (manager.gpsSources[i].active) {
                manager.gpsSources[i].active = false;
                manager.activeGpsSourceIndex = -1;
                selectBestSource(SensorType::GPS);  // Select next best
            }
        }
    }

    // Check compass sources
    for (int i = 0; i < manager.compassSourceCount; i++) {
        int globalIndex = i + manager.gpsSourceCount;
        if (isSourceStale(globalIndex, currentTime)) {
            manager.compassSources[i].available = false;

            // If this was the active source, trigger failover
            if (manager.compassSources[i].active) {
                manager.compassSources[i].active = false;
                manager.activeCompassSourceIndex = -1;
                selectBestSource(SensorType::COMPASS);  // Select next best
            }
        }
    }
}

// =============================================================================
// PRIVATE HELPER METHODS
// =============================================================================

SensorSource* SourcePrioritizer::getSourceArray(SensorType type, int& outCount, int& outActiveIndex) {
    if (type == SensorType::GPS) {
        outCount = manager.gpsSourceCount;
        outActiveIndex = manager.activeGpsSourceIndex;
        return manager.gpsSources;
    } else {
        outCount = manager.compassSourceCount;
        outActiveIndex = manager.activeCompassSourceIndex;
        return manager.compassSources;
    }
}

void SourcePrioritizer::setActiveIndex(SensorType type, int index) {
    if (type == SensorType::GPS) {
        manager.activeGpsSourceIndex = index;
    } else {
        manager.activeCompassSourceIndex = index;
    }
}

void SourcePrioritizer::calculateFrequency(SensorSource* source, unsigned long currentTime) {
    if (source->updateCount == 0 || source->lastUpdateTime == 0) {
        source->updateFrequency = 0.0;
        return;
    }

    // Calculate elapsed time since first update (approximate)
    // Note: This is simplified - real implementation would track firstUpdateTime
    unsigned long elapsed = currentTime - source->lastUpdateTime + 10000;  // Approximate 10s window

    if (elapsed > 0) {
        source->updateFrequency = (double)source->updateCount / (elapsed / 1000.0);
    }
}

void SourcePrioritizer::selectBestSource(SensorType type) {
    int count, activeIndex;
    SensorSource* sources = getSourceArray(type, count, activeIndex);

    if (count == 0) {
        setActiveIndex(type, -1);
        return;
    }

    // Check for manual override first
    for (int i = 0; i < count; i++) {
        if (sources[i].manualOverride && sources[i].available) {
            // Manual override - force this source active
            for (int j = 0; j < count; j++) {
                sources[j].active = (j == i);
            }
            setActiveIndex(type, i);
            return;
        }
    }

    // Calculate frequencies for all sources
    unsigned long currentTime = millis();
    for (int i = 0; i < count; i++) {
        calculateFrequency(&sources[i], currentTime);
    }

    // Find highest frequency available source
    int bestIndex = -1;
    double bestFreq = 0.0;

    for (int i = 0; i < count; i++) {
        if (sources[i].available && sources[i].updateFrequency > bestFreq) {
            bestFreq = sources[i].updateFrequency;
            bestIndex = i;
        }
    }

    // Set active source
    if (bestIndex >= 0) {
        for (int i = 0; i < count; i++) {
            sources[i].active = (i == bestIndex);
        }
        setActiveIndex(type, bestIndex);
    } else {
        // No available sources
        setActiveIndex(type, -1);
    }
}
