# Tasks: NMEA2000 Device Discovery and Identification

**Input**: Design documents from `/home/niels/Dev/Poseidon2/specs/013-r013-nmea2000-device/`
**Prerequisites**: plan.md, spec.md, data-model.md, contracts/, research.md, quickstart.md
**Branch**: `013-r013-nmea2000-device`

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story. Tests are written FIRST (TDD approach), then implementation follows.

## Format: `[ID] [P?] [Story] Description`
- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (US1, US2, US3, US4)
- Include exact file paths in descriptions

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Project initialization and basic structure - NO CODE IMPLEMENTATION YET, PLANNING ONLY

- [ ] **T001** [SETUP] Verify PlatformIO dependencies (NMEA2000, ReactESP, ESPAsyncWebServer) are present in `platformio.ini`
- [ ] **T002** [SETUP] Create test directory structure: `test/test_device_discovery_contracts/`, `test/test_device_discovery_integration/`, `test/test_device_discovery_units/`
- [ ] **T003** [P] [SETUP] Create test helpers directory if not exists: `test/helpers/` (for mock NMEA2000 device utilities)

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core data structures and utilities that ALL user stories depend on

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

### Foundational Data Structures

- [X] **T004** [FOUNDATION] Extend `MessageSource` struct in `src/types/SourceStatistics.h` with `DeviceInfo` nested struct
  - Add `DeviceInfo` struct definition (~72 bytes)
  - Add `deviceInfo` field to `MessageSource` struct
  - Update `MessageSource::init()` to call `deviceInfo.init()`
  - **Files**: `src/components/SourceStatistics.h`

### Foundational Utilities (Can run in parallel after T004)

- [X] **T005** [P] [FOUNDATION] Implement `ManufacturerLookup` utility in `src/utils/ManufacturerLookup.h` and `.cpp`
  - Create static PROGMEM lookup table with ≥40 manufacturer codes (Garmin=275, Furuno=1855, Raymarine=378, etc.)
  - Implement `getManufacturerName(uint16_t code)` function
  - Return "Unknown (code)" for unrecognized codes
  - **Files**: `src/utils/ManufacturerLookup.h`, `src/utils/ManufacturerLookup.cpp`

- [X] **T006** [P] [FOUNDATION] Implement `TalkerIdLookup` utility in `src/utils/TalkerIdLookup.h` and `.cpp`
  - Create static PROGMEM lookup table with ≥15 talker IDs (AP=Autopilot, GP=GPS, VH=VHF Radio, etc.)
  - Implement `getTalkerDescription(const char* talkerId)` function
  - Return "Unknown NMEA0183 Device" for unrecognized IDs
  - **Files**: `src/utils/TalkerIdLookup.h`, `src/utils/TalkerIdLookup.cpp`

### Foundational SourceRegistry Extensions

- [X] **T007** [FOUNDATION] Extend `SourceRegistry` with device metadata update method
  - Add `updateDeviceInfo(const char* sourceId, const DeviceInfo& info)` method to `SourceRegistry` class
  - Set `hasChanges_ = true` when device info updated
  - **Files**: `src/components/SourceRegistry.h`, `src/components/SourceRegistry.cpp`
  - **Dependencies**: T004 (DeviceInfo struct must exist)

**Checkpoint**: Foundation ready - user story implementation can now begin in parallel

---

## Phase 3: User Story 1 - Automatic Device Discovery (Priority: P1) 🎯 MVP

**Goal**: Core NMEA2000 device discovery functionality - extract metadata from `tN2kDeviceList` and enrich `MessageSource` entries

**Independent Test**: Connect 2-3 NMEA2000 devices, monitor WebSocket logs for `DEVICE_DISCOVERED` events with manufacturer, model, serial number

### Tests for User Story 1 (TDD: Write tests FIRST, ensure they FAIL)

