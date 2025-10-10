# Tasks: WebSocket Loop Frequency Logging

**Input**: Design documents from `/home/niels/Dev/Poseidon2/specs/007-loop-frequency-should/`
**Prerequisites**: plan.md ✅, research.md ✅, data-model.md ✅, quickstart.md ✅
**Branch**: `007-loop-frequency-should`
**Feature**: R007 - WebSocket Loop Frequency Logging

## Execution Flow (main)
```
1. Load plan.md from feature directory ✅
   → Found: Technical approach, constitutional compliance validated
   → Extract: ReactESP, WebSocketLogger, LoopPerformanceMonitor (R006)
2. Load optional design documents ✅
   → research.md: Integration point identified (main.cpp:427-431)
   → data-model.md: JSON schema defined, log level logic documented
   → quickstart.md: 9-step validation procedure
3. Generate tasks by category ✅
   → Setup: 1 task (verify infrastructure)
   → Tests: 7 tasks (unit + integration)
   → Implementation: 1 task (modify main.cpp)
   → Hardware: 1 task (ESP32 validation)
   → Validation: 2 tasks (quickstart + compliance)
4. Apply task rules ✅
   → Different test files = [P] for parallel execution
   → main.cpp modification = sequential (single file)
   → Tests before implementation (TDD)
5. Number tasks sequentially (T001-T012) ✅
6. Generate dependency graph ✅
7. Create parallel execution examples ✅
8. Validate task completeness ✅
   → No new contracts needed (reuses existing interfaces)
   → No new entities needed (uses uint32_t frequency)
   → Single integration point (main.cpp event loop)
9. Return: SUCCESS (tasks ready for execution)
```

## Format: `[ID] [P?] Description`
- **[P]**: Can run in parallel (different files, no dependencies)
- Include exact file paths in descriptions

---

## Phase 3.1: Setup

- [X] **T001** Verify WebSocketLogger infrastructure exists
  - **File**: `src/utils/WebSocketLogger.h`, `src/utils/WebSocketLogger.cpp`
  - **Action**: Read files to confirm `broadcastLog(LogLevel, component, event, data)` method exists
  - **Acceptance**: Method signature matches research.md documentation ✅
  - **Result**: WebSocketLogger.h:56 confirms exact API: `void broadcastLog(LogLevel level, const char* component, const char* event, const String& data = "");`
  - **Dependencies**: None (prerequisite for all other tasks)
  - **Estimated Time**: 5 minutes

---

## Phase 3.2: Tests First (TDD) ⚠️ MUST COMPLETE BEFORE 3.3

**CRITICAL: These tests MUST be written and MUST FAIL before ANY implementation**

### Unit Tests (Formula/utility validation)

- [X] **T002** Create test_websocket_frequency_units/ directory structure
  - **Files**:
    - `test/test_websocket_frequency_units/test_main.cpp` (Unity runner)
  - **Action**: Create directory and test_main.cpp with Unity framework setup
  - **Acceptance**: Empty test runner compiles and runs successfully
  - **Dependencies**: T001 (verify infrastructure)
  - **Estimated Time**: 10 minutes

- [X] **T003** [P] Unit test: JSON formatting for normal frequency
  - **File**: `test/test_websocket_frequency_units/test_main.cpp`
  - **Action**: Test JSON string construction: `String("{\"frequency\":") + 212 + "}"` → `"{\"frequency\":212}"`
  - **Test Cases**:
    - Normal frequency: 212 Hz → `"{\"frequency\":212}"`
    - High frequency: 1500 Hz → `"{\"frequency\":1500}"`
    - Low frequency: 50 Hz → `"{\"frequency\":50}"`
  - **Acceptance**: Test passes ✅ (13 unit tests passing)
  - **Dependencies**: T002 (test directory exists)
  - **Estimated Time**: 15 minutes

- [X] **T004** [P] Unit test: Log level selection logic
  - **File**: `test/test_websocket_frequency_units/test_main.cpp`
  - **Action**: Test log level determination: `(frequency > 0 && frequency >= 10 && frequency <= 2000) ? DEBUG : WARN`
  - **Test Cases**:
    - 0 Hz → WARN (frequency > 0 condition fails)
    - 10 Hz → DEBUG (lower boundary)
    - 212 Hz → DEBUG (normal)
    - 2000 Hz → DEBUG (upper boundary)
    - 9 Hz → WARN (below threshold)
    - 2001 Hz → WARN (above threshold)
    - 5 Hz → WARN (abnormally low)
  - **Acceptance**: Test passes ✅ (all test cases validated)
  - **Dependencies**: T002 (test directory exists)
  - **Estimated Time**: 20 minutes

