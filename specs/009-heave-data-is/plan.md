# Implementation Plan: NMEA2000 PGN 127252 Heave Handler

**Branch**: `009-heave-data-is` | **Date**: 2025-10-11 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/home/niels/Dev/Poseidon2/specs/009-heave-data-is/spec.md`

## Execution Flow (/plan command scope)
```
1. Load feature spec from Input path ✓
   → Spec loaded successfully
2. Fill Technical Context ✓
   → No NEEDS CLARIFICATION markers detected
   → Project Type: ESP32 embedded system (PlatformIO)
   → Structure Decision: HAL pattern with grouped tests
3. Fill the Constitution Check section ✓
4. Evaluate Constitution Check section ✓
   → No violations - follows existing NMEA2000 handler pattern
   → Update Progress Tracking: Initial Constitution Check ✓
5. Execute Phase 0 → research.md ✓
6. Execute Phase 1 → contracts, data-model.md, quickstart.md, CLAUDE.md ✓
7. Re-evaluate Constitution Check section ✓
   → No new violations after design phase
   → Update Progress Tracking: Post-Design Constitution Check ✓
8. Plan Phase 2 → task generation approach described ✓
9. STOP - Ready for /tasks command ✓
```

**IMPORTANT**: The /plan command STOPS here. Phases 2-4 are executed by other commands:
- Phase 2: /tasks command creates tasks.md
- Phase 3-4: Implementation execution (manual or via tools)

## Summary
This feature implements a NMEA2000 PGN 127252 message handler to capture heave data (vertical displacement) from marine sensors. The handler follows the existing pattern established in `src/components/NMEA2000Handlers.cpp`, parsing incoming PGN 127252 messages, validating heave values (±5.0 meters), updating the `CompassData.heave` field, and logging all operations via WebSocket. This completes the heave data integration that was partially addressed in Enhanced BoatData v2.0.0 (R005), where PGN 127257 (Attitude) handler noted that heave would need to come from a separate PGN.

**Technical Approach**: Single handler function `HandleN2kPGN127252` added to existing `NMEA2000Handlers.cpp`, reusing existing validation infrastructure (`DataValidation::clampHeave`, `DataValidation::isValidHeave`), WebSocket logging, and BoatData update mechanisms. No new HAL interfaces required - leverages NMEA2000 library's `ParseN2kPGN127252` function. Minimal code change (~60 lines) with high reuse of existing components.

## Technical Context
**Language/Version**: C++ (C++14, Arduino framework)
**Primary Dependencies**:
- NMEA2000 library (ttlappalainen/NMEA2000) - provides ParseN2kPGN127252
- Existing DataValidation utilities (clampHeave, isValidHeave)
- Existing WebSocketLogger (src/utils/WebSocketLogger.h)
- Existing BoatData component (src/components/BoatData.h)
**Storage**: BoatData in-memory structure (CompassData.heave field already exists)
**Testing**: Unity framework, PlatformIO grouped tests, native platform for mocked tests
**Target Platform**: ESP32 (ESP32, ESP32-S2, ESP32-C3, ESP32-S3)
**Project Type**: ESP32 embedded system with PlatformIO grouped test organization
**Performance Goals**: Non-blocking PGN processing (<1ms per message), ReactESP event-driven architecture
**Constraints**:
- Memory: Minimal RAM impact (~0 bytes - reuses existing structures)
- Flash: Estimated +2KB for handler function and registration
- No blocking operations allowed (ReactESP requirement)
**Scale/Scope**: Single PGN handler function, extends existing NMEA2000Handlers module

## Constitution Check
*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

**Hardware Abstraction (Principle I)**:
- [X] All hardware interactions use HAL interfaces
  - NMEA2000 library provides abstraction for CAN bus communication
  - No direct hardware access in handler code
- [X] Mock implementations provided for testing
  - Will use existing MockBoatData and MockWebSocketLogger for tests
  - NMEA2000 messages mocked in integration tests
- [X] Business logic separable from hardware I/O
  - Handler contains only parsing, validation, and data storage logic
  - No GPIO or peripheral access

**Resource Management (Principle II)**:
- [X] Static allocation preferred; heap usage justified
  - No dynamic allocation in handler
  - Reuses existing BoatData structure (CompassData.heave already exists)
- [X] Stack usage estimated and within 8KB per task
  - Handler stack usage ~200 bytes (local variables: SID, heave, delay, delaySource, CompassData struct)
  - Well within 8KB limit
- [X] Flash usage impact documented
  - Estimated +2KB flash for handler function (~60 lines) and registration
  - Total project flash remains <50% (currently 47.7%)
- [X] String literals use F() macro or PROGMEM
  - All log messages use F() macro (e.g., F("{\"reason\":\"Heave not available\"}"))

**QA Review Process (Principle III - NON-NEGOTIABLE)**:
- [X] QA subagent review planned for all code changes
  - Standard PR review process applies
  - Constitutional compliance validated in COMPLIANCE.md
- [X] Hardware-dependent tests minimized
  - Integration tests use mocked NMEA2000 messages (native platform)
  - Hardware tests only for NMEA2000 bus timing validation
- [X] Critical paths flagged for human review
  - Single handler function - straightforward code review

**Modular Design (Principle IV)**:
- [X] Components have single responsibility
  - HandleN2kPGN127252 has single responsibility: parse PGN 127252 and update heave
  - Validation delegated to DataValidation utilities
  - Logging delegated to WebSocketLogger
- [X] Dependency injection used for hardware dependencies
  - Handler receives BoatData* and WebSocketLogger* as parameters
  - No global coupling
- [X] Public interfaces documented
  - Doxygen-style documentation in NMEA2000Handlers.h

**Network Debugging (Principle V)**:
- [X] WebSocket logging implemented (ws://<device-ip>/logs)
  - DEBUG level for successful heave updates
  - WARN level for out-of-range values (clamping)
  - ERROR level for parse failures
- [X] Log levels defined (DEBUG/INFO/WARN/ERROR/FATAL)
  - Uses existing LogLevel enum
- [X] Flash fallback for critical errors if WebSocket unavailable
  - Existing WebSocketLogger handles buffering/fallback

**Always-On Operation (Principle VI)**:
- [X] WiFi always-on requirement met
  - No changes to WiFi management
  - Handler operates independently of WiFi status
- [X] No deep sleep/light sleep modes used
  - Handler uses ReactESP event-driven callbacks (non-blocking)
- [X] Designed for 24/7 operation
  - No resource leaks, stateless handler function

**Fail-Safe Operation (Principle VII)**:
- [X] Watchdog timer enabled (production)
  - No changes to existing watchdog configuration
- [X] Safe mode/recovery mode implemented
  - Handler fails gracefully on parse errors (logs ERROR, continues)
  - Invalid data clamped to valid range with warning
- [X] Graceful degradation for failures
  - N2kDoubleNA (not available) handled without error
  - Parse failures set availability=false, log ERROR, continue operation
  - Out-of-range values clamped with warning

**Technology Stack Compliance**:
- [X] Using approved libraries (NMEA2000, ReactESP, ESPAsyncWebServer)
  - NMEA2000 library already approved and in use
  - No new library dependencies
- [X] File organization follows src/ structure (components/, utils/)
  - Handler added to existing `src/components/NMEA2000Handlers.cpp`
  - Validation utilities in existing `src/utils/DataValidation.h`
- [X] Conventional commits format
  - Commit message: `feat(nmea2000): add PGN 127252 heave handler`

## Project Structure

### Documentation (this feature)
```
specs/009-heave-data-is/
├── spec.md              # Feature specification (completed)
├── plan.md              # This file (/plan command output)
├── research.md          # Phase 0 output (generated below)
├── data-model.md        # Phase 1 output (generated below)
├── quickstart.md        # Phase 1 output (generated below)
├── contracts/           # Phase 1 output (generated below)
│   └── HandleN2kPGN127252.md
└── tasks.md             # Phase 2 output (/tasks command - NOT created by /plan)
```

### Source Code (repository root)
**Poseidon2 uses ESP32/PlatformIO architecture with Hardware Abstraction Layer**

```
src/
├── components/
│   ├── NMEA2000Handlers.cpp     # ADD: HandleN2kPGN127252 function (~60 lines)
│   └── NMEA2000Handlers.h       # ADD: Function declaration + Doxygen comments
├── utils/
│   └── DataValidation.h         # EXISTING: clampHeave, isValidHeave (already implemented)
└── types/
    └── BoatDataTypes.h          # EXISTING: CompassData.heave field (already exists)

