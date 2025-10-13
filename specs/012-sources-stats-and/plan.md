# Implementation Plan: Source Statistics Tracking and WebUI

**Branch**: `012-sources-stats-and` | **Date**: 2025-10-13 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `/specs/012-sources-stats-and/spec.md`

## Summary

This feature implements real-time tracking and visualization of NMEA 2000/0183 message sources, enabling marine system administrators to diagnose sensor connectivity issues and understand system topology. The implementation provides:

- **Source discovery**: Automatic detection of NMEA sources by SID (NMEA2000) or talker ID (NMEA0183)
- **Frequency tracking**: Rolling 10-sample average for update rate calculation (Hz)
- **Staleness monitoring**: 5-second threshold for detecting inactive sources
- **Garbage collection**: Automatic removal of sources stale >5 minutes
- **WebSocket streaming**: Full snapshot + incremental delta updates (500ms batching)
- **WebUI dashboard**: Real-time visualization organized by BoatData categories
- **Node.js proxy**: Multi-client support via WebSocket relay

**Technical Approach** (from research.md):
- Static memory allocation (~5.3KB for 50 sources)
- Hierarchical data structure (Category → Message Type → Source)
- Circular timestamp buffer for frequency calculation
- ReactESP event-driven architecture (500ms update cycle, 60s GC cycle)

## Technical Context

**Language/Version**: C++14 (Arduino framework for ESP32)
**Primary Dependencies**:
- NMEA2000 (ttlappalainen/NMEA2000) - PGN parsing, SID extraction
- NMEA0183 (ttlappalainen/NMEA0183) - Sentence parsing, talker ID extraction
- ReactESP (mairas/ReactESP) - Event loop management
- ESPAsyncWebServer (ESP32Async) - WebSocket and HTTP endpoints
- ArduinoJson v6.x - JSON serialization

**Storage**: RAM-only (no flash persistence for statistics)
**Testing**: PlatformIO native environment (unit/contract tests), ESP32 hardware (integration tests)
**Target Platform**: ESP32 family (ESP32, ESP32-S2, ESP32-C3, ESP32-S3), Arduino framework, 24/7 always-on operation
**Project Type**: Embedded firmware (single codebase)
**Performance Goals**:
- <10KB RAM for 50 sources
- <50ms WebSocket serialization
- ±10% frequency accuracy
- 500ms ±50ms delta update interval

**Constraints**:
- Static allocation only (no heap fragmentation)
- Single-threaded (ReactESP event loop)
- Always-on WiFi (no sleep modes)
- Network-based debugging (WebSocket logging)

**Scale/Scope**:
- 19 NMEA message types (13 PGNs + 5 sentences + 1 reserved)
- 50 concurrent sources maximum
- 9 BoatData categories
- 5 sources per message type
- ~4.5KB JSON payload (full snapshot, 30 sources)

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

### ✅ I. Hardware Abstraction Layer (HAL)

**Assessment**: PASS - No new HAL interfaces required

**Analysis**:
- Source statistics component is pure C++ business logic
- NMEA message handling already abstracted (NMEA2000/NMEA0183 libraries)
- WebSocket provided by ESPAsyncWebServer (already abstracted)
- No direct hardware I/O in statistics tracking

**Testability**:
- Unit tests in native environment (FrequencyCalculator)
- Mock SourceRegistry for integration tests
- Contract tests verify invariants without hardware

**Existing HAL Usage**:
- ISerialPort (NMEA0183Handler) - already implemented
- NMEA2000/CAN bus - already abstracted via NMEA2000_esp32

### ✅ II. Resource-Aware Development

**Assessment**: PASS - Static allocation with validated budget

**Memory Analysis** (from data-model.md):
```
Per-Source Memory:
  MessageSource struct: ~95 bytes
  - sourceId[20]: 20 bytes
  - timestampBuffer[10]: 40 bytes
  - Other fields: ~35 bytes

Total Registry (50 sources):
  50 sources × 95 bytes = 4,750 bytes
  + CategoryEntry overhead: 144 bytes
  + MessageTypeEntry overhead: 380 bytes
  = 5,274 bytes (~5.3KB)

JSON Buffers:
  Full snapshot buffer: 4,096 bytes (static)
  Delta update buffer: 2,048 bytes (static)
  = 6,144 bytes (~6KB)

TOTAL: ~11.3KB (within ESP32 constraints)
```

