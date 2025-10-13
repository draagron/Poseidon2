# Data Model: NMEA2000 Device Discovery

**Feature**: NMEA2000 Device Discovery and Identification
**Date**: 2025-10-13
**Phase**: 1 (Design)

## Overview

This document defines the data structures, schemas, and relationships for device metadata storage and transmission in the NMEA2000 device discovery feature. All structures prioritize static memory allocation and fixed-size buffers per constitutional principle II (Resource-Aware Development).

## Core Data Structures

### 1. DeviceInfo Struct

**Location**: `src/types/SourceStatistics.h` (embedded within `MessageSource` struct)

**Purpose**: Store NMEA2000 device metadata extracted from `tN2kDeviceList`.

**Definition**:
```cpp
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
```

**Memory Analysis**:
- **Static allocation**: Embedded within `MessageSource` struct (no dynamic allocation)
- **Fixed-size buffers**: All strings use `char[]` arrays (no `String` class)
- **Padding/alignment**: Struct size may be ~72 bytes due to compiler padding (acceptable)
- **Total system impact**: 50 sources × 72 bytes = 3,600 bytes (~3.5KB)

**Lifecycle**:
1. **Initialization**: `deviceInfo.init()` called when `MessageSource` created
2. **First message arrival**: `firstSeenTime` set to `millis()`
3. **Discovery**: `hasInfo = true`, metadata fields populated from `tDevice` object
4. **Timeout**: If 60 seconds elapse without discovery, `isDiscoveryTimedOut()` returns true
5. **Update**: If device re-announces with changed metadata, fields overwritten

### 2. Modified MessageSource Struct

**Location**: `src/types/SourceStatistics.h`

**Changes**:
```cpp
struct MessageSource {
    // === Existing fields (unchanged) ===
    char sourceId[20];
    uint8_t sid;
    char talkerId[4];
    ProtocolType protocol;
    double frequency;
    uint32_t timeSinceLast;
    bool isStale;
    uint32_t lastUpdateTime;
    uint16_t updateCount;
    uint32_t timestampBuffer[10];
    uint8_t bufferIndex;
    bool bufferFull;
    bool active;

    // === NEW: Device metadata ===
    DeviceInfo deviceInfo;           ///< Device metadata (NMEA2000 only, hasInfo=false for NMEA0183)

    /**
     * @brief Initialize source with default values
     */
    void init() {
        // Existing initialization code...
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
        for (int i = 0; i < 10; i++) {
            timestampBuffer[i] = 0;
        }

        // NEW: Initialize device info
        deviceInfo.init();
    }
};
```

**Memory Impact**:
- **Before**: ~95 bytes per MessageSource
- **After**: ~95 + 72 = ~167 bytes per MessageSource
- **Total (50 sources)**: 50 × 167 = 8,350 bytes (~8.2KB)
- **Additional overhead**: 8.2KB - 4.75KB = ~3.5KB (matches DeviceInfo struct size)

**Backward Compatibility**: Not required per constitutional principle (lean, latest requirements only).

## WebSocket Schema (Version 2)

### Schema Version Increment

**Current**: v1 (no device metadata)
**New**: v2 (includes device metadata)

**Version Constant**: `src/components/SourceStatsSerializer.h`
```cpp
#define SOURCE_STATS_SCHEMA_VERSION 2
```

### Full Snapshot Message (Version 2)

**Event**: `fullSnapshot`
**Trigger**: Client connects to `/source-stats` WebSocket endpoint

