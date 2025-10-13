# Tasks: Simple WebUI for BoatData Streaming

**Feature Branch**: `011-simple-webui-as`
**Input**: Design documents from `/home/niels/Dev/Poseidon2/specs/011-simple-webui-as/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/ (all completed)

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`
- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Verify existing infrastructure and create new components structure

- [ ] T001 Verify ArduinoJson v6.21.0 installed in platformio.ini
- [ ] T002 Verify ESPAsyncWebServer integration in src/main.cpp
- [ ] T003 Verify LittleFS initialized in src/main.cpp
- [ ] T004 Create src/components/BoatDataSerializer.h header file with class definition
- [ ] T005 Create src/components/BoatDataSerializer.cpp implementation file stub
- [ ] T006 Create data/ directory for HTML dashboard file (if not exists)

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core JSON serialization component that ALL user stories depend on

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [ ] T007 [P] Write contract test for BoatDataSerializer::toJSON() in test/test_webui_contracts/test_serializer_contract.cpp
- [ ] T008 [US1] Implement BoatDataSerializer::toJSON() core structure with StaticJsonDocument<2048>
- [ ] T009 [US1] Add GPS data serialization to BoatDataSerializer::toJSON()
- [ ] T010 [US1] Add Compass data serialization to BoatDataSerializer::toJSON()
- [ ] T011 [US1] Add Wind data serialization to BoatDataSerializer::toJSON()
- [ ] T012 [US1] Add DST data serialization to BoatDataSerializer::toJSON()
- [ ] T013 [US1] Add Rudder data serialization to BoatDataSerializer::toJSON()
- [ ] T014 [US1] Add Engine data serialization to BoatDataSerializer::toJSON()
- [ ] T015 [US1] Add Saildrive data serialization to BoatDataSerializer::toJSON()
- [ ] T016 [US1] Add Battery data serialization to BoatDataSerializer::toJSON()
- [ ] T017 [US1] Add ShorePower data serialization to BoatDataSerializer::toJSON()
- [ ] T018 [US1] Add error handling (null checks, buffer overflow) to BoatDataSerializer::toJSON()
- [ ] T019 [US1] Add performance monitoring (timing check <50ms) to BoatDataSerializer::toJSON()
- [ ] T020 [US1] Add WebSocket logging for serialization events in BoatDataSerializer.cpp
- [ ] T021 Run contract test for BoatDataSerializer (test/test_webui_contracts/)

**Checkpoint**: JSON serialization working - WebSocket endpoint can now be implemented

---

## Phase 3: User Story 1 - Real-time BoatData Streaming via WebSocket (Priority: P1) 🎯 MVP

**Goal**: Stream JSON-formatted BoatData to web clients via WebSocket at 1 Hz

**Independent Test**: Connect to `ws://192.168.1.100/boatdata` from WebSocket test client, verify JSON messages received at 1 Hz

### Tests for User Story 1

**NOTE: Write these tests FIRST, ensure they FAIL before implementation**

- [ ] T022 [P] [US1] Write contract test for WebSocket endpoint connection in test/test_webui_contracts/test_websocket_endpoint_contract.cpp
- [ ] T023 [P] [US1] Write integration test for WebSocket connection flow in test/test_webui_integration/test_websocket_connection.cpp
- [ ] T024 [P] [US1] Write integration test for JSON data flow in test/test_webui_integration/test_json_data_flow.cpp
- [ ] T025 [P] [US1] Write integration test for multi-client support in test/test_webui_integration/test_multi_client.cpp
- [ ] T026 [P] [US1] Write unit test for JSON format validation in test/test_webui_units/test_json_format.cpp
- [ ] T027 [P] [US1] Write unit test for serialization performance in test/test_webui_units/test_serialization_performance.cpp
- [ ] T028 [P] [US1] Write unit test for broadcast throttling in test/test_webui_units/test_throttling.cpp

### Implementation for User Story 1

