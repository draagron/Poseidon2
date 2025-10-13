# Research: Source Statistics Tracking and WebUI

**Feature**: 012-sources-stats-and
**Date**: 2025-10-13
**Spec**: [spec.md](./spec.md)

## Existing Architecture Analysis

### NMEA Message Handling

**NMEA 2000 PGNs (13 total)**:
- Location: `src/components/NMEA2000Handlers.h`
- PGNs: 129025, 129026, 129029, 127258, 127250, 127251, 127252, 127257, 128267, 128259, 130316, 127488, 127489, 130306
- Architecture: Handler functions receive `tN2kMsg`, extract SID from message, update BoatData
- Source identification: SID (Source Identifier) available from PGN message header
- Registration: `RegisterN2kHandlers()` called during setup

**NMEA 0183 Sentences (5 total)**:
- Location: `src/components/NMEA0183Handler.h`
- Sentences: RSA, HDM, GGA, RMC, VTG
- Architecture: NMEA0183Handler class processes sentences in ReactESP loop
- Source identification: Talker ID (first 2 characters, e.g., "AP", "VH")
- Dispatch table: Static HandlerEntry array maps message codes to handler functions

### BoatData Organization

**Data Categories**:
- Location: `src/types/BoatDataTypes.h`
- Categories: GPS, Compass, Wind, DST, Rudder, Engine, Saildrive, Battery, ShorePower
- Structure: Flat structure with 9 sensor groups
- Each group has: data fields, `available` flag, `lastUpdate` timestamp

**Existing Multi-Source Support**:
- `SourceManager` structure already defined in BoatDataTypes.h:265-380
- Tracks GPS and Compass sources with SensorSource metadata
- Automatic frequency-based prioritization implemented
- MAX_GPS_SOURCES = 5, MAX_COMPASS_SOURCES = 5

### WebSocket Infrastructure

**WebSocketLogger**:
- Location: `src/utils/WebSocketLogger.h`
- Endpoint: `ws://<ESP32_IP>/logs`
- Features: Log filtering, level control, JSON formatting
- Persistence: Filter settings saved to `/log-filter.json`
- Broadcast mechanism: `broadcastLog()` sends to all connected clients

**BoatDataSerializer**:
- Location: `src/components/BoatDataSerializer.h`
- Function: `toJSON()` serializes complete BoatData structure
- Buffer: 2048 bytes static allocation
- Performance: <50ms on ESP32 @ 240 MHz

**ESPAsyncWebServer**:
- Already in use for HTTP endpoints and WebSocket
- Serving HTML from LittleFS (example: `/stream` endpoint with `data/stream.html`)

### Node.js Proxy

**Current Implementation**:
- Location: `nodejs-boatdata-viewer/server.js`
- Architecture: WebSocket proxy relay between ESP32 and browser clients
- Endpoints: `/boatdata` (WebSocket), `/stream.html` (dashboard)
- Features: Auto-reconnect, status updates, multi-client support
- Configuration: `config.json` with ESP32 IP/port settings

### ReactESP Event Loop

**Pattern**:
- Asynchronous event-driven architecture
- Non-blocking periodic tasks
- Example: `app.onRepeat(10, []() { handler.processSentences(); });`
- Budget: NMEA0183 processing <50ms per cycle

### Test Organization

**PlatformIO Test Structure**:
- Location: `test/` directory
- Naming: `test_<feature>_<type>/` (contracts, integration, units, hardware)
- Examples: `test_nmea0183_contracts/`, `test_boatdata_contracts/`
- Helpers: `test/helpers/` (not a test, shared utilities)
- Execution: `pio test -e native -f test_<pattern>`

## Technology Stack Compatibility

### Libraries

**Pre-approved and Available**:
- NMEA2000 (ttlappalainen/NMEA2000): PGN parsing, SID extraction
- NMEA0183 (ttlappalainen/NMEA0183): Sentence parsing, talker ID extraction
- ESPAsyncWebServer: WebSocket and HTTP endpoints
- ArduinoJson v6.x: JSON serialization (used in BoatDataSerializer)
- ReactESP: Event loop management