**JSON Schema**:
```json
{
  "event": "fullSnapshot",
  "version": 2,
  "timestamp": 1697232000000,
  "sources": [
    {
      "sourceId": "NMEA2000-42",
      "protocol": "NMEA2000",
      "sid": 42,
      "talkerId": "",
      "frequency": 10.2,
      "timeSinceLast": 98,
      "isStale": false,
      "updateCount": 1520,
      "deviceInfo": {
        "hasInfo": true,
        "manufacturerCode": 275,
        "manufacturer": "Garmin",
        "modelId": "GPS 17x",
        "productCode": 1234,
        "serialNumber": 123456789,
        "softwareVersion": "v2.30",
        "deviceInstance": 0,
        "deviceClass": 25,
        "deviceFunction": 130,
        "status": "Discovered"
      }
    },
    {
      "sourceId": "NMEA2000-7",
      "protocol": "NMEA2000",
      "sid": 7,
      "talkerId": "",
      "frequency": 1.0,
      "timeSinceLast": 1005,
      "isStale": false,
      "updateCount": 152,
      "deviceInfo": {
        "hasInfo": false,
        "status": "Discovering..."
      }
    },
    {
      "sourceId": "NMEA0183-AP",
      "protocol": "NMEA0183",
      "sid": 255,
      "talkerId": "AP",
      "frequency": 1.0,
      "timeSinceLast": 1002,
      "isStale": false,
      "updateCount": 305,
      "deviceInfo": {
        "hasInfo": false,
        "description": "Autopilot",
        "status": "N/A (NMEA0183)"
      }
    }
  ]
}
```

**Field Definitions**:
- `event`: Always "fullSnapshot"
- `version`: Schema version (2)
- `timestamp`: millis() snapshot timestamp
- `sources[]`: Array of MessageSource objects with device metadata

**deviceInfo Object** (NMEA2000 source, discovered):
- `hasInfo`: true
- `manufacturerCode`: Numeric manufacturer code
- `manufacturer`: Human-readable manufacturer name (from ManufacturerLookup)
- `modelId`: Model identifier string
- `productCode`: Product code
- `serialNumber`: Unique serial number
- `softwareVersion`: Firmware version
- `deviceInstance`: Device instance number
- `deviceClass`: NMEA2000 device class
- `deviceFunction`: NMEA2000 device function
- `status`: "Discovered"

**deviceInfo Object** (NMEA2000 source, discovering):
- `hasInfo`: false
- `status`: "Discovering..." (or "Unknown (timeout)" after 60s)

**deviceInfo Object** (NMEA0183 source):
- `hasInfo`: false
- `description`: Talker ID description (from TalkerIdLookup, e.g., "Autopilot")
- `status`: "N/A (NMEA0183)"

### Delta Update Message (Version 2)

**Event**: `sourceUpdate`
**Trigger**: Source statistics change (frequency update, staleness change, device discovered)

**JSON Schema** (device discovered):
```json
{
  "event": "sourceUpdate",
  "sourceId": "NMEA2000-42",
  "timestamp": 1697232005000,
  "changes": {
    "deviceInfo": {
      "hasInfo": true,
      "manufacturerCode": 275,
      "manufacturer": "Garmin",
      "modelId": "GPS 17x",
      "productCode": 1234,
      "serialNumber": 123456789,
      "softwareVersion": "v2.30",
      "deviceInstance": 0,
      "deviceClass": 25,
      "deviceFunction": 130,
      "status": "Discovered"
    }
  }
}
```

**JSON Schema** (discovery timeout):
```json
{
  "event": "sourceUpdate",
  "sourceId": "NMEA2000-7",
  "timestamp": 1697232065000,
  "changes": {
    "deviceInfo": {
      "hasInfo": false,
      "reason": "discovery_timeout",
      "status": "Unknown (timeout)"
    }
  }
}
```

**JSON Schema** (statistics update with existing device info):
```json
{
  "event": "sourceUpdate",
  "sourceId": "NMEA2000-42",
  "timestamp": 1697232010000,
  "changes": {
    "frequency": 10.5,
    "timeSinceLast": 95,
    "isStale": false
  }
}
```

**Note**: `deviceInfo` only included in `changes` when device metadata is first discovered, updated, or timed out. Regular frequency updates omit `deviceInfo` to minimize message size.

### Schema Migration (v1 → v2)

**Backward Compatibility**: Not required (per constitutional principle).

**Client Detection**:
- Clients check `version` field in `fullSnapshot` message
- v1 clients (expecting no device metadata): Will ignore unknown `deviceInfo` field (JSON parsers typically tolerate extra fields)
- v2 clients: Use `version === 2` to enable device metadata display