- [ ] T029 [US1] Add global AsyncWebSocket wsBoatData("/boatdata") declaration in src/main.cpp
- [ ] T030 [US1] Implement setupBoatDataWebSocket() function in src/main.cpp with event handlers
- [ ] T031 [US1] Add setupBoatDataWebSocket() call to onWiFiConnected() in src/main.cpp (after logger.begin)
- [ ] T032 [US1] Add ReactESP broadcast timer (1000ms interval) to setup() in src/main.cpp
- [ ] T033 [US1] Implement broadcast timer callback with BoatDataSerializer::toJSON() and wsBoatData.textAll()
- [ ] T034 [US1] Add client count check (skip broadcast if wsBoatData.count() == 0) for optimization
- [ ] T035 [US1] Add maximum client limit enforcement (10 clients) in setupBoatDataWebSocket()
- [ ] T036 [US1] Add WebSocket logging for connection/disconnection events in setupBoatDataWebSocket()
- [ ] T037 [US1] Add WebSocket logging for broadcast events in timer callback (DEBUG level, optional)
- [ ] T038 [US1] Run all US1 tests to verify WebSocket streaming works (pio test -e native -f test_webui_*)

**Checkpoint**: At this point, User Story 1 should be fully functional - WebSocket clients can connect and receive BoatData JSON at 1 Hz

---

## Phase 4: User Story 2 - Static HTML Dashboard Hosting (Priority: P2)

**Goal**: Serve HTML dashboard file from LittleFS storage at /stream endpoint

**Independent Test**: Navigate to `http://192.168.1.100/stream` in browser, verify HTML page loads from LittleFS

### Tests for User Story 2

- [ ] T039 [P] [US2] Write contract test for HTTP file server endpoint in test/test_webui_contracts/test_http_fileserver_contract.cpp
- [ ] T040 [P] [US2] Write integration test for HTML serving in test/test_webui_integration/test_html_serving.cpp

### Implementation for User Story 2

- [ ] T041 [US2] Create minimal test HTML file in data/stream.html with WebSocket connection stub
- [ ] T042 [US2] Add HTTP endpoint handler for /stream in src/main.cpp onWiFiConnected() function
- [ ] T043 [US2] Implement LittleFS.exists() check in /stream handler
- [ ] T044 [US2] Implement request->send(LittleFS, "/stream.html", "text/html") in /stream handler
- [ ] T045 [US2] Add error handling for file not found (HTTP 404) in /stream handler
- [ ] T046 [US2] Add error handling for file read error (HTTP 500) in /stream handler
- [ ] T047 [US2] Add WebSocket logging for file serving events (FILE_SERVED, FILE_NOT_FOUND, FILE_READ_ERROR)
- [ ] T048 [US2] Upload test HTML to LittleFS (pio run --target uploadfs)
- [ ] T049 [US2] Test /stream endpoint returns HTML with correct Content-Type header
- [ ] T050 [US2] Run US2 integration tests to verify HTML serving works

**Checkpoint**: At this point, User Stories 1 AND 2 should both work independently - HTML page loads and WebSocket streaming works

---

## Phase 5: User Story 3 - Organized Data Display with BoatData Structure Grouping (Priority: P3)

**Goal**: Display BoatData in organized sensor groups (GPS, Compass, Wind, etc.) with real-time updates

**Independent Test**: Load `/stream` page, verify data organized into 9 sensor group cards with labels matching BoatData structure

### Tests for User Story 3

- [ ] T051 [P] [US3] Write contract test for HTML dashboard requirements in test/test_webui_contracts/test_html_dashboard_contract.cpp
- [ ] T052 [P] [US3] Manual browser test checklist document in specs/011-simple-webui-as/manual-tests.md

### Implementation for User Story 3

#### HTML Structure (all tasks modify data/stream.html)

- [ ] T053 [P] [US3] Create HTML document structure with head, body, header, main, footer in data/stream.html
- [ ] T054 [P] [US3] Add connection status indicator in header with id="connection-status"
- [ ] T055 [US3] Create GPS sensor card with 7 data fields in main section
- [ ] T056 [US3] Create Compass sensor card with 8 data fields in main section
- [ ] T057 [US3] Create Wind sensor card with 4 data fields in main section
- [ ] T058 [US3] Create DST sensor card with 5 data fields in main section
- [ ] T059 [US3] Create Rudder sensor card with 3 data fields in main section
- [ ] T060 [US3] Create Engine sensor card with 5 data fields in main section
- [ ] T061 [US3] Create Saildrive sensor card with 3 data fields in main section
- [ ] T062 [US3] Create Battery sensor card with 12 data fields (dual banks) in main section
- [ ] T063 [US3] Create ShorePower sensor card with 4 data fields in main section
- [ ] T064 [US3] Add last update timestamp display in footer