**Additional Requirements**:
- None - all required libraries already in use

### Platform Constraints

**ESP32 Memory**:
- Target: <10KB RAM for source statistics (SC-007)
- Calculation: 50 sources × ~200 bytes/source = ~10KB
- Static allocation required (no dynamic memory)
- JSON payload target: <5KB (SC-010)

**Network Limits**:
- WebSocket clients: Minimize ESP32 client count (use Node.js proxy for multi-user)
- Batching: 500ms update interval to reduce message frequency (FR-024)

## Data Flow Analysis

### Message Ingestion Points

**NMEA 2000**:
- Entry point: Handler functions in NMEA2000Handlers.cpp
- Message frequency: Varies by PGN (1Hz - 10Hz)
- SID availability: Yes, from tN2kMsg structure

**NMEA 0183**:
- Entry point: `NMEA0183Handler::processSentences()`
- Message frequency: Typically 1Hz per sentence type
- Talker ID: Extracted from message header

### Update Propagation

**Current Flow**:
```
NMEA Handler → BoatData Update → WebSocket Serialization → Broadcast
```

**New Flow (with source tracking)**:
```
NMEA Handler → SourceRegistry Update → BoatData Update → WebSocket Serialization → Broadcast
              ↓
         Statistics Update → Batched Delta → WebSocket Broadcast (500ms)
```

## Pattern Recommendations

### Data Structure Design

**Hierarchical Organization** (per FR-015):
```
Category → Message Type → Source → {frequency, timeSinceLast, isStale}
```

**Memory Optimization**:
- Use fixed-size char arrays for source IDs (e.g., `char sourceId[20]`)
- Pack booleans into bitfields where possible
- Use `uint32_t` for timestamps (millis() rolls over at 49.7 days, acceptable)
- Use `uint16_t` for counts where maximum <65k

### Frequency Calculation

**Circular Buffer**:
- Size: 10 timestamps (FR-005)
- Type: `uint32_t[10]` per source (40 bytes)
- Calculation: Average interval = (last_timestamp - first_timestamp) / 9
- Frequency (Hz) = 1000.0 / average_interval_ms

### WebSocket Message Design

**Full Snapshot** (FR-022):
```json
{
  "event": "fullSnapshot",
  "version": 1,
  "timestamp": 123456789,
  "sources": {
    "GPS": {
      "PGN129025": [
        {"sourceId": "NMEA2000-42", "frequency": 10.1, "timeSinceLast": 98, "isStale": false}
      ]
    }
  }
}
```

**Delta Update** (FR-023):
```json
{
  "event": "sourceUpdate",
  "sourceId": "NMEA2000-42",
  "timestamp": 123456789,
  "changes": {"frequency": 10.2, "timeSinceLast": 100}
}
```

**Removal Event** (FR-021):
```json
{
  "event": "sourceRemoved",
  "sourceId": "NMEA2000-42",
  "timestamp": 123456789
}
```

### Garbage Collection Strategy

**Trigger Conditions**:
1. Periodic check every 60 seconds (ReactESP timer)
2. Source count reaches 50 (immediate eviction)

**Eviction Priority**:
1. Stale >5 minutes: Remove immediately
2. Max limit reached: Evict oldest by `timeSinceLast`

## Integration Points

### Code Modification Areas

**New Components** (to be created):
- `src/components/SourceStatistics.h/cpp`: Core statistics tracking
- `src/components/SourceRegistry.h/cpp`: Source management and garbage collection
- `src/components/FrequencyCalculator.h/cpp`: Rolling average calculation
- `src/components/SourceStatsSerializer.h/cpp`: JSON serialization for statistics
- `src/components/SourceStatsHandler.h/cpp`: WebSocket endpoint handler