**Stack Usage**:
- Main loop operations: <1KB
- JSON serialization: ~500 bytes (ArduinoJson)
- No deep recursion

**Flash Usage**:
- Estimated code size: ~15KB
- HTML dashboard: ~8KB (served from LittleFS)
- Total: ~23KB (acceptable)

**Global Variables**:
- `SourceRegistry sourceRegistry;` (~5.3KB, justified - central authority)
- No other significant globals

### ✅ III. QA-First Review Process

**Assessment**: PASS - Comprehensive test strategy defined

**Test Coverage**:
- Unit tests: FrequencyCalculator, JSON serialization logic
- Contract tests: SourceRegistry invariants, memory bounds
- Integration tests: End-to-end NMEA → WebSocket → Dashboard
- Hardware tests: Minimal (ReactESP timer validation only)

**QA Review Gates**:
- Memory safety: Static allocation, no heap fragmentation
- Resource usage: <10KB RAM validated via calculations
- Error handling: Null pointer checks, buffer overflow guards
- Arduino best practices: F() macro for strings, PROGMEM for constants

### ✅ IV. Modular Component Design

**Assessment**: PASS - Single-responsibility components

**Component Breakdown**:
```
src/components/SourceRegistry.{h,cpp}
  - Responsibility: Source lifecycle management (add/remove/GC)
  - Public interface: recordUpdate(), garbageCollect(), getCategory()
  - Dependencies: CategoryEntry, MessageSource structs

src/components/SourceStatistics.h
  - Responsibility: Data structure definitions (CategoryEntry, MessageSource)
  - No code, header-only structures

src/utils/FrequencyCalculator.{h,cpp}
  - Responsibility: Frequency calculation from circular buffer
  - Stateless utility functions
  - No dependencies (testable in isolation)

src/components/SourceStatsSerializer.{h,cpp}
  - Responsibility: JSON serialization for WebSocket
  - Public interface: toFullSnapshotJSON(), toDeltaJSON(), toRemovalJSON()
  - Dependencies: SourceRegistry (read-only), ArduinoJson

src/components/SourceStatsHandler.{h,cpp}
  - Responsibility: WebSocket endpoint management
  - Handles client connections, broadcasts updates
  - Dependencies: SourceRegistry, SourceStatsSerializer
```

**Dependency Injection**:
- SourceRegistry passed to NMEA handlers (not global access from handlers)
- WebSocketLogger injected into SourceRegistry for removal events
- AsyncWebSocket passed to SourceStatsHandler constructor

### ✅ V. Network-Based Debugging

**Assessment**: PASS - WebSocket logging integrated throughout

**Logging Strategy**:
- **Component**: "SourceRegistry"
  - Events: SOURCE_DISCOVERED, SOURCE_REMOVED, GC_COMPLETE
  - Levels: INFO (discovery), DEBUG (updates), WARN (eviction)

- **Component**: "SourceStatsHandler"
  - Events: CLIENT_CONNECTED, SNAPSHOT_SENT, DELTA_SENT
  - Levels: INFO (client events), DEBUG (message details)

- **Component**: "FrequencyCalculator"
  - Events: FREQUENCY_CALCULATED, BUFFER_FULL
  - Levels: DEBUG only (high frequency, filter in production)

**Example Log Message**:
```json
{
  "level": "INFO",
  "component": "SourceRegistry",
  "event": "SOURCE_DISCOVERED",
  "timestamp": 123456,
  "data": {
    "sourceId": "NMEA2000-42",
    "category": "GPS",
    "messageType": "PGN129025"
  }
}
```

### ✅ VI. Always-On Operation

**Assessment**: PASS - No sleep modes, 24/7 operation

**Design Considerations**:
- ReactESP event loops run continuously
- No blocking operations (all async)
- WebSocket connections maintained indefinitely
- Garbage collection prevents memory leaks during long-term operation

### ✅ VII. Fail-Safe Operation