test/
├── test_boatdata_contracts/     # ADD: PGN 127252 handler signature test
│   └── test_main.cpp            # MODIFY: Add heave handler contract test
├── test_boatdata_integration/   # ADD: Heave data end-to-end scenario test
│   └── test_heave_from_pgn127252.cpp  # NEW: Integration test file
├── test_boatdata_units/         # EXISTING: Heave validation tests (may already exist)
│   └── test_main.cpp            # VERIFY: Heave validation functions tested
└── test_boatdata_hardware/      # MODIFY: Add PGN 127252 timing placeholder
    └── test_main.cpp            # MODIFY: Update NMEA2000 timing test
```

**Structure Decision**:
- **ESP32 embedded system** using PlatformIO grouped test organization
- **Test groups** organized by feature + type: `test_boatdata_[contracts|integration|units|hardware]/`
- **No new HAL interfaces**: Leverages existing NMEA2000 library abstraction
- **Minimal new code**: Single handler function added to existing module
- **High reuse**: Validation, logging, data storage infrastructure already exists

## Phase 0: Outline & Research

### Research Questions
1. **NMEA2000 Library Support**: Does the NMEA2000 library provide ParseN2kPGN127252?
   - **Answer**: YES - confirmed at https://github.com/ttlappalainen/NMEA2000/blob/master/src/N2kMessages.h
   - Function signature: `bool ParseN2kPGN127252(const tN2kMsg &N2kMsg, unsigned char &SID, double &Heave, double &Delay, tN2kDelaySource &DelaySource);`
   - Heave returned in meters

2. **CompassData.heave Field**: Does the field already exist?
   - **Answer**: YES - added in Enhanced BoatData v2.0.0 (R005)
   - Location: `src/types/BoatDataTypes.h` - `struct CompassData { ... double heave; ... }`
   - Valid range: ±5.0 meters
   - Sign convention: positive = upward motion

3. **DataValidation Helpers**: Do heave validation functions exist?
   - **Answer**: YES - implemented in Enhanced BoatData v2.0.0 (R005)
   - Functions: `DataValidation::clampHeave(double heave)`, `DataValidation::isValidHeave(double heave)`
   - Range: [-5.0, 5.0] meters
   - Location: `src/utils/DataValidation.h`

4. **Existing Handler Pattern**: What is the implementation pattern?
   - **Answer**: Well-established pattern in `src/components/NMEA2000Handlers.cpp`
   - Structure:
     1. Null pointer checks (boatData, logger)
     2. Call NMEA2000 library parse function
     3. Check N2kIsNA (not available)
     4. Validate data range
     5. Clamp out-of-range values with WARN log
     6. Get current BoatData structure
     7. Update field(s)
     8. Set availability flag and timestamp
     9. Store updated structure
     10. Log update at DEBUG level
     11. Increment NMEA2000 message counter
     12. Log parse failures at ERROR level
   - Reference handlers: PGN 127251 (Rate of Turn), PGN 127257 (Attitude)

5. **Handler Registration**: How are handlers registered?
   - **Answer**: Via `RegisterN2kHandlers()` function in NMEA2000Handlers.cpp
   - Function calls `SetN2kPGNHandler()` for each PGN
   - Placeholder exists in `src/main.cpp` for when NMEA2000 bus is initialized
   - New handler must be added to registration function

6. **Testing Infrastructure**: What test suites exist?
   - **Answer**: Full test infrastructure from Enhanced BoatData v2.0.0:
     - `test_boatdata_contracts/` - HAL interface contract tests
     - `test_boatdata_integration/` - End-to-end scenarios (15 tests)
     - `test_boatdata_units/` - Validation/conversion tests (13 tests)
     - `test_boatdata_hardware/` - ESP32 hardware tests (7 tests)
   - New heave handler tests will integrate into existing suites

### Technical Decisions

**1. Handler Function Signature**:
```cpp
void HandleN2kPGN127252(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger);
```
- Matches existing handler pattern (PGN 127251, 127257, etc.)
- Takes const reference to NMEA2000 message
- Receives BoatData and WebSocketLogger pointers for dependency injection

**2. Heave Field Location**:
- Store in `CompassData.heave` (already exists)
- Rationale: Heave is vessel motion data, logically grouped with heel, pitch, rate of turn
- Alternative considered: Separate HeaveData structure (rejected - unnecessary complexity)

**3. Validation Range**:
- Use existing ±5.0 meter range from DataValidation helpers
- Rationale: Typical marine sensor operating range
- Out-of-range values clamped with WARN log (not rejected)

**4. Optional Parameters**:
- PGN 127252 includes optional `Delay` and `DelaySource` parameters
- Decision: Parse but DO NOT store (focus on heave value only)
- Rationale: Delay parameters rarely used, not needed for primary use case
- Future enhancement: Can add DelayData structure if needed

**5. Test Strategy**:
- **Contract Test**: Validate handler signature matches expected pattern
- **Integration Test**: Mock PGN 127252 message, verify heave stored in CompassData
- **Unit Test**: Validation functions already tested (Enhanced BoatData v2.0.0)
- **Hardware Test**: Placeholder for NMEA2000 bus timing (actual test pending bus initialization)

### Dependencies
- **Upstream**: NMEA2000 library (already integrated)
- **Horizontal**: DataValidation.h, WebSocketLogger, BoatData component (all exist)
- **Downstream**: None - handler is a leaf component

### Risks & Mitigations
1. **Risk**: NMEA2000 bus not yet initialized in main.cpp
   - **Mitigation**: Handler ready for integration; placeholder exists in main.cpp
   - **Impact**: Low - handler can be tested independently with mocked messages

2. **Risk**: Heave data may arrive at high frequency (>10 Hz)
   - **Mitigation**: Handler is stateless, non-blocking, ReactESP handles queueing
   - **Impact**: Low - ReactESP event loop tested for high-frequency operation

3. **Risk**: Memory footprint increase
   - **Mitigation**: No new allocations, reuses existing structures
   - **Impact**: Negligible - estimated +2KB flash, +0 bytes RAM

## Phase 1: Design

### Data Model
See `data-model.md` for detailed entity definitions and relationships.

**Summary**:
- **PGN 127252 Message**: NMEA2000 message with SID, heave (meters), optional delay/delay source
- **CompassData.heave**: Existing field in BoatData structure for storing heave value
- **No new data structures required**: Reuses existing infrastructure

### API Contracts
See `contracts/HandleN2kPGN127252.md` for detailed function contract.

**Summary**:
```cpp
/**
 * @brief Handle PGN 127252 - Heave
 *
 * Updates CompassData.heave with validated heave value.
 * Sign convention: positive = upward motion
 *
 * @param N2kMsg NMEA2000 message
 * @param boatData BoatData instance to update
 * @param logger WebSocket logger for debug output
 */
