/**
 * @file test_isourceprioritizer_contract.cpp
 * @brief Contract test for ISourcePrioritizer interface
 *
 * This test verifies the ISourcePrioritizer interface contract:
 * - registerSource() adds new sources
 * - updateSourceTimestamp() tracks update times
 * - getActiveSource() returns highest-priority available source
 * - setManualOverride() forces specific source active
 * - clearManualOverride() restores automatic priority
 * - isSourceStale() detects sources with no updates >5 seconds
 *
 * @see specs/003-boatdata-feature-as/contracts/README.md lines 56-66
 * @test T007
 */

#include <unity.h>
#include "../../src/types/BoatDataTypes.h"

// Forward declaration - interface will be implemented in Phase 3.3
class ISourcePrioritizer {
public:
    virtual ~ISourcePrioritizer() {}

    /**
     * @brief Register a new data source
     * @param sourceId Human-readable identifier (e.g., "GPS-NMEA2000")
     * @param type Sensor type (GPS, COMPASS)
     * @param protocol Protocol type (NMEA0183, NMEA2000, ONEWIRE)
     * @return Source index (0-4), or -1 if registration failed
     */
    virtual int registerSource(const char* sourceId, SensorType type, ProtocolType protocol) = 0;

    /**
     * @brief Update timestamp for a source (call on each data update)
     * @param sourceIndex Index returned from registerSource()
     * @param timestamp Current time in milliseconds (millis())
     */
    virtual void updateSourceTimestamp(int sourceIndex, unsigned long timestamp) = 0;

    /**
     * @brief Get active source for a sensor type
     * @param type Sensor type (GPS or COMPASS)
     * @return Source index of active source, or -1 if none available
     */
    virtual int getActiveSource(SensorType type) = 0;

    /**
     * @brief Set manual override (user forces specific source)
     * @param sourceIndex Index of source to force active
     */
    virtual void setManualOverride(int sourceIndex) = 0;

    /**
     * @brief Clear manual override (restore automatic prioritization)
     * @param type Sensor type to clear override for
     */
    virtual void clearManualOverride(SensorType type) = 0;

    /**
     * @brief Check if source is stale (no updates >5 seconds)
     * @param sourceIndex Index of source to check
     * @param currentTime Current time in milliseconds (millis())
     * @return true if stale, false if fresh
     */
    virtual bool isSourceStale(int sourceIndex, unsigned long currentTime) = 0;

    /**
     * @brief Get source information (for testing)
     * @param sourceIndex Index of source
     * @return Source structure
     */
    virtual SensorSource getSource(int sourceIndex) = 0;

    /**
     * @brief Update priorities based on frequency (call periodically)
     */
    virtual void updatePriorities() = 0;

    /**
     * @brief Check for stale sources and trigger failover
     * @param currentTime Current time in milliseconds
     */
    virtual void checkStale(unsigned long currentTime) = 0;
};

// Mock implementation for testing the interface contract
class MockSourcePrioritizer : public ISourcePrioritizer {
private:
    SourceManager manager;
    static const unsigned long STALE_THRESHOLD = 5000;  // 5 seconds

    SensorSource* getSourceArray(SensorType type, int& count, int& activeIndex) {
        if (type == SensorType::GPS) {
            count = manager.gpsSourceCount;
            activeIndex = manager.activeGpsSourceIndex;
            return manager.gpsSources;
        } else {
            count = manager.compassSourceCount;
            activeIndex = manager.activeCompassSourceIndex;
            return manager.compassSources;
        }
    }

    void setActiveIndex(SensorType type, int index) {
        if (type == SensorType::GPS) {
            manager.activeGpsSourceIndex = index;
        } else {
            manager.activeCompassSourceIndex = index;
        }
    }

public:
    MockSourcePrioritizer() {
        memset(&manager, 0, sizeof(SourceManager));
        manager.activeGpsSourceIndex = -1;
        manager.activeCompassSourceIndex = -1;
    }

