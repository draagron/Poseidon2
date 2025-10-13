# Implementation Plan: Simple WebUI for BoatData Streaming

**Branch**: `011-simple-webui-as` | **Date**: 2025-10-13 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `/home/niels/Dev/Poseidon2/specs/011-simple-webui-as/spec.md`

## Summary

This feature implements a web-based dashboard for real-time boat sensor data visualization. It consists of three main components: (1) a WebSocket endpoint `/boatdata` that streams JSON-serialized BoatData at 1-2 Hz, (2) an HTTP endpoint `/stream` that serves a static HTML dashboard from LittleFS storage, and (3) client-side JavaScript that connects to the WebSocket, parses JSON messages, and dynamically updates the HTML interface with organized sensor data grouped by category (GPS, Compass, Wind, DST, Engine, Battery, etc.).

**Key Technical Approach**:
- Leverage existing ESPAsyncWebServer and WebSocketLogger infrastructure
- Create new WebSocket endpoint `/boatdata` separate from existing `/logs` endpoint
- Implement BoatDataSerializer component to convert BoatData structs to JSON
- Use ReactESP timer for throttled broadcasts (1-2 Hz) to prevent network congestion
- Store HTML dashboard in LittleFS (`/data/stream.html`) for firmware-independent updates
- Use vanilla JavaScript (no frameworks) for minimal flash footprint and browser compatibility

**Implementation Complexity**: LOW-MEDIUM
- Existing WebSocket infrastructure (WebSocketLogger, ESPAsyncWebServer)
- BoatData structures already defined and populated by NMEA handlers
- New components: JSON serializer, broadcast timer, HTML file, HTTP file server endpoint
- Client-side JavaScript for DOM updates and WebSocket client connection

**Estimated Effort**: 8-12 hours (4-6 hours implementation + 4-6 hours testing/polish)

## Technical Context

**Language/Version**: C++ (C++14) for backend, JavaScript (ES6) for frontend
**Primary Dependencies**:
- Arduino framework for ESP32
- ESPAsyncWebServer (already integrated, async HTTP and WebSocket)
- ArduinoJson v6.x (for JSON serialization, pre-approved library)
- ReactESP v2.0.0 (for throttled broadcast timer)
- LittleFS (ESP32 built-in filesystem)

**Storage**:
- RAM: ~2-3KB for WebSocket endpoint, JSON buffer (~2KB), broadcast logic
- Flash: ~8-10KB new C++ code (serializer, endpoint handlers, broadcast loop)
- LittleFS: ~15-20KB for HTML dashboard file (`/data/stream.html`)

**Testing**:
- PlatformIO Unity framework (native environment for serialization logic tests)
- ESP32 integration tests (WebSocket connection, JSON format validation)
- Manual browser testing (Chrome, Firefox, Safari, Mobile Safari)
- WebSocket stress testing (5+ concurrent clients)

**Target Platform**:
- ESP32 (espressif32 platform)
- SH-ESP32 board (WiFi connectivity required)
- Web browsers: Chrome 90+, Firefox 88+, Safari 14+, Mobile browsers

**Project Type**: Embedded web application (ESP32 backend + HTML/JS frontend)

**Performance Goals**:
- JSON serialization time <50ms per BoatData snapshot
- WebSocket broadcast latency <100ms from trigger to all clients
- HTML page load time <2 seconds on typical WiFi
- Dashboard update latency <1 second from sensor change to display

**Constraints**:
- Static memory allocation for JSON buffer (no dynamic heap, Principle II)
- Non-blocking WebSocket operations (ReactESP compatible, Principle VI)
- WebSocket logging for all operations (Principle V)
- Graceful degradation on errors (Principle VII)
- HTML file size <25KB (LittleFS partition constraints)

**Scale/Scope**:
- 1 WebSocket endpoint (`/boatdata`)
- 1 HTTP endpoint (`/stream`)
- 9 BoatData structure groups to serialize (GPS, Compass, Wind, DST, Rudder, Engine, Saildrive, Battery, ShorePower)
- 1 HTML dashboard file with embedded JavaScript
- ~40-50 data fields to display across all sensor groups

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

### Principle I: Hardware Abstraction Layer (HAL)

✅ **COMPLIANT**: WebSocket and HTTP services abstracted via ESPAsyncWebServer library.
- ESPAsyncWebServer provides HAL for TCP/IP, WebSocket protocol, HTTP protocol
- LittleFS provides HAL for flash filesystem access
- No direct WiFi or network hardware access in feature code
- Business logic (JSON serialization, data formatting) separated from network I/O