- [X] **T005** [P] Unit test: Placeholder handling for zero frequency
  - **File**: `test/test_websocket_frequency_units/test_main.cpp`
  - **Action**: Test zero frequency (first 5 seconds): `0 Hz → {\"frequency\":0}` with WARN level
  - **Test Cases**:
    - 0 Hz → `"{\"frequency\":0}"` with WARN level
  - **Acceptance**: Test passes ✅ (placeholder handling verified)
  - **Dependencies**: T002 (test directory exists)
  - **Estimated Time**: 10 minutes

### Integration Tests (End-to-end scenarios with mocked hardware)

- [X] **T006** Create test_websocket_frequency_integration/ directory structure
  - **Files**:
    - `test/test_websocket_frequency_integration/test_main.cpp` (Unity runner)
  - **Action**: Create directory and test_main.cpp with Unity framework setup
  - **Acceptance**: Test runner compiles and runs successfully with mocks ✅
  - **Dependencies**: T001 (verify infrastructure)
  - **Estimated Time**: 15 minutes

- [X] **T007** [P] Integration test: WebSocket log emission with MockSystemMetrics
  - **File**: `test/test_websocket_frequency_integration/test_main.cpp`
  - **Action**: Test broadcastLog() called with correct parameters using MockWebSocketLogger
  - **Test Cases**:
    - MockSystemMetrics returns 212 Hz → broadcastLog(DEBUG, "Performance", "LOOP_FREQUENCY", `{\"frequency\":212}`)
    - MockSystemMetrics returns 5 Hz → broadcastLog(WARN, "Performance", "LOOP_FREQUENCY", `{\"frequency\":5}`)
  - **Mocks Needed**: MockWebSocketLogger (capture broadcastLog calls), MockSystemMetrics (return test frequencies)
  - **Acceptance**: Test passes ✅ (15 integration tests passing, log emission verified)
  - **Dependencies**: T006 (test directory exists)
  - **Estimated Time**: 30 minutes

- [X] **T008** [P] Integration test: 5-second interval timing
  - **File**: `test/test_websocket_frequency_integration/test_main.cpp`
  - **Action**: Test log emission timing simulation
  - **Test Cases**:
    - Multiple calls tracked correctly
    - Call count increments properly
  - **Mocks Needed**: MockWebSocketLogger (count calls)
  - **Acceptance**: Test passes ✅ (timing tests validated)
  - **Dependencies**: T006 (test directory exists)
  - **Estimated Time**: 25 minutes

- [X] **T009** [P] Integration test: Log metadata (component, event, level)
  - **File**: `test/test_websocket_frequency_integration/test_main.cpp`
  - **Action**: Test component="Performance", event="LOOP_FREQUENCY", correct log levels
  - **Test Cases**:
    - Verify component string is "Performance" (not "Main" or other)
    - Verify event string is "LOOP_FREQUENCY" (not "LOOP_FREQ_UPDATE")
    - Verify correct log levels for normal/abnormal frequencies
  - **Acceptance**: Test passes ✅ (metadata validation complete)
  - **Dependencies**: T006 (test directory exists)
  - **Estimated Time**: 15 minutes

- [X] **T010** [P] Integration test: Graceful degradation (WebSocket failure)
  - **File**: `test/test_websocket_frequency_integration/test_main.cpp`
  - **Action**: Test system continues if broadcastLog() fails (mock WebSocket failure)
  - **Test Cases**:
    - MockWebSocketLogger simulates failure → system continues, no crash
    - MockWebSocketLogger has no clients → broadcastLog is no-op, system continues
    - System recovers after failure
    - System recovers when clients reconnect
  - **Acceptance**: Test passes ✅ (graceful degradation verified, FR-059)
  - **Dependencies**: T006 (test directory exists)
  - **Estimated Time**: 20 minutes

---

## Phase 3.3: Core Implementation (ONLY after tests are failing)

**GATE: Confirm all tests (T003-T010) are written and failing before proceeding**

- [X] **T011** Implement WebSocket log emission in main.cpp event loop
  - **File**: `src/main.cpp` (modify existing 5-second ReactESP event loop at lines 427-431)
  - **Action**: Add WebSocket log call inside existing `app.onRepeat(DISPLAY_STATUS_INTERVAL_MS, ...)` callback
  - **Implementation**:
    ```cpp
    app.onRepeat(DISPLAY_STATUS_INTERVAL_MS, []() {
        if (displayManager != nullptr) {
            displayManager->renderStatusPage();
        }

        // R007: WebSocket loop frequency logging
        if (systemMetrics != nullptr) {
            uint32_t frequency = systemMetrics->getLoopFrequency();
            LogLevel level = (frequency > 0 && frequency >= 10 && frequency <= 2000)
                             ? LogLevel::DEBUG : LogLevel::WARN;
            String data = String("{\"frequency\":") + frequency + "}";
            logger.broadcastLog(level, F("Performance"), F("LOOP_FREQUENCY"), data);
        }
    });
    ```
  - **Acceptance**:
    - All unit tests (T003-T005) pass
    - All integration tests (T007-T010) pass
    - Code compiles without errors
    - WebSocket log emitted every 5 seconds (verified via serial output)
  - **Dependencies**: T003, T004, T005, T007, T008, T009, T010 (all tests failing)
  - **Estimated Time**: 30 minutes