**Modifications to Existing Code**:
- `src/components/NMEA2000Handlers.cpp`: Add `SourceRegistry::recordUpdate()` calls
- `src/components/NMEA0183Handler.cpp`: Add `SourceRegistry::recordUpdate()` calls
- `src/main.cpp`: Register new WebSocket endpoint, initialize SourceRegistry

**New Files**:
- `data/sources.html`: Source statistics dashboard
- `nodejs-boatdata-viewer/public/sources.html`: Proxy relay version

### HAL Abstraction Requirements

**No New HAL Interfaces Needed**:
- Statistics component is pure C++ business logic
- WebSocket provided by ESPAsyncWebServer (already abstracted)
- No direct hardware interaction

**Testability**:
- Mock SourceRegistry for unit tests
- Mock WebSocket for integration tests
- Native environment tests for frequency calculations

## Risk Assessment

### Memory Constraints

**Risk**: 50 sources × ~200 bytes = ~10KB may exceed budget
**Mitigation**:
- Pack data structures aggressively
- Monitor heap usage during testing
- Reduce MAX_SOURCES if needed (e.g., 30 sources)

### WebSocket Throughput

**Risk**: Batched updates every 500ms may still overwhelm ESP32
**Mitigation**:
- Only send deltas (changed fields)
- Implement client-side filtering if needed
- Use Node.js proxy for multiple clients

### CPU Budget

**Risk**: Frequency calculations and staleness checks add CPU overhead
**Mitigation**:
- Lazy calculation (only when requested via WebSocket)
- Staleness check only during periodic GC cycle (60s)
- Frequency calculation using simple circular buffer (O(1) insert, O(1) average)

## Dependencies and Prerequisites

**Required Before Implementation**:
- None - all infrastructure in place

**Optional Enhancements** (post-MVP):
- Priority-based source selection (already supported by SourceManager)
- Persistent source preferences (LittleFS storage)
- Manual source override via WebUI

## Example Code Reference

**Existing Serialization Pattern** (BoatDataSerializer.cpp):
```cpp
static void serializeGPS(JsonDocument& doc, const GPSData& data) {
    JsonObject gps = doc.createNestedObject("gps");
    gps["latitude"] = data.latitude;
    gps["longitude"] = data.longitude;
    gps["available"] = data.available;
    gps["lastUpdate"] = data.lastUpdate;
}
```

**Existing WebSocket Broadcast** (WebSocketLogger.cpp):
```cpp
void WebSocketLogger::broadcastLog(LogLevel level, const char* component,
                                   const char* event, const String& data) {
    if (!isInitialized || !ws) return;
    String message = buildLogMessage(level, component, event, data);
    ws->textAll(message);
}
```

**Existing ReactESP Timer** (main.cpp pattern):
```cpp
app.onRepeat(200, []() {
    // Periodic calculation task
});
```

## Success Metrics from Spec

**Measurable**:
- SC-001: Track 5+ NMEA 2000 + 2 NMEA 0183 sources simultaneously
- SC-002: Frequency accuracy ±10%
- SC-003: Staleness detection within 5.5s
- SC-007: Memory <10KB for 20 sources

**Performance**:
- SC-004: WebSocket batching 500ms ±50ms
- SC-005: Dashboard loads <2s
- SC-006: Visual updates <200ms latency

## Conclusion

**Feasibility**: HIGH
- All required infrastructure exists
- No new libraries or HAL abstractions needed
- Clear integration points identified
- Memory budget achievable with careful design

**Complexity**: MEDIUM
- Hierarchical data structure (3 levels)
- Circular buffer for frequency calculation
- WebSocket delta updates with batching
- Garbage collection with eviction logic

**Recommended Approach**:
1. Implement SourceRegistry with static allocation
2. Add FrequencyCalculator utility class
3. Integrate hooks into NMEA handlers
4. Create SourceStatsSerializer for JSON output
5. Add WebSocket endpoint with batched delta updates
6. Implement web dashboard (HTML/JS)
7. Extend Node.js proxy for `/sources` endpoint

**Next Steps**: Proceed to Phase 1 (Data Model and Contracts)
