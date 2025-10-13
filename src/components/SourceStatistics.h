/**
 * @file SourceStatistics.h
 * @brief Data structures for NMEA source statistics tracking
 *
 * Defines hierarchical data model for tracking NMEA 2000/0183 message sources:
 * - Category (9 types: GPS, Compass, Wind, DST, Rudder, Engine, Saildrive, Battery, ShorePower)
 * - Message Type (19 types: PGN + sentence IDs)
 * - Source (50 max: individual NMEA devices by SID/talker ID)
 *
 * Features:
 * - Frequency tracking: 10-sample circular buffer for Hz calculation
 * - Staleness detection: 5-second threshold for inactive sources
 * - Garbage collection: Auto-remove sources stale >5 minutes
 *
 * Memory footprint: ~5.3KB static allocation for 50 sources
 *
 * @see specs/012-sources-stats-and/data-model.md
 * @version 1.0.0
 * @date 2025-10-13
 */

#ifndef SOURCE_STATISTICS_H
#define SOURCE_STATISTICS_H

#include <Arduino.h>
#include "types/BoatDataTypes.h"  // For ProtocolType enum

// =============================================================================
// ENUMERATIONS
// =============================================================================

/**
 * @brief BoatData category enumeration (maps to dashboard sections)
 *
 * Organizes message types by functional group for UI display.
 */
enum class CategoryType : uint8_t {
    GPS = 0,          ///< GPS data (position, COG, SOG, variation)
    COMPASS = 1,      ///< Heading and attitude data
    WIND = 2,         ///< Wind sensor data
    DST = 3,          ///< Depth, Speed, Temperature
    RUDDER = 4,       ///< Steering angle
    ENGINE = 5,       ///< Engine telemetry
    SAILDRIVE = 6,    ///< Saildrive status
    BATTERY = 7,      ///< Battery monitoring
    SHORE_POWER = 8,  ///< Shore power connection
    COUNT = 9         ///< Total category count (not a valid category)
};

// =============================================================================
// SOURCE TRACKING STRUCTURES
// =============================================================================

/**
 * @brief NMEA2000 device metadata
 *
 * Extracted from tN2kDeviceList during device discovery polling.
 * Embedded within MessageSource struct for static allocation.
 *
 * Memory footprint: 72 bytes per source (with alignment padding)
 * - 1 bool: 1 byte
 * - 9 numeric fields: 2×uint16_t (4) + 3×uint32_t (12) + 3×uint8_t (3) = 19 bytes
 * - 3 strings: 16+24+12 = 52 bytes
 * - Raw total: 72 bytes (1 + 19 + 52 = 72 bytes exactly with padding)
 */
struct DeviceInfo {
    // === Discovery Status ===
    bool hasInfo;                    ///< true if device was discovered via tN2kDeviceList

    // === NMEA2000 Identification ===
    uint16_t manufacturerCode;       ///< NMEA2000 manufacturer code (e.g., 275=Garmin, 1855=Furuno)
    char manufacturer[16];           ///< Human-readable manufacturer name (e.g., "Garmin", "Furuno")
    char modelId[24];                ///< Model identifier string (e.g., "GPS 17x", "GP330B")
    uint16_t productCode;            ///< Product code (manufacturer-specific)
    uint32_t serialNumber;           ///< Unique device serial number
    char softwareVersion[12];        ///< Firmware version (e.g., "v2.30", "1.05")

    // === NMEA2000 Classification ===
    uint8_t deviceInstance;          ///< Device instance (0-255, for multi-device setups)
    uint8_t deviceClass;             ///< NMEA2000 device class (e.g., 25=Navigation, 30=Communication)
    uint8_t deviceFunction;          ///< NMEA2000 device function (e.g., 130=GPS, 140=Compass)

    // === Discovery Tracking ===
    uint32_t firstSeenTime;          ///< millis() timestamp when source first detected
    uint32_t discoveryTimeout;       ///< 60000ms (60 seconds) - time to wait before marking "Unknown"

    // === NMEA0183 Description (alternative to full metadata) ===
    char description[32];            ///< Device type description for NMEA0183 (e.g., "Autopilot")

    /**
     * @brief Initialize with default values (no device info)
     */
    void init() {
        hasInfo = false;
        manufacturerCode = 0;
        manufacturer[0] = '\0';
        modelId[0] = '\0';
        productCode = 0;
        serialNumber = 0;
        softwareVersion[0] = '\0';
        deviceInstance = 255;  // 255 = unknown/not specified
        deviceClass = 255;
        deviceFunction = 255;
        firstSeenTime = 0;
        discoveryTimeout = 60000;  // 60 seconds
        description[0] = '\0';
    }

    /**
     * @brief Check if discovery timeout has expired
     * @param currentTime Current millis() timestamp
     * @return true if timeout exceeded and device still not discovered
     */
    bool isDiscoveryTimedOut(uint32_t currentTime) const {
        return !hasInfo && firstSeenTime > 0 &&
               (currentTime - firstSeenTime) >= discoveryTimeout;
    }

    /**
     * @brief Get discovery status string for logging/UI
     * @param currentTime Current millis() timestamp
     * @return "Discovered", "Discovering...", or "Unknown (timeout)"
     */
    const char* getStatusString(uint32_t currentTime) const {
        if (hasInfo) {
            return "Discovered";
        } else if (isDiscoveryTimedOut(currentTime)) {
            return "Unknown (timeout)";
        } else {
            return "Discovering...";
        }
    }
};

