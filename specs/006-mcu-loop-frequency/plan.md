# Implementation Plan: MCU Loop Frequency Display

**Branch**: `006-mcu-loop-frequency` | **Date**: 2025-10-10 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `/specs/006-mcu-loop-frequency/spec.md`

## Execution Flow (/plan command scope)
```
1. ✅ Load feature spec from Input path
2. ✅ Fill Technical Context (no NEEDS CLARIFICATION markers)
3. ✅ Fill Constitution Check section
4. ✅ Evaluate Constitution Check → All principles satisfied
5. ✅ Execute Phase 0 → research.md created
6. ✅ Execute Phase 1 → contracts, data-model.md, quickstart.md, CLAUDE.md updated
7. ✅ Re-evaluate Constitution Check → No violations
8. ✅ Plan Phase 2 → Task generation approach described
9. ✅ STOP - Ready for /tasks command
```

**STATUS**: ✅ Planning complete - Ready for `/tasks` command

## Summary

Replace static "CPU Idle: 85%" display metric with dynamic "MCU Loop Frequency: XXX Hz" measurement. The system will measure actual main loop iteration count over 5-second windows and display the average frequency on the OLED display. Implementation uses lightweight utility class (16 bytes RAM), integrates with existing `ISystemMetrics` HAL interface, and follows constitutional principles for resource management and fail-safe operation.

**Key Features**:
- Real-time loop frequency monitoring (5-second measurement window)
- Display format: "Loop: XXX Hz" (fits 21-character line limit)
- Graceful degradation: Shows "---" placeholder before first measurement
- Minimal overhead: < 1% performance impact, < 5 µs per loop
- Static allocation only: 16 bytes RAM total

## Technical Context

**Language/Version**: C++ (C++14 via Arduino framework for ESP32)
**Primary Dependencies**: Arduino.h (micros/millis), ReactESP (event loops), Adafruit_SSD1306 (display)
**Storage**: In-memory only (counters in utility class, no persistent storage)
**Testing**: Unity test framework on native platform + ESP32 hardware tests
**Target Platform**: ESP32 (SH-ESP32 board) - 240 MHz dual-core, 320 KB RAM, 4 MB flash
**Project Type**: ESP32 embedded system with PlatformIO grouped test organization
**Performance Goals**: < 1% measurement overhead, ±5 Hz accuracy, 5-second update interval
**Constraints**: < 50ms display update, 21-character line limit, 5-second measurement window
**Scale/Scope**: Single device, continuous measurement, 5-second averaging window

## Constitution Check
*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

**Hardware Abstraction (Principle I)**:
- [x] All hardware interactions use HAL interfaces (`ISystemMetrics`, `IDisplayAdapter`)
- [x] Mock implementations provided for testing (`MockSystemMetrics`)
- [x] Business logic separable from hardware I/O (LoopPerformanceMonitor is pure logic)

**Resource Management (Principle II)**:
- [x] Static allocation preferred; heap usage justified (16 bytes static, ZERO heap)
- [x] Stack usage estimated and within 8KB per task (< 100 bytes on stack)
- [x] Flash usage impact documented (< 5% / ~100 KB for feature)
- [x] String literals use F() macro or PROGMEM (all display strings use F())

**QA Review Process (Principle III - NON-NEGOTIABLE)**:
- [x] QA subagent review planned for all code changes (post-implementation)
- [x] Hardware-dependent tests minimized (only timing validation on ESP32)
- [x] Critical paths flagged for human review (main loop instrumentation)

**Modular Design (Principle IV)**:
- [x] Components have single responsibility (LoopPerformanceMonitor = measurement only)
- [x] Dependency injection used for hardware dependencies (ISystemMetrics owns monitor)
- [x] Public interfaces documented (HAL contract created)

**Network Debugging (Principle V)**:
- [x] WebSocket logging implemented for frequency updates and warnings
- [x] Log levels defined (DEBUG for updates, WARN for anomalies)
- [x] Flash fallback for critical errors (display init failure logged)

**Always-On Operation (Principle VI)**:
- [x] WiFi always-on requirement met (no power management changes)
- [x] No deep sleep/light sleep modes used (continuous measurement)
- [x] Designed for 24/7 operation (counter overflow handled)

**Fail-Safe Operation (Principle VII)**:
- [x] Watchdog timer enabled (production) - no blocking operations
- [x] Safe mode/recovery mode implemented (graceful degradation if display fails)
- [x] Graceful degradation for failures (shows "---" if measurement unavailable)

**Workflow Selection (Principle VIII)**:
- [x] Using Feature Development workflow (`/specify` → `/plan` → `/tasks` → `/implement`)
- [x] Full specification created (spec.md with 10 FR + 3 NFR)
- [x] TDD approach planned (tests written before implementation)