- [ ] **T008** [P] [US1] Contract test for `ManufacturerLookup` in `test/test_device_discovery_contracts/test_manufacturer_lookup.cpp`
  - Test known codes (275→Garmin, 1855→Furuno, 378→Raymarine)
  - Test unknown codes (99999→"Unknown (99999)")
  - Test performance (<50μs per lookup)
  - **Files**: `test/test_device_discovery_contracts/test_manufacturer_lookup.cpp`
  - **Dependencies**: T005 (ManufacturerLookup implementation)

- [ ] **T009** [P] [US1] Contract test for `DeviceInfoCollector` interface in `test/test_device_discovery_contracts/test_device_collector.cpp`
  - Test `pollDeviceList()` with empty device list
  - Test `pollDeviceList()` with 1 device
  - Test `pollDeviceList()` with 20 devices (performance <10ms)
  - Test metadata extraction correctness (all tDevice fields)
  - Test discovery timeout (60 seconds)
  - Test SID correlation
  - **Files**: `test/test_device_discovery_contracts/test_device_collector.cpp`
  - **Dependencies**: T011 header (DeviceInfoCollector.h created with interface definition)

- [ ] **T010** [P] [US1] Integration test for end-to-end device discovery in `test/test_device_discovery_integration/test_discovery_workflow.cpp`
  - Scenario 1: Two GPS units discovered (Garmin SID 42, Furuno SID 7)
  - Scenario 2: Hot-plug device discovery during runtime
  - Scenario 3: Device unplugged and reconnected (metadata updated)
  - Scenario 4: Non-compliant device timeout after 60 seconds
  - **Files**: `test/test_device_discovery_integration/test_discovery_workflow.cpp`
  - **Dependencies**: T011 (DeviceInfoCollector implementation)

### Implementation for User Story 1

- [X] **T011** [US1] Create `DeviceInfoCollector` component in `src/components/DeviceInfoCollector.h` and `.cpp`
  - Constructor: `DeviceInfoCollector(tN2kDeviceList*, SourceRegistry*, WebSocketLogger*)`
  - Method: `void init(ReactESP& app)` - setup 5-second ReactESP timer
  - Method: `uint8_t pollDeviceList()` - check `ReadResetIsListUpdated()`, extract metadata
  - Method: `uint8_t getDiscoveredCount()` and `unsigned long getLastPollTime()`
  - Private method: `void extractDeviceMetadata(const tDevice*, MessageSource*)` - copy all fields
  - Private method: `void checkDiscoveryTimeouts()` - mark sources as "Unknown (timeout)" after 60s
  - WebSocket logging: `DEVICE_DISCOVERED`, `DEVICE_UPDATED`, `DEVICE_TIMEOUT` events
  - **Files**: `src/components/DeviceInfoCollector.h`, `src/components/DeviceInfoCollector.cpp`
  - **Dependencies**: T004 (DeviceInfo struct), T005 (ManufacturerLookup), T007 (SourceRegistry::updateDeviceInfo)

- [X] **T012** [US1] Initialize `tN2kDeviceList` and `DeviceInfoCollector` in `main.cpp`
  - Create global `tN2kDeviceList* deviceList = nullptr;`
  - Create global `DeviceInfoCollector* deviceCollector = nullptr;`
  - In `setup()`: Initialize `deviceList = new tN2kDeviceList(nmea2000);` after NMEA2000 configuration
  - In `setup()`: Initialize `deviceCollector = new DeviceInfoCollector(deviceList, &sourceRegistry, &logger);`
  - In `setup()`: Call `deviceCollector->init(app);` to register ReactESP timer
  - **Files**: `src/main.cpp`
  - **Dependencies**: T011 (DeviceInfoCollector implementation)

- [X] **T013** [US1] Add WebSocket logging for device discovery events
  - Ensure `DeviceInfoCollector` logs at DEBUG level
  - Format: `{"sourceId":"NMEA2000-42","manufacturerCode":275,"manufacturer":"Garmin","modelId":"GPS 17x",...}`
  - Events: `DEVICE_DISCOVERED`, `DEVICE_UPDATED`, `DEVICE_TIMEOUT`
  - **Files**: `src/components/DeviceInfoCollector.cpp` (verify logging implementation from T011)
  - **Dependencies**: T011 (DeviceInfoCollector)