**Assessment**: PASS - Graceful degradation implemented

**Error Handling**:
- **Source limit reached**: Evict oldest source, continue operation
- **WebSocket disconnect**: Statistics continue accumulating, resync on reconnect
- **JSON serialization fails**: Return empty string, log error, skip update cycle
- **Invalid NMEA data**: Skip source update, log at DEBUG level

**Recovery Mechanisms**:
- Garbage collection removes stale data automatically
- No persistent state to corrupt (RAM-only)
- WebSocket reconnect sends full snapshot (clients self-heal)

### ✅ VIII. Workflow Selection

**Assessment**: PASS - Feature Development workflow

**Workflow**: `/specify` (Feature Development)
- Full specification completed (spec.md)
- Planning phase in progress (this document)
- TDD approach planned (contracts → tests → implementation)

**Rationale**: New functionality, not bug fix/refactor/modification

### No Backward Compatibility Required

**Assessment**: PASS - New feature, no breaking changes

**Compatibility Notes**:
- New WebSocket endpoint (`/source-stats`) - no conflict with existing endpoints
- New HTML dashboard (`/sources`) - no conflict with `/stream`
- No changes to existing BoatData structures
- NMEA handlers extended (backward compatible additions)

## Project Structure

### Documentation (this feature)

```
specs/012-sources-stats-and/
├── spec.md                  # Feature specification (completed)
├── plan.md                  # This file (in progress)
├── research.md              # Phase 0 codebase analysis (completed)
├── data-model.md            # Data structures and schemas (completed)
├── quickstart.md            # Validation guide (completed)
└── contracts/               # Component contracts (completed)
    ├── SourceRegistryContract.md
    ├── FrequencyCalculatorContract.md
    └── SourceStatsSerializerContract.md
```

### Source Code (repository root)

```
src/
├── components/
│   ├── SourceRegistry.h           # [NEW] Source lifecycle management
│   ├── SourceRegistry.cpp          # [NEW] Implementation
│   ├── SourceStatistics.h          # [NEW] Data structures (header-only)
│   ├── SourceStatsSerializer.h     # [NEW] JSON serialization
│   ├── SourceStatsSerializer.cpp   # [NEW] Implementation
│   ├── SourceStatsHandler.h        # [NEW] WebSocket endpoint handler
│   ├── SourceStatsHandler.cpp      # [NEW] Implementation
│   ├── NMEA2000Handlers.cpp        # [MODIFIED] Add recordUpdate() calls
│   └── NMEA0183Handler.cpp         # [MODIFIED] Add recordUpdate() calls
│
├── utils/
│   ├── FrequencyCalculator.h       # [NEW] Stateless frequency calc utility
│   └── FrequencyCalculator.cpp     # [NEW] Implementation
│
├── main.cpp                         # [MODIFIED] Register WebSocket endpoint
└── config.h                         # [MODIFIED] Add MAX_SOURCES constant

test/
├── test_source_stats_units/        # [NEW] Unit tests
│   ├── test_main.cpp
│   ├── FrequencyCalculatorTest.cpp
│   ├── SourceRegistryTest.cpp
│   └── SourceStatsSerializerTest.cpp
│
├── test_source_stats_contracts/    # [NEW] Contract tests
│   ├── test_main.cpp
│   ├── SourceRegistryContractTest.cpp
│   └── MemoryBoundsTest.cpp
│
└── test_source_stats_integration/  # [NEW] Integration tests
    ├── test_main.cpp
    ├── EndToEndTest.cpp
    └── WebSocketIntegrationTest.cpp

data/
└── sources.html                     # [NEW] WebUI dashboard

nodejs-boatdata-viewer/
├── server.js                        # [MODIFIED] Add /source-stats relay
├── public/
│   └── sources.html                 # [NEW] Proxy dashboard
└── config.json                      # [MODIFIED] Add source-stats endpoint
```

**Structure Decision**: Single embedded project with ESP32-specific components, web frontend served from LittleFS, optional Node.js proxy for multi-client support.

## Complexity Tracking

*This section is empty - no constitutional violations to justify.*

## Phase 0: Research (COMPLETED)