    int registerSource(const char* sourceId, SensorType type, ProtocolType protocol) override {
        int count, activeIndex;
        SensorSource* sources = getSourceArray(type, count, activeIndex);

        if (count >= MAX_GPS_SOURCES) {
            return -1;  // Array full
        }

        int index = count;
        strncpy(sources[index].sourceId, sourceId, 15);
        sources[index].sourceId[15] = '\0';
        sources[index].sensorType = type;
        sources[index].protocolType = protocol;
        sources[index].updateFrequency = 0.0;
        sources[index].priority = -1;  // Automatic
        sources[index].manualOverride = false;
        sources[index].active = false;
        sources[index].available = false;
        sources[index].lastUpdateTime = 0;
        sources[index].updateCount = 0;
        sources[index].rejectedCount = 0;
        sources[index].avgUpdateInterval = 0.0;

        if (type == SensorType::GPS) {
            manager.gpsSourceCount++;
        } else {
            manager.compassSourceCount++;
        }

        return index;
    }

    void updateSourceTimestamp(int sourceIndex, unsigned long timestamp) override {
        // Determine which array this source is in
        if (sourceIndex < manager.gpsSourceCount) {
            manager.gpsSources[sourceIndex].lastUpdateTime = timestamp;
            manager.gpsSources[sourceIndex].updateCount++;
            manager.gpsSources[sourceIndex].available = true;
        } else {
            int compassIndex = sourceIndex - MAX_GPS_SOURCES;
            if (compassIndex < manager.compassSourceCount) {
                manager.compassSources[compassIndex].lastUpdateTime = timestamp;
                manager.compassSources[compassIndex].updateCount++;
                manager.compassSources[compassIndex].available = true;
            }
        }
    }

    int getActiveSource(SensorType type) override {
        int count, activeIndex;
        getSourceArray(type, count, activeIndex);
        return activeIndex;
    }