void HandleN2kPGN127252(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger);
```

**Contract Requirements**:
- **Preconditions**: boatData != nullptr, logger != nullptr
- **Postconditions**: If valid heave parsed, CompassData.heave updated, availability=true, timestamp=millis()
- **Side Effects**: WebSocket log entries (DEBUG/WARN/ERROR), NMEA2000 message counter incremented
- **Error Handling**: Null pointers ignored, N2kDoubleNA logged at DEBUG, parse failures logged at ERROR

### Integration Scenarios
See `quickstart.md` for step-by-step validation scenarios.

**Summary**:
1. **Scenario 1**: Valid heave message → Value stored in CompassData
2. **Scenario 2**: Out-of-range heave → Value clamped, WARN logged
3. **Scenario 3**: Unavailable heave (N2kDoubleNA) → DEBUG logged, update skipped
4. **Scenario 4**: Parse failure → ERROR logged, availability=false

### File Organization
```
src/components/NMEA2000Handlers.cpp  # ADD: HandleN2kPGN127252 function
src/components/NMEA2000Handlers.h    # ADD: Function declaration
```

**Changes Required**:
1. **NMEA2000Handlers.h**: Add function declaration and Doxygen comments (~15 lines)
2. **NMEA2000Handlers.cpp**: Add handler function implementation (~60 lines)
3. **NMEA2000Handlers.cpp**: Update RegisterN2kHandlers() to register PGN 127252 (~3 lines)

### Testing Approach
**TDD Required**: Tests must be written BEFORE implementation (Phase 2 gate)

**Test Files**:
1. **Contract Test** (`test_boatdata_contracts/test_main.cpp`):
   - Validate handler signature matches expected pattern
   - Ensure handler can be called with mock parameters

2. **Integration Test** (`test_boatdata_integration/test_heave_from_pgn127252.cpp`):
   - **Scenario 1**: Valid heave (2.5m) → Stored correctly
   - **Scenario 2**: Out-of-range heave (6.0m) → Clamped to 5.0m
   - **Scenario 3**: Negative heave (-3.2m) → Stored correctly (valid range)
   - **Scenario 4**: Unavailable (N2kDoubleNA) → No update, DEBUG log
   - **Scenario 5**: Parse failure → ERROR log, availability=false

3. **Unit Test** (`test_boatdata_units/test_main.cpp`):
   - Heave validation functions already tested in Enhanced BoatData v2.0.0
   - No new unit tests required (reusing existing validation tests)

4. **Hardware Test** (`test_boatdata_hardware/test_main.cpp`):
   - Add placeholder for PGN 127252 timing test
   - Actual test pending NMEA2000 bus initialization

**Test Execution Order**:
1. Write contract test → Must FAIL (handler doesn't exist)
2. Write integration tests → Must FAIL (handler doesn't exist)
3. Implement handler → All tests PASS
4. Verify tests PASS on native platform

### Memory Footprint
**RAM Impact**: ~0 bytes
- No new global variables
- No new heap allocations
- Reuses existing CompassData structure (heave field already exists)
- Stack usage: ~200 bytes during handler execution (local variables)

**Flash Impact**: ~2KB
- Handler function: ~60 lines × ~20 bytes/line = 1,200 bytes
- Function declaration + comments: ~300 bytes
- Handler registration: ~100 bytes
- Total estimated: 1,600 bytes (~0.08% of 1.9 MB partition)
- Post-implementation flash usage: 47.7% → 47.8% (negligible increase)

**Compile-Time Verification**:
- Flash usage will be checked during build
- Warning triggered if >80% capacity
- Current usage: 47.7% (plenty of headroom)

## Phase 2: Task Breakdown (Planning Only)

**Approach**: The /tasks command will generate a detailed, dependency-ordered task list following the TDD approach established in Enhanced BoatData v2.0.0 (R005).

**Task Organization Pattern** (to be generated by /tasks):
1. **Phase 1: Setup** (if needed)
   - No setup tasks required - reuses existing infrastructure

2. **Phase 2: Tests First (TDD Gate)**
   - Write contract test for HandleN2kPGN127252 signature
   - Write integration test for valid heave scenario
   - Write integration test for out-of-range heave scenario
   - Write integration test for unavailable heave scenario
   - Write integration test for parse failure scenario
   - Verify all tests FAIL (TDD gate)

3. **Phase 3: Implementation**
   - Add HandleN2kPGN127252 function declaration to NMEA2000Handlers.h
   - Implement HandleN2kPGN127252 function in NMEA2000Handlers.cpp
   - Update RegisterN2kHandlers() to register PGN 127252
   - Verify all tests PASS

4. **Phase 4: Validation & Documentation**
   - Run hardware test suite (placeholder for PGN 127252)
   - Update CLAUDE.md with PGN 127252 handler documentation
   - Update CHANGELOG.md with feature summary
   - Verify build succeeds (flash usage check)
   - Verify constitutional compliance

**Dependency Rules**:
- Tests must be written before implementation (TDD gate)
- Contract tests before integration tests
- Integration tests before implementation
- Implementation must not proceed until all tests fail (TDD verification)

**Parallel Execution**:
- Integration test scenarios can be written in parallel (independent)
- Documentation updates can be done in parallel (independent)

**Task Count Estimate**: ~12-15 tasks
- 5 test tasks (contract + 4 integration scenarios)
- 3 implementation tasks (header, implementation, registration)
- 4 validation/documentation tasks

## Progress Tracking

### Phase 0: Research ✓
- [X] Technical research completed
- [X] Dependencies identified (NMEA2000 library, DataValidation, WebSocketLogger, BoatData)
- [X] Implementation pattern established (existing handler pattern)
- [X] Risks assessed and mitigated

### Phase 1: Design ✓
- [X] Data model defined (reuses CompassData.heave)
- [X] API contracts specified (HandleN2kPGN127252 signature)
- [X] Integration scenarios documented (4 scenarios)
- [X] Test approach defined (contract, integration, unit, hardware)
- [X] Memory footprint estimated (~2KB flash, ~0 bytes RAM)

### Phase 2: Tasks (Pending /tasks command)
- [ ] Task list generated with dependencies
- [ ] TDD approach validated (tests before implementation)
- [ ] Parallel execution opportunities identified

### Constitution Re-Check ✓
- [X] No new violations introduced
- [X] Hardware abstraction maintained (NMEA2000 library)
- [X] Resource management verified (no new allocations)
- [X] WebSocket logging confirmed (DEBUG/WARN/ERROR)
- [X] Graceful degradation ensured (N2kDoubleNA handling)

## Next Steps

**Ready for /tasks command** ✓

The /tasks command will generate the detailed task breakdown in `tasks.md`, following the TDD approach outlined in Phase 2 above. Tasks will be dependency-ordered, with clear gates for TDD verification (tests must fail before implementation proceeds).

After /tasks completes, the /implement command can execute the implementation following the generated task list.

---

**Plan Status**: ✅ COMPLETE
**Constitution Check**: ✅ PASS
**Ready for**: /tasks command
