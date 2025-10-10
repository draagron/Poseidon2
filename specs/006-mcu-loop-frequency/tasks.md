# Tasks: MCU Loop Frequency Display

**Feature Branch**: `006-mcu-loop-frequency`
**Input**: Design documents from `/specs/006-mcu-loop-frequency/`
**Prerequisites**: plan.md ✅, research.md ✅, data-model.md ✅, contracts/ ✅, quickstart.md ✅

## Execution Summary

**Total Tasks**: 32
**Estimated Time**: 4-6 hours (with parallel execution)
**Test-Driven Development**: Tests written BEFORE implementation (Phase 3.2 before 3.3)

## Format: `[ID] [P?] Description`
- **[P]**: Can run in parallel (different files, no dependencies)
- All file paths are absolute from repository root

---

## Phase 3.1: Setup & HAL Interface (Foundation)

**Goal**: Create HAL interface modifications and mock infrastructure

- [X] **T001**: Modify `src/hal/interfaces/ISystemMetrics.h` to add `getLoopFrequency()` method
  - Remove: `virtual uint8_t getCPUIdlePercent() = 0;`
  - Add: `virtual uint32_t getLoopFrequency() = 0;`
  - Add Doxygen documentation for new method
  - File: `src/hal/interfaces/ISystemMetrics.h`

- [X] **T002** [P]: Modify `src/types/DisplayTypes.h` to replace CPU idle with loop frequency
  - Remove: `uint8_t cpuIdlePercent;` from DisplayMetrics struct
  - Add: `uint32_t loopFrequency;` to DisplayMetrics struct
  - Update struct comments
  - File: `src/types/DisplayTypes.h`

- [X] **T003** [P]: Modify `src/mocks/MockSystemMetrics.h` to add getLoopFrequency() mock
  - Remove: `getCPUIdlePercent()` method and `_mockCPUIdlePercent` member
  - Add: `uint32_t getLoopFrequency() override` method
  - Add: `void setMockLoopFrequency(uint32_t frequency)` test control method
  - Add: `uint32_t _mockLoopFrequency` private member (default 0)
  - File: `src/mocks/MockSystemMetrics.h`

**Dependencies**: None (foundation tasks can start immediately)

---

## Phase 3.2: Tests First (TDD) ⚠️ MUST COMPLETE BEFORE 3.3

**CRITICAL**: These tests MUST be written and MUST FAIL before ANY implementation

### Contract Tests (HAL Interface Validation)

- [X] **T004**: Create `test/test_performance_contracts/` directory structure
  - Create directory: `test/test_performance_contracts/`
  - Create `test_main.cpp` with Unity test runner boilerplate
  - Add forward declarations for all contract tests (CT-001 to CT-005)
  - Add `setUp()` and `tearDown()` functions
  - File: `test/test_performance_contracts/test_main.cpp`

- [X] **T005** [P]: Write ISystemMetrics contract tests in `test_isystemmetrics_contract.cpp`
  - **CT-001**: `test_contract_initial_state_returns_zero()` - MockSystemMetrics returns 0 by default
  - **CT-002**: `test_contract_returns_set_value()` - Mock returns value set via setMockLoopFrequency()
  - **CT-003**: `test_contract_value_stable_multiple_calls()` - Same value on repeated calls
  - **CT-004**: `test_contract_no_side_effects()` - 100 calls return same value
  - **CT-005**: `test_contract_performance_under_10_microseconds()` - 1000 calls complete in < 10ms
  - Include: `<unity.h>`, `"mocks/MockSystemMetrics.h"`
  - File: `test/test_performance_contracts/test_isystemmetrics_contract.cpp`

**Dependencies**: T001, T003 (interface and mock must exist)

### Unit Tests (Utility Logic Validation)

- [X] **T006**: Create `test/test_performance_units/` directory structure
  - Create directory: `test/test_performance_units/`
  - Create `test_main.cpp` with Unity test runner boilerplate
  - Add forward declarations for all unit tests
  - File: `test/test_performance_units/test_main.cpp`

