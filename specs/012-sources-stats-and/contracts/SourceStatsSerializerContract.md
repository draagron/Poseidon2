# Contract: SourceStatsSerializer

**Component**: Source Statistics Tracking
**Feature**: 012-sources-stats-and
**Version**: 1.0.0

## Purpose

SourceStatsSerializer converts SourceRegistry data structures into JSON format for WebSocket transmission. It supports three message types: full snapshots (initial connection), delta updates (periodic changes), and removal events (garbage collection).

## Interface

```cpp
class SourceStatsSerializer {
public:
    /**
     * @brief Serialize complete registry to JSON (full snapshot)
     *
     * Generates FR-022 compliant JSON with all active sources organized by
     * category and message type.
     *
     * @param registry SourceRegistry instance (must not be nullptr)
     *
     * @return JSON string (~4.5KB for 30 sources), empty string on error
     *
     * @post registry unchanged (const access)
     *
     * @note Called once per WebSocket client connection
     * @note Memory: Uses 4096-byte static buffer (no heap)
     * @note Performance: <50ms on ESP32 @ 240 MHz for 30 sources
     *
     * @example
     * String json = SourceStatsSerializer::toFullSnapshotJSON(&sourceRegistry);
     * wsSourceStats.textAll(json);
     */
    static String toFullSnapshotJSON(const SourceRegistry* registry);

    /**
     * @brief Serialize changed sources to JSON (delta update)
     *
     * Generates FR-023 compliant JSON with only sources that have changed
     * since last serialization. Includes only modified fields per source.
     *
     * @param registry SourceRegistry instance (must not be nullptr)
     * @param changedSources Array of source IDs that changed (max 50)
     * @param changeCount Number of changed sources
     *
     * @return JSON string (~600 bytes for 5 sources), empty string on error
     *
     * @post registry unchanged (const access)
     *
     * @note Called every 500ms if registry->hasChanges() == true
     * @note Memory: Uses 2048-byte static buffer
     * @note Performance: <20ms for typical 5 changed sources
     *
     * @example
     * if (sourceRegistry.hasChanges()) {
     *     String json = SourceStatsSerializer::toDeltaJSON(&sourceRegistry);
     *     wsSourceStats.textAll(json);
     *     sourceRegistry.clearChangeFlag();
     * }
     */
    static String toDeltaJSON(const SourceRegistry* registry);

    /**
     * @brief Serialize source removal event (garbage collection)
     *
     * Generates FR-021 compliant JSON for removed sources.
     *
     * @param sourceId Source identifier that was removed
     * @param reason Removal reason ("stale" or "evicted")
     *
     * @return JSON string (~100 bytes)
     *
     * @note Called by SourceRegistry::garbageCollect() and ::evictOldestSource()
     * @note Memory: Uses stack allocation only (~200 bytes)
     *
     * @example
     * String json = SourceStatsSerializer::toRemovalJSON("NMEA2000-42", "stale");
     * wsSourceStats.textAll(json);
     */
    static String toRemovalJSON(const char* sourceId, const char* reason);

private:
    // Static JSON buffer sizes
    static constexpr size_t FULL_SNAPSHOT_BUFFER_SIZE = 4096;
    static constexpr size_t DELTA_UPDATE_BUFFER_SIZE = 2048;

    /**
     * @brief Serialize a single source to JSON object
     *
     * @param doc ArduinoJson document
     * @param source MessageSource structure
     */
    static void serializeSource(JsonObject& obj, const MessageSource& source);

    /**
     * @brief Get protocol name string
     *
     * @param protocol ProtocolType enum value
     * @return "NMEA2000" or "NMEA0183"
     */
    static const char* getProtocolName(ProtocolType protocol);

    /**
     * @brief Get category name string
     *
     * @param category CategoryType enum value
     * @return "GPS", "Compass", "Wind", etc.
     */
    static const char* getCategoryName(CategoryType category);
};
```

## JSON Output Formats

### Full Snapshot (FR-022)

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
      ]
    }
  }
}
```

**Size Calculation** (30 sources):
- Header: ~80 bytes
- Per category (5 active): ~30 bytes each = 150 bytes
- Per message type (15 active): ~20 bytes each = 300 bytes
- Per source (30): ~150 bytes each = 4500 bytes
- **Total**: ~5,030 bytes (~4.9 KB)

**Optimization** (if exceeds 4KB buffer):
- Reduce MAX_SOURCES to 25: 25 × 150 = 3,750 bytes + overhead = ~4.1 KB ✅

### Delta Update (FR-023)

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
      "timeSinceLast": 600
    }
  ]
}
```

**Field Inclusion Rules**:
- Always include: `sourceId`, `timestamp`
- Include only if changed: `frequency`, `timeSinceLast`, `isStale`
- Omit unchanged fields

**Size Calculation** (typical 5 sources):
- Header: ~60 bytes
- Per changed source: ~120 bytes average
- **Total**: 5 × 120 + 60 = ~660 bytes

### Source Removed (FR-021)

```json
{
  "event": "sourceRemoved",
  "sourceId": "NMEA2000-42",
  "timestamp": 123456789,
  "reason": "stale"
}
```

**Size**: ~100 bytes

## Implementation Details

### ArduinoJson Usage

**Buffer Allocation**:
```cpp
StaticJsonDocument<FULL_SNAPSHOT_BUFFER_SIZE> doc;
```