**Technology Stack Compliance**:
- [x] Using approved libraries (Arduino.h, ReactESP, Adafruit_SSD1306)
- [x] File organization follows src/ structure (hal/, components/, utils/, mocks/)
- [x] Conventional commits format (will use "feat:" prefix)

**Overall Assessment**: ✅ **PASS** - All constitutional principles satisfied, no violations

## Project Structure

### Documentation (this feature)
```
specs/006-mcu-loop-frequency/
├── spec.md              ✅ Complete - 10 FR + 3 NFR, no clarifications needed
├── plan.md              ✅ This file (Phase 0-1 complete)
├── research.md          ✅ Complete - All technical decisions documented
├── data-model.md        ✅ Complete - Data structures and state machines defined
├── quickstart.md        ✅ Complete - 8-step validation procedure
├── contracts/           ✅ Complete
│   └── ISystemMetrics-getLoopFrequency.md  ✅ HAL contract with 5 test scenarios
└── tasks.md             ⏳ To be created by /tasks command
```

### Source Code (repository root)

**New Files to Create**:
```
src/
├── utils/
│   ├── LoopPerformanceMonitor.h     [NEW] Performance measurement utility (16 bytes)
│   └── LoopPerformanceMonitor.cpp   [NEW] Implementation
├── hal/interfaces/
│   └── ISystemMetrics.h             [MODIFY] Add getLoopFrequency() method
├── hal/implementations/
│   └── ESP32SystemMetrics.h/.cpp    [MODIFY] Implement getLoopFrequency(), add _loopMonitor
├── components/
│   ├── DisplayFormatter.h/.cpp      [MODIFY] Add formatFrequency() method
│   └── MetricsCollector.cpp         [MODIFY] Call getLoopFrequency() instead of getCPUIdlePercent()
├── types/
│   └── DisplayTypes.h               [MODIFY] Replace cpuIdlePercent with loopFrequency
├── mocks/
│   └── MockSystemMetrics.h          [MODIFY] Add getLoopFrequency() mock method
└── main.cpp                         [MODIFY] Call instrumentLoop() every iteration

test/
├── test_performance_contracts/      [NEW] HAL interface contract tests
│   ├── test_main.cpp
│   └── test_isystemmetrics_contract.cpp  (5 contract tests)
├── test_performance_integration/    [NEW] End-to-end scenarios
│   ├── test_main.cpp
│   ├── test_loop_frequency_display.cpp   (display integration)
│   ├── test_measurement_window.cpp       (5-second timing)
│   ├── test_initial_state.cpp            ("---" placeholder)
│   ├── test_frequency_update.cpp         (5-second updates)
│   ├── test_overflow_handling.cpp        (millis() wrap)
│   ├── test_low_frequency_warning.cpp    (< 10 Hz)
│   └── test_high_frequency_format.cpp    (> 999 Hz abbreviation)
├── test_performance_units/          [NEW] Unit tests
│   ├── test_main.cpp
│   ├── test_loop_performance_monitor.cpp (counter logic, overflow)
│   ├── test_frequency_calculation.cpp    (division accuracy)
│   └── test_display_formatting.cpp       (string formatting)
└── test_performance_hardware/       [NEW] Hardware validation (ESP32)
    └── test_main.cpp                     (timing accuracy, overhead)
```

**Structure Decision**:
- **ESP32 embedded system** using PlatformIO grouped test organization
- **Test groups** organized by feature + type: `test_performance_[contracts|integration|units|hardware]/`
- **HAL pattern**: All hardware via `ISystemMetrics` interface, ESP32 implementation in `hal/implementations/`
- **Mock-first testing**: All logic testable on native platform via `MockSystemMetrics`
- **Hardware tests minimal**: Only timing accuracy validation on ESP32

## Phase 0: Outline & Research ✅

**Status**: ✅ COMPLETE

**Output**: [research.md](./research.md) (9,200 words)

### Key Decisions

1. **Performance Measurement**: Lightweight utility class (LoopPerformanceMonitor) with 16-byte footprint
2. **Display Integration**: Extend ISystemMetrics interface with getLoopFrequency() method
3. **Display Format**: "Loop: XXX Hz" (13 chars max, fits 21-char line limit)
4. **Measurement Window**: 5-second averaging with counter reset (aligns with display refresh)
5. **Memory Footprint**: 16 bytes static allocation, ZERO heap usage
6. **Performance Overhead**: < 6 µs per loop (0.1% impact for 5ms loops)
7. **Testing Strategy**: 4-tier organization (contracts, integration, units, hardware)
8. **Overflow Handling**: Explicit millis() wrap detection at ~49.7 days
9. **ReactESP Integration**: No new event loops needed (uses existing 5-second refresh)
10. **Fail-Safe**: Graceful degradation, shows "---" if measurement unavailable