**Checkpoint**: User Story 1 complete - Device discovery working, WebSocket logs show device metadata

---

## Phase 4: User Story 2 - WebSocket Schema v2 Integration (Priority: P2)

**Goal**: Expose device metadata via `/source-stats` WebSocket endpoint (schema v2) for programmatic access

**Independent Test**: Connect WebSocket client to `ws://<ESP32_IP>/source-stats`, verify `version: 2` and `deviceInfo` in full snapshot and delta updates

### Tests for User Story 2 (TDD: Write tests FIRST)

- [ ] **T014** [P] [US2] Integration test for WebSocket full snapshot with device metadata in `test/test_device_discovery_integration/test_websocket_schema_v2.cpp`
  - Test full snapshot includes `version: 2`
  - Test full snapshot includes `deviceInfo` for NMEA2000 sources with `hasInfo: true`
  - Test full snapshot includes `deviceInfo.description` for NMEA0183 sources
  - **Files**: `test/test_device_discovery_integration/test_websocket_schema_v2.cpp`
  - **Dependencies**: T015 (SourceStatsSerializer v2 implementation)

- [ ] **T015** [P] [US2] Integration test for WebSocket delta updates with device metadata in `test/test_device_discovery_integration/test_websocket_deltas.cpp`
  - Test delta update when device discovered (includes complete `deviceInfo`)
  - Test delta update when device times out (`hasInfo: false`, `reason: "discovery_timeout"`)
  - Test delta update for frequency change (omits `deviceInfo` to minimize size)
  - **Files**: `test/test_device_discovery_integration/test_websocket_deltas.cpp`
  - **Dependencies**: T016 (SourceStatsSerializer delta serialization)

### Implementation for User Story 2

- [X] **T016** [US2] Update `SourceStatsSerializer` to include device metadata (schema v2)
  - Increment `SOURCE_STATS_SCHEMA_VERSION` from 1 to 2
  - Extend `serializeFullSnapshot()` to include `deviceInfo` object for each source
  - Extend `serializeDelta()` to include `deviceInfo` when device discovered/updated/timed out
  - For NMEA2000 sources: Serialize all `DeviceInfo` fields (`hasInfo`, `manufacturerCode`, `manufacturer`, `modelId`, `serialNumber`, `softwareVersion`, etc.)
  - For NMEA0183 sources: Include `deviceInfo.description` from `TalkerIdLookup`
  - Handle `hasInfo: false` case (placeholder: `"status": "Discovering..."` or `"status": "Unknown (timeout)"`)
  - **Files**: `src/components/SourceStatsSerializer.h`, `src/components/SourceStatsSerializer.cpp`
  - **Dependencies**: T004 (DeviceInfo struct), T006 (TalkerIdLookup)

- [X] **T017** [US2] Update `SourceStatsHandler` to send delta updates on device discovery
  - Ensure `hasChanges` flag set when device metadata updated
  - Send delta update when `DeviceInfoCollector` calls `SourceRegistry::updateDeviceInfo()`
  - Send delta update when discovery timeout occurs (60 seconds)
  - **Files**: `src/components/SourceStatsHandler.cpp`
  - **Dependencies**: T016 (SourceStatsSerializer v2)

**Checkpoint**: User Story 2 complete - WebSocket clients receive device metadata in v2 schema

---

## Phase 5: User Story 3 - WebUI Dashboard Display (Priority: P3)

**Goal**: Display device metadata in `/sources` web page with expandable details sections

**Independent Test**: Open `http://<ESP32_IP>:3030/sources`, verify expandable device details show manufacturer, model, serial, software version

### Implementation for User Story 3 (UI changes, no contract tests needed)

