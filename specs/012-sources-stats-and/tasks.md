# Tasks: Source Statistics Tracking and WebUI

**Feature**: 012-sources-stats-and
**Branch**: `012-sources-stats-and`
**Input**: Design documents from `/specs/012-sources-stats-and/`

## Format: `[ID] [P?] [Story] Description`
- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (US1, US2, US3)
- Include exact file paths in descriptions

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Project initialization and basic structure

- [X] T001 Add SourceStatistics data structures in `src/components/SourceStatistics.h` (header-only definitions for MessageSource, MessageTypeEntry, CategoryEntry, CategoryType enum, ProtocolType)
- [X] T002 Update `src/config.h` with MAX_SOURCES=50, SOURCE_STALE_THRESHOLD_MS=5000, SOURCE_GC_THRESHOLD_MS=300000, WEBSOCKET_UPDATE_INTERVAL_MS=500
- [X] T003 Update `platformio.ini` with build flags for source stats constants and verify ArduinoJson dependency (v6.x)

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core utility components and base infrastructure needed by all user stories

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [X] T004 [P] Implement FrequencyCalculator utility in `src/utils/FrequencyCalculator.h` and `src/utils/FrequencyCalculator.cpp` (calculate() and addTimestamp() functions)
- [X] T005 [P] Write unit tests for FrequencyCalculator in `test/test_source_stats_units/FrequencyCalculatorTest.cpp` (10Hz, 1Hz, edge cases, buffer wrapping)
- [X] T006 [P] Implement SourceRegistry class skeleton in `src/components/SourceRegistry.h` and `src/components/SourceRegistry.cpp` (init(), data members, basic structure)
- [X] T007 Implement SourceRegistry::recordUpdate() method with source discovery, timestamp buffer management, frequency calculation, 50-source limit enforcement
- [X] T008 Implement SourceRegistry::updateStaleFlags() method (iterate all sources, update timeSinceLast and isStale)
- [X] T009 Implement SourceRegistry::garbageCollect() and evictOldestSource() methods
- [ ] T010 [P] Write contract tests for SourceRegistry in `test/test_source_stats_contracts/SourceRegistryContractTest.cpp` (invariants, source limit, memory bounds)
- [ ] T011 [P] Write unit tests for SourceRegistry in `test/test_source_stats_units/SourceRegistryTest.cpp` (recordUpdate, GC, eviction scenarios)

**Checkpoint**: Foundation ready - user story implementation can now begin in parallel

---

## Phase 3: User Story 1 - Real-time Source Discovery and Statistics (Priority: P1) 🎯 MVP

**Goal**: Enable visibility into which NMEA sources are active, their frequencies, and staleness for diagnostic purposes

**Independent Test**: Connect multiple NMEA devices, verify they appear in WebSocket endpoint with correct frequencies and staleness indicators

### Implementation for User Story 1

- [X] T012 [US1] Implement SourceStatsSerializer class in `src/components/SourceStatsSerializer.h` and `src/components/SourceStatsSerializer.cpp` (toFullSnapshotJSON(), toDeltaJSON(), toRemovalJSON() methods)
- [ ] T013 [P] [US1] Write unit tests for SourceStatsSerializer in `test/test_source_stats_units/SourceStatsSerializerTest.cpp` (JSON format validation, size checks, all three message types)
- [X] T014 [US1] Implement SourceStatsHandler class in `src/components/SourceStatsHandler.h` and `src/components/SourceStatsHandler.cpp` (WebSocket client connection handling, full snapshot on connect, delta broadcasts)
- [X] T015 [US1] Modify NMEA2000Handlers.cpp to add SourceRegistry::recordUpdate() calls in all 13 PGN handlers (extract SID, format sourceId, call recordUpdate with correct CategoryType and messageTypeId)
- [X] T016 [US1] Modify NMEA0183Handler.cpp to add SourceRegistry::recordUpdate() calls in all 5 sentence handlers (extract talker ID, format sourceId, call recordUpdate with correct CategoryType)
- [X] T017 [US1] Update main.cpp setup() to initialize SourceRegistry, register /source-stats WebSocket endpoint, create SourceStatsHandler instance
- [X] T018 [US1] Add ReactESP timers in main.cpp: 500ms for updateStaleFlags() and delta broadcasts, 60s for garbageCollect()
- [ ] T019 [P] [US1] Write integration tests in `test/test_source_stats_integration/EndToEndTest.cpp` (simulate NMEA messages, verify source discovery, frequency calculation, staleness detection)
- [ ] T020 [P] [US1] Write WebSocket integration test in `test/test_source_stats_integration/WebSocketIntegrationTest.cpp` (verify full snapshot, delta updates, removal events)

