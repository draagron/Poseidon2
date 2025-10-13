# Data Model: Source Statistics Tracking

**Feature**: 012-sources-stats-and
**Version**: 1.0.0
**Date**: 2025-10-13

## Overview

This data model defines the structures for tracking NMEA source statistics across NMEA 2000 and NMEA 0183 protocols. The design emphasizes:
- **Static allocation**: All structures use fixed-size arrays (no heap)
- **Hierarchical organization**: Category → Message Type → Source
- **Memory efficiency**: Target <10KB for 50 sources
- **Frequency tracking**: Circular buffer for rolling average (10 samples)

## Core Entities

### 1. MessageSource

Represents a single source providing a specific message type.

**Structure** (`src/components/SourceStatistics.h`):
```cpp
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
};
```

**Memory Footprint**: ~80 bytes per source

**Field Details**:
- `sourceId`: Unique identifier formatted as "NMEA2000-42" or "NMEA0183-AP"
- `sid`: Raw SID value (0-252) for NMEA2000, 255 for NMEA0183
- `talkerId`: Empty string for NMEA2000, 2-char talker for NMEA0183 (null-terminated)
- `protocol`: Enum from BoatDataTypes.h (NMEA0183=0, NMEA2000=1)
- `frequency`: Hz calculated from circular buffer average
- `timeSinceLast`: Updated every WebSocket batch cycle (500ms)
- `isStale`: Updated when `timeSinceLast > 5000`
- `timestampBuffer`: Rolling window of last 10 `millis()` timestamps
- `bufferIndex`: Wraps 0-9
- `bufferFull`: Prevents frequency calculation until 10 samples

**Invariants**:
- `sourceId` must be unique across all sources
- `sid` ≤ 252 if protocol == NMEA2000
- `talkerId` non-empty if protocol == NMEA0183
- `frequency` = 0 until `bufferFull == true`
- `isStale` = false if `timeSinceLast < 5000`

---

### 2. MessageTypeEntry

Groups all sources providing a specific PGN or sentence type.

**Structure** (`src/components/SourceStatistics.h`):
```cpp
struct MessageTypeEntry {
    char messageTypeId[16];         ///< "PGN129025" or "RSA"
    ProtocolType protocol;          ///< NMEA2000 or NMEA0183
    MessageSource sources[5];       ///< Max 5 sources per message type
    uint8_t sourceCount;            ///< Active sources (0-5)
};
```

**Memory Footprint**: ~420 bytes (5 sources × 80 bytes + metadata)

**Field Details**:
- `messageTypeId`: PGN number (e.g., "PGN129025") or sentence type (e.g., "RSA")
- `protocol`: Inherited from message definition
- `sources`: Fixed array of MessageSource entries
- `sourceCount`: Number of active sources (0-5)

**Operations**:
- `addSource(sourceId, protocol)`: Add new source, returns index or -1 if full
- `findSource(sourceId)`: Linear search by sourceId, returns index or -1
- `removeSource(index)`: Shift array left, decrement sourceCount

**Invariants**:
- `sourceCount ≤ 5`
- All active sources have unique `sourceId`
- Inactive sources (index ≥ sourceCount) have `active = false`

---

### 3. CategoryEntry

Groups all message types belonging to a BoatData category.

**Structure** (`src/components/SourceStatistics.h`):
```cpp
enum class CategoryType : uint8_t {
    GPS = 0,
    COMPASS = 1,
    WIND = 2,
    DST = 3,
    RUDDER = 4,
    ENGINE = 5,
    SAILDRIVE = 6,
    BATTERY = 7,
    SHORE_POWER = 8,
    COUNT = 9
};

struct CategoryEntry {
    CategoryType category;
    MessageTypeEntry messages[8];   ///< Max 8 message types per category
    uint8_t messageCount;           ///< Active message types (0-8)
};
```

**Memory Footprint**: ~3,380 bytes (8 messages × 420 bytes + metadata)