**Action Required**: None. Existing network HAL satisfies requirements.

### Principle II: Resource-Aware Development

✅ **COMPLIANT**: Static allocation for all buffers, minimal heap usage.
- JSON buffer: Pre-allocated static buffer (~2KB) for serialization
- WebSocket broadcast: Uses ESPAsyncWebServer's internal buffers (no additional heap)
- HTML file: Stored in LittleFS (no RAM cost except during file reads)
- **Total RAM**: ~3KB (0.9% of ESP32 320KB RAM)
- **Total Flash**: ~10KB C++ code + ~20KB HTML file = ~30KB (1.6% of 1.9MB partition)

**Memory Budget**:
- Existing system: ~13.7% RAM, ~51.7% Flash (post-NMEA2000 feature)
- This feature: +0.9% RAM, +1.6% Flash
- **Projected total**: ~14.6% RAM, ~53.3% Flash (well within limits)

**Action Required**: None. Memory footprint within constitutional limits.

### Principle III: QA-First Review Process

✅ **COMPLIANT**: TDD approach with comprehensive test suite.
- Contract tests for JSON serializer interface
- Integration tests for WebSocket connection and data flow
- Unit tests for JSON format validation
- Browser compatibility testing (manual QA)
- QA subagent review required before merge

**Action Required**:
- Write tests BEFORE implementing serializer and endpoints
- Manual browser testing across multiple devices
- Request QA subagent review after implementation complete

### Principle IV: Modular Component Design

✅ **COMPLIANT**: Feature organized into focused components.
- BoatDataSerializer: Single responsibility (convert BoatData → JSON)
- WebSocket endpoint handler: Single responsibility (manage connections, broadcast)
- HTTP file server: Single responsibility (serve HTML from LittleFS)
- HTML dashboard: Self-contained (no external dependencies)
- Dependencies injected (BoatData pointer, AsyncWebServer, ReactESP app)

**Action Required**: None. Modular design satisfies principle.

### Principle V: Network-Based Debugging

✅ **COMPLIANT**: WebSocket logging for all operations.
- INFO-level log when WebSocket client connects/disconnects
- DEBUG-level log for each JSON broadcast (with client count)
- WARN-level log for serialization errors or buffer overflows
- ERROR-level log for LittleFS file read failures
- Log format: JSON with component="BoatDataStream", event names, client counts

**Action Required**: None. WebSocket logging integrated into all operations.

### Principle VI: Always-On Operation

✅ **COMPLIANT**: Non-blocking, event-driven architecture.
- WebSocket operations handled by ESPAsyncWebServer (non-blocking)
- Broadcast timer uses ReactESP (non-blocking event loop)
- JSON serialization runs in main loop context (<50ms budget)
- No sleep modes or blocking I/O
- LittleFS file reads are blocking but occur only during HTTP requests (not in hot path)

**Action Required**: Ensure JSON serialization completes within 50ms budget (add timeout check).

### Principle VII: Fail-Safe Operation

✅ **COMPLIANT**: Graceful degradation on all errors.
- WebSocket connection failures: Log ERROR, continue serving other clients
- JSON serialization failures: Log WARN, skip broadcast, retry next cycle
- LittleFS read failures: Return HTTP 404, log ERROR
- Buffer overflow: Log WARN, truncate JSON, mark as incomplete
- System continues normal operation without crashes

**Action Required**: None. Fail-safe error handling included in design.

### Principle VIII: Workflow Selection

✅ **COMPLIANT**: Feature Development workflow (/specify → /plan → /tasks → /implement).
- This is new functionality (WebUI dashboard)
- Full specification completed (spec.md)
- Implementation planning completed (this document)
- Task breakdown to follow (/tasks command)
- TDD approach with comprehensive testing

**Action Required**:
- Run `/tasks` to generate dependency-ordered task breakdown
- Run `/implement` to execute tasks following TDD approach

## Project Structure

### Documentation (this feature)

```
specs/011-simple-webui-as/
├── spec.md                        # Feature specification (completed)
├── plan.md                        # This file (implementation plan)
├── research.md                    # Phase 0 research (to be generated)
├── data-model.md                  # Phase 1 data model (to be generated)
├── quickstart.md                  # Phase 1 quickstart guide (to be generated)
├── contracts/                     # Phase 1 contracts (to be generated)
│   ├── BoatDataSerializerContract.md     # JSON serializer interface
│   ├── WebSocketEndpointContract.md      # /boatdata endpoint interface
│   ├── HTTPFileServerContract.md         # /stream endpoint interface
│   └── HTMLDashboardContract.md          # HTML/JS dashboard requirements
└── tasks.md                       # Phase 2 task breakdown (to be generated)
```