    void setManualOverride(int sourceIndex) override {
        if (sourceIndex < manager.gpsSourceCount) {
            manager.gpsSources[sourceIndex].manualOverride = true;
            manager.gpsSources[sourceIndex].active = true;
            manager.activeGpsSourceIndex = sourceIndex;

            // Deactivate other GPS sources
            for (int i = 0; i < manager.gpsSourceCount; i++) {
                if (i != sourceIndex) {
                    manager.gpsSources[i].active = false;
                }
            }
        } else {
            int compassIndex = sourceIndex - MAX_GPS_SOURCES;
            if (compassIndex < manager.compassSourceCount) {
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
    }

    void clearManualOverride(SensorType type) override {
        int count, activeIndex;
        SensorSource* sources = getSourceArray(type, count, activeIndex);

        for (int i = 0; i < count; i++) {
            sources[i].manualOverride = false;
        }
    }

    bool isSourceStale(int sourceIndex, unsigned long currentTime) override {
        if (sourceIndex < manager.gpsSourceCount) {
            unsigned long lastUpdate = manager.gpsSources[sourceIndex].lastUpdateTime;
            return (currentTime - lastUpdate) > STALE_THRESHOLD;
        } else {
            int compassIndex = sourceIndex - MAX_GPS_SOURCES;
            if (compassIndex < manager.compassSourceCount) {
                unsigned long lastUpdate = manager.compassSources[compassIndex].lastUpdateTime;
                return (currentTime - lastUpdate) > STALE_THRESHOLD;
            }
        }
        return true;  // Unknown source is stale
    }

    SensorSource getSource(int sourceIndex) override {
        if (sourceIndex < manager.gpsSourceCount) {
            return manager.gpsSources[sourceIndex];
        } else {
            int compassIndex = sourceIndex - MAX_GPS_SOURCES;
            if (compassIndex < manager.compassSourceCount) {
                return manager.compassSources[compassIndex];
            }
        }
        return SensorSource();  // Empty source
    }

    void updatePriorities() override {
        // Calculate frequencies and select highest
        for (int i = 0; i < manager.gpsSourceCount; i++) {
            if (manager.gpsSources[i].updateCount > 0) {
                unsigned long elapsed = millis() - manager.gpsSources[i].lastUpdateTime;
                if (elapsed > 0) {
                    manager.gpsSources[i].updateFrequency =
                        (double)manager.gpsSources[i].updateCount / (elapsed / 1000.0);
                }
            }
        }

        // Select highest frequency GPS source (if no manual override)
        int bestGPS = -1;
        double bestGPSFreq = 0.0;
        for (int i = 0; i < manager.gpsSourceCount; i++) {
            if (manager.gpsSources[i].available && !manager.gpsSources[i].manualOverride) {
                if (manager.gpsSources[i].updateFrequency > bestGPSFreq) {
                    bestGPSFreq = manager.gpsSources[i].updateFrequency;
                    bestGPS = i;
                }
            }
        }

        if (bestGPS >= 0 && manager.activeGpsSourceIndex != bestGPS) {
            manager.activeGpsSourceIndex = bestGPS;
            for (int i = 0; i < manager.gpsSourceCount; i++) {
                manager.gpsSources[i].active = (i == bestGPS);
            }
        }
    }

    void checkStale(unsigned long currentTime) override {
        // Check GPS sources
        for (int i = 0; i < manager.gpsSourceCount; i++) {
            if (isSourceStale(i, currentTime)) {
                manager.gpsSources[i].available = false;
                if (manager.gpsSources[i].active) {
                    manager.gpsSources[i].active = false;
                    manager.activeGpsSourceIndex = -1;
                    // Trigger failover (updatePriorities will select next best)
                    updatePriorities();
                }
            }
        }

        // Check compass sources
        for (int i = 0; i < manager.compassSourceCount; i++) {
            if (isSourceStale(i + MAX_GPS_SOURCES, currentTime)) {
                manager.compassSources[i].available = false;
                if (manager.compassSources[i].active) {
                    manager.compassSources[i].active = false;
                    manager.activeCompassSourceIndex = -1;
                }
            }
        }
    }
};

// Test fixture
static MockSourcePrioritizer* prioritizer = nullptr;
static unsigned long mockTime = 0;

void setUp(void) {
    prioritizer = new MockSourcePrioritizer();
    mockTime = 10000;  // Start at 10 seconds
}

void tearDown(void) {
    delete prioritizer;
    prioritizer = nullptr;
}

// =============================================================================
// SOURCE REGISTRATION TESTS
// =============================================================================

void test_registerSource_adds_gps_source() {
    // Act: Register GPS source
    int index = prioritizer->registerSource("GPS-NMEA0183", SensorType::GPS, ProtocolType::NMEA0183);

    // Assert: Registration succeeded
    TEST_ASSERT_GREATER_OR_EQUAL(0, index);

    // Assert: Source stored correctly
    SensorSource source = prioritizer->getSource(index);
    TEST_ASSERT_EQUAL_STRING("GPS-NMEA0183", source.sourceId);
    TEST_ASSERT_EQUAL(SensorType::GPS, source.sensorType);
    TEST_ASSERT_EQUAL(ProtocolType::NMEA0183, source.protocolType);
}

void test_registerSource_adds_compass_source() {
    // Act: Register compass source
    int index = prioritizer->registerSource("Compass-NMEA2000", SensorType::COMPASS, ProtocolType::NMEA2000);

    // Assert: Registration succeeded
    TEST_ASSERT_GREATER_OR_EQUAL(0, index);

    // Assert: Source stored correctly
    SensorSource source = prioritizer->getSource(index);
    TEST_ASSERT_EQUAL_STRING("Compass-NMEA2000", source.sourceId);
    TEST_ASSERT_EQUAL(SensorType::COMPASS, source.sensorType);
    TEST_ASSERT_EQUAL(ProtocolType::NMEA2000, source.protocolType);
}

void test_registerSource_handles_multiple_sources() {
    // Act: Register two GPS sources
    int gpsA = prioritizer->registerSource("GPS-A", SensorType::GPS, ProtocolType::NMEA0183);
    int gpsB = prioritizer->registerSource("GPS-B", SensorType::GPS, ProtocolType::NMEA2000);

    // Assert: Both registered with different indices
    TEST_ASSERT_GREATER_OR_EQUAL(0, gpsA);
    TEST_ASSERT_GREATER_OR_EQUAL(0, gpsB);
    TEST_ASSERT_NOT_EQUAL(gpsA, gpsB);
}

// =============================================================================
// TIMESTAMP UPDATE TESTS
// =============================================================================

void test_updateSourceTimestamp_tracks_updates() {
    // Arrange: Register source
    int index = prioritizer->registerSource("GPS-Test", SensorType::GPS, ProtocolType::NMEA0183);

    // Act: Update timestamp
    prioritizer->updateSourceTimestamp(index, mockTime);

    // Assert: Timestamp recorded
    SensorSource source = prioritizer->getSource(index);
    TEST_ASSERT_EQUAL_UINT32(mockTime, source.lastUpdateTime);
    TEST_ASSERT_EQUAL_UINT32(1, source.updateCount);
    TEST_ASSERT_TRUE(source.available);
}

void test_updateSourceTimestamp_increments_count() {
    // Arrange: Register source
    int index = prioritizer->registerSource("GPS-Test", SensorType::GPS, ProtocolType::NMEA0183);

    // Act: Update multiple times
    prioritizer->updateSourceTimestamp(index, mockTime);
    prioritizer->updateSourceTimestamp(index, mockTime + 1000);
    prioritizer->updateSourceTimestamp(index, mockTime + 2000);

    // Assert: Count incremented
    SensorSource source = prioritizer->getSource(index);
    TEST_ASSERT_EQUAL_UINT32(3, source.updateCount);
}

// =============================================================================
// ACTIVE SOURCE TESTS
// =============================================================================

void test_getActiveSource_returns_minus_one_when_no_sources() {
    // Act: Get active GPS source (none registered)
    int active = prioritizer->getActiveSource(SensorType::GPS);

    // Assert: No active source
    TEST_ASSERT_EQUAL_INT(-1, active);
}

void test_getActiveSource_returns_highest_priority() {
    // Arrange: Register two GPS sources
    int gpsA = prioritizer->registerSource("GPS-A", SensorType::GPS, ProtocolType::NMEA0183);
    int gpsB = prioritizer->registerSource("GPS-B", SensorType::GPS, ProtocolType::NMEA2000);

    // Simulate GPS-A: 1 Hz (1 update per second for 10 seconds)
    for (int i = 0; i < 10; i++) {
        prioritizer->updateSourceTimestamp(gpsA, mockTime + i * 1000);
    }

    // Simulate GPS-B: 10 Hz (10 updates per second for 10 seconds)
    for (int i = 0; i < 100; i++) {
        prioritizer->updateSourceTimestamp(gpsB, mockTime + i * 100);
    }

    // Act: Update priorities
    prioritizer->updatePriorities();

    // Assert: GPS-B is active (higher frequency)
    int active = prioritizer->getActiveSource(SensorType::GPS);
    TEST_ASSERT_EQUAL_INT(gpsB, active);

    // Assert: GPS-B has higher frequency
    SensorSource sourceB = prioritizer->getSource(gpsB);
    TEST_ASSERT_GREATER_THAN(1.0, sourceB.updateFrequency);
}

// =============================================================================
// MANUAL OVERRIDE TESTS
// =============================================================================

void test_setManualOverride_forces_source_active() {
    // Arrange: Register two sources
    int gpsA = prioritizer->registerSource("GPS-A", SensorType::GPS, ProtocolType::NMEA0183);
    int gpsB = prioritizer->registerSource("GPS-B", SensorType::GPS, ProtocolType::NMEA2000);

    // Make GPS-B auto-prioritized (higher frequency)
    for (int i = 0; i < 100; i++) {
        prioritizer->updateSourceTimestamp(gpsB, mockTime + i * 100);
    }
    prioritizer->updatePriorities();

    // Act: Force GPS-A active (manual override)
    prioritizer->setManualOverride(gpsA);

    // Assert: GPS-A is now active despite lower frequency
    int active = prioritizer->getActiveSource(SensorType::GPS);
    TEST_ASSERT_EQUAL_INT(gpsA, active);

    // Assert: GPS-A has manual override flag
    SensorSource sourceA = prioritizer->getSource(gpsA);
    TEST_ASSERT_TRUE(sourceA.manualOverride);
    TEST_ASSERT_TRUE(sourceA.active);
}

void test_clearManualOverride_restores_automatic() {
    // Arrange: Set manual override
    int gpsA = prioritizer->registerSource("GPS-A", SensorType::GPS, ProtocolType::NMEA0183);
    prioritizer->setManualOverride(gpsA);

    // Act: Clear manual override
    prioritizer->clearManualOverride(SensorType::GPS);

    // Assert: Manual override flag cleared
    SensorSource sourceA = prioritizer->getSource(gpsA);
    TEST_ASSERT_FALSE(sourceA.manualOverride);
}

// =============================================================================
// STALE DETECTION TESTS
// =============================================================================

void test_isSourceStale_detects_stale_after_5_seconds() {
    // Arrange: Register and update source
    int index = prioritizer->registerSource("GPS-Test", SensorType::GPS, ProtocolType::NMEA0183);
    prioritizer->updateSourceTimestamp(index, mockTime);

    // Act: Check stale after 4 seconds (not stale yet)
    bool stale4s = prioritizer->isSourceStale(index, mockTime + 4000);

    // Assert: Not stale (< 5 seconds)
    TEST_ASSERT_FALSE(stale4s);

    // Act: Check stale after 6 seconds (stale)
    bool stale6s = prioritizer->isSourceStale(index, mockTime + 6000);

    // Assert: Stale (> 5 seconds)
    TEST_ASSERT_TRUE(stale6s);
}

void test_checkStale_marks_source_unavailable() {
    // Arrange: Register and update source
    int index = prioritizer->registerSource("GPS-Test", SensorType::GPS, ProtocolType::NMEA0183);
    prioritizer->updateSourceTimestamp(index, mockTime);

    // Act: Check stale after 6 seconds
    prioritizer->checkStale(mockTime + 6000);

    // Assert: Source marked unavailable
    SensorSource source = prioritizer->getSource(index);
    TEST_ASSERT_FALSE(source.available);
}

void test_checkStale_triggers_failover() {
    // Arrange: Register two GPS sources
    int gpsA = prioritizer->registerSource("GPS-A", SensorType::GPS, ProtocolType::NMEA0183);
    int gpsB = prioritizer->registerSource("GPS-B", SensorType::GPS, ProtocolType::NMEA2000);

    // GPS-B is active (higher frequency)
    for (int i = 0; i < 100; i++) {
        prioritizer->updateSourceTimestamp(gpsB, mockTime + i * 100);
    }
    prioritizer->updatePriorities();

    // GPS-A continues updating
    for (int i = 0; i < 10; i++) {
        prioritizer->updateSourceTimestamp(gpsA, mockTime + 10000 + i * 1000);
    }

    // Act: GPS-B becomes stale (no updates for >5 seconds)
    prioritizer->checkStale(mockTime + 16000);  // 6 seconds after last GPS-B update

    // Assert: GPS-A is now active (failover)
    int active = prioritizer->getActiveSource(SensorType::GPS);
    TEST_ASSERT_EQUAL_INT(gpsA, active);
}

// =============================================================================
// TEST RUNNER
// =============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // Source registration tests
    RUN_TEST(test_registerSource_adds_gps_source);
    RUN_TEST(test_registerSource_adds_compass_source);
    RUN_TEST(test_registerSource_handles_multiple_sources);

    // Timestamp update tests
    RUN_TEST(test_updateSourceTimestamp_tracks_updates);
    RUN_TEST(test_updateSourceTimestamp_increments_count);

    // Active source tests
    RUN_TEST(test_getActiveSource_returns_minus_one_when_no_sources);
    RUN_TEST(test_getActiveSource_returns_highest_priority);

    // Manual override tests
    RUN_TEST(test_setManualOverride_forces_source_active);
    RUN_TEST(test_clearManualOverride_restores_automatic);

    // Stale detection tests
    RUN_TEST(test_isSourceStale_detects_stale_after_5_seconds);
    RUN_TEST(test_checkStale_marks_source_unavailable);
    RUN_TEST(test_checkStale_triggers_failover);

    return UNITY_END();
}
