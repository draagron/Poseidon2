# Tasks: Enhanced BoatData

**Input**: Design documents from `/home/niels/Dev/Poseidon2/specs/008-enhanced-boatdata-following/`
**Prerequisites**: plan.md, research.md, data-model.md, contracts/, quickstart.md

## Overview

This tasks.md implements Enhanced BoatData (R005) following Test-Driven Development (TDD) and Constitutional principles. The feature extends marine sensor data structures to include:
- GPS variation (moved from CompassData to GPSData)
- Compass motion sensors (rate of turn, heel, pitch, heave)
- DST sensor data (SpeedData renamed to DSTData with depth and temperature)
- Engine telemetry (new EngineData structure)
- Saildrive status (new SaildriveData structure via 1-wire)
- Battery monitoring (new BatteryData structure via 1-wire)
- Shore power monitoring (new ShorePowerData structure via 1-wire)

**Total Tasks**: 52 tasks organized in TDD order

## Format: `[ID] [P?] Description`
- **[P]**: Can run in parallel (different files, no dependencies)
- Include exact file paths in descriptions

## Phase 3.1: Setup & Data Model (Foundational)

### HAL Interface & Types
- [X] **T001** Create BoatDataTypes.h v2.0.0 in `src/types/BoatDataTypes.h` with all 7 enhanced/new data structures (GPSData, CompassData, DSTData, EngineData, SaildriveData, BatteryData, ShorePowerData)
- [X] **T002** Create HAL interface IOneWireSensors in `src/hal/interfaces/IOneWireSensors.h` for 1-wire sensor abstraction (saildrive, battery, shore power)

### Mock Implementations
- [X] **T003** [P] Create MockOneWireSensors in `src/mocks/MockOneWireSensors.cpp/h` with simulated sensor data for testing

## Phase 3.2: Tests First (TDD) ⚠️ MUST COMPLETE BEFORE 3.3
**CRITICAL: These tests MUST be written and MUST FAIL before ANY implementation**

### Contract Tests (HAL interface validation)
- [X] **T004** Create test directory `test/test_boatdata_contracts/` with test_main.cpp Unity runner
- [X] **T005** [P] Contract test for IOneWireSensors in `test/test_boatdata_contracts/test_ionewire.cpp` (initialize, readSaildriveStatus, readBatteryA/B, readShorePower, isBusHealthy)
- [X] **T006** [P] Contract test for BoatDataTypes_v2 structures in `test/test_boatdata_contracts/test_data_structures.cpp` (field presence, types, memory layout)
- [X] **T007** [P] Contract test for memory footprint in `test/test_boatdata_contracts/test_memory_footprint.cpp` (sizeof(BoatDataStructure) ≤ 600 bytes)

### Integration Tests (End-to-end scenarios - from quickstart.md)
- [X] **T008** Create test directory `test/test_boatdata_integration/` with test_main.cpp Unity runner
- [X] **T009** [P] Integration test for GPS variation migration in `test/test_boatdata_integration/test_gps_variation.cpp` (FR-001, FR-009)
- [X] **T010** [P] Integration test for compass rate of turn in `test/test_boatdata_integration/test_compass_rate_of_turn.cpp` (FR-005, PGN 127251)
- [X] **T011** [P] Integration test for compass attitude in `test/test_boatdata_integration/test_compass_attitude.cpp` (FR-006-008, PGN 127257: heel/pitch/heave)
- [X] **T012** [P] Integration test for DSTData structure in `test/test_boatdata_integration/test_dst_sensors.cpp` (FR-002, FR-010-012, depth/speed/temp)
- [X] **T013** [P] Integration test for engine telemetry in `test/test_boatdata_integration/test_engine_telemetry.cpp` (FR-013-016, PGN 127488/127489)
- [X] **T014** [P] Integration test for saildrive status in `test/test_boatdata_integration/test_saildrive_status.cpp` (FR-017-018, 1-wire polling)
- [X] **T015** [P] Integration test for battery monitoring in `test/test_boatdata_integration/test_battery_monitoring.cpp` (FR-019-025, dual battery banks via 1-wire)
- [X] **T016** [P] Integration test for shore power monitoring in `test/test_boatdata_integration/test_shore_power.cpp` (FR-026-028, connection status and power draw)