✅ **Artifacts Generated**:
- [research.md](./research.md)

**Key Findings**:
- All required infrastructure exists (libraries, WebSocket, ReactESP)
- SourceManager structure already defined in BoatDataTypes.h (unused, can be referenced)
- Test organization follows PlatformIO grouped test pattern
- Node.js proxy architecture established, easily extensible

**Risks Identified**:
- JSON payload may exceed 4KB for 50 sources → Mitigated: Reduce MAX_SOURCES to 30
- WebSocket throughput with ESP32 → Mitigated: Use Node.js proxy for multi-client
- Frequency calculation accuracy with jitter → Mitigated: 10-sample rolling average

## Phase 1: Design (COMPLETED)

✅ **Artifacts Generated**:
- [data-model.md](./data-model.md)
- [contracts/SourceRegistryContract.md](./contracts/SourceRegistryContract.md)
- [contracts/FrequencyCalculatorContract.md](./contracts/FrequencyCalculatorContract.md)
- [contracts/SourceStatsSerializerContract.md](./contracts/SourceStatsSerializerContract.md)
- [quickstart.md](./quickstart.md)

**Design Decisions**:

1. **Hierarchical Data Model**:
   - Category (9) → Message Type (19) → Source (5 per type, 50 total)
   - Rationale: Matches BoatData organization, intuitive for users
   - Trade-off: O(n) search vs O(1) hash map (acceptable for n≤50)

2. **Circular Buffer (10 samples)**:
   - Rationale: Balance between smoothing and responsiveness
   - Alternative: 5 samples (faster response, less smoothing) - rejected
   - Alternative: 20 samples (smoother, slower response) - rejected

3. **Static Allocation**:
   - Rationale: Constitutional requirement, predictable memory
   - Trade-off: Cannot exceed MAX_SOURCES limit
   - Mitigation: Eviction strategy when limit reached

4. **WebSocket Delta Updates**:
   - Rationale: Reduce bandwidth vs sending full snapshots every 500ms
   - Alternative: Full snapshots every cycle - rejected (high bandwidth)
   - Alternative: Client polling HTTP - rejected (higher latency)

5. **500ms Batching**:
   - Rationale: Balance between latency and message frequency
   - Alternative: 200ms (lower latency, higher CPU) - rejected
   - Alternative: 1000ms (lower CPU, higher latency) - rejected

## Phase 2: Implementation Plan

### Task Organization

Implementation follows TDD approach: Contracts → Tests → Implementation

**Dependency Order**:
1. Data structures (SourceStatistics.h)
2. Utility functions (FrequencyCalculator)
3. Core logic (SourceRegistry)
4. Serialization (SourceStatsSerializer)
5. WebSocket handler (SourceStatsHandler)
6. NMEA integration (modify existing handlers)
7. WebUI dashboard
8. Node.js proxy extension

**Estimated Effort**: 3-4 development days
- Day 1: Core components (SourceRegistry, FrequencyCalculator)
- Day 2: Serialization and WebSocket handler
- Day 3: NMEA integration and testing
- Day 4: WebUI dashboard and Node.js proxy

### Implementation Tasks

**NOTE**: Detailed task breakdown created by `/tasks` command (tasks.md), not included in plan.md per Spec Kit workflow.

**High-Level Task Groups**:

1. **Data Structures** (1-2 hours)
   - Define SourceStatistics.h structures
   - Write unit tests for size validation
   - Document memory layout

2. **Frequency Calculator** (2-3 hours)
   - Implement calculate() function
   - Implement addTimestamp() helper
   - Write unit tests (10Hz, 1Hz, edge cases)
   - Validate accuracy ±10%

3. **Source Registry** (6-8 hours)
   - Implement init(), recordUpdate()
   - Implement garbageCollect(), evictOldestSource()
   - Implement updateStaleFlags()
   - Write contract tests (invariants, memory bounds)
   - Write integration tests (multi-source scenarios)

4. **JSON Serialization** (4-5 hours)
   - Implement toFullSnapshotJSON()
   - Implement toDeltaJSON()
   - Implement toRemovalJSON()
   - Write unit tests (format validation, size checks)
   - Optimize buffer sizes