/**
 * @brief Individual message source (NMEA device)
 *
 * Represents a single source providing a specific message type.
 * Examples: "NMEA2000-42" (GPS with SID 42), "NMEA0183-AP" (Autopilot)
 *
 * Memory footprint: ~167 bytes per source (was ~95 bytes before device metadata)
 */
struct MessageSource {
    // === Identification ===
    char sourceId[20];              ///< Format: "NMEA2000-<SID>" or "NMEA0183-<TalkerID>"
    uint8_t sid;                    ///< NMEA2000 SID or 255 for NMEA0183
    char talkerId[4];               ///< NMEA0183 talker ID (e.g., "AP", "VH") or empty
    ProtocolType protocol;          ///< NMEA2000 or NMEA0183

    // === Statistics ===
    double frequency;               ///< Calculated update rate (Hz), range [0, 20]
    uint32_t timeSinceLast;         ///< ms since last update (from millis() delta)
    bool isStale;                   ///< true if timeSinceLast > 5000ms

    // === Internal Tracking ===
    uint32_t lastUpdateTime;        ///< millis() timestamp of last message
    uint16_t updateCount;           ///< Total updates received (for diagnostics)
    uint32_t timestampBuffer[10];   ///< Circular buffer for frequency calculation
    uint8_t bufferIndex;            ///< Current position in circular buffer
    bool bufferFull;                ///< true when 10 samples collected

    // === Lifecycle ===
    bool active;                    ///< true if source is registered

    // === Device Metadata ===
    DeviceInfo deviceInfo;          ///< Device metadata (NMEA2000 only, hasInfo=false for NMEA0183)

    /**
     * @brief Initialize source with default values
     */
    void init() {
        sourceId[0] = '\0';
        sid = 255;
        talkerId[0] = '\0';
        protocol = ProtocolType::UNKNOWN;
        frequency = 0.0;
        timeSinceLast = 0;
        isStale = false;
        lastUpdateTime = 0;
        updateCount = 0;
        bufferIndex = 0;
        bufferFull = false;
        active = false;

        // Initialize timestamp buffer
        for (int i = 0; i < 10; i++) {
            timestampBuffer[i] = 0;
        }

        // Initialize device info
        deviceInfo.init();
    }
};

/**
 * @brief Message type entry (PGN or sentence)
 *
 * Groups all sources providing a specific message type.
 * Examples: "PGN129025" (GPS Position), "RSA" (Rudder Sensor Angle)
 *
 * Memory footprint: ~420 bytes (5 sources × 95 bytes + metadata)
 */
struct MessageTypeEntry {
    char messageTypeId[16];         ///< "PGN129025" or "RSA"
    ProtocolType protocol;          ///< NMEA2000 or NMEA0183
    MessageSource sources[5];       ///< Max 5 sources per message type
    uint8_t sourceCount;            ///< Active sources (0-5)

    /**
     * @brief Initialize message type with default values
     */
    void init() {
        messageTypeId[0] = '\0';
        protocol = ProtocolType::UNKNOWN;
        sourceCount = 0;
        for (int i = 0; i < 5; i++) {
            sources[i].init();
        }
    }
};

/**
 * @brief Category entry (BoatData category)
 *
 * Groups all message types belonging to a functional category.
 * Examples: GPS category contains PGN129025, PGN129026, GGA, RMC, etc.
 *
 * Memory footprint: ~3,380 bytes (8 messages × 420 bytes + metadata)
 */
struct CategoryEntry {
    CategoryType category;
    MessageTypeEntry messages[8];   ///< Max 8 message types per category
    uint8_t messageCount;           ///< Active message types (0-8)

    /**
     * @brief Initialize category with default values
     */
    void init(CategoryType cat) {
        category = cat;
        messageCount = 0;
        for (int i = 0; i < 8; i++) {
            messages[i].init();
        }
    }
};

// =============================================================================
// HELPER FUNCTIONS
// =============================================================================

/**
 * @brief Get category name as string (for JSON serialization)
 *
 * @param category Category type
 * @return Category name string (e.g., "GPS", "Compass")
 */
inline const char* getCategoryName(CategoryType category) {
    switch (category) {
        case CategoryType::GPS: return "GPS";
        case CategoryType::COMPASS: return "Compass";
        case CategoryType::WIND: return "Wind";
        case CategoryType::DST: return "DST";
        case CategoryType::RUDDER: return "Rudder";
        case CategoryType::ENGINE: return "Engine";
        case CategoryType::SAILDRIVE: return "Saildrive";
        case CategoryType::BATTERY: return "Battery";
        case CategoryType::SHORE_POWER: return "ShorePower";
        default: return "Unknown";
    }
}

/**
 * @brief Get protocol name as string (for JSON serialization)
 *
 * @param protocol Protocol type
 * @return Protocol name string (e.g., "NMEA2000", "NMEA0183")
 */
inline const char* getProtocolName(ProtocolType protocol) {
    switch (protocol) {
        case ProtocolType::NMEA2000: return "NMEA2000";
        case ProtocolType::NMEA0183: return "NMEA0183";
        case ProtocolType::ONEWIRE: return "1-Wire";
        default: return "Unknown";
    }
}

#endif // SOURCE_STATISTICS_H