**Serialization Pattern**:
```cpp
JsonObject sources = doc.createNestedObject("sources");
JsonObject gpsCategory = sources.createNestedObject("GPS");
JsonArray pgn129025Array = gpsCategory.createNestedArray("PGN129025");

for (each source in PGN129025) {
    JsonObject sourceObj = pgn129025Array.createNestedObject();
    sourceObj["sourceId"] = source.sourceId;
    sourceObj["protocol"] = getProtocolName(source.protocol);
    sourceObj["frequency"] = round(source.frequency * 10) / 10.0;  // 1 decimal
    sourceObj["timeSinceLast"] = source.timeSinceLast;
    sourceObj["isStale"] = source.isStale;
}

String output;
serializeJson(doc, output);
return output;
```

### Frequency Formatting

**Precision**: 1 decimal place (e.g., 10.1 Hz)

**Rounding**:
```cpp
double formatted = round(source.frequency * 10) / 10.0;
```

### Delta Detection

**Changed Fields Logic**:
```cpp
// In SourceRegistry, track previous state per source
struct SourcePreviousState {
    double frequency;
    uint32_t timeSinceLast;
    bool isStale;
};

// On serialization
bool changed = (current.frequency != previous.frequency) ||
               (abs(current.timeSinceLast - previous.timeSinceLast) > 100) ||  // 100ms threshold
               (current.isStale != previous.isStale);
```

**Alternative Approach** (simpler):
- Include all sources with `timeSinceLast` updated in last 500ms
- Include all sources with `isStale` flag that changed
- Include all sources with `frequency` that changed by >0.1 Hz

## Usage Example

```cpp
// In main.cpp setup()
AsyncWebSocket wsSourceStats("/source-stats");
server.addHandler(&wsSourceStats);

// On WebSocket client connect
wsSourceStats.onEvent([](AsyncWebSocket* server, AsyncWebSocketClient* client,
                         AwsEventType type, void* arg, uint8_t* data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        // Send full snapshot to new client
        String json = SourceStatsSerializer::toFullSnapshotJSON(&sourceRegistry);
        client->text(json);
    }
});

// In ReactESP loop (500ms)
app.onRepeat(500, [&]() {
    sourceRegistry.updateStaleFlags();

    if (sourceRegistry.hasChanges()) {
        String json = SourceStatsSerializer::toDeltaJSON(&sourceRegistry);
        wsSourceStats.textAll(json);
        sourceRegistry.clearChangeFlag();
    }
});

// In SourceRegistry::garbageCollect()
void SourceRegistry::garbageCollect() {
    for (each stale source) {
        String json = SourceStatsSerializer::toRemovalJSON(source.sourceId, "stale");
        wsSourceStats.textAll(json);  // Global access via extern
        removeSource(source.sourceId);
    }
}
```

## Performance Characteristics

**`toFullSnapshotJSON()` (30 sources)**:
- Time: ~40ms on ESP32 @ 240 MHz
- Memory: 4096 bytes static buffer
- Heap: 0 bytes (ArduinoJson uses provided buffer)

**`toDeltaJSON()` (5 sources)**:
- Time: ~15ms
- Memory: 2048 bytes static buffer

**`toRemovalJSON()`**:
- Time: <5ms
- Memory: ~200 bytes stack

## Testing Strategy

**Unit Tests** (`test/test_source_stats_units/SourceStatsSerializerTest.cpp`):

```cpp
void test_full_snapshot_format() {
    // Mock SourceRegistry with 3 sources
    SourceRegistry registry;
    registry.recordUpdate(CategoryType::GPS, "PGN129025", "NMEA2000-42", ProtocolType::NMEA2000);
    // ... add more sources

    String json = SourceStatsSerializer::toFullSnapshotJSON(&registry);

    // Parse JSON and verify structure
    StaticJsonDocument<4096> doc;
    deserializeJson(doc, json);

    assert(doc["event"] == "fullSnapshot");
    assert(doc["version"] == 1);
    assert(doc["sources"]["GPS"]["PGN129025"][0]["sourceId"] == "NMEA2000-42");
}

void test_delta_update_includes_only_changes() {
    // Create registry with 5 sources
    // Modify 2 sources
    // Serialize delta
    // Verify only 2 sources in "changes" array
}

void test_removal_event_format() {
    String json = SourceStatsSerializer::toRemovalJSON("NMEA2000-42", "stale");

    StaticJsonDocument<256> doc;
    deserializeJson(doc, json);

    assert(doc["event"] == "sourceRemoved");
    assert(doc["sourceId"] == "NMEA2000-42");
    assert(doc["reason"] == "stale");
}
```

**Integration Tests** (`test/test_source_stats_integration/`):
- Create SourceRegistry with 30 sources
- Serialize full snapshot
- Verify JSON size <4096 bytes
- Verify all sources present in output

## Dependencies

**Required**:
- `ArduinoJson` v6.x (already in use)
- `SourceRegistry` (read-only access)
- `CategoryType`, `ProtocolType` enums

**Used By**:
- WebSocket event handler (full snapshot on connect)
- ReactESP periodic task (delta updates every 500ms)
- SourceRegistry (removal events during GC)

## Error Handling

**Null Pointer**:
- Input: `registry == nullptr`
- Action: Return empty string
- Log: WebSocketLogger at ERROR level

**Buffer Overflow**:
- Condition: JSON exceeds static buffer size
- Detection: `ArduinoJson` returns incomplete serialization
- Action: Return empty string
- Log: WebSocketLogger at ERROR level with size information

**Invalid Data**:
- Condition: Category or protocol enum out of range
- Action: Use fallback string ("Unknown")
- Log: WebSocketLogger at WARN level

## Future Enhancements

**Considered but Deferred**:
- Compression (gzip) for full snapshots >4KB
- Binary format (MessagePack) for lower overhead
- Pagination for large source counts (>50)
- Client-side filtering (send only requested categories)

**Rationale for JSON**:
- Human-readable for debugging
- Standard web format
- ArduinoJson library well-tested
- Sufficient performance for 500ms update rate