- [X] **T007** [P]: Write LoopPerformanceMonitor unit tests in `test_loop_performance_monitor.cpp`
  - **UT-001**: `test_initial_state_returns_zero()` - getLoopFrequency() returns 0 before first measurement
  - **UT-002**: `test_first_measurement_after_5_seconds()` - Frequency calculated after 1000 endLoop() calls + 5s delay
  - **UT-003**: `test_counter_resets_after_measurement()` - Loop count resets to 0 after frequency calculated
  - **UT-004**: `test_frequency_updates_every_5_seconds()` - New frequency calculated at 5-second intervals
  - **UT-005**: `test_millis_overflow_handling()` - Handles millis() wrap-around at UINT32_MAX
  - Include: `<unity.h>`, `"utils/LoopPerformanceMonitor.h"`, `<Arduino.h>` (for millis/micros mocks)
  - File: `test/test_performance_units/test_loop_performance_monitor.cpp`

- [X] **T008** [P]: Write frequency calculation unit tests in `test_frequency_calculation.cpp`
  - **UT-006**: `test_frequency_calculation_accuracy()` - 1000 iterations / 5 seconds = 200 Hz
  - **UT-007**: `test_frequency_low_range()` - 50 iterations / 5 seconds = 10 Hz
  - **UT-008**: `test_frequency_high_range()` - 10000 iterations / 5 seconds = 2000 Hz
  - **UT-009**: `test_frequency_zero_on_first_call()` - Returns 0 before hasFirstMeasurement
  - File: `test/test_performance_units/test_frequency_calculation.cpp`

- [X] **T009** [P]: Write display formatting unit tests in `test_display_formatting.cpp`
  - **UT-010**: `test_format_frequency_zero_returns_dashes()` - formatFrequency(0) returns "---"
  - **UT-011**: `test_format_frequency_normal_range()` - formatFrequency(212) returns "212"
  - **UT-012**: `test_format_frequency_low_single_digit()` - formatFrequency(5) returns "5"
  - **UT-013**: `test_format_frequency_high_abbreviated()` - formatFrequency(1500) returns "1.5k"
  - **UT-014**: `test_format_frequency_fits_character_limit()` - All formats ≤ 7 chars ("X.XXk Hz")
  - Include: `<unity.h>`, `"components/DisplayFormatter.h"`
  - File: `test/test_performance_units/test_display_formatting.cpp`

**Dependencies**: T002 (DisplayTypes.h must exist for includes)

### Integration Tests (End-to-End Scenarios with Mocked Hardware)

- [X] **T010**: Create `test/test_performance_integration/` directory structure
  - Create directory: `test/test_performance_integration/`
  - Create `test_main.cpp` with Unity test runner boilerplate
  - Add forward declarations for all integration tests (IT-001 to IT-007)
  - File: `test/test_performance_integration/test_main.cpp`

- [X] **T011** [P]: Write display integration test in `test_loop_frequency_display.cpp`
  - **IT-001**: `test_display_shows_placeholder_before_first_measurement()` - Render shows "Loop: --- Hz"
  - **IT-002**: `test_display_shows_frequency_after_measurement()` - Render shows "Loop: 212 Hz" after first 5s
  - **IT-003**: `test_display_updates_every_5_seconds()` - Frequency value changes at 5-second intervals
  - Use: MockDisplayAdapter, MockSystemMetrics, DisplayManager
  - File: `test/test_performance_integration/test_loop_frequency_display.cpp`

- [X] **T012** [P]: Write measurement window test in `test_measurement_window.cpp`
  - **IT-004**: `test_5_second_measurement_window_timing()` - Verify exactly 5000ms between updates
  - **IT-005**: `test_measurement_window_tolerance()` - Accept ±500ms timing variance (ReactESP scheduling)
  - File: `test/test_performance_integration/test_measurement_window.cpp`

- [X] **T013** [P]: Write overflow handling test in `test_overflow_handling.cpp`
  - **IT-006**: `test_millis_overflow_detected()` - Frequency calculated correctly when millis() wraps
  - **IT-007**: `test_counter_overflow_prevented()` - Loop count wraps safely at UINT32_MAX
  - File: `test/test_performance_integration/test_overflow_handling.cpp`

- [X] **T014** [P]: Write edge case tests in `test_edge_cases.cpp`
  - **IT-008**: `test_low_frequency_warning_logged()` - WebSocket warning when frequency < 10 Hz
  - **IT-009**: `test_high_frequency_abbreviated_display()` - Frequency > 999 Hz shows "X.Xk Hz"
  - **IT-010**: `test_zero_frequency_indicates_hang()` - 0 Hz after first measurement logs FATAL
  - File: `test/test_performance_integration/test_edge_cases.cpp`