### Source Code (repository root)

```
src/
├── main.cpp                       # Application entry point
│   └── [MODIFICATION REQUIRED]    # Add /boatdata WebSocket endpoint
│                                  # Add /stream HTTP endpoint
│                                  # Add ReactESP broadcast timer (1-2 Hz)
│
├── components/
│   └── BoatDataSerializer.cpp     # [NEW] JSON serialization component
│       BoatDataSerializer.h       # [NEW] Serializer interface
│
├── utils/
│   └── WebSocketLogger.h          # [ALREADY EXISTS] Use for operational logging
│       WebSocketLogger.cpp
│
└── types/
    └── BoatDataTypes.h            # [ALREADY EXISTS] Source data structures

data/
└── stream.html                    # [NEW] HTML dashboard (LittleFS storage)
                                  # Contains: HTML structure, CSS styles, JavaScript

test/
├── test_webui_contracts/          # [NEW] Contract tests
│   ├── test_serializer_contract.cpp           # JSON serializer interface
│   ├── test_websocket_endpoint_contract.cpp   # WebSocket endpoint behavior
│   └── test_http_fileserver_contract.cpp      # HTTP file serving
│
├── test_webui_integration/        # [NEW] Integration tests
│   ├── test_websocket_connection.cpp          # Client connection flow
│   ├── test_json_data_flow.cpp                # End-to-end data streaming
│   ├── test_multi_client.cpp                  # Multiple concurrent clients
│   └── test_html_serving.cpp                  # Static file serving
│
└── test_webui_units/              # [NEW] Unit tests
    ├── test_json_format.cpp                   # JSON schema validation
    ├── test_serialization_performance.cpp     # <50ms timing requirement
    └── test_throttling.cpp                    # 1-2 Hz broadcast rate
```

**Structure Decision**: Single project structure with separate component for JSON serialization. HTML dashboard is a static asset stored in LittleFS. Tests organized by contract/integration/unit pattern following project conventions.

**File Locations**:
- C++ implementation: `src/components/BoatDataSerializer.{cpp,h}`
- Main integration: `src/main.cpp` (add WebSocket endpoint, HTTP endpoint, broadcast timer)
- HTML dashboard: `data/stream.html` (uploaded to LittleFS via PlatformIO)
- Tests: `test/test_webui_*` (following existing test organization)

## Complexity Tracking

*No constitutional violations. This section intentionally left empty.*

## Phase 0: Research and Technology Survey

**Objective**: Research existing WebSocket infrastructure, JSON serialization libraries, and LittleFS usage patterns.

### Research Areas

1. **Existing WebSocket Infrastructure**
   - Analyze `src/utils/WebSocketLogger.h` - How is `/logs` endpoint implemented?
   - Review `src/main.cpp` - How is AsyncWebServer initialized?
   - Identify patterns for creating additional WebSocket endpoints

2. **JSON Serialization Options**
   - **Option A**: ArduinoJson library (pre-approved, widely used, efficient)
   - **Option B**: Manual JSON string building (lightweight, no library dependency)
   - **Option C**: Streaming JSON with fixed-size buffers (memory-efficient)
   - **Recommendation**: Use ArduinoJson v6.x (balances efficiency, safety, maintainability)

3. **LittleFS File Serving**
   - Research ESPAsyncWebServer `serveStatic()` method
   - Review PlatformIO data folder upload process (`pio run --target uploadfs`)
   - Identify best practices for HTML file updates without firmware recompile

4. **Broadcast Throttling Mechanisms**
   - **Option A**: ReactESP `onRepeat()` timer at 1 Hz (simple, reliable)
   - **Option B**: Change detection with debouncing (complex, unnecessary)
   - **Recommendation**: Use ReactESP timer for predictable 1 Hz broadcast rate

5. **Browser Compatibility**
   - WebSocket API support: All modern browsers (IE11+ with polyfill)
   - JavaScript ES6 features: Template literals, arrow functions, const/let
   - Target: Chrome 90+, Firefox 88+, Safari 14+ (2021+ browsers)