**Server Behavior**:
- Always send v2 schema (no v1 fallback)
- Include `version: 2` in all `fullSnapshot` messages

## Manufacturer Lookup Table

**Location**: `src/utils/ManufacturerLookup.h` / `.cpp`

**Purpose**: Map NMEA2000 manufacturer codes (uint16_t) to human-readable names.

**Data Structure**:
```cpp
/**
 * @brief Manufacturer code to name mapping entry
 */
struct ManufacturerEntry {
    uint16_t code;
    const char* name;
};

/**
 * @brief Static lookup table (stored in PROGMEM to save RAM)
 *
 * Sorted by code for potential binary search optimization (future).
 */
const ManufacturerEntry PROGMEM manufacturerTable[] = {
    {137, "Airmar"},
    {135, "B&G"},
    {275, "Garmin"},
    {1855, "Furuno"},
    {1857, "Navico (Simrad/Lowrance/B&G)"},
    {378, "Raymarine"},
    {307, "Yanmar"},
    {163, "Maretron"},
    {529, "Victron Energy"},
    {304, "Mercury Marine"},
    {355, "Honda"},
    {799, "Volvo Penta"},
    // ... add ~40 more common manufacturers
    {0, nullptr}  // Sentinel
};
```

**Lookup Function**:
```cpp
/**
 * @brief Get manufacturer name from code
 *
 * @param code NMEA2000 manufacturer code
 * @return Manufacturer name or "Unknown ({code})" if not found
 */
const char* getManufacturerName(uint16_t code);
```

**Implementation Notes**:
- Use `PROGMEM` to store table in flash memory (save RAM)
- Linear search acceptable for ~50 entries (O(n) = 50 comparisons, ~10μs)
- Return format for unknown codes: "Unknown (275)" (shows numeric code)
- Future optimization: Binary search if table grows >100 entries

**Reference**: NMEA2000 manufacturer codes from https://www.nmea.org/nmea-2000.html

## Talker ID Lookup Table

**Location**: `src/utils/TalkerIdLookup.h` / `.cpp`

**Purpose**: Map NMEA0183 talker IDs (2-character strings) to device type descriptions.

**Data Structure**:
```cpp
/**
 * @brief Talker ID to description mapping entry
 */
struct TalkerEntry {
    const char id[3];        // 2-char talker ID + null terminator
    const char* description;
};

/**
 * @brief Static lookup table (stored in PROGMEM)
 */
const TalkerEntry PROGMEM talkerTable[] = {
    {"AP", "Autopilot"},
    {"GP", "GPS Receiver"},
    {"HC", "Heading Compass"},
    {"VH", "VHF Radio"},
    {"VW", "Wind Sensor"},
    {"SD", "Depth Sounder"},
    {"II", "Integrated Instrumentation"},
    {"YX", "Transducer"},
    {"EC", "Electronic Chart System"},
    {"CD", "Digital Selective Calling"},
    {"", nullptr}  // Sentinel
};
```

**Lookup Function**:
```cpp
/**
 * @brief Get device type description from talker ID
 *
 * @param talkerId 2-character talker ID (e.g., "AP", "GP")
 * @return Device type description or "Unknown NMEA0183 Device" if not found
 */
const char* getTalkerDescription(const char* talkerId);
```

**Implementation Notes**:
- Use `PROGMEM` for flash storage
- Linear search (table size ~15 entries, negligible performance impact)
- Case-sensitive comparison (NMEA0183 talker IDs are uppercase by standard)
- Return "Unknown NMEA0183 Device" for unrecognized talker IDs

**Reference**: NMEA0183 talker IDs from IEC 61162-1 standard.

## Database / Persistent Storage

**Design Decision**: NO persistent storage for device metadata.

**Rationale**:
1. **Constitutional Principle VI (Always-On Operation)**: System never powers down, so rediscovery on reboot is acceptable
2. **Simplicity**: Avoid LittleFS write cycles (flash wear), simplify error handling
3. **Freshness**: Device metadata may change (firmware updates), rediscovery ensures accuracy
4. **Memory Constraints**: LittleFS storage is limited, prioritize web UI assets