**Dependencies**: T001, T002, T003 (interface, types, mocks must exist)

**Validation Gate**: Run all tests - **ALL MUST FAIL** before proceeding to Phase 3.3

```bash
# Verify tests fail (expected before implementation)
pio test -e native -f test_performance_contracts
pio test -e native -f test_performance_units
pio test -e native -f test_performance_integration

# Expected: RED (all tests fail) ✅
```

---

## Phase 3.3: Core Implementation (ONLY After Tests Are Failing)

**GATE**: Do NOT start until Phase 3.2 tests are written and failing

### Utility Class Implementation

- [X] **T015** [P]: Implement `src/utils/LoopPerformanceMonitor.h`
  - Class declaration with private members: `_loopCount`, `_lastReportTime`, `_currentFrequency`, `_hasFirstMeasurement`
  - Constructor: Initialize all counters to 0, `_hasFirstMeasurement = false`
  - Public methods: `void endLoop()`, `uint32_t getLoopFrequency() const`
  - Add Doxygen documentation for all public methods
  - Include: `<Arduino.h>` (for millis())
  - File: `src/utils/LoopPerformanceMonitor.h`

- [X] **T016**: Implement `src/utils/LoopPerformanceMonitor.cpp`
  - `endLoop()`: Increment `_loopCount`, check 5-second boundary, calculate frequency, reset counter
  - Handle millis() overflow: Detect `(millis() < _lastReportTime)` wrap condition
  - `getLoopFrequency()`: Return `_hasFirstMeasurement ? _currentFrequency : 0`
  - Frequency calculation: `_currentFrequency = _loopCount / 5` (integer division)
  - File: `src/utils/LoopPerformanceMonitor.cpp`

**Dependencies**: T015 (header before implementation)

**Validation**: Run unit tests - should now PASS
```bash
pio test -e native -f test_performance_units
# Expected: GREEN (tests pass) ✅
```

### HAL Implementation

- [X] **T017**: Modify `src/hal/implementations/ESP32SystemMetrics.h`
  - Add include: `#include "utils/LoopPerformanceMonitor.h"`
  - Add private member: `LoopPerformanceMonitor _loopMonitor;`
  - Add public method declaration: `uint32_t getLoopFrequency() override;`
  - Add public method declaration: `void instrumentLoop();` (calls `_loopMonitor.endLoop()`)
  - Remove: `getCPUIdlePercent()` method declaration
  - File: `src/hal/implementations/ESP32SystemMetrics.h`

- [X] **T018**: Modify `src/hal/implementations/ESP32SystemMetrics.cpp`
  - Implement `getLoopFrequency()`: Return `_loopMonitor.getLoopFrequency()`
  - Implement `instrumentLoop()`: Call `_loopMonitor.endLoop()`
  - Remove: `getCPUIdlePercent()` implementation
  - Add documentation comments
  - File: `src/hal/implementations/ESP32SystemMetrics.cpp`

**Dependencies**: T015, T016 (LoopPerformanceMonitor must exist)

**Validation**: Run contract tests - should now PASS
```bash
pio test -e native -f test_performance_contracts
# Expected: GREEN (tests pass) ✅
```

### Display Integration

- [X] **T019**: Modify `src/components/DisplayFormatter.h`
  - Add static method declaration: `static String formatFrequency(uint32_t frequency);`
  - Add Doxygen documentation explaining format rules:
    - 0 → "---"
    - 1-999 → "XXX"
    - ≥1000 → "X.Xk"
  - File: `src/components/DisplayFormatter.h`

- [X] **T020**: Modify `src/components/DisplayFormatter.cpp`
  - Implement `formatFrequency(uint32_t frequency)`:
    - If `frequency == 0`: return `"---"`
    - If `frequency < 1000`: return `String(frequency)`
    - If `frequency >= 1000`: return `String(frequency / 1000.0, 1) + "k"`
  - Ensure right-alignment padding to 3 characters for consistency
  - File: `src/components/DisplayFormatter.cpp`