#### CSS Styles (inline in data/stream.html <style> tag)

- [ ] T065 [P] [US3] Add marine theme color scheme (dark blue background, navy cards, light gray text)
- [ ] T066 [P] [US3] Add responsive CSS Grid layout (3-column desktop, 2-column tablet, 1-column mobile)
- [ ] T067 [P] [US3] Add sensor card styles (border-radius, box-shadow, padding)
- [ ] T068 [P] [US3] Add connection status indicator styles (green/yellow/red colors)
- [ ] T069 [P] [US3] Add availability indicator styles (green/red dots)
- [ ] T070 [P] [US3] Add unavailable data styles (gray text, italic, "--" placeholder)
- [ ] T071 [P] [US3] Add typography styles (font sizes, monospace for numbers)

#### JavaScript WebSocket Client (inline in data/stream.html <script> tag)

- [ ] T072 [US3] Implement connectWebSocket() function with WebSocket URL construction
- [ ] T073 [US3] Implement handleConnect() callback for ws.onopen event
- [ ] T074 [US3] Implement handleMessage() callback for ws.onmessage event with JSON.parse()
- [ ] T075 [US3] Implement handleError() callback for ws.onerror event
- [ ] T076 [US3] Implement handleClose() callback for ws.onclose event with auto-reconnect (5s delay)
- [ ] T077 [US3] Implement updateConnectionStatus() function (connected/connecting/disconnected states)

#### JavaScript Data Processing (inline in data/stream.html <script> tag)

- [ ] T078 [P] [US3] Implement radToDeg() unit conversion function (radians → degrees 0-360)
- [ ] T079 [P] [US3] Implement radToSignedDeg() function (radians → degrees -180 to +180)
- [ ] T080 [P] [US3] Implement msToKnots() unit conversion function (m/s → knots)
- [ ] T081 [P] [US3] Implement formatLastUpdate() function (timestamp → "5s ago" format)
- [ ] T082 [P] [US3] Implement updateValue() helper function (handles null, undefined, unavailable states)
- [ ] T083 [US3] Implement updateDashboard() main update function (calls all sensor update functions)
- [ ] T084 [US3] Implement updateGPS() function (updates GPS card with 7 fields)
- [ ] T085 [US3] Implement updateCompass() function (updates Compass card with 8 fields)
- [ ] T086 [US3] Implement updateWind() function (updates Wind card with 4 fields)
- [ ] T087 [US3] Implement updateDST() function (updates DST card with 5 fields)
- [ ] T088 [US3] Implement updateRudder() function (updates Rudder card with 3 fields)
- [ ] T089 [US3] Implement updateEngine() function (updates Engine card with 5 fields)
- [ ] T090 [US3] Implement updateSaildrive() function (updates Saildrive card with 3 fields)
- [ ] T091 [US3] Implement updateBattery() function (updates Battery card with 12 fields)
- [ ] T092 [US3] Implement updateShorePower() function (updates ShorePower card with 4 fields)
- [ ] T093 [US3] Add DOMContentLoaded event listener to call connectWebSocket() on page load

#### Integration and Testing

- [ ] T094 [US3] Upload complete HTML dashboard to LittleFS (pio run --target uploadfs)
- [ ] T095 [US3] Test HTML file size <25 KB (verify with ls -lh data/stream.html)
- [ ] T096 [US3] Test dashboard loads in Chrome desktop browser
- [ ] T097 [US3] Test dashboard loads in Firefox desktop browser
- [ ] T098 [US3] Test dashboard loads in Safari desktop browser (if available)
- [ ] T099 [US3] Test dashboard on mobile browser (iOS Safari or Chrome Android)
- [ ] T100 [US3] Verify WebSocket connects automatically on page load
- [ ] T101 [US3] Verify all 9 sensor cards display correctly
- [ ] T102 [US3] Verify data updates at ~1 Hz rate
- [ ] T103 [US3] Verify unit conversions correct (radians→degrees, m/s→knots)
- [ ] T104 [US3] Verify "Last update" timestamps update correctly
- [ ] T105 [US3] Verify unavailable sensors show "N/A" or "--"
- [ ] T106 [US3] Verify connection status indicator shows correct state
- [ ] T107 [US3] Test auto-reconnect after ESP32 restart (5s delay)
- [ ] T108 [US3] Run US3 contract tests for HTML dashboard