**Category Mappings** (from spec FR-001, FR-002):
- **GPS**: PGN129025, PGN129026, PGN129029, PGN127258, GGA, RMC, VTG (7 types)
- **Compass**: PGN127250, PGN127251, PGN127252, PGN127257, HDM (5 types)
- **Wind**: PGN130306 (1 type)
- **DST**: PGN128267, PGN128259, PGN130316 (3 types)
- **Rudder**: RSA (1 type)
- **Engine**: PGN127488, PGN127489 (2 types)
- **Saildrive**: (No NMEA messages, reserved for future)
- **Battery**: (No NMEA messages in current scope)
- **ShorePower**: (No NMEA messages in current scope)

**Operations**:
- `findMessageType(messageTypeId)`: Linear search, returns index or -1
- `getOrCreateMessageType(messageTypeId, protocol)`: Lazy initialization

**Invariants**:
- `messageCount ≤ 8`
- All active message types have unique `messageTypeId`

---

### 4. SourceRegistry

Top-level registry managing all source statistics.

**Structure** (`src/components/SourceRegistry.h`):
```cpp
class SourceRegistry {
public:
    // === Lifecycle ===
    void init();                    ///< Initialize registry (called once in setup)

    // === Source Management ===
    bool recordUpdate(CategoryType category, const char* messageTypeId,
                      const char* sourceId, ProtocolType protocol);
    bool removeSource(const char* sourceId);
    void garbageCollect();          ///< Remove stale sources (>5 min)

    // === Statistics ===
    void updateStaleFlags();        ///< Refresh timeSinceLast and isStale flags
    uint8_t getTotalSourceCount() const;
    bool hasChanges() const;        ///< true if changes since last serialize
    void clearChangeFlag();         ///< Reset change flag after serialize

    // === Access ===
    const CategoryEntry* getCategory(CategoryType category) const;
    const MessageSource* findSource(const char* sourceId) const;

private:
    CategoryEntry categories_[9];   ///< Fixed array for all 9 categories
    uint8_t totalSources_;          ///< Total active sources across all categories
    bool hasChanges_;               ///< Dirty flag for WebSocket updates
    uint32_t lastGCTime_;           ///< millis() timestamp of last GC run

    // === Internal Methods ===
    void evictOldestSource();       ///< Remove source with max timeSinceLast
    MessageSource* findSourceMutable(const char* sourceId);
};
```

**Memory Footprint**: ~30,420 bytes (9 categories × 3,380 bytes)

**Key Methods**:

**`recordUpdate()`**:
- Lookup or create source in hierarchy (category → message → source)
- Update `lastUpdateTime = millis()`
- Add timestamp to circular buffer
- Recalculate frequency if buffer full
- Set `hasChanges_ = true`
- If source limit (50) reached, call `evictOldestSource()`

**`garbageCollect()`**:
- Called every 60 seconds via ReactESP timer
- Iterate all sources, remove if `timeSinceLast > 300000` (5 min)
- Emit WebSocket removal event for each deleted source
- Set `hasChanges_ = true`

**`updateStaleFlags()`**:
- Called every 500ms in WebSocket batch cycle
- Update `timeSinceLast = millis() - lastUpdateTime` for all sources
- Update `isStale = (timeSinceLast > 5000)`
- Set `hasChanges_ = true` if any flag changed

**Invariants**:
- `totalSources_ ≤ 50` (MAX_SOURCES)
- Sum of all `CategoryEntry::sourceCount` == `totalSources_`
- No duplicate `sourceId` across all categories

---

## WebSocket Message Schemas

### 1. Full Snapshot (FR-022)

Sent on initial WebSocket connection.

**JSON Schema**:
```json
{
  "event": "fullSnapshot",
  "version": 1,
  "timestamp": 123456789,
  "sources": {
    "GPS": {
      "PGN129025": [
        {
          "sourceId": "NMEA2000-42",
          "protocol": "NMEA2000",
          "frequency": 10.1,
          "timeSinceLast": 98,
          "isStale": false
        }
      ],
      "GGA": [
        {
          "sourceId": "NMEA0183-VH",
          "protocol": "NMEA0183",
          "frequency": 1.0,
          "timeSinceLast": 1020,
          "isStale": false
        }
      ]
    },
    "Compass": {
      "PGN127250": [
        {
          "sourceId": "NMEA2000-10",
          "protocol": "NMEA2000",
          "frequency": 10.0,
          "timeSinceLast": 100,
          "isStale": false
        }
      ],
      "HDM": [
        {
          "sourceId": "NMEA0183-AP",
          "protocol": "NMEA0183",
          "frequency": 1.0,
          "timeSinceLast": 985,
          "isStale": false
        }
      ]
    }
  }
}
```