### Unit Tests (Validation logic and conversions)
- [X] **T017** Create test directory `test/test_boatdata_units/` with test_main.cpp Unity runner
- [X] **T018** [P] Unit test for validation and clamping in `test/test_boatdata_units/test_validation.cpp` (pitch ±30°, heave ±5m, RPM [0,6000], voltage ranges)
- [X] **T019** [P] Unit test for unit conversions in `test/test_boatdata_units/test_unit_conversions.cpp` (Kelvin→Celsius for PGN 130316)
- [X] **T020** [P] Unit test for sign conventions in `test/test_boatdata_units/test_sign_conventions.cpp` (battery amperage +charge/-discharge, heel +starboard/-port, pitch +bow_up/-bow_down)

## Phase 3.3: Core Implementation (ONLY after tests are failing)

### Data Structure Migration
- [X] **T021** Update `src/types/BoatDataTypes.h` to v2.0.0 schema (implement T001 contract)
- [X] **T022** Add backward compatibility typedef `typedef DSTData SpeedData;` in BoatDataTypes.h for migration support
- [X] **T023** Update global `BoatDataStructure boatData;` instance in `src/main.cpp` to use new structure

### HAL Implementation (1-Wire Sensors)
- [X] **T024** [P] Implement ESP32OneWireSensors in `src/hal/implementations/ESP32OneWireSensors.cpp/h` using OneWire and DallasTemperature libraries (GPIO 4)
- [X] **T025** [P] Implement MockOneWireSensors test implementation (complete T003 stub)

### NMEA2000 PGN Handlers (New and Enhanced)
- [X] **T026** [P] Add PGN 127251 handler (Rate of Turn) in `src/components/NMEA2000Handlers.cpp` → CompassData.rateOfTurn
- [X] **T027** [P] Add PGN 127257 handler (Attitude) in `src/components/NMEA2000Handlers.cpp` → CompassData heel/pitch/heave
- [X] **T028** [P] Enhance PGN 129029 handler (GNSS Position) in `src/components/NMEA2000Handlers.cpp` → GPSData.variation (add variation field extraction)
- [X] **T029** [P] Add PGN 128267 handler (Water Depth) in `src/components/NMEA2000Handlers.cpp` → DSTData.depth
- [X] **T030** [P] Add PGN 128259 handler (Speed - Water Referenced) in `src/components/NMEA2000Handlers.cpp` → DSTData.measuredBoatSpeed
- [X] **T031** [P] Add PGN 130316 handler (Temperature Extended Range) in `src/components/NMEA2000Handlers.cpp` → DSTData.seaTemperature (with Kelvin→Celsius conversion)
- [X] **T032** [P] Add PGN 127488 handler (Engine Parameters, Rapid Update) in `src/components/NMEA2000Handlers.cpp` → EngineData.engineRev
- [X] **T033** [P] Add PGN 127489 handler (Engine Parameters, Dynamic) in `src/components/NMEA2000Handlers.cpp` → EngineData.oilTemperature, alternatorVoltage

### Validation Logic
- [X] **T034** [P] Implement validation helper functions in `src/utils/DataValidation.h` (clampPitchAngle, clampHeave, clampEngineRPM, clampBatteryVoltage, clampTemperature)
- [X] **T035** Integrate validation functions into PGN handlers (T026-T033) with WebSocket logging for out-of-range values

## Phase 3.4: Integration (Wire into main.cpp)

### 1-Wire Polling Integration
- [X] **T036** Initialize IOneWireSensors in `src/main.cpp` setup() function after I2C initialization
- [X] **T037** Add ReactESP event loop for saildrive polling (1000ms) in `src/main.cpp` using `app.onRepeat(1000, pollSaildriveStatus)`
- [X] **T038** Add ReactESP event loop for battery polling (2000ms) in `src/main.cpp` using `app.onRepeat(2000, pollBatteryStatus)`
- [X] **T039** Add ReactESP event loop for shore power polling (2000ms) in `src/main.cpp` using `app.onRepeat(2000, pollShorePowerStatus)`

### NMEA2000 Handler Registration
- [X] **T040** Register new PGN handlers (127251, 127257, 128267, 128259, 130316, 127488, 127489) in `src/main.cpp` NMEA2000 setup section
- [X] **T041** Update PGN handler array size in `src/components/NMEA2000Handlers.h` to accommodate new handlers