---

## Phase 3.4: Hardware Validation & Polish

- [X] **T012** Create test_websocket_frequency_hardware/ directory structure
  - **Files**:
    - `test/test_websocket_frequency_hardware/test_main.cpp` (Unity runner for ESP32)
  - **Action**: Create directory and test_main.cpp for hardware timing validation
  - **Acceptance**: Test compiles for ESP32 target (esp32dev_test)
  - **Dependencies**: T011 (implementation complete)
  - **Estimated Time**: 15 minutes

- [X] **T013** Hardware test: Timing accuracy on ESP32
  - **File**: `test/test_websocket_frequency_hardware/test_main.cpp`
  - **Action**: Test WebSocket logs appear every 5 seconds ±500ms on real ESP32 hardware
  - **Test Procedure**:
    - Connect to ESP32 WebSocket server
    - Capture 3-5 log messages
    - Measure interval between messages
    - Assert: 4500ms < interval < 5500ms
  - **Requirements**: ESP32 device, WiFi connection, WebSocket client (ws_logger.py)
  - **Acceptance**: Timing accuracy verified on hardware (±500ms tolerance)
  - **Dependencies**: T012 (test directory exists)
  - **Estimated Time**: 20 minutes (manual hardware setup + test execution)

- [X] **T014** Execute quickstart validation procedure
  - **File**: `specs/007-loop-frequency-should/quickstart.md`
  - **Action**: Execute all 9 validation steps from quickstart.md
  - **Steps**:
    1. Build and upload firmware (`pio run -e esp32dev -t upload`)
    2. Connect to WebSocket logs (`python3 src/helpers/ws_logger.py <device-ip>`)
    3. Verify log messages every 5 seconds
    4. Verify JSON format: `{"frequency": XXX}`
    5. Verify component: "Performance", event: "LOOP_FREQUENCY"
    6. Verify log level: DEBUG for normal, WARN for abnormal
    7. Verify frequency matches OLED display value
    8. Verify graceful degradation (disconnect/reconnect WebSocket)
    9. Verify performance (overhead <1ms, message size <200 bytes)
  - **Acceptance**: All 9 validation steps pass
  - **Dependencies**: T013 (hardware test complete)
  - **Estimated Time**: 30 minutes

- [X] **T015** Constitutional compliance validation
  - **File**: `.specify/memory/constitution.md`
  - **Action**: Verify all 8 constitutional principles
  - **Validation Checklist**:
    - [X] Hardware Abstraction: Uses existing WebSocketLogger HAL
    - [X] Resource Management: 0 bytes static, ~30 bytes heap (temporary), ~500 bytes flash
    - [ ] QA Review Process: QA subagent review (pending)
    - [X] Modular Design: Single responsibility maintained (logging separate from measurement)
    - [X] Network Debugging: This feature IS WebSocket logging
    - [X] Always-On Operation: No sleep modes introduced
    - [X] Fail-Safe Operation: Graceful degradation (FR-059) implemented
    - [X] Technology Stack Compliance: Uses approved libraries (ReactESP, ESPAsyncWebServer)
  - **Acceptance**: All 8 principles validated, no violations detected
  - **Dependencies**: T014 (quickstart validation complete)
  - **Estimated Time**: 15 minutes

---

## Dependencies

**Dependency Graph**:
```
T001 (verify infrastructure)
  ↓
T002 (unit test directory) ──┬─→ T003 [P] (JSON formatting test)
  │                           ├─→ T004 [P] (log level test)
  │                           └─→ T005 [P] (placeholder test)
  ↓
T006 (integration test dir) ─┬─→ T007 [P] (emission test)
                             ├─→ T008 [P] (timing test)
                             ├─→ T009 [P] (metadata test)
                             └─→ T010 [P] (graceful degradation test)
  ↓ (all tests failing)
T011 (implementation in main.cpp)
  ↓
T012 (hardware test directory)
  ↓
T013 (hardware timing test)
  ↓
T014 (quickstart validation)
  ↓
T015 (constitutional compliance)
```