**Checkpoint**: All user stories should now be independently functional - Complete WebUI system with organized dashboard

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: Improvements that affect multiple user stories, documentation, and final validation

- [ ] T109 [P] Run full test suite for all user stories (pio test -e native -f test_webui)
- [ ] T110 [P] Verify all contract tests pass (test/test_webui_contracts/)
- [ ] T111 [P] Verify all integration tests pass (test/test_webui_integration/)
- [ ] T112 [P] Verify all unit tests pass (test/test_webui_units/)
- [ ] T113 Test with 5 concurrent WebSocket clients (multi-browser test)
- [ ] T114 Test with 10 concurrent clients (verify limit enforcement)
- [ ] T115 Test page load performance <2 seconds (browser DevTools network tab)
- [ ] T116 Test WebSocket latency <100 ms (browser console timing)
- [ ] T117 Verify memory usage stable over 1000 broadcasts (no memory leaks)
- [ ] T118 Monitor ESP32 free heap with 5 clients connected (should be >200 KB)
- [ ] T119 Test LittleFS file update workflow (modify HTML, uploadfs, verify new version)
- [ ] T120 Test error handling: missing LittleFS file (HTTP 404)
- [ ] T121 Test error handling: WebSocket disconnect and reconnect
- [ ] T122 Test error handling: invalid JSON (should be graceful, no crashes)
- [ ] T123 [P] Update CLAUDE.md with Simple WebUI integration guide
- [ ] T124 [P] Update CHANGELOG.md with feature description and memory footprint
- [ ] T125 Run quickstart.md validation (specs/011-simple-webui-as/quickstart.md)
- [ ] T126 Code cleanup: Remove debug logging (or reduce to INFO level for production)
- [ ] T127 Code cleanup: Add Doxygen comments to BoatDataSerializer.h
- [ ] T128 Code cleanup: Verify all constitutional compliance (8 principles)
- [ ] T129 Create git commit with conventional commit message
- [ ] T130 Request QA subagent review (constitutional requirement)

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies - can start immediately
- **Foundational (Phase 2)**: Depends on Setup completion - BLOCKS all user stories
- **User Stories (Phase 3+)**: All depend on Foundational phase completion
  - User Story 1 (US1): Can start after Phase 2
  - User Story 2 (US2): Can start after Phase 2 (independent of US1)
  - User Story 3 (US3): Depends on US1 + US2 completion (needs both WebSocket streaming and HTML hosting)
- **Polish (Phase 6)**: Depends on all user stories being complete

### User Story Dependencies

- **User Story 1 (P1)**: Can start after Foundational (Phase 2) - No dependencies on other stories
- **User Story 2 (P2)**: Can start after Foundational (Phase 2) - Independent of US1, can run in parallel
- **User Story 3 (P3)**: Depends on US1 + US2 completion - Needs both WebSocket endpoint and HTML hosting working

### Within Each User Story

- Tests MUST be written and FAIL before implementation (T022-T028 before T029-T038)
- JSON serialization (Phase 2) before WebSocket endpoint (Phase 3)
- WebSocket endpoint before HTML dashboard client code (Phase 5)
- HTML structure before CSS styles and JavaScript (can parallelize within Phase 5)
- All implementation before browser testing
- Story complete before moving to next priority

### Parallel Opportunities

- **Phase 1 (Setup)**: All tasks can run in parallel (T001-T006 are independent checks)
- **Phase 2 (Foundational)**: Contract test T007 can run in parallel with implementation start T008
- **US1 Tests (T022-T028)**: All test file creation tasks can run in parallel [P]
- **US2 Tests (T039-T040)**: Both test tasks can run in parallel [P]
- **US3 HTML Structure (T053-T054)**: Document structure and status indicator can run in parallel [P]
- **US3 CSS Styles (T065-T071)**: All CSS tasks can run in parallel [P] (same file but independent sections)
- **US3 Unit Conversions (T078-T082)**: All conversion functions can run in parallel [P]
- **Phase 6 (Polish)**: Documentation tasks T123-T124 can run in parallel [P], test suites T109-T112 can run in parallel [P]