5. **WebSocket Handler** (3-4 hours)
   - Implement SourceStatsHandler class
   - Handle client connections (send full snapshot)
   - Handle periodic updates (send deltas)
   - Write integration tests (WebSocket communication)

6. **NMEA Integration** (4-5 hours)
   - Modify NMEA2000Handlers.cpp (13 handlers)
   - Modify NMEA0183Handler.cpp (5 handlers)
   - Extract SID/talker ID correctly
   - Test with real NMEA devices
   - Validate source discovery

7. **Main Loop Integration** (2-3 hours)
   - Register /source-stats endpoint
   - Add ReactESP timers (500ms, 60s)
   - Initialize SourceRegistry
   - Wire up WebSocket broadcasts

8. **WebUI Dashboard** (4-6 hours)
   - Create sources.html (HTML/CSS/JS)
   - Implement WebSocket client
   - Implement real-time table updates
   - Add staleness indicators (green/red)
   - Test responsiveness

9. **Node.js Proxy** (2-3 hours)
   - Add /source-stats relay in server.js
   - Copy sources.html to public/
   - Test multi-client support
   - Update configuration

10. **Testing and Validation** (4-6 hours)
    - Run all unit tests
    - Run contract tests
    - Run integration tests
    - Execute quickstart validation guide
    - Performance profiling (memory, CPU)

### Test-First Development Workflow

**For Each Component**:

1. **Write Contract** (if not already written)
   - Define interface signature
   - Document preconditions/postconditions
   - Define invariants

2. **Write Tests**
   ```bash
   # Create test file
   touch test/test_source_stats_units/<Component>Test.cpp

   # Write failing tests
   # Compile (should fail)
   pio test -e native -f test_source_stats_units
   ```

3. **Implement Component**
   ```cpp
   // Minimal implementation to pass tests
   // Refactor for clarity
   ```

4. **Verify Tests Pass**
   ```bash
   pio test -e native -f test_source_stats_units
   ```

5. **Integration Testing**
   ```bash
   # Build for ESP32
   pio run

   # Upload and test
   pio run --target upload
   pio device monitor
   ```

### Code Review Checkpoints

**After Each Major Component**:
1. Run unit tests (must pass 100%)
2. Check memory footprint (add diagnostic logs)
3. Code review (self or peer)
4. Document any deviations from contracts

**Before Final PR**:
1. QA subagent review (mandatory per constitution)
2. Full integration test suite
3. Quickstart validation guide execution
4. Performance profiling report
5. Memory usage report (<10KB verified)

## Integration Strategy

### NMEA Handler Modifications

**Pattern** (example: HandleN2kPGN129025):

```cpp
void HandleN2kPGN129025(const tN2kMsg &N2kMsg, BoatData* boatData,
                        WebSocketLogger* logger, SourceRegistry* registry) {
    unsigned char sid = N2kMsg.Source;  // Extract SID from message

    // Existing parsing logic...
    double latitude, longitude;
    if (ParseN2kPGN129025(N2kMsg, latitude, longitude)) {
        boatData->gps.latitude = latitude;
        boatData->gps.longitude = longitude;
        boatData->gps.lastUpdate = millis();
        boatData->gps.available = true;
    }

    // NEW: Record source statistics
    char sourceId[20];
    snprintf(sourceId, sizeof(sourceId), "NMEA2000-%u", sid);
    registry->recordUpdate(CategoryType::GPS, "PGN129025", sourceId,
                          ProtocolType::NMEA2000);
}
```

**Handler Signature Change**:
- Add parameter: `SourceRegistry* registry`
- Update RegisterN2kHandlers() calls to pass registry pointer

**NMEA 0183 Handler** (similar pattern):

```cpp
void NMEA0183Handler::handleRSA(const tNMEA0183Msg& msg) {
    // Extract talker ID
    char talkerId[4] = {msg.Sender[0], msg.Sender[1], '\0'};

    // Existing parsing logic...

    // NEW: Record source statistics
    char sourceId[20];
    snprintf(sourceId, sizeof(sourceId), "NMEA0183-%s", talkerId);
    registry_->recordUpdate(CategoryType::RUDDER, "RSA", sourceId,
                           ProtocolType::NMEA0183);
}
```