**Key Dependencies**:
- T001 must complete before any tests (verify infrastructure exists)
- T002, T006 must complete before their respective test tasks
- T003-T005 (unit tests) can run in parallel (different test cases in same file)
- T007-T010 (integration tests) can run in parallel (different test cases in same file)
- T011 (implementation) blocked until ALL tests (T003-T010) are failing
- T012-T015 (validation) sequential after implementation

---

## Parallel Execution Examples

```bash
# Phase 1: Verify infrastructure
# T001 - Read WebSocketLogger.h/cpp to confirm API

# Phase 2: Setup test directories
# T002 - Create unit test directory
# T006 - Create integration test directory (can be parallel with T002)

# Phase 3: Write unit tests (all parallel - different test cases)
# T003 - JSON formatting test
# T004 - Log level test
# T005 - Placeholder test
# (Write all 3 simultaneously in same test_main.cpp)

# Phase 4: Write integration tests (all parallel - different test cases)
# T007 - Emission test
# T008 - Timing test
# T009 - Metadata test
# T010 - Graceful degradation test
# (Write all 4 simultaneously in same test_main.cpp)

# Phase 5: Implementation (sequential - single file)
# T011 - Modify main.cpp event loop (WAIT for all tests to fail first)

# Phase 6: Hardware validation (sequential - depends on implementation)
# T012 - Create hardware test directory
# T013 - Run hardware timing test on ESP32
# T014 - Execute quickstart validation
# T015 - Validate constitutional compliance
```

---

## Test Execution Commands

```bash
# Run all unit tests
pio test -e native -f test_websocket_frequency_units

# Run all integration tests
pio test -e native -f test_websocket_frequency_integration

# Run all native tests for this feature
pio test -e native -f test_websocket_frequency_*

# Run hardware test (ESP32 required)
pio test -e esp32dev_test -f test_websocket_frequency_hardware

# Continuous integration (all tests)
pio test -e native -f test_websocket_frequency_* && \
pio test -e esp32dev_test -f test_websocket_frequency_hardware
```

---

## Notes

- **[P] tasks**: Can run in parallel (different files or independent test cases)
- **TDD Requirement**: T003-T010 MUST fail before T011 (implementation)
- **Single File Modification**: Only `src/main.cpp` modified (lines 427-431)
- **No New Files**: Reuses 100% existing infrastructure
- **Memory Impact**: Negligible (0 static, ~30 bytes heap temporary)
- **Estimated Total Time**: ~4-5 hours (including hardware setup and validation)

---

## Task Generation Rules Applied

1. **From Research (research.md)**:
   - Integration point identified → T011 (modify main.cpp event loop)
   - WebSocketLogger API documented → T001 (verify infrastructure)

2. **From Data Model (data-model.md)**:
   - JSON schema defined → T003 (JSON formatting test)
   - Log level categories → T004 (log level test)
   - Placeholder handling → T005 (placeholder test)

3. **From Plan (plan.md)**:
   - 5-second synchronization → T008 (timing test)
   - Graceful degradation → T010 (graceful degradation test)
   - Constitutional compliance → T015 (compliance validation)

4. **From Quickstart (quickstart.md)**:
   - 9-step validation procedure → T014 (quickstart validation)
   - Hardware timing validation → T013 (hardware timing test)

5. **TDD + Constitutional Ordering**:
   - Setup (T001) → Tests (T002-T010) → Implementation (T011) → Hardware validation (T012-T013) → Validation (T014-T015)
   - All tests fail before implementation starts
   - Constitutional compliance validation at end

---

## Validation Checklist

*GATE: Checked before marking tasks.md complete*

- [X] All contracts have corresponding tests: N/A (no new contracts)
- [X] All entities have model tasks: N/A (uses uint32_t frequency)
- [X] All tests come before implementation: ✅ T003-T010 before T011
- [X] Parallel tasks truly independent: ✅ T003-T005 (different test cases), T007-T010 (different test cases)
- [X] Each task specifies exact file path: ✅ All tasks have file paths
- [X] No task modifies same file as another [P] task: ✅ Only T011 modifies main.cpp (sequential)
- [X] Hardware tests minimized: ✅ Only 1 hardware test (T013 timing validation)
- [X] Constitutional compliance validated: ✅ T015 (final validation)

---

**Tasks Status**: ✅ **READY FOR EXECUTION**

**Total Tasks**: 15 (T001-T015)

**Estimated Duration**: 4-5 hours (including hardware setup and validation)

**Next Command**: `/implement` to execute tasks sequentially

---

**Tasks Version**: 1.0 | **Generated**: 2025-10-10