**Note**: Tasks modifying the same file (e.g., data/stream.html) must run sequentially unless they edit independent sections

---

## Parallel Example: Phase 2 Foundational

```bash
# These two can start together:
Task: "Write contract test for BoatDataSerializer::toJSON() in test/test_webui_contracts/test_serializer_contract.cpp"
Task: "Implement BoatDataSerializer::toJSON() core structure with StaticJsonDocument<2048>"

# All US1 test creation tasks can run together:
Task: "Write contract test for WebSocket endpoint connection"
Task: "Write integration test for WebSocket connection flow"
Task: "Write integration test for JSON data flow"
Task: "Write integration test for multi-client support"
Task: "Write unit test for JSON format validation"
Task: "Write unit test for serialization performance"
Task: "Write unit test for broadcast throttling"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup (T001-T006)
2. Complete Phase 2: Foundational (T007-T021) - CRITICAL, blocks all stories
3. Complete Phase 3: User Story 1 (T022-T038)
4. **STOP and VALIDATE**: Test WebSocket endpoint with test client
5. Deploy/demo if ready (WebSocket streaming functional)

### Incremental Delivery

1. **Foundation**: Complete Setup (Phase 1) + Foundational (Phase 2) → JSON serialization ready
2. **MVP**: Add User Story 1 (Phase 3) → Test independently → **Deploy/Demo (WebSocket streaming works!)**
3. **Enhancement**: Add User Story 2 (Phase 4) → Test independently → **Deploy/Demo (HTML page serves!)**
4. **Complete**: Add User Story 3 (Phase 5) → Test independently → **Deploy/Demo (Full dashboard!)**
5. **Production**: Polish (Phase 6) → Final validation → **Deploy to boat!**

Each story adds value without breaking previous stories.

### Parallel Team Strategy

With multiple developers:

1. **Together**: Complete Setup (Phase 1) + Foundational (Phase 2)
2. **Once Foundational is done**:
   - Developer A: User Story 1 (Phase 3) - WebSocket endpoint
   - Developer B: User Story 2 (Phase 4) - HTTP file server (parallel with A)
   - Developer C: Start User Story 3 HTML structure (T053-T064) while waiting for A+B
3. **After US1 + US2 complete**:
   - Developer C: Complete User Story 3 (Phase 5) - Full dashboard
   - Developer A/B: Start Polish tasks (Phase 6) in parallel

---

## Memory Footprint Summary

**RAM Usage** (cumulative):
- BoatDataSerializer: ~2KB transient (StaticJsonDocument<2048>)
- WebSocket endpoint: ~100 bytes persistent
- HTTP file server: ~50 bytes persistent
- 5 concurrent clients: ~20KB (ESPAsyncWebServer buffers)
- **Total peak**: ~22KB (~6.9% of ESP32 RAM)

**Flash Usage** (cumulative):
- BoatDataSerializer: ~3KB code
- WebSocket endpoint: ~2KB code
- HTTP file server: ~1KB code
- HTML dashboard: ~18KB (LittleFS storage)
- Test code: ~12KB (not included in production build)
- **Total production**: ~24KB (~1.3% of 1.9MB partition)

**Constitutional Compliance**: ✓ Well under resource limits (Principle II)

---

## Notes

- [P] tasks = different files, no dependencies, can run in parallel
- [Story] label maps task to specific user story (US1, US2, US3) for traceability
- Each user story should be independently completable and testable
- Verify tests fail before implementing (TDD approach)
- Commit after each task or logical group
- Stop at any checkpoint to validate story independently
- File paths are absolute: `/home/niels/Dev/Poseidon2/...`
- HTML tasks (T053-T093) all modify `data/stream.html` - coordinate carefully or work sequentially
- Browser testing (T096-T108) requires ESP32 hardware and WiFi connection
- QA review (T130) is mandatory per Constitutional Principle III

---

**Tasks Version**: 1.0.0 | **Generated**: 2025-10-13
