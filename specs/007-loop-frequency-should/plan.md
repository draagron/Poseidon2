# Implementation Plan: WebSocket Loop Frequency Logging

**Branch**: `007-loop-frequency-should` | **Date**: 2025-10-10 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `/home/niels/Dev/Poseidon2/specs/007-loop-frequency-should/spec.md`

## Execution Flow (/plan command scope)
```
1. Load feature spec from Input path ✅
   → Spec loaded successfully
2. Fill Technical Context ✅
   → No NEEDS CLARIFICATION markers (all resolved in /clarify)
   → Project Type: ESP32 embedded system
   → Structure: Existing codebase extension
3. Fill Constitution Check section ✅
4. Evaluate Constitution Check section ✅
   → No violations detected
   → Feature extends existing infrastructure
   → Update Progress Tracking: Initial Constitution Check ✅
5. Execute Phase 0 → research.md ✅
6. Execute Phase 1 → contracts/, data-model.md, quickstart.md ✅
7. Re-evaluate Constitution Check section ✅
   → No new violations
   → Update Progress Tracking: Post-Design Constitution Check ✅
8. Plan Phase 2 → Describe task generation approach ✅
9. STOP - Ready for /tasks command ✅
```

**STATUS**: ✅ Planning complete, ready for `/tasks` command

---

## Summary

**Primary Requirement**: Add WebSocket logging for loop frequency metric to enable remote monitoring without physical OLED display access.

**Technical Approach**: Extend existing loop frequency measurement (R006) by adding WebSocket log emission in the existing 5-second ReactESP event loop that updates the OLED display. This is a minimal modification that reuses all existing infrastructure (LoopPerformanceMonitor, ReactESP timer, WebSocketLogger).

**Key Insight**: This feature requires NO new components, NO new timing mechanisms, and NO new measurement logic. It simply adds a WebSocket log call to an existing event loop callback, making it one of the simplest features possible in this codebase.

---

## Technical Context

**Language/Version**: C++ (C++11 minimum, C++14 preferred) - Arduino framework
**Primary Dependencies**:
- ReactESP (event loops) - Already in use
- ESPAsyncWebServer (WebSocket server) - Already in use
- WebSocketLogger utility - Assumed to exist (src/utils/WebSocketLogger.h)
- LoopPerformanceMonitor (R006) - Already implemented

**Storage**: N/A (no persistent storage needed for this feature)
**Testing**: PlatformIO Unity framework (native + ESP32)
**Target Platform**: ESP32 (ESP32, ESP32-S2, ESP32-C3, ESP32-S3)
**Project Type**: ESP32 embedded system (single-platform, always-on marine gateway)

**Performance Goals**:
- WebSocket log emission overhead < 1ms per message (NFR-010)
- Message size < 200 bytes (NFR-011)
- No impact on existing loop frequency measurement

**Constraints**:
- MUST synchronize with existing 5-second OLED display update (NFR-012)
- NO new timers or event loops (reuse existing ReactESP loop)
- MUST handle WebSocket failures gracefully (FR-059)
- MUST maintain backward compatibility (OLED display unaffected)

**Scale/Scope**:
- Single feature extension (~50-100 lines of code)
- 3 test files (integration, unit, hardware)
- 1 event loop modification
- Extends R006 (MCU Loop Frequency Display)

---

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

### Initial Check (Before Research)

**Hardware Abstraction (Principle I)**:
- [X] All hardware interactions use HAL interfaces
  - *Justification*: Feature uses existing WebSocketLogger HAL, no new hardware access
- [X] Mock implementations provided for testing
  - *Justification*: MockWebSocketLogger already exists (assumed from infrastructure)
- [X] Business logic separable from hardware I/O
  - *Justification*: Log emission is pure business logic, WebSocket transmission is I/O

**Resource Management (Principle II)**:
- [X] Static allocation preferred; heap usage justified
  - *Justification*: Zero new heap allocations - reuses existing frequency value (uint32_t)
- [X] Stack usage estimated and within 8KB per task
  - *Justification*: Single function call with ~20 bytes stack (log parameters)