### WebSocket Logging Integration
- [X] **T042** Add WebSocket log events for GPS variation updates in `src/components/NMEA2000Handlers.cpp` (DEBUG level)
- [X] **T043** Add WebSocket log events for compass attitude updates in `src/components/NMEA2000Handlers.cpp` (DEBUG level)
- [X] **T044** Add WebSocket log events for DST sensor updates in `src/components/NMEA2000Handlers.cpp` (DEBUG level)
- [X] **T045** Add WebSocket log events for engine telemetry updates in `src/components/NMEA2000Handlers.cpp` (DEBUG level)
- [X] **T046** Add WebSocket log events for 1-wire sensor polling in `src/main.cpp` event loops (DEBUG level for normal, WARN for failures)

## Phase 3.5: Hardware Validation & Polish

### Hardware Tests (ESP32 Required)
- [X] **T047** Create test directory `test/test_boatdata_hardware/` with test_main.cpp Unity runner for ESP32 platform
- [X] **T048** Hardware test for 1-wire bus communication in `test/test_boatdata_hardware/test_main.cpp` (device enumeration, CRC validation, bus health)
- [X] **T049** Hardware test for NMEA2000 PGN reception timing in `test/test_boatdata_hardware/test_main.cpp` (verify PGN handlers called within expected intervals)

### Documentation & Validation
- [X] **T050** [P] Update `CLAUDE.md` with Enhanced BoatData integration guide (data structures, PGN handlers, 1-wire sensor setup, validation rules)
- [X] **T051** [P] Update `README.md` with R005 feature status and memory footprint metrics
- [X] **T052** Constitutional compliance validation checklist (Principle I: HAL abstraction ✓, Principle II: Resource management ✓, Principle V: WebSocket logging ✓, Principle VII: Graceful degradation ✓)

## Dependencies

### Critical Path (TDD Flow)
```
T001-T003 (Setup)
  ↓
T004-T020 (All tests - MUST FAIL before implementation)
  ↓
T021-T025 (Data structures and HAL implementation)
  ↓
T026-T035 (PGN handlers and validation)
  ↓
T036-T046 (Integration into main.cpp)
  ↓
T047-T049 (Hardware validation)
  ↓
T050-T052 (Documentation and compliance)
```

### Detailed Dependencies
- **T001-T002** (HAL interface & types) → **T004-T020** (all tests)
- **T003** (Mock) → **T005, T014-T016** (tests using MockOneWireSensors)
- **T004-T020** (All tests) → **T021-T035** (Implementation - TDD gate)
- **T021-T023** (Data structures) → **T024-T033** (HAL and PGN handlers)
- **T024** (ESP32OneWireSensors) → **T036-T039** (1-wire polling integration)
- **T026-T033** (PGN handlers) → **T040-T041** (NMEA2000 registration)
- **T034-T035** (Validation) → **T042-T046** (Logging integration)
- **T036-T046** (Integration) → **T047-T049** (Hardware tests)
- **T047-T049** (Hardware validation) → **T052** (Constitutional compliance)

## Parallel Execution Examples

### Setup Phase (Independent File Creation)
```bash
# T003 can run independently after T001-T002
# Create mock implementation while preparing test infrastructure
```

### Test Phase (Write All Test Groups Simultaneously)
```bash
# After T004, T008, T017 create test directories:
# T005-T007 (contract tests) - independent [P]
# T009-T016 (integration tests) - independent [P]
# T018-T020 (unit tests) - independent [P]

# All test writing can happen in parallel (different files)
```

### Implementation Phase (Component and Adapter in Parallel)
```bash
# T024 (ESP32OneWireSensors) independent of T026-T033 (PGN handlers)
# T025 (MockOneWireSensors) independent of other implementations
# T026-T033 (8 PGN handlers) - all independent [P] (append to same file sequentially or use separate commits)
```

### Integration Phase (Sequential - Same File)
```bash
# T036-T041 modify src/main.cpp - MUST be sequential
# T042-T046 add logging - can be done during handler implementation
```

### Documentation Phase (Independent Files)
```bash
# T050 (CLAUDE.md) and T051 (README.md) - independent [P]
# T052 (compliance validation) after hardware tests complete
```

## Test Execution Commands