### Main Loop Integration

**setup() additions**:

```cpp
// In main.cpp setup()
#include "components/SourceRegistry.h"
#include "components/SourceStatsHandler.h"

SourceRegistry sourceRegistry;
SourceStatsHandler* sourceStatsHandler;

void setup() {
    // ... existing WiFi, I2C, NMEA2000 setup ...

    // Initialize source registry
    sourceRegistry.init();
    logger.broadcastLog(LogLevel::INFO, "SourceRegistry", "INITIALIZED",
                       "{\"messageTypes\":19}");

    // Register WebSocket endpoint for source statistics
    sourceStatsHandler = new SourceStatsHandler(&wsSourceStats, &sourceRegistry, &logger);
    server.addHandler(&wsSourceStats);

    // Register NMEA handlers with source registry
    RegisterN2kHandlers(&NMEA2000, &boatData, &logger, &sourceRegistry);
    nmea0183Handler.setSourceRegistry(&sourceRegistry);  // Add setter method

    // ... existing ReactESP setup ...
}
```

**ReactESP event loops**:

```cpp
// Existing loops remain unchanged
app.onRepeat(200, []() {
    // Existing: Run calculations
    RunCalculations(&boatData, &calculationContext, &logger);
});

app.onRepeat(100, []() {
    // Existing: Stream boat data
    StreamBoatData(&boatData, &wsBoatData, &logger);
});

// NEW: Source statistics updates (500ms)
app.onRepeat(500, [&]() {
    sourceRegistry.updateStaleFlags();

    if (sourceRegistry.hasChanges()) {
        sourceStatsHandler->sendDeltaUpdate();
        sourceRegistry.clearChangeFlag();
    }
});

// NEW: Garbage collection (60 seconds)
app.onRepeat(60000, [&]() {
    sourceRegistry.garbageCollect();
});
```

### WebSocket Handler Implementation

**SourceStatsHandler class**:

```cpp
// src/components/SourceStatsHandler.h
class SourceStatsHandler {
public:
    SourceStatsHandler(AsyncWebSocket* ws, SourceRegistry* registry,
                      WebSocketLogger* logger);

    void onEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                AwsEventType type, void* arg, uint8_t* data, size_t len);

    void sendDeltaUpdate();
    void sendRemovalEvent(const char* sourceId, const char* reason);

private:
    AsyncWebSocket* ws_;
    SourceRegistry* registry_;
    WebSocketLogger* logger_;

    void sendFullSnapshot(AsyncWebSocketClient* client);
};

// src/components/SourceStatsHandler.cpp
void SourceStatsHandler::onEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                                 AwsEventType type, void* arg, uint8_t* data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        logger_->broadcastLog(LogLevel::INFO, "SourceStatsHandler", "CLIENT_CONNECTED",
                             "{\"clientId\":" + String(client->id()) + "}");
        sendFullSnapshot(client);
    }
    else if (type == WS_EVT_DISCONNECT) {
        logger_->broadcastLog(LogLevel::INFO, "SourceStatsHandler", "CLIENT_DISCONNECTED",
                             "{\"clientId\":" + String(client->id()) + "}");
    }
}

void SourceStatsHandler::sendFullSnapshot(AsyncWebSocketClient* client) {
    String json = SourceStatsSerializer::toFullSnapshotJSON(registry_);
    if (json.length() > 0) {
        client->text(json);
        logger_->broadcastLog(LogLevel::DEBUG, "SourceStatsHandler", "SNAPSHOT_SENT",
                             "{\"size\":" + String(json.length()) + "}");
    }
}

void SourceStatsHandler::sendDeltaUpdate() {
    String json = SourceStatsSerializer::toDeltaJSON(registry_);
    if (json.length() > 0) {
        ws_->textAll(json);
        logger_->broadcastLog(LogLevel::DEBUG, "SourceStatsHandler", "DELTA_SENT",
                             "{\"size\":" + String(json.length()) + "}");
    }
}
```

## Testing Strategy

### Unit Tests (Native Environment)