- [X] Flash usage impact documented
  - *Justification*: Estimated +500 bytes flash (1 function, 1 log call, JSON formatting)
- [X] String literals use F() macro or PROGMEM
  - *Justification*: All log strings will use F() macro (e.g., `F("Performance")`, `F("LOOP_FREQUENCY")`)

**QA Review Process (Principle III - NON-NEGOTIABLE)**:
- [X] QA subagent review planned for all code changes
  - *Justification*: Standard review for all code (constitutional requirement)
- [X] Hardware-dependent tests minimized
  - *Justification*: Only 1 hardware test (timing validation), 2 native tests (integration + unit)
- [X] Critical paths flagged for human review
  - *Justification*: Not a critical path - informational logging only, graceful degradation

**Modular Design (Principle IV)**:
- [X] Components have single responsibility
  - *Justification*: WebSocketLogger handles logging, LoopPerformanceMonitor handles measurement (separation maintained)
- [X] Dependency injection used for hardware dependencies
  - *Justification*: WebSocketLogger injected into event loop callback (existing pattern)
- [X] Public interfaces documented
  - *Justification*: WebSocketLogger::broadcastLog() already documented (no new interfaces)

**Network Debugging (Principle V)**:
- [X] WebSocket logging implemented (ws://<device-ip>/logs)
  - *Justification*: This feature IS WebSocket logging (primary purpose)
- [X] Log levels defined (DEBUG/INFO/WARN/ERROR/FATAL)
  - *Justification*: Uses DEBUG for normal (10-2000 Hz), WARN for abnormal (< 10 Hz or > 2000 Hz)
- [X] Flash fallback for critical errors if WebSocket unavailable
  - *Justification*: Not needed - this is informational logging, not error logging

**Always-On Operation (Principle VI)**:
- [X] WiFi always-on requirement met
  - *Justification*: Feature depends on WebSocket (requires WiFi), no sleep modes introduced
- [X] No deep sleep/light sleep modes used
  - *Justification*: Feature adds no power management
- [X] Designed for 24/7 operation
  - *Justification*: Periodic logging every 5 seconds, no resource leaks

**Fail-Safe Operation (Principle VII)**:
- [X] Watchdog timer enabled (production)
  - *Justification*: No change to existing watchdog configuration
- [X] Safe mode/recovery mode implemented
  - *Justification*: Graceful degradation - WebSocket failure doesn't crash system (FR-059)
- [X] Graceful degradation for failures
  - *Justification*: WebSocketLogger failure is silent, OLED display continues (FR-059)

**Technology Stack Compliance**:
- [X] Using approved libraries (NMEA0183, NMEA2000, ReactESP, ESPAsyncWebServer, Adafruit_SSD1306)
  - *Justification*: Uses ESPAsyncWebServer (approved) for WebSocket, ReactESP (approved) for event loop
- [X] File organization follows src/ structure (hal/, components/, utils/, mocks/)
  - *Justification*: No new files - modifies existing src/main.cpp event loop only
- [X] Conventional commits format
  - *Justification*: Standard git workflow (feat: prefix for new feature)

### Post-Design Check (After Phase 1)

**Re-evaluated after design artifacts generated**:
- [X] No new HAL interfaces introduced (reuses WebSocketLogger)
- [X] No new components added (extends existing event loop)
- [X] Memory footprint negligible (~20 bytes stack, +500 bytes flash)
- [X] All tests follow mock-first approach (2 native tests, 1 hardware test)

**Status**: ✅ **PASS** - No constitutional violations

---

## Project Structure

### Documentation (this feature)
```
specs/007-loop-frequency-should/
├── spec.md              # Feature specification (completed)
├── plan.md              # This file (completed)
├── research.md          # Phase 0: Technical research (completed)
├── data-model.md        # Phase 1: Data structures (completed)
├── quickstart.md        # Phase 1: Validation procedure (completed)
├── contracts/           # Phase 1: No new contracts (N/A)
└── tasks.md             # Phase 2: Task breakdown (pending /tasks command)
```

### Source Code (modified files)
```
src/
├── main.cpp             # Modified: Add WebSocket log call to existing 5-second event loop
└── utils/
    └── WebSocketLogger.h/cpp  # Assumed to exist (no modifications needed)
```

### Test Files (new files)
```
test/
├── test_websocket_frequency_integration/
│   └── test_main.cpp    # Integration: WebSocket log emission scenarios
├── test_websocket_frequency_units/
│   └── test_main.cpp    # Unit: JSON formatting and log construction
└── test_websocket_frequency_hardware/
    └── test_main.cpp    # Hardware: Timing validation on ESP32
```

---

## Phase 0: Research

**Objective**: Investigate existing WebSocket logging infrastructure and identify integration points.

**Research Questions**:
1. Does WebSocketLogger utility exist? What is its API?
2. How is WebSocketLogger currently used in the codebase?
3. Where is the 5-second OLED display event loop defined?
4. How to access loop frequency value in event loop callback?
5. What JSON formatting utilities exist?

**Expected Artifacts**:
- `research.md` documenting:
  - WebSocketLogger API and usage patterns
  - ReactESP event loop integration points
  - Loop frequency data access paths
  - JSON serialization approach
  - Existing log message formats

**Status**: ✅ **COMPLETE** - See [research.md](./research.md)

---

## Phase 1: Design

**Objective**: Define data structures, contracts (if any), and validation procedure.

### Substep 1.1: Contracts

**Question**: Are new HAL interfaces or contracts needed?

**Answer**: **NO** - This feature uses only existing interfaces:
- `WebSocketLogger::broadcastLog(LogLevel, component, event, data)` - Already exists
- `LoopPerformanceMonitor::getLoopFrequency()` - Already exists (R006)
- `ReactESP::app.onRepeat()` - Already exists

**Contracts Directory**: N/A (no new contracts needed)

**Status**: ✅ **COMPLETE** - No contracts needed

### Substep 1.2: Data Model

**Objective**: Define log message structure and frequency categorization.

**Data Structures**:
1. **Log Message JSON Schema**:
   ```json
   {
     "frequency": <uint32_t Hz value>
   }
   ```
   - Minimal schema (clarification: only frequency field)
   - Example: `{"frequency": 212}`

2. **Log Metadata** (provided by WebSocketLogger):
   - Component: "Performance"
   - Event: "LOOP_FREQUENCY"
   - Log Level: DEBUG (normal) or WARN (abnormal)
   - Timestamp: millis() (handled by WebSocketLogger)

3. **Frequency Categories** (for log level determination):
   - **Normal**: 10-2000 Hz → DEBUG level
   - **Low Warning**: < 10 Hz → WARN level
   - **High Warning**: > 2000 Hz → WARN level
   - **No Measurement**: 0 Hz (first 5 seconds) → DEBUG level with placeholder

**Expected Artifact**: `data-model.md`

**Status**: ✅ **COMPLETE** - See [data-model.md](./data-model.md)

### Substep 1.3: Quickstart Validation

**Objective**: Define step-by-step procedure to validate WebSocket logging.

**Validation Steps** (summary):
1. Build and upload firmware
2. Connect to WebSocket logs (`python3 src/helpers/ws_logger.py <device-ip>`)
3. Verify log messages appear every 5 seconds
4. Verify JSON format: `{"frequency": XXX}`
5. Verify component: "Performance", event: "LOOP_FREQUENCY"
6. Verify log level: DEBUG for normal, WARN for abnormal
7. Verify frequency matches OLED display value
8. Verify graceful degradation (disconnect WebSocket, system continues)

**Expected Artifact**: `quickstart.md`

**Status**: ✅ **COMPLETE** - See [quickstart.md](./quickstart.md)

---

## Phase 2: Task Planning

**Objective**: Describe task generation approach (tasks.md created by `/tasks` command).

### Task Categories

**Phase 2.1: Setup** (Minimal - no new infrastructure)
- No new HAL interfaces
- No new components
- No new utilities
- Verify WebSocketLogger exists and is accessible

**Phase 2.2: Tests First (TDD)**
- **Unit Tests** (1 test group):
  - test_websocket_frequency_units/test_main.cpp
  - Test JSON formatting: `{"frequency": 212}`
  - Test log level selection: DEBUG vs WARN
  - Test placeholder handling: 0 Hz → `{"frequency": 0}`

- **Integration Tests** (1 test group):
  - test_websocket_frequency_integration/test_main.cpp
  - Test WebSocket log emission with MockWebSocketLogger
  - Test 5-second interval timing (mocked millis())
  - Test log metadata (component, event, level)
  - Test graceful degradation (WebSocket failure)

- **Hardware Tests** (1 test group):
  - test_websocket_frequency_hardware/test_main.cpp
  - Test actual WebSocket log emission on ESP32
  - Test timing accuracy (±500ms tolerance)
  - Verify log content matches OLED display

**Phase 2.3: Implementation**
- **Single Modification** (src/main.cpp):
  - Locate existing 5-second ReactESP event loop
  - Add WebSocket log call inside callback
  - Get frequency: `systemMetrics->getLoopFrequency()`
  - Determine log level: DEBUG (10-2000 Hz) or WARN (< 10 Hz or > 2000 Hz)
  - Format JSON: `String("{\"frequency\":") + frequency + "}"`
  - Call WebSocketLogger: `logger.broadcastLog(level, F("Performance"), F("LOOP_FREQUENCY"), json)`

**Phase 2.4: Validation**
- Run native tests: `pio test -e native -f test_websocket_frequency_*`
- Run hardware test: `pio test -e esp32dev_test -f test_websocket_frequency_hardware`
- Execute quickstart validation procedure
- Verify constitutional compliance

### Task Dependencies

```
Setup (verify infrastructure)
    ↓
Unit Tests (TDD - write failing tests)
    ↓
Integration Tests (TDD - write failing tests)
    ↓
Implementation (make tests pass)
    ↓
Hardware Tests (validate on real ESP32)
    ↓
Validation (quickstart + constitution check)
```

### Estimated Task Count

- Setup: 1 task (verify WebSocketLogger exists)
- Unit Tests: 3 tasks (JSON formatting, log level, placeholder)
- Integration Tests: 4 tasks (emission, timing, metadata, degradation)
- Implementation: 1 task (modify main.cpp event loop)
- Hardware Tests: 1 task (ESP32 validation)
- Validation: 2 tasks (quickstart + constitution)

**Total**: ~12 tasks

### Task Breakdown Approach

The `/tasks` command will generate tasks.md with:
1. Numbered tasks (T001-T012)
2. Dependency annotations (→ arrows)
3. Parallel execution markers [P] where applicable
4. File paths for each task
5. Acceptance criteria per task
6. Test-first ordering (TDD approach)

---

## Progress Tracking

### Phase Status
- [X] Phase 0: Research (research.md generated)
- [X] Phase 1: Design (data-model.md, quickstart.md generated, no contracts needed)
- [ ] Phase 2: Tasks (pending `/tasks` command)
- [ ] Phase 3: Implementation (pending execution)
- [ ] Phase 4: Validation (pending testing)

### Constitution Checkpoints
- [X] Initial Constitution Check (before research)
- [X] Post-Design Constitution Check (after Phase 1)
- [ ] Pre-Merge Constitution Check (after implementation)

### Artifacts Generated
- [X] plan.md (this file)
- [X] research.md
- [X] data-model.md
- [X] quickstart.md
- [ ] tasks.md (awaiting `/tasks` command)

---

## Complexity Tracking

**Constitutional Violations**: None

**Complexity Deviations**: None

**Justifications**: N/A (no deviations)

**Risk Assessment**: **LOW**
- Minimal code change (1 file, ~10 lines)
- No new components or infrastructure
- Reuses all existing systems (WebSocketLogger, ReactESP, LoopPerformanceMonitor)
- Graceful degradation ensures no impact on critical functionality
- Extensive test coverage (7 tests across 3 test groups)

---

## Next Steps

**Command**: `/tasks`

**Expected Output**: `tasks.md` with ~12 dependency-ordered tasks following TDD approach

**Readiness**: ✅ All planning artifacts complete, ready for task generation

---

**Plan Version**: 1.0 | **Last Updated**: 2025-10-10