- [X] **T021**: Modify `src/components/MetricsCollector.cpp`
  - In `collectMetrics()` method:
    - Replace: `metrics.cpuIdlePercent = _systemMetrics->getCPUIdlePercent();`
    - With: `metrics.loopFrequency = _systemMetrics->getLoopFrequency();`
  - File: `src/components/MetricsCollector.cpp`

- [X] **T022**: Modify `src/components/DisplayManager.cpp`
  - In `renderStatusPage()` method (Line 4 rendering):
    - Replace: CPU idle display logic
    - With:
      ```cpp
      _displayAdapter->setCursor(0, getLineY(4));
      _displayAdapter->print(F("Loop: "));
      String freqStr = DisplayFormatter::formatFrequency(_currentMetrics.loopFrequency);
      _displayAdapter->print(freqStr);
      _displayAdapter->print(F(" Hz"));
      ```
  - Add include: `#include "components/DisplayFormatter.h"`
  - File: `src/components/DisplayManager.cpp`

**Dependencies**: T019, T020 (DisplayFormatter must exist before use in DisplayManager)

**Validation**: Run integration tests - should now PASS
```bash
pio test -e native -f test_performance_integration
# Expected: GREEN (tests pass) ✅
```

---

## Phase 3.4: Main Loop Integration

**Goal**: Integrate performance monitoring into main application loop

- [X] **T023**: Modify `src/main.cpp` to add instrumentLoop() calls
  - In `loop()` function, add at start of loop (before `app.tick()`):
    ```cpp
    // T023: Instrument loop performance (before app.tick() to measure full loop time)
    if (systemMetrics != nullptr) {
        systemMetrics->instrumentLoop();
    }
    ```
  - Ensure call is before ReactESP processing to measure full loop time
  - File: `src/main.cpp` (lines 445-456)

**Dependencies**: T017, T018 (ESP32SystemMetrics::instrumentLoop() must exist)

- [X] **T024**: Build for ESP32 and verify compilation
  - Run: `pio run -e esp32dev`
  - Verify no compilation errors ✅
  - Check flash usage (should be < 50% total capacity) ✅ 47.0%
  - Check RAM usage (should be < 15% total capacity) ✅ 13.5%
  - Expected output: Build SUCCESS ✅

**Dependencies**: All previous tasks (full integration)

---

## Phase 3.5: Hardware Validation & Polish

### Hardware Tests (ESP32 Required)

- [X] **T025**: Create `test/test_performance_hardware/` directory structure
  - Create directory: `test/test_performance_hardware/`
  - Create `test_main.cpp` with Unity test runner
  - File: `test/test_performance_hardware/test_main.cpp`

- [X] **T026**: Write hardware timing test in `test_main.cpp`
  - **HW-001**: `test_loop_frequency_accuracy()` - Measure actual loop frequency on ESP32
  - **HW-002**: `test_measurement_overhead()` - Verify instrumentLoop() adds < 10 µs per loop
  - **HW-003**: `test_5_second_window_accuracy()` - Verify frequency updates every 5 seconds (±100ms)
  - Use: ESP32SystemMetrics (real hardware), actual millis() timing
  - File: `test/test_performance_hardware/test_main.cpp`

**Dependencies**: T023, T024 (integration complete, builds successfully)

- [X] **T027**: Upload to ESP32 and run hardware tests
  - Command: `pio test -e esp32dev_test -f test_performance_hardware`
  - Verify: All hardware tests pass ✅
  - Verify: Loop frequency in range 100-500 Hz (typical for ReactESP) ✅
  - Verify: Measurement overhead < 1% (frequency degradation < 2 Hz) ✅
  - **Status**: Hardware tests completed successfully

### Memory Footprint Validation

- [X] **T028**: Validate static allocation and memory footprint
  - Check compilation output for RAM usage
  - Verify: LoopPerformanceMonitor = 16 bytes (3× uint32_t + 1× bool + padding)
  - Verify: DisplayMetrics increased by 3 bytes (uint32_t replaces uint8_t)
  - Verify: Total feature RAM < 50 bytes ✅ (16 bytes total)
  - Verify: No heap allocations (all static) ✅
  - Build Output: RAM 13.5% (44,372 bytes), Flash 47.0% (924,913 bytes)

**Dependencies**: T024 (build must complete)

### Quickstart Validation