**FrequencyCalculatorTest.cpp**:
```cpp
#include <unity.h>
#include "utils/FrequencyCalculator.h"

void test_calculate_10hz_source() {
    uint32_t buffer[10] = {1000, 1100, 1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900};
    double freq = FrequencyCalculator::calculate(buffer, 10);
    TEST_ASSERT_FLOAT_WITHIN(0.1, 10.0, freq);  // ±10% tolerance
}

void test_calculate_insufficient_samples() {
    uint32_t buffer[10] = {1000};
    double freq = FrequencyCalculator::calculate(buffer, 1);
    TEST_ASSERT_EQUAL_FLOAT(0.0, freq);
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_calculate_10hz_source);
    RUN_TEST(test_calculate_insufficient_samples);
    UNITY_END();
}

void loop() {}
```

**Run Command**:
```bash
pio test -e native -f test_source_stats_units
```

### Contract Tests

**SourceRegistryContractTest.cpp**:
```cpp
void test_invariant_source_limit() {
    SourceRegistry registry;
    registry.init();

    // Add 50 sources
    for (int i = 0; i < 50; i++) {
        char sourceId[20];
        snprintf(sourceId, sizeof(sourceId), "NMEA2000-%d", i);
        registry.recordUpdate(CategoryType::GPS, "PGN129025", sourceId,
                             ProtocolType::NMEA2000);
    }

    TEST_ASSERT_EQUAL_UINT8(50, registry.getTotalSourceCount());

    // Add 51st source - should evict oldest
    registry.recordUpdate(CategoryType::GPS, "PGN129025", "NMEA2000-99",
                         ProtocolType::NMEA2000);

    TEST_ASSERT_EQUAL_UINT8(50, registry.getTotalSourceCount());  // Still 50
}
```

### Integration Tests

**EndToEndTest.cpp**:
```cpp
void test_nmea_to_websocket_flow() {
    // Setup
    SourceRegistry registry;
    registry.init();

    // Simulate NMEA2000 message
    tN2kMsg msg;
    msg.Source = 42;  // SID
    // ... set message data ...

    // Process message
    HandleN2kPGN129025(msg, &boatData, &logger, &registry);

    // Verify source created
    const MessageSource* source = registry.findSource("NMEA2000-42");
    TEST_ASSERT_NOT_NULL(source);

    // Verify category mapping
    const CategoryEntry* gpsCategory = registry.getCategory(CategoryType::GPS);
    TEST_ASSERT_EQUAL_UINT8(1, gpsCategory->messageCount);

    // Simulate 10 more messages for frequency calculation
    for (int i = 0; i < 10; i++) {
        delay(100);  // 100ms intervals = 10 Hz
        HandleN2kPGN129025(msg, &boatData, &logger, &registry);
    }

    // Verify frequency calculated
    source = registry.findSource("NMEA2000-42");
    TEST_ASSERT_FLOAT_WITHIN(1.0, 10.0, source->frequency);  // ±10% tolerance
}
```

## Deployment Plan

### Build Configuration

**platformio.ini**:
```ini
[env:esp32]
platform = espressif32
board = esp32dev
framework = arduino

build_flags =
    -DMAX_SOURCES=50
    -DSOURCE_STALE_THRESHOLD_MS=5000
    -DSOURCE_GC_THRESHOLD_MS=300000
    -DWEBSOCKET_UPDATE_INTERVAL_MS=500

lib_deps =
    ttlappalainen/NMEA2000
    ttlappalainen/NMEA0183
    ESP32Async/ESPAsyncWebServer
    bblanchon/ArduinoJson@^6.21.0
    mairas/ReactESP

[env:native]
platform = native
test_framework = unity
build_flags = -std=c++14
```

### Firmware Deployment

```bash
# Build production firmware
pio run -e esp32 -t clean
pio run -e esp32

# Upload to ESP32
pio run -e esp32 --target upload

# Monitor output
pio device monitor --baud 115200
```

### LittleFS Deployment

```bash
# Prepare data directory
cp specs/012-sources-stats-and/sources.html data/sources.html

# Upload filesystem
pio run -e esp32 --target uploadfs

# Verify
curl http://192.168.1.100:3030/sources
```