**Field Types**:
- `event`: string ("fullSnapshot")
- `version`: integer (schema version, currently 1)
- `timestamp`: integer (millis() timestamp)
- `sources`: object (category names as keys)
- `frequency`: number (Hz, 1 decimal place)
- `timeSinceLast`: integer (milliseconds)
- `isStale`: boolean

---

### 2. Delta Update (FR-023)

Sent every 500ms if sources changed.

**JSON Schema**:
```json
{
  "event": "deltaUpdate",
  "timestamp": 123457289,
  "changes": [
    {
      "sourceId": "NMEA2000-42",
      "frequency": 10.2,
      "timeSinceLast": 102,
      "isStale": false
    },
    {
      "sourceId": "NMEA2000-10",
      "timeSinceLast": 600,
      "isStale": false
    }
  ]
}
```

**Field Rules**:
- Include only changed sources
- Include only changed fields per source
- `sourceId` is always present (key field)
- Omit unchanged fields (e.g., if only `timeSinceLast` changed, omit `frequency`)

---

### 3. Source Removed (FR-021)

Sent when garbage collection removes a source.

**JSON Schema**:
```json
{
  "event": "sourceRemoved",
  "sourceId": "NMEA2000-42",
  "timestamp": 123456789,
  "reason": "stale"
}
```

**Reason Codes**:
- `"stale"`: No updates for 5+ minutes
- `"evicted"`: Removed due to 50-source limit

---

## Calculations

### Frequency Calculation

**Algorithm** (implemented in FrequencyCalculator utility):
```cpp
double calculateFrequency(const uint32_t* buffer, uint8_t count) {
    if (count < 2) return 0.0;

    // Calculate average interval between samples
    uint32_t totalInterval = buffer[count - 1] - buffer[0];
    double avgInterval = (double)totalInterval / (count - 1);

    // Convert to Hz
    return (avgInterval > 0) ? 1000.0 / avgInterval : 0.0;
}
```

**Example**:
- Buffer: [1000, 1100, 1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900] ms
- Total interval: 1900 - 1000 = 900 ms
- Avg interval: 900 / 9 = 100 ms
- Frequency: 1000 / 100 = 10.0 Hz

**Edge Cases**:
- <2 samples: Return 0.0 Hz
- Rollover (millis() wraps at 49.7 days): Incorrect for one buffer cycle, then self-corrects
- Zero interval: Return 0.0 Hz (guard against division by zero)

---

### Staleness Detection

**Logic**:
```cpp
void updateStaleFlags(SourceRegistry* registry) {
    uint32_t now = millis();
    for (each source) {
        source.timeSinceLast = now - source.lastUpdateTime;
        source.isStale = (source.timeSinceLast > 5000);
    }
}
```

**Timing**:
- Updated every 500ms during WebSocket batch cycle
- Threshold: 5000ms (5 seconds)
- Hysteresis: Source becomes fresh immediately on new message

---

## Category-to-Message Mappings

**Static Initialization Table** (`SourceRegistry::init()`):