**Rediscovery Behavior**:
- On system reboot, all `deviceInfo.hasInfo = false`
- Devices rediscovered within 10-30 seconds of bus startup (typical NMEA2000 behavior)
- UI displays "Discovering..." placeholder until discovery completes

**Alternative Considered**: Store device metadata in `/device-cache.json` (LittleFS) for faster startup.

**Rejection Reason**: Adds complexity (file I/O, JSON parsing, cache invalidation), minimal UX benefit (30-second delay acceptable for marine gateway reboot scenario).

## Data Flow Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                        NMEA2000 CAN Bus                          │
│  (Devices broadcast Product Info PGN 126996, Config PGN 126998) │
└────────────────────────────┬────────────────────────────────────┘
                             │
                             ▼
                    ┌─────────────────┐
                    │   tNMEA2000     │
                    │  (Library CAN   │
                    │   RX Handler)   │
                    └────────┬────────┘
                             │ (Automatic HandleMsg routing)
                             ▼
                    ┌─────────────────┐
                    │ tN2kDeviceList  │
                    │  (Device cache) │
                    │ - ReadResetIs   │
                    │   ListUpdated() │
                    │ - FindDevice    │
                    │   BySource()    │
                    └────────┬────────┘
                             │ (Polled every 5 seconds)
                             ▼
               ┌─────────────────────────────┐
               │   DeviceInfoCollector       │
               │ - pollDeviceList()          │
               │ - extractDeviceMetadata()   │
               │ - updateSourceRegistry()    │
               └────────┬───────────┬────────┘
                        │           │
           ┌────────────┘           └────────────┐
           ▼                                     ▼
┌──────────────────────┐            ┌────────────────────────┐
│  ManufacturerLookup  │            │   TalkerIdLookup       │
│ (Static table:       │            │ (Static table:         │
│  code → name)        │            │  talkerId → desc)      │
└──────────┬───────────┘            └────────────┬───────────┘
           │                                     │
           └───────────────┬─────────────────────┘
                           ▼
                ┌──────────────────────┐
                │   SourceRegistry     │
                │ - MessageSource[]    │
                │   - deviceInfo       │
                │ - recordUpdate()     │
                │ - updateDeviceInfo() │
                └──────────┬───────────┘
                           │ (hasChanges = true)
                           ▼
                ┌──────────────────────┐
                │ SourceStatsHandler   │
                │ - Batch timer (100ms)│
                │ - Send delta updates │
                └──────────┬───────────┘
                           │
                           ▼
                ┌──────────────────────┐
                │ SourceStatsSerializer│
                │ - Serialize v2 JSON  │
                │ - Include deviceInfo │
                └──────────┬───────────┘
                           │
                           ▼
                ┌──────────────────────┐
                │ WebSocket Clients    │
                │ - /source-stats      │
                │ - Browser UI         │
                └──────────────────────┘
```

## Memory Budget Summary

| Component | Size (bytes) | Count | Total | Notes |
|-----------|--------------|-------|-------|-------|
| DeviceInfo struct | 72 | 50 | 3,600 | Embedded in MessageSource |
| MessageSource struct | 167 | 50 | 8,350 | Includes DeviceInfo (was ~95 bytes) |
| ManufacturerLookup table | ~1,200 | 1 | 1,200 | PROGMEM (flash, not RAM) |
| TalkerIdLookup table | ~300 | 1 | 300 | PROGMEM (flash, not RAM) |
| tN2kDeviceList internal | ~2,000 | 1 | 2,000 | Estimated (library overhead) |
| **Total RAM increase** | - | - | **~3,600** | DeviceInfo structs only |
| **Total Flash increase** | - | - | **~1,500** | Lookup tables (PROGMEM) |

**Constitutional Compliance**: <5KB RAM increase (spec allows <5KB), static allocation, no dynamic memory.

**ESP32 RAM Available**: ~300KB → 3.6KB is 1.2% of total (acceptable).

---

**Data Model Complete**: Ready for contract definitions (Phase 1, next step).