### Resolved Questions

All "NEEDS CLARIFICATION" items from Technical Context resolved:
- ✅ Language: C++14 via Arduino framework
- ✅ Dependencies: Arduino.h, ReactESP (no new dependencies)
- ✅ Storage: In-memory only (no persistent storage)
- ✅ Testing: Unity on native + ESP32 hardware
- ✅ Platform: ESP32 (SH-ESP32 board)
- ✅ Performance: < 1% overhead, ±5 Hz accuracy
- ✅ Constraints: 21-char display, 5-second window
- ✅ Scale: Single device, continuous operation

## Phase 1: Design & Contracts ✅

**Status**: ✅ COMPLETE

**Outputs**:
- [data-model.md](./data-model.md) - Data structures and state machines (3,800 words)
- [contracts/ISystemMetrics-getLoopFrequency.md](./contracts/ISystemMetrics-getLoopFrequency.md) - HAL contract (2,600 words)
- [quickstart.md](./quickstart.md) - 8-step validation procedure (2,500 words)
- CLAUDE.md - Updated with loop frequency patterns

### Data Model Summary

**Core Structures**:
1. **PerformanceMetrics** (internal state): 16 bytes
   - `_loopCount`: uint32_t (iterations in window)
   - `_lastReportTime`: uint32_t (millis() timestamp)
   - `_currentFrequency`: uint32_t (calculated Hz)
   - `_hasFirstMeasurement`: bool (validity flag)

2. **DisplayMetrics** (modified): +3 bytes
   - **REMOVE**: `uint8_t cpuIdlePercent`
   - **ADD**: `uint32_t loopFrequency` (0 = not measured)

3. **ISystemMetrics** (extended):
   - **REMOVE**: `getCPUIdlePercent()` method
   - **ADD**: `getLoopFrequency()` method

**State Machine**:
```
BOOT → MEASURING (0-5s, shows "---")
     → FIRST_MEASUREMENT (calculate frequency)
     → STEADY_STATE (update every 5s)
```

### HAL Contract Summary

**Method**: `ISystemMetrics::getLoopFrequency()`
- **Return**: uint32_t (Hz, 0 = not measured)
- **Behavior Requirements**: 5 requirements (BR-001 to BR-005)
- **Performance**: < 10 µs execution time
- **Thread Safety**: Read-only, ISR-safe
- **Test Scenarios**: 5 contract tests (CT-001 to CT-005)

**Integration Points**:
- **Upstream**: LoopPerformanceMonitor (via instrumentLoop())
- **Downstream**: MetricsCollector, DisplayManager, WebSocketLogger

### Quickstart Summary

**8-Step Validation Procedure**:
1. Build and upload firmware
2. Verify placeholder "---" during first 5 seconds
3. Verify numeric frequency appears after 5 seconds
4. Verify frequency updates every 5 seconds
5. Verify display format for edge cases (low/high/normal)
6. Verify WebSocket logging (optional)
7. Verify resource usage (< 5% flash, < 1% RAM)
8. Verify graceful degradation (display init failure)

**Success Criteria**: 10 FR + 3 NFR + 8 constitutional principles validated

## Phase 2: Task Planning Approach
*This section describes what the /tasks command will do - DO NOT execute during /plan*

**Task Generation Strategy**:
1. Load `.specify/templates/tasks-template.md` as base
2. Generate tasks from Phase 1 design docs (contracts, data model, quickstart)
3. Follow TDD order: Tests before implementation
4. Group by dependency: HAL → Components → Integration → Main

**Task Categories** (estimated 28-32 tasks):

### Category 1: HAL Interface (Tests First) - 4 tasks
- **T001** [P]: Write ISystemMetrics contract tests (test_performance_contracts/)
- **T002**: Extend ISystemMetrics with getLoopFrequency() declaration
- **T003**: Update MockSystemMetrics with getLoopFrequency() implementation
- **T004**: Run contract tests (should pass)

### Category 2: Utility Class (Tests First) - 6 tasks
- **T005** [P]: Write LoopPerformanceMonitor unit tests (test_performance_units/)
- **T006** [P]: Write frequency calculation unit tests
- **T007** [P]: Write overflow handling unit tests
- **T008**: Implement LoopPerformanceMonitor.h
- **T009**: Implement LoopPerformanceMonitor.cpp
- **T010**: Run unit tests (should pass)

### Category 3: HAL Implementation - 3 tasks
- **T011**: Implement ESP32SystemMetrics::getLoopFrequency()
- **T012**: Add ESP32SystemMetrics::instrumentLoop() method
- **T013**: Run contract tests again (verify implementation)