### Technology Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| JSON Library | ArduinoJson v6.x | Pre-approved, efficient, type-safe, good documentation |
| JSON Buffer Size | 2048 bytes static | Sufficient for BoatData snapshot (~1500 bytes), 25% margin |
| Broadcast Rate | 1 Hz (1000ms ReactESP timer) | Balance responsiveness and network efficiency |
| WebSocket Library | ESPAsyncWebServer (existing) | Already integrated, proven reliable |
| HTML Serving | `server.serveStatic()` | Built-in ESPAsyncWebServer feature, efficient |
| JavaScript Approach | Vanilla JS (no frameworks) | Minimal size, fast load, no build step required |
| CSS Approach | Inline CSS in HTML | Single-file dashboard, no external dependencies |

**Output**: [research.md](./research.md) - Detailed findings from each research area

## Phase 1: Design Artifacts

### Data Model

**Core Data Flow**:
```
BoatData (C++ structs)
    ↓
BoatDataSerializer::toJSON()
    ↓
ArduinoJson StaticJsonDocument<2048>
    ↓
String serialization
    ↓
WebSocket broadcast (AsyncWebSocket::textAll())
    ↓
Client browsers (WebSocket API)
    ↓
JSON.parse() in JavaScript
    ↓
DOM updates (innerHTML)
```

**JSON Message Schema**:
```json
{
  "timestamp": 1697000000,
  "gps": {
    "latitude": 37.7749,
    "longitude": -122.4194,
    "cog": 1.5708,
    "sog": 5.5,
    "variation": 0.2618,
    "available": true,
    "lastUpdate": 123456
  },
  "compass": {
    "trueHeading": 1.5708,
    "magneticHeading": 1.5708,
    "rateOfTurn": 0.1,
    "heelAngle": 0.05,
    "pitchAngle": 0.02,
    "heave": 0.1,
    "available": true,
    "lastUpdate": 123456
  },
  ... (additional sensor groups)
}
```

**Output**: [data-model.md](./data-model.md) - Complete JSON schema, field mappings, unit conversions

### Contracts

1. **BoatDataSerializer Contract**: Interface for converting BoatData to JSON
2. **WebSocket Endpoint Contract**: Behavior requirements for `/boatdata` endpoint
3. **HTTP File Server Contract**: Requirements for `/stream` endpoint
4. **HTML Dashboard Contract**: Client-side interface requirements

**Output**: [contracts/](./contracts/) - Detailed contracts for each component

### Quickstart Guide

**Purpose**: Enable developers to test the WebUI feature independently.

**Contents**:
1. Prerequisites (hardware, software, network setup)
2. Step-by-step setup instructions
3. Testing the WebSocket endpoint with a WebSocket client tool
4. Testing the HTML dashboard in a browser
5. Troubleshooting common issues
6. How to update the HTML file in LittleFS

**Output**: [quickstart.md](./quickstart.md) - Complete validation guide

## Phase 2: Task Generation

**Command**: `/tasks` (executed after /plan completes)

**Expected Task Groups**:
1. **Setup**: Review existing infrastructure, verify dependencies
2. **Contract Tests**: Write failing tests for all components
3. **Implementation**: Implement BoatDataSerializer, endpoints, HTML dashboard
4. **Integration Tests**: End-to-end WebSocket and HTTP testing
5. **Unit Tests**: JSON format, performance, throttling
6. **Polish**: Browser testing, documentation, CHANGELOG update

**Output**: [tasks.md](./tasks.md) - Dependency-ordered task list

## Progress Tracking

### Execution Status

- [x] Phase 0: Research (completed - see research.md)
- [x] Phase 1: Design Artifacts (completed - see data-model.md, contracts/, quickstart.md)
- [ ] Phase 2: Task Generation (pending - run `/tasks` command)
- [ ] Phase 3: Implementation (pending - run `/implement` command)

### Generated Artifacts

- [ ] `research.md` - Research findings and technology decisions
- [ ] `data-model.md` - JSON schema and data flow documentation
- [ ] `contracts/BoatDataSerializerContract.md` - Serializer interface contract
- [ ] `contracts/WebSocketEndpointContract.md` - WebSocket endpoint contract
- [ ] `contracts/HTTPFileServerContract.md` - HTTP file server contract
- [ ] `contracts/HTMLDashboardContract.md` - HTML dashboard requirements
- [ ] `quickstart.md` - Testing and validation guide

**Next Step**: Generate Phase 0 research document, then Phase 1 artifacts.