```bash
# Run all BoatData tests (native platform, mocked hardware)
pio test -e native -f test_boatdata_*

# Run specific test groups
pio test -e native -f test_boatdata_contracts       # Contract tests only (T005-T007)
pio test -e native -f test_boatdata_integration     # Integration tests only (T009-T016)
pio test -e native -f test_boatdata_units           # Unit tests only (T018-T020)

# Run hardware tests (ESP32 required)
pio test -e esp32dev_test -f test_boatdata_hardware # Hardware tests (T048-T049)

# Run build to check memory footprint
pio run | grep "RAM:"
pio run | grep "Flash:"
```

## Validation Checklist
*GATE: Verify before marking feature complete*

- [ ] All contracts have corresponding tests (T005-T007 cover IOneWireSensors + data structures)
- [ ] All entities have implementation tasks (7 data structures in T001, T021)
- [ ] All tests come before implementation (T004-T020 before T021-T035)
- [ ] Parallel tasks truly independent (verified [P] markers for different files)
- [ ] Each task specifies exact file path (all tasks include file paths)
- [ ] No task modifies same file as another [P] task (main.cpp tasks sequential T036-T041)
- [ ] All 10 quickstart.md scenarios have integration tests (T009-T016 cover scenarios 1-8, T018 covers scenario 9, T007 covers scenario 10)
- [ ] TDD gate enforced (T004-T020 MUST FAIL before T021 begins)
- [ ] Constitutional compliance validated (T052)

## Notes

### TDD Workflow Reminder
1. **Write test first** (T004-T020) - tests MUST FAIL
2. **Verify test failure** - run `pio test -e native -f test_boatdata_*`
3. **Implement minimal code** (T021-T035) to pass tests
4. **Verify test passes** - re-run test suite
5. **Refactor** - improve code while keeping tests green
6. **Integrate** (T036-T046) - wire into main.cpp
7. **Hardware validate** (T047-T049) - ESP32 platform tests

### Memory Budget Tracking
- **Baseline (v1.0.0)**: ~304 bytes BoatDataStructure
- **Target (v2.0.0)**: ≤600 bytes BoatDataStructure (~560 bytes expected)
- **Delta**: +256 bytes (acceptable per Constitution Principle II)
- **Validation**: T007 static assertion test, T052 compliance check

### Migration Strategy
- **Phase 1** (T021-T023): Introduce DSTData alongside SpeedData typedef
- **Phase 2** (T026-T033): Update handlers to populate new structures
- **Phase 3** (Future): Remove SpeedData typedef in release 2.1.0

### Constitutional Compliance Notes
- **Principle I** (Hardware Abstraction): IOneWireSensors HAL (T002, T024)
- **Principle II** (Resource Management): Memory footprint test (T007), static allocation only
- **Principle III** (QA Review): PR review before merge to main
- **Principle IV** (Modular Design): HAL interfaces, single-responsibility components
- **Principle V** (Network Debugging): WebSocket logging (T042-T046)
- **Principle VI** (Always-On): ReactESP non-blocking event loops (T037-T039)
- **Principle VII** (Fail-Safe): Validation with availability flags (T034-T035), graceful degradation

## Task Generation Rules Applied
*Documented for transparency*

1. **From HAL Interfaces** (contracts/IOneWireSensors.h):
   - ✓ Contract test task T005
   - ✓ Mock implementation task T003, T025
   - ✓ ESP32 hardware adapter task T024

2. **From Data Model** (data-model.md - 7 structures):
   - ✓ Types file task T001
   - ✓ Structure validation tests T006-T007

3. **From User Stories** (quickstart.md - 10 scenarios):
   - ✓ Integration tests T009-T016 (scenarios 1-8)
   - ✓ Unit tests T018-T020 (scenarios 9-10)

4. **From Research Findings** (research.md - 8 PGNs):
   - ✓ PGN handler tasks T026-T033
   - ✓ Validation logic tasks T034-T035

5. **From Constitutional Requirements**:
   - ✓ WebSocket logging tasks T042-T046
   - ✓ Hardware validation tasks T047-T049
   - ✓ Compliance validation task T052

**Total Generated**: 52 tasks across 5 phases

## Ready for Execution

This tasks.md is immediately executable. Each task is specific, actionable, and includes exact file paths. The TDD gate (T004-T020 before T021) ensures test-first development. Dependencies are clearly documented. Parallel execution opportunities marked with [P] for efficiency.

**Next Step**: Begin with T001 (create data structures), then T002 (HAL interface), then T003 (mock), then start test writing (T004-T020).