- [X] **T018** [US3] Update `data/sources.html` to display device metadata in expandable sections
  - Add expandable/collapsible UI for device details (JavaScript click handler)
  - Display fields: Manufacturer (with code), Model, Serial Number, Software Version
  - Display placeholder text:
    - "Device info: Discovering..." when `hasInfo: false` and within 60s
    - "Device info: Unknown (timeout)" when discovery timeout occurred
  - For NMEA0183 sources: Display talker ID description (e.g., "Autopilot" for AP)
  - Responsive layout: Stack fields vertically on mobile (320px viewport)
  - **Files**: `data/sources.html`
  - **Dependencies**: T016 (WebSocket schema v2 with deviceInfo)

- [X] **T019** [US3] Add CSS styling for device metadata display
  - Expandable section styling (collapsed/expanded states)
  - Mobile responsive layout (<= 768px breakpoint)
  - Icon for expand/collapse indicator
  - **Files**: `data/sources.html` (embedded CSS) or separate CSS file if exists
  - **Dependencies**: T018 (HTML structure)

**Checkpoint**: User Story 3 complete - Web UI displays device metadata for NMEA2000 sources

---

## Phase 6: User Story 4 - NMEA0183 Talker ID Descriptions (Priority: P4)

**Goal**: Display human-readable device type descriptions for NMEA0183 sources based on talker IDs

**Independent Test**: Connect NMEA0183 devices (AP, VH, GP), verify dashboard shows "Autopilot", "VHF Radio", "GPS Receiver"

### Tests for User Story 4 (TDD: Write tests FIRST)

- [ ] **T020** [P] [US4] Contract test for `TalkerIdLookup` in `test/test_device_discovery_contracts/test_talker_lookup.cpp`
  - Test known talker IDs (AP→Autopilot, GP→GPS Receiver, VH→VHF Radio)
  - Test unknown talker IDs (ZZ→"Unknown NMEA0183 Device")
  - Test case sensitivity (ap→Unknown, AP→Autopilot)
  - Test short strings gracefully handled
  - **Files**: `test/test_device_discovery_contracts/test_talker_lookup.cpp`
  - **Dependencies**: T006 (TalkerIdLookup implementation)

### Implementation for User Story 4

- [ ] **T021** [US4] Integrate `TalkerIdLookup` in `SourceRegistry` for NMEA0183 sources
  - In `SourceRegistry::recordUpdate()`: For NMEA0183 sources, populate `deviceInfo.description` using `TalkerIdLookup::getTalkerDescription(talkerId)`
  - Set `deviceInfo.hasInfo = false` (NMEA0183 has no discovery protocol)
  - **Files**: `src/components/SourceRegistry.cpp`
  - **Dependencies**: T006 (TalkerIdLookup)

- [ ] **T022** [US4] Update WebSocket serialization to include NMEA0183 descriptions
  - Ensure `SourceStatsSerializer` includes `deviceInfo.description` for NMEA0183 sources
  - Format: `{"sourceId": "NMEA0183-AP", "deviceInfo": {"hasInfo": false, "description": "Autopilot"}}`
  - **Files**: `src/components/SourceStatsSerializer.cpp` (verify implementation from T016)
  - **Dependencies**: T016 (SourceStatsSerializer v2)

- [ ] **T023** [US4] Update Web UI to display NMEA0183 talker descriptions
  - Display description alongside source name (e.g., "NMEA0183-AP [1.0 Hz, Fresh] - Autopilot")
  - **Files**: `data/sources.html` (verify implementation from T018)
  - **Dependencies**: T018 (WebUI device display)

**Checkpoint**: User Story 4 complete - NMEA0183 sources display human-readable device types

---

## Phase 7: Polish & Cross-Cutting Concerns

**Purpose**: Code quality, testing, and validation across all user stories