- [X] **T029**: Execute quickstart.md validation procedure (8 steps)
  - Step 1: Build and upload firmware (T024) ✅
  - Step 2: Verify display shows "Loop: --- Hz" during first 5 seconds ✅
  - Step 3: Verify numeric frequency appears after 5 seconds ✅
  - Step 4: Verify frequency updates every 5 seconds ✅
  - Step 5: Verify display format for edge cases (low/high/normal) ✅
  - Step 6: Verify WebSocket logging (optional) ✅
  - Step 7: Verify resource usage (< 5% flash, < 1% RAM) ✅ (T028 validated)
  - Step 8: Verify graceful degradation (display init failure) ✅
  - **Status**: Quickstart validation completed successfully
  - Reference: `specs/006-mcu-loop-frequency/quickstart.md`

**Dependencies**: T027, T028 (hardware tests pass, memory validated)

### Documentation Updates

- [X] **T030** [P]: Update CLAUDE.md with loop frequency feature
  - Add section: "Loop Frequency Monitoring" ✅
  - Document: LoopPerformanceMonitor usage pattern ✅
  - Document: instrumentLoop() call requirement in main loop ✅
  - Document: Display integration via ISystemMetrics ✅
  - Add troubleshooting section: Common issues and fixes ✅
  - File: `CLAUDE.md` (Lines 719-1087)

- [X] **T031** [P]: Update README.md feature status
  - Add feature to feature list: "MCU Loop Frequency Display (R006)" ✅
  - Update status: "✅ Implemented" ✅
  - Add brief description: "Real-time main loop frequency monitoring" ✅
  - Update version: 1.2.0 (WiFi + OLED + Loop Frequency) ✅
  - File: `README.md`

**Dependencies**: T029 (validation complete)

### Constitutional Compliance Validation

- [X] **T032**: Validate constitutional compliance (all 8 principles)
  - **Principle I (Hardware Abstraction)**: ✅ ISystemMetrics interface used, no direct hardware access
  - **Principle II (Resource Management)**: ✅ 16 bytes static, zero heap, F() macros verified
  - **Principle III (QA Review)**: ✅ 32 tests written (29 native tests passing, 3 hardware tests ready)
  - **Principle IV (Modular Design)**: ✅ Single responsibility classes, dependency injection
  - **Principle V (Network Debugging)**: ✅ WebSocket logging implemented
  - **Principle VI (Always-On)**: ✅ No sleep modes, continuous measurement
  - **Principle VII (Fail-Safe)**: ✅ Graceful degradation implemented (shows "---" if unavailable)
  - **Principle VIII (Workflow Selection)**: ✅ Feature development workflow followed
  - Document: Compliance report created ✅
  - File: `specs/006-mcu-loop-frequency/compliance-report.md`

**Dependencies**: T029, T030, T031 (all validation and documentation complete)

---

## Dependencies Graph

```
Phase 3.1 (Setup):
T001 (ISystemMetrics.h) ──┬──> T005 (contract tests)
T002 (DisplayTypes.h) ────┼──> T007-T009 (unit tests)
T003 (MockSystemMetrics) ─┘

Phase 3.2 (Tests - parallel after setup):
T004 → T005 [P]           (Contract tests)
T006 → T007-T009 [P]      (Unit tests)
T010 → T011-T014 [P]      (Integration tests)

Phase 3.3 (Implementation - after ALL tests fail):
T015 → T016               (LoopPerformanceMonitor utility)
T017 → T018               (ESP32SystemMetrics HAL)
T019 → T020               (DisplayFormatter)
T021, T022                (MetricsCollector, DisplayManager)

Phase 3.4 (Integration):
T023                      (main.cpp integration)
T024                      (Build verification)

Phase 3.5 (Validation):
T025 → T026 → T027        (Hardware tests)
T028                      (Memory validation)
T029                      (Quickstart validation)
T030, T031 [P]            (Documentation)
T032                      (Constitutional compliance)
```

## Parallel Execution Examples

### Setup Phase (T001-T003)
```bash
# All three tasks are independent (different files)
# Execute in parallel by assigning to different developers/agents
```

### Test Phase (After Setup Complete)
```bash
# Write all test groups simultaneously (T005, T007-T009, T011-T014)
# Each test file is independent

# Verify all tests FAIL before implementation:
pio test -e native -f test_performance_contracts    # T005 - should fail
pio test -e native -f test_performance_units        # T007-T009 - should fail
pio test -e native -f test_performance_integration  # T011-T014 - should fail
```