### Node.js Proxy Deployment

```bash
cd nodejs-boatdata-viewer

# Install dependencies
npm install

# Copy dashboard
cp ../specs/012-sources-stats-and/sources.html public/sources.html

# Update config
# Edit config.json: Add "sourceStatsPath": "/source-stats"

# Start server
ESP32_IP=192.168.1.100 npm start

# Verify
curl http://localhost:3000/sources.html
```

## Success Metrics

**From spec.md success criteria**:

| Metric | Target | Validation Method |
|--------|--------|-------------------|
| SC-001 | Track 5+ N2K + 2+ N0183 sources | Integration test with real devices |
| SC-002 | Frequency accuracy ±10% | Unit test with known intervals |
| SC-003 | Staleness detection <5.5s | Integration test with disconnect |
| SC-004 | WebSocket batching 500ms ±50ms | Performance profiling |
| SC-005 | Dashboard loads <2s | Browser network tab |
| SC-006 | Visual updates <200ms | Browser performance tools |
| SC-007 | Memory <10KB for 20 sources | ESP.getFreeHeap() diagnostics |
| SC-008 | GC reclaims memory | Heap monitoring before/after |
| SC-009 | Node.js supports 10+ clients | Load testing |
| SC-010 | JSON payload <5KB | Serialization size checks |
| SC-011 | 90% identify issues <1 min | User acceptance testing |
| SC-012 | Zero discovery failures 24h | Long-term stability test |

## Risks and Mitigation

### Risk 1: Memory Exhaustion

**Risk**: 50 sources × 95 bytes may exceed available RAM

**Likelihood**: Medium
**Impact**: High (system crash)

**Mitigation**:
- Reduce MAX_SOURCES to 30 if needed (validated in data-model.md)
- Add heap monitoring diagnostics
- Test with 50 sources in integration tests
- QA review memory allocation

### Risk 2: WebSocket Throughput

**Risk**: ESP32 WebSocket may drop messages with multiple clients

**Likelihood**: Medium
**Impact**: Medium (clients miss updates)

**Mitigation**:
- Document Node.js proxy as recommended for >1 client
- Test with 3+ clients directly to ESP32
- Add client connection limit (e.g., max 3 direct connections)

### Risk 3: Frequency Calculation Accuracy

**Risk**: NMEA message jitter affects frequency accuracy

**Likelihood**: Low
**Impact**: Low (minor inaccuracy acceptable)

**Mitigation**:
- 10-sample rolling average smooths jitter
- Validate with real NMEA devices (not just unit tests)
- Document acceptable tolerance (±10%)

### Risk 4: CPU Budget

**Risk**: Frequency calculations every 500ms consume too much CPU

**Likelihood**: Low
**Impact**: Medium (delays other tasks)

**Mitigation**:
- Profile CPU usage with 50 active sources
- Optimize FrequencyCalculator if needed
- Use lazy calculation (only when WebSocket clients connected)

## Progress Tracking

**Phase 0**: ✅ COMPLETE
- research.md generated
- Codebase analysis complete
- Dependencies validated

**Phase 1**: ✅ COMPLETE
- data-model.md generated
- Contracts generated (3 files)
- quickstart.md generated

**Phase 2**: ✅ COMPLETE (this document)
- plan.md populated with technical details
- Constitution check completed
- Test strategy defined
- Implementation plan ready

**Phase 3**: PENDING
- Awaiting `/tasks` command to generate tasks.md

**Phase 4**: PENDING
- Awaiting `/implement` command to execute tasks

## Next Steps

1. **Execute `/tasks` command** to generate dependency-ordered tasks.md
2. **Review plan.md** with stakeholder (if applicable)
3. **Execute `/implement` command** to begin TDD implementation
4. **QA review** after each major component
5. **Integration testing** with real NMEA devices
6. **User acceptance testing** via quickstart.md validation guide
7. **Documentation** update README.md with feature description
8. **Release** tag with firmware version, create PR

---

**Document Version**: 1.0.0
**Last Updated**: 2025-10-13
**Status**: Ready for Task Generation