```cpp
static const struct {
    CategoryType category;
    const char* messageTypeId;
    ProtocolType protocol;
} MESSAGE_MAPPINGS[] = {
    // GPS (7 types)
    {CategoryType::GPS, "PGN129025", ProtocolType::NMEA2000},
    {CategoryType::GPS, "PGN129026", ProtocolType::NMEA2000},
    {CategoryType::GPS, "PGN129029", ProtocolType::NMEA2000},
    {CategoryType::GPS, "PGN127258", ProtocolType::NMEA2000},
    {CategoryType::GPS, "GGA", ProtocolType::NMEA0183},
    {CategoryType::GPS, "RMC", ProtocolType::NMEA0183},
    {CategoryType::GPS, "VTG", ProtocolType::NMEA0183},

    // Compass (5 types)
    {CategoryType::COMPASS, "PGN127250", ProtocolType::NMEA2000},
    {CategoryType::COMPASS, "PGN127251", ProtocolType::NMEA2000},
    {CategoryType::COMPASS, "PGN127252", ProtocolType::NMEA2000},
    {CategoryType::COMPASS, "PGN127257", ProtocolType::NMEA2000},
    {CategoryType::COMPASS, "HDM", ProtocolType::NMEA0183},

    // Wind (1 type)
    {CategoryType::WIND, "PGN130306", ProtocolType::NMEA2000},

    // DST (3 types)
    {CategoryType::DST, "PGN128267", ProtocolType::NMEA2000},
    {CategoryType::DST, "PGN128259", ProtocolType::NMEA2000},
    {CategoryType::DST, "PGN130316", ProtocolType::NMEA2000},

    // Rudder (1 type)
    {CategoryType::RUDDER, "RSA", ProtocolType::NMEA0183},

    // Engine (2 types)
    {CategoryType::ENGINE, "PGN127488", ProtocolType::NMEA2000},
    {CategoryType::ENGINE, "PGN127489", ProtocolType::NMEA2000},
};
```

Total: 19 message types across 5 categories (GPS, Compass, Wind, DST, Rudder, Engine)

---

## Memory Budget Analysis

### Per-Source Memory

```
MessageSource:
  - sourceId[20]           20 bytes
  - sid                     1 byte
  - talkerId[4]             4 bytes
  - protocol                1 byte
  - frequency               8 bytes (double)
  - timeSinceLast           4 bytes
  - isStale                 1 byte
  - lastUpdateTime          4 bytes
  - updateCount             2 bytes
  - timestampBuffer[10]    40 bytes
  - bufferIndex             1 byte
  - bufferFull              1 byte
  - active                  1 byte
  - padding                ~7 bytes (alignment)
                          --------
  TOTAL:                  ~95 bytes
```

### Total Registry Memory (50 sources)

```
Worst case: 50 sources evenly distributed
- 9 categories × 8 message types × 5 sources = theoretical max 360 sources
- Actual limit: 50 sources total

50 sources × 95 bytes = 4,750 bytes (~4.6 KB)
+ CategoryEntry overhead (9 × 16 bytes) = 144 bytes
+ MessageTypeEntry overhead (19 × 20 bytes) = 380 bytes
                                           --------
TOTAL:                                     ~5.3 KB
```

✅ **Within 10KB budget** (SC-007)

### JSON Payload Size

**Full Snapshot** (50 sources):
- Per source: ~150 bytes JSON
- Total: 50 × 150 = 7,500 bytes (~7.3 KB)

⚠️ **Exceeds 5KB target** (SC-010)

**Mitigation**:
- Reduce MAX_SOURCES to 30: 30 × 150 = 4,500 bytes (~4.4 KB) ✅
- Or use gzip compression (AsyncWebSocket supports it)

**Delta Update** (typical 5 sources):
- Per source: ~120 bytes JSON
- Total: 5 × 120 = 600 bytes

✅ **Well within budget**

---

## Thread Safety

**Single-Threaded Model**:
- ReactESP event loop guarantees serial execution
- No mutex/semaphore required
- All updates occur in main loop context

**Invariant**:
- No concurrent access to `SourceRegistry` from multiple tasks

---

## Persistence

**RAM-Only** (FR-014):
- No flash storage for statistics
- Statistics reset on reboot
- Source preferences (future feature) would use LittleFS

---

## Validation Rules

**Source ID Format**:
- NMEA2000: `sprintf(sourceId, "NMEA2000-%u", sid);`
- NMEA0183: `sprintf(sourceId, "NMEA0183-%s", talkerId);`

**Validation**:
- `sid ≤ 252` (253-255 reserved in NMEA2000)
- `talkerId` must be 2 uppercase letters (A-Z)
- `frequency ≥ 0` and `frequency ≤ 20` Hz (realistic NMEA range)
- `timeSinceLast ≥ 0`

---

## Next Steps

Proceed to Phase 1 (Contracts) to define:
- SourceRegistry interface
- FrequencyCalculator utility
- SourceStatsSerializer interface
- WebSocket handler interface