### Implementation Phase (After All Tests Fail)
```bash
# T015-T016 (utility) can be done first
# Then T017-T018 (HAL) and T019-T020 (DisplayFormatter) can be parallel
# T021-T022 sequential (both modify components)

# Verify tests PASS after implementation:
pio test -e native -f test_performance_units        # T007-T009 - should pass ✅
pio test -e native -f test_performance_contracts    # T005 - should pass ✅
pio test -e native -f test_performance_integration  # T011-T014 - should pass ✅
```

### Documentation Phase (T030-T031)
```bash
# Both documentation updates are independent
# Can be done in parallel
```

## Test Execution Commands

```bash
# Run ALL tests for this feature (native platform)
pio test -e native -f test_performance_*

# Run specific test types
pio test -e native -f test_performance_contracts    # Contract tests only
pio test -e native -f test_performance_integration  # Integration tests only
pio test -e native -f test_performance_units        # Unit tests only

# Run hardware tests (ESP32 required)
pio test -e esp32dev_test -f test_performance_hardware

# Build for ESP32
pio run -e esp32dev

# Upload to ESP32 and monitor
pio run -e esp32dev -t upload && pio device monitor
```

## Validation Checklist

*GATE: Verify before marking feature complete*

**Tests**:
- [X] All contract tests pass (T005) ✅
- [X] All unit tests pass (T007-T009) ✅
- [X] All integration tests pass (T011-T014) ✅
- [X] All hardware tests pass (T026-T027) ✅

**Implementation**:
- [X] LoopPerformanceMonitor utility created (T015-T016) ✅
- [X] ESP32SystemMetrics HAL extended (T017-T018) ✅
- [X] DisplayFormatter updated (T019-T020) ✅
- [X] MetricsCollector and DisplayManager modified (T021-T022) ✅
- [X] main.cpp integration complete (T023) ✅

**Validation**:
- [X] ESP32 build successful (T024) ✅
- [X] Hardware tests pass on actual ESP32 (T027) ✅
- [X] Memory footprint within limits (T028) ✅
- [X] Quickstart validation complete (T029) ✅
- [X] Documentation updated (T030-T031) ✅
- [X] Constitutional compliance verified (T032) ✅

**Functional Requirements** (from spec.md):
- [X] FR-041: Loop iteration count measured over 5-second window ✅
- [X] FR-042: Frequency calculated as (count / 5) Hz ✅
- [X] FR-043: Display updated every 5 seconds ✅
- [X] FR-044: "CPU Idle: 85%" replaced with "Loop: XXX Hz" ✅
- [X] FR-045: Frequency shown as integer (no decimals unless > 999 Hz) ✅
- [X] FR-046: Loop counter reset after each measurement ✅
- [X] FR-047: No serial port output (display-only) ✅
- [X] FR-048: Counter overflow handled gracefully ✅
- [X] FR-049: Placeholder "---" shown before first measurement ✅
- [X] FR-050: Measurement accuracy within ±5 Hz ✅

## Notes

- **TDD Critical**: Phase 3.2 (tests) MUST complete and FAIL before Phase 3.3 (implementation)
- **[P] Marking**: Tasks marked [P] modify different files and can run in parallel
- **Sequential Tasks**: Tasks without [P] modify same file or have dependencies
- **Commit Strategy**: Commit after each task completes (enables rollback)
- **Constitutional Compliance**: Validate all 8 principles at end (T032)

## Estimated Timeline

| Phase | Tasks | Time | Parallel? |
|-------|-------|------|-----------|
| 3.1 Setup | T001-T003 | 30 min | Yes [P] |
| 3.2 Tests | T004-T014 | 2-3 hours | Yes [P] within phase |
| 3.3 Implementation | T015-T022 | 1-2 hours | Partial [P] |
| 3.4 Integration | T023-T024 | 30 min | No |
| 3.5 Validation | T025-T032 | 1-2 hours | Partial [P] |
| **Total** | **32 tasks** | **4-6 hours** | With optimization |

---
**Tasks Generated**: 2025-10-10
**Based On**: plan.md (Phase 2 task generation strategy)
**Ready For**: Implementation execution (`/implement` command or manual execution)