**Checkpoint**: At this point, User Story 1 should be fully functional - WebSocket endpoint streams source statistics in real-time

---

## Phase 4: User Story 2 - WebUI Dashboard for Source Monitoring (Priority: P2)

**Goal**: Provide human-readable web dashboard for monitoring sensor health organized by BoatData categories

**Independent Test**: Open browser to ESP32 IP:3030/sources, verify all sources displayed in organized table with real-time updates

### Implementation for User Story 2

- [X] T021 [P] [US2] Create HTML structure in `data/sources.html` (categories, table layout, WebSocket connection status indicator)
- [X] T022 [P] [US2] Implement CSS styling in `data/sources.html` (responsive design, staleness indicators green/red, mobile-friendly layout)
- [X] T023 [US2] Implement JavaScript WebSocket client in `data/sources.html` (connect to ws://ESP32_IP/source-stats, handle fullSnapshot, deltaUpdate, sourceRemoved events)
- [X] T024 [US2] Implement JavaScript table rendering logic in `data/sources.html` (create rows per source, update frequency/timeSinceLast/isStale dynamically)
- [X] T025 [US2] Add visual staleness indicators in `data/sources.html` (green circle for fresh, red for stale, update on isStale flag change)
- [X] T026 [US2] Register HTTP endpoint in main.cpp to serve `data/sources.html` at `/sources` path (LittleFS)
- [X] T027 [US2] Test dashboard on multiple browsers (Chrome, Firefox, Safari) and verify responsiveness on mobile devices

**Checkpoint**: At this point, User Stories 1 AND 2 should both work - WebSocket endpoint works AND dashboard displays data

---

## Phase 5: User Story 3 - Node.js Proxy Integration for Multi-Client Support (Priority: P3)

**Goal**: Extend Node.js proxy to relay source statistics, enabling multi-client scenarios without ESP32 overload

**Independent Test**: Run Node.js proxy, connect multiple browsers to localhost:3000/sources, verify all clients receive updates

### Implementation for User Story 3

- [X] T028 [P] [US3] Update `nodejs-boatdata-viewer/server.js` to add /source-stats WebSocket relay endpoint (connect to ESP32, forward messages to all connected browsers)
- [X] T029 [P] [US3] Copy `data/sources.html` to `nodejs-boatdata-viewer/public/sources.html` and update WebSocket URL to connect to proxy instead of ESP32 directly
- [X] T030 [US3] Update `nodejs-boatdata-viewer/config.json` to add sourceStatsPath configuration option
- [X] T031 [US3] Add auto-reconnect logic in Node.js proxy for ESP32 WebSocket connection (handle disconnect/reconnect scenarios)
- [X] T032 [US3] Test multi-client support by connecting 10+ browsers simultaneously to proxy and verify no dropped messages
- [X] T033 [US3] Update Node.js proxy README.md with usage instructions for /sources endpoint

**Checkpoint**: All user stories should now be independently functional - Direct ESP32 access works AND proxy relay works

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: Improvements that affect multiple user stories

- [X] T034 [P] Add WebSocketLogger integration throughout SourceRegistry (SOURCE_DISCOVERED, SOURCE_REMOVED, GC_COMPLETE events with component="SourceRegistry")
- [X] T035 [P] Add WebSocketLogger integration in SourceStatsHandler (CLIENT_CONNECTED, SNAPSHOT_SENT, DELTA_SENT events)
- [X] T036 Add memory diagnostics endpoint in main.cpp (/diagnostics with freeHeap and sourcesCount)
- [ ] T037 Performance profiling: measure WebSocket serialization time (<50ms target), memory footprint (<10KB for 20 sources target)
- [X] T038 [P] Update main project README.md with feature description, WebSocket endpoint documentation, dashboard URL
- [X] T039 [P] Create feature documentation in `specs/012-sources-stats-and/README.md` with quickstart instructions
- [ ] T040 Run quickstart.md validation scenarios 1-8 with real NMEA devices
- [X] T041 Run all test suites: `pio test -e native -f test_source_stats_*` and verify 100% pass rate (11/11 unit tests passed; contract/integration tests not implemented per tasks T010-T011, T013, T019-T020)
- [X] T042 Code cleanup: verify F() macro for strings, PROGMEM for constants, static allocation only (no heap) - Fixed SourceStatsHandler string literals to use F() macro
- [X] T043 QA subagent review: memory safety, resource usage, error handling, Arduino best practices - **CONDITIONAL PASS** (B+ grade; 0 HIGH, 3 MEDIUM, 4 LOW severity issues identified)

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies - can start immediately
- **Foundational (Phase 2)**: Depends on Setup completion - BLOCKS all user stories
- **User Stories (Phase 3+)**: All depend on Foundational phase completion
  - User stories can then proceed in parallel (if staffed)
  - Or sequentially in priority order (P1 → P2 → P3)
- **Polish (Final Phase)**: Depends on all user stories being complete

### User Story Dependencies

- **User Story 1 (P1)**: Can start after Foundational (Phase 2) - No dependencies on other stories - Delivers MVP
- **User Story 2 (P2)**: Can start after Foundational (Phase 2) - Depends on US1 WebSocket endpoint - Independently testable with mock WebSocket data
- **User Story 3 (P3)**: Can start after US2 dashboard exists - Depends on US1 WebSocket protocol - Independently testable by connecting proxy to ESP32

### Within Each User Story

- Tests written first (if applicable)
- Data structures before logic
- Core logic before integration
- Integration before polish
- Story complete before moving to next priority

### Parallel Opportunities

**Phase 1 (Setup)**:
- All three tasks can run in parallel (different files)

**Phase 2 (Foundational)**:
- T004 (FrequencyCalculator) and T005 (tests) can run in parallel
- T006 (SourceRegistry skeleton) must complete before T007-T009
- T010 (contract tests) and T011 (unit tests) can run in parallel after T009 completes

**Phase 3 (US1)**:
- T012 (Serializer) and T013 (tests) can be developed in parallel
- T015 (NMEA2000 integration) and T016 (NMEA0183 integration) can run in parallel (different files)
- T019 (integration tests) and T020 (WebSocket tests) can run in parallel

**Phase 4 (US2)**:
- T021 (HTML structure) and T022 (CSS) can run in parallel (same file but different concerns)

**Phase 5 (US3)**:
- T028 (server.js) and T029 (dashboard HTML) can run in parallel (different files)
- T030 (config.json) can run in parallel with T028-T029

**Phase 6 (Polish)**:
- T034 (logger integration) and T035 (handler logging) can run in parallel
- T038 (README) and T039 (feature docs) can run in parallel

---

## Parallel Example: Foundational Phase

```bash
# Launch FrequencyCalculator development and tests together:
Task: "Implement FrequencyCalculator utility in src/utils/FrequencyCalculator.h and .cpp"
Task: "Write unit tests for FrequencyCalculator in test/test_source_stats_units/FrequencyCalculatorTest.cpp"

# After SourceRegistry core methods complete, launch contract and unit tests together:
Task: "Write contract tests for SourceRegistry in test/test_source_stats_contracts/SourceRegistryContractTest.cpp"
Task: "Write unit tests for SourceRegistry in test/test_source_stats_units/SourceRegistryTest.cpp"
```

---

## Parallel Example: User Story 1

```bash
# Launch serializer and tests together:
Task: "Implement SourceStatsSerializer in src/components/SourceStatsSerializer.h and .cpp"
Task: "Write unit tests for SourceStatsSerializer in test/test_source_stats_units/SourceStatsSerializerTest.cpp"

# Launch NMEA handler modifications in parallel:
Task: "Modify NMEA2000Handlers.cpp to add recordUpdate() calls"
Task: "Modify NMEA0183Handler.cpp to add recordUpdate() calls"

# Launch integration tests in parallel:
Task: "Write integration tests in test/test_source_stats_integration/EndToEndTest.cpp"
Task: "Write WebSocket integration test in test/test_source_stats_integration/WebSocketIntegrationTest.cpp"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup (T001-T003)
2. Complete Phase 2: Foundational (T004-T011) - CRITICAL
3. Complete Phase 3: User Story 1 (T012-T020)
4. **STOP and VALIDATE**: Test with real NMEA devices, verify WebSocket endpoint works
5. Deploy/demo if ready (diagnostic tool ready)

### Incremental Delivery

1. Complete Setup + Foundational → Foundation ready (~8 hours)
2. Add User Story 1 → Test independently → Deploy/Demo (MVP! - Diagnostic capability) (~12 hours)
3. Add User Story 2 → Test independently → Deploy/Demo (User-friendly dashboard) (~6 hours)
4. Add User Story 3 → Test independently → Deploy/Demo (Multi-client support) (~3 hours)
5. Each story adds value without breaking previous stories

### Test-First Development (TDD)

For each component:
1. Write contract/test file first
2. Run test - should FAIL (red)
3. Implement minimum code to pass (green)
4. Refactor for clarity (refactor)
5. Move to next component

Example for FrequencyCalculator:
```bash
# Write test first (T005)
touch test/test_source_stats_units/FrequencyCalculatorTest.cpp
# Write failing test cases
pio test -e native -f test_source_stats_units  # SHOULD FAIL

# Implement FrequencyCalculator (T004)
# Write src/utils/FrequencyCalculator.cpp

# Run tests again
pio test -e native -f test_source_stats_units  # SHOULD PASS
```

---

## Testing Commands

```bash
# Run all source stats unit tests
pio test -e native -f test_source_stats_units

# Run contract tests
pio test -e native -f test_source_stats_contracts

# Run integration tests
pio test -e native -f test_source_stats_integration

# Run all source stats tests
pio test -e native -f test_source_stats_*

# Build for ESP32
pio run

# Upload firmware and LittleFS
pio run --target upload
pio run --target uploadfs

# Monitor serial output
pio device monitor --baud 115200

# Connect to WebSocket logger
python3 src/helpers/ws_logger.py <ESP32_IP> --endpoint /source-stats
```

---

## Validation Checkpoints

### After Phase 2 (Foundational)
- [ ] FrequencyCalculator unit tests pass (10Hz, 1Hz accuracy ±10%)
- [ ] SourceRegistry contract tests pass (50-source limit enforced)
- [ ] SourceRegistry unit tests pass (recordUpdate, GC, eviction)
- [ ] Memory footprint validated (<5.3KB for 50 sources)

### After Phase 3 (User Story 1 - MVP)
- [ ] NMEA 2000 sources discovered (scenario 1 from quickstart.md)
- [ ] NMEA 0183 sources discovered (scenario 2)
- [ ] Staleness detection works (scenario 3)
- [ ] Source reactivation works (scenario 4)
- [ ] Multiple sources per message type (scenario 5)
- [ ] Garbage collection removes stale sources (scenario 7)
- [ ] 50-source limit enforced (scenario 8)
- [ ] WebSocket endpoint delivers data within 500ms ±50ms

### After Phase 4 (User Story 2)
- [ ] Dashboard loads <2 seconds (SC-005)
- [ ] Visual updates <200ms latency (SC-006)
- [ ] Dashboard responsive on mobile devices
- [ ] Real-time updates without page refresh (scenario 6)

### After Phase 5 (User Story 3)
- [ ] Node.js proxy relays source statistics
- [ ] Multi-client support (10+ browsers simultaneously)
- [ ] Auto-reconnect works when ESP32 disconnects

### Final Validation (Phase 6)
- [ ] All quickstart scenarios pass (1-9)
- [ ] Memory <10KB for 20 sources (SC-007)
- [ ] Frequency accuracy ±10% (SC-002)
- [ ] Staleness detection <5.5s (SC-003)
- [ ] QA review passed

---

## Notes

- [P] tasks = different files, no dependencies (can run in parallel)
- [Story] label maps task to specific user story for traceability
- Each user story should be independently completable and testable
- TDD approach: tests first, implementation second
- Commit after each task or logical group
- Stop at checkpoints to validate story independently
- File paths are absolute from repository root
- Platform: ESP32 with PlatformIO (Arduino framework)
- Test organization: PlatformIO grouped tests (test_<feature>_<type>/)

---

**Document Version**: 1.0.0
**Last Updated**: 2025-10-13
**Total Estimated Time**: ~35-40 hours
**MVP Time (Phase 1-3)**: ~20 hours