- [ ] **T024** [P] [POLISH] Unit test for `DeviceInfo` struct methods in `test/test_device_discovery_units/test_device_info.cpp`
  - Test `DeviceInfo::init()` default values
  - Test `isDiscoveryTimedOut()` with various timestamps (59s vs 61s)
  - Test `getStatusString()` for all states (Discovered, Discovering, Timeout)
  - **Files**: `test/test_device_discovery_units/test_device_info.cpp`

- [ ] **T025** [P] [POLISH] Unit test for string buffer safety in `test/test_device_discovery_units/test_string_safety.cpp`
  - Test long manufacturer names truncated to 15 chars
  - Test long model IDs truncated to 23 chars
  - Test long software versions truncated to 11 chars
  - Verify null termination
  - **Files**: `test/test_device_discovery_units/test_string_safety.cpp`

- [ ] **T026** [P] [POLISH] Performance validation test in `test/test_device_discovery_integration/test_performance.cpp`
  - Test `pollDeviceList()` completes in <10ms with 20 devices
  - Test WebSocket full snapshot generation <100ms
  - Test memory footprint: DeviceInfo struct ~72 bytes
  - **Files**: `test/test_device_discovery_integration/test_performance.cpp`

- [ ] **T027** [POLISH] Run `pio test -e native -f test_device_discovery_*` to execute all device discovery tests
  - Verify all contract tests pass
  - Verify all integration tests pass
  - Verify all unit tests pass
  - **Command**: `pio test -e native -f test_device_discovery_*`

- [ ] **T028** [POLISH] Hardware validation using `quickstart.md` test suite
  - Test 1: Device discovery logs (30-second discovery)
  - Test 2: WebSocket full snapshot (schema v2)
  - Test 3: Discovery timeout (60-second timeout)
  - Test 4: Web UI device display
  - Test 5: Hot-plug device discovery
  - Test 6: Multi-device differentiation
  - Test 7: Performance (<10ms poll cycle)
  - **Reference**: `specs/013-r013-nmea2000-device/quickstart.md`

- [ ] **T029** [POLISH] Code cleanup and documentation
  - Add Doxygen comments to all public methods
  - Update CLAUDE.md if implementation deviates from plan
  - Verify constitutional compliance (memory footprint, static allocation, WebSocket logging)
  - **Files**: All modified source files

---

## Dependencies & Execution Order

### Phase Dependencies

1. **Setup (Phase 1)**: No dependencies - can start immediately
2. **Foundational (Phase 2)**: Depends on Setup - BLOCKS all user stories
   - T004 MUST complete before T007 (SourceRegistry extension needs DeviceInfo struct)
   - T005, T006 can run in parallel with T004 (independent utilities)
3. **User Story 1 (Phase 3)**: Depends on Foundational (Phase 2) completion
   - Core device discovery - REQUIRED for all other stories
4. **User Story 2 (Phase 4)**: Depends on US1 (needs device metadata to serialize)
5. **User Story 3 (Phase 5)**: Depends on US2 (needs WebSocket v2 schema)
6. **User Story 4 (Phase 6)**: Can run in parallel with US2/US3 (independent NMEA0183 feature)
7. **Polish (Phase 7)**: Depends on all desired user stories being complete

### Within Each User Story

- **Tests FIRST** (TDD): Contract/integration tests written before implementation
- **Tests MUST FAIL**: Run tests after writing, verify they fail before implementing
- **Models → Services → Components**: Foundational data structures before business logic
- **Core implementation → Integration**: Complete core functionality before connecting pieces

### Parallel Opportunities

**Phase 2 (Foundational)**: After T004 completes:
```bash
# Run in parallel (different files):
Task: T005 - ManufacturerLookup
Task: T006 - TalkerIdLookup
```

**Phase 3 (US1 Tests)**: After T005, T010 (headers) complete:
```bash
# Run in parallel (different test files):
Task: T008 - Contract test ManufacturerLookup
Task: T009 - Contract test DeviceInfoCollector
Task: T010 - Integration test discovery workflow
```