### Category 4: Display Integration (Tests First) - 6 tasks
- **T014** [P]: Write display formatting unit tests (test_performance_units/)
- **T015** [P]: Write display integration tests (test_performance_integration/)
- **T016**: Update DisplayTypes.h (remove cpuIdlePercent, add loopFrequency)
- **T017**: Add DisplayFormatter::formatFrequency() method
- **T018**: Update MetricsCollector to call getLoopFrequency()
- **T019**: Run display integration tests (should pass)

### Category 5: Main Loop Integration - 3 tasks
- **T020**: Update main.cpp: Add instrumentLoop() calls in loop()
- **T021**: Update main.cpp: Initialize ESP32SystemMetrics with LoopPerformanceMonitor
- **T022**: Build for ESP32 (verify compilation)

### Category 6: Edge Cases (Tests First) - 5 tasks
- **T023** [P]: Write initial state test (placeholder "---")
- **T024** [P]: Write low frequency warning test
- **T025** [P]: Write high frequency abbreviation test
- **T026** [P]: Write overflow handling test (millis() wrap)
- **T027**: Run all integration tests (verify edge cases)

### Category 7: Hardware Validation - 3 tasks
- **T028**: Write hardware timing test (test_performance_hardware/)
- **T029**: Upload to ESP32 and run hardware tests
- **T030**: Verify actual loop frequency and measurement overhead

### Category 8: Documentation & Validation - 2 tasks
- **T031**: Execute quickstart.md validation procedure (8 steps)
- **T032**: Update README.md with loop frequency feature documentation

**Ordering Strategy**:
- **TDD order**: Tests before implementation (T001→T002, T005→T008, etc.)
- **Dependency order**: HAL interfaces → Utility classes → Components → Main loop
- **Parallel execution**: [P] marked tasks are independent (can run simultaneously)
- **Validation gates**: Tests must pass before proceeding to next category

**Estimated Timeline**: 4-6 hours (assuming parallel execution where possible)

**IMPORTANT**: This phase is executed by the `/tasks` command, NOT by `/plan`

## Phase 3+: Future Implementation
*These phases are beyond the scope of the /plan command*

**Phase 3**: Task execution (/tasks command creates tasks.md with 28-32 tasks)
**Phase 4**: Implementation (execute tasks.md following TDD and constitutional principles)
**Phase 5**: Validation (run tests, execute quickstart.md, performance validation)

## Complexity Tracking

**No Complexity Violations** - All constitutional principles satisfied without exceptions.

| Principle | Status | Notes |
|-----------|--------|-------|
| Hardware Abstraction | ✅ PASS | ISystemMetrics interface used, no direct hardware access |
| Resource Management | ✅ PASS | 16 bytes static, zero heap, F() macros |
| QA Review | ✅ PASS | Planned for all code changes |
| Modular Design | ✅ PASS | Single responsibility classes |
| Network Debugging | ✅ PASS | WebSocket logging implemented |
| Always-On Operation | ✅ PASS | No sleep modes |
| Fail-Safe Operation | ✅ PASS | Graceful degradation |
| Workflow Selection | ✅ PASS | Using feature development workflow |

## Progress Tracking

**Phase Status**:
- [x] Phase 0: Research complete (/plan command) ✅
- [x] Phase 1: Design complete (/plan command) ✅
- [x] Phase 2: Task planning complete (/plan command - approach described) ✅
- [ ] Phase 3: Tasks generated (/tasks command) ⏳ NEXT STEP
- [ ] Phase 4: Implementation complete
- [ ] Phase 5: Validation passed

**Gate Status**:
- [x] Initial Constitution Check: PASS ✅
- [x] Post-Design Constitution Check: PASS ✅
- [x] All NEEDS CLARIFICATION resolved ✅
- [x] Complexity deviations documented (none) ✅

**Artifact Generation**:
- [x] research.md created (9,200 words)
- [x] data-model.md created (3,800 words)
- [x] contracts/ISystemMetrics-getLoopFrequency.md created (2,600 words)
- [x] quickstart.md created (2,500 words)
- [x] CLAUDE.md updated with feature patterns
- [ ] tasks.md to be created by /tasks command

**Quality Metrics**:
- **Documentation**: 18,100 words across 5 files
- **Test Coverage**: 28+ tests planned (contracts + integration + units + hardware)
- **Memory Impact**: 16 bytes RAM (0.005% of ESP32)
- **Flash Impact**: ~100 KB (< 5% of partition)
- **Performance Impact**: < 0.1% (< 6 µs per loop)

---
**Plan Status**: ✅ **COMPLETE** - Ready for `/tasks` command

*Based on Constitution v1.2.0 - See `.specify/memory/constitution.md`*