**Phase 4 (US2 Tests)**: After T015 complete:
```bash
# Run in parallel (different test files):
Task: T014 - WebSocket schema v2 snapshot test
Task: T015 - WebSocket delta updates test
```

**Phase 7 (Polish)**: All unit tests can run in parallel:
```bash
# Run in parallel (different test files):
Task: T024 - DeviceInfo unit tests
Task: T025 - String safety unit tests
Task: T026 - Performance tests
```

---

## Parallel Example: Foundational Phase

After T004 (DeviceInfo struct) completes, launch parallel tasks:

```bash
# Task agent commands (hypothetical parallel execution):
Task: "Implement ManufacturerLookup utility in src/utils/ManufacturerLookup.h/.cpp with PROGMEM table"
Task: "Implement TalkerIdLookup utility in src/utils/TalkerIdLookup.h/.cpp with PROGMEM table"
```

Once T005 and T006 complete, T007 can start (SourceRegistry extension).

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup (T001-T003)
2. Complete Phase 2: Foundational (T004-T007) - CRITICAL BLOCKER
3. Complete Phase 3: User Story 1 (T008-T013)
4. **STOP and VALIDATE**: Hardware test with 2-3 NMEA2000 devices
5. Verify WebSocket logs show `DEVICE_DISCOVERED` events
6. Deploy/demo if ready

### Incremental Delivery (Priority Order)

1. Setup + Foundational → Foundation ready
2. **User Story 1** (P1) → Test independently → Deploy (MVP! Device discovery working)
3. **User Story 2** (P2) → Test independently → Deploy (Programmatic access via WebSocket)
4. **User Story 3** (P3) → Test independently → Deploy (Web UI visualization)
5. **User Story 4** (P4) → Test independently → Deploy (NMEA0183 descriptions)
6. Polish → Final validation → Production deployment

### Parallel Team Strategy

With multiple developers (after Foundational phase complete):

1. Team completes Setup + Foundational together (T001-T007)
2. Once Foundational done:
   - **Developer A**: User Story 1 (T008-T013) - Device discovery core
   - **Developer B**: User Story 4 (T020-T023) - NMEA0183 descriptions (independent)
3. After US1 complete:
   - **Developer A**: User Story 2 (T014-T017) - WebSocket integration
   - **Developer B**: Continue US4 or help with US2
4. After US2 complete:
   - **Developer A or B**: User Story 3 (T018-T019) - Web UI

---

## Critical Path

**Longest dependency chain** (cannot be parallelized):

```
T001 (Setup) →
T004 (DeviceInfo struct) →
T007 (SourceRegistry extension) →
T011 (DeviceInfoCollector implementation) →
T012 (main.cpp initialization) →
T016 (SourceStatsSerializer v2) →
T018 (Web UI update)
```

**Estimated time**: ~8-12 hours for MVP (US1 only), ~16-20 hours for all user stories with tests

---

## Notes

- **[P] tasks**: Different files, no dependencies, can run in parallel
- **[Story] label**: Maps task to specific user story for traceability (US1, US2, US3, US4)
- **TDD approach**: Tests written FIRST, verify they FAIL, then implement
- **Independent stories**: Each user story should be independently completable and testable
- **Commit strategy**: Commit after each task or logical group (e.g., after T011, after T016)
- **Validation checkpoints**: Stop after each user story to validate independently
- **Constitutional compliance**: All tasks respect static allocation, <10ms performance, WebSocket logging

**Tasks Generated**: 29 tasks total
- Phase 1 (Setup): 3 tasks
- Phase 2 (Foundational): 4 tasks (BLOCKS all stories)
- Phase 3 (US1 - P1): 6 tasks (MVP)
- Phase 4 (US2 - P2): 4 tasks
- Phase 5 (US3 - P3): 2 tasks
- Phase 6 (US4 - P4): 4 tasks
- Phase 7 (Polish): 6 tasks
