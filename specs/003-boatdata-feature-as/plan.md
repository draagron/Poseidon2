
# Implementation Plan: BoatData - Centralized Marine Data Model

**Branch**: `003-boatdata-feature-as` | **Date**: 2025-10-06 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `/specs/003-boatdata-feature-as/spec.md`

## Execution Flow (/plan command scope)
```
1. Load feature spec from Input path
   → ✅ Feature spec loaded successfully
2. Fill Technical Context (scan for NEEDS CLARIFICATION)
   → ✅ All clarifications resolved in Session 2025-10-06
   → Project Type: Embedded ESP32 (single project)
   → Structure Decision: src/ with hal/, components/, utils/
3. Fill the Constitution Check section
   → ✅ Constitution v1.1.0 requirements identified and documented
4. Evaluate Constitution Check section
   → ✅ Initial Constitution Check: PASS
5. Execute Phase 0 → research.md
   → ✅ Research complete: concurrency model (ReactESP), formulas validated, outlier algorithm defined
6. Execute Phase 1 → contracts, data-model.md, quickstart.md
   → ✅ Design complete: 3 entities, 4 contracts, 8 test scenarios, CLAUDE.md updated
7. Re-evaluate Constitution Check section
   → ✅ Post-Design Constitution Check: PASS (all principles satisfied)
8. Plan Phase 2 → Describe task generation approach
   → ✅ Task planning strategy documented (35-40 tasks, TDD order, dependency tracking)
9. STOP - Ready for /tasks command
   → ✅ Planning phase complete - ready for /tasks
```

**IMPORTANT**: The /plan command STOPS at step 8. Phases 2-4 are executed by other commands:
- Phase 2: /tasks command creates tasks.md
- Phase 3-4: Implementation execution (manual or via tools)

## Summary
The BoatData feature provides a centralized data model for marine sensor data with automatic source prioritization, real-time derived parameter calculations (200ms cycle), and a web-based calibration interface. The system acts as the core data hub for the Poseidon2 gateway, aggregating data from NMEA0183, NMEA2000, and 1-Wire sensors while calculating derived sailing parameters (true wind, leeway, VMG, current speed/direction).

**Technical Approach**: Use a statically-allocated C++ structure with HAL-abstracted interfaces for sensor updates, ReactESP for 200ms calculation cycles, LittleFS for calibration persistence, and ESPAsyncWebServer for the calibration web UI.

## Technical Context
**Language/Version**: C++ (C++14 preferred, C++11 minimum)
**Primary Dependencies**:
- ReactESP v2.0.0 (asynchronous event loops)
- ESPAsyncWebServer (calibration web interface)
- LittleFS (calibration parameter persistence)
- NMEA2000, NMEA0183 libraries (data source interfaces - not direct dependencies)

**Storage**:
- Flash: LittleFS for calibration parameters only (K factor, wind angle offset)
- RAM: Static allocation for all sensor data, derived parameters, diagnostic counters

**Testing**:
- PlatformIO native environment with GoogleTest (unit tests with mocked HAL)
- ESP32 hardware tests for timing validation
- Contract tests for sensor update interfaces

**Target Platform**: ESP32 (SH-ESP32 board), Arduino framework, 24/7 always-on operation

**Project Type**: Single embedded project (src/ with hal/, components/, utils/ subdirectories)

**Performance Goals**:
- 200ms calculation cycle (5 Hz update rate for derived parameters)
- <100ms web API response time for calibration updates
- Zero memory leaks over 24/7 operation

**Constraints**:
- Static memory allocation only (no heap fragmentation)
- Calculation cycle must complete within 200ms (skip + log if exceeded)
- 5-second stale data threshold for source failover
- Calibration ranges: K factor > 0, wind offset -2π to 2π radians

**Scale/Scope**:
- ~30 data fields (raw sensors + derived parameters)
- Up to 5 sensor sources per data type (GPS, compass)
- 2 calibration parameters
- 3 diagnostic counters

## Constitution Check
*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

**Hardware Abstraction (Principle I)**:
- [x] All hardware interactions use HAL interfaces
  - No direct hardware access in BoatData component
  - Sensor updates via abstract interfaces (NMEA handlers call update methods)
  - LittleFS abstraction already exists (IFileSystem)
- [x] Mock implementations provided for testing
  - Mock sensor sources for testing prioritization logic
  - Mock file system for testing calibration persistence
- [x] Business logic separable from hardware I/O
  - Calculation engine pure functions (inputs → outputs)
  - Data validation logic independent of sensor protocol

**Resource Management (Principle II)**:
- [x] Static allocation preferred; heap usage justified
  - BoatData struct: static global or singleton
  - SensorSource array: fixed size (MAX_SOURCES per type)
  - No dynamic allocation in calculation cycles
- [x] Stack usage estimated and within 8KB per task
  - Calculation functions: ~2KB max (vector math, no recursion)
  - Web handlers: ~3KB max (JSON serialization)
- [x] Flash usage impact documented
  - Estimated +15KB code (calculation engine, validation, prioritization)
  - +10KB for web UI endpoints
  - +5KB for persistence layer
  - Total: ~30KB additional flash (well within budget)
- [x] String literals use F() macro or PROGMEM
  - All log messages use F() macro
  - Web UI HTML/JSON templates in PROGMEM

**QA Review Process (Principle III - NON-NEGOTIABLE)**:
- [x] QA subagent review planned for all code changes
  - Critical review: calculation engine (mathematical correctness)
  - Critical review: angle wraparound handling
  - Critical review: outlier detection logic
- [x] Hardware-dependent tests minimized
  - Calculation engine: pure function unit tests (no hardware)
  - Source prioritization: logic tests with mock sources
  - Hardware test only for 200ms timing validation
- [x] Critical paths flagged for human review
  - Mathematical formulas (TWS, TWA, leeway, current)
  - Thread safety for concurrent sensor updates
  - Atomic calibration parameter updates

**Modular Design (Principle IV)**:
- [x] Components have single responsibility
  - BoatData: data storage only
  - CalculationEngine: pure calculation functions
  - SourcePrioritizer: source selection logic
  - CalibrationManager: persistence + web API
  - DataValidator: range checking + outlier detection
- [x] Dependency injection used for hardware dependencies
  - CalculationEngine depends on BoatData (interface)
  - CalibrationManager depends on IFileSystem
  - No direct LittleFS calls in business logic
- [x] Public interfaces documented
  - Doxygen comments for all public methods
  - Usage examples for NMEA handler integration

**Network Debugging (Principle V)**:
- [x] UDP broadcast logging implemented
  - WebSocket logging already available (WiFi foundation feature)
  - Log calculation cycle overruns (NFR-001)
  - Log rejected sensor readings (FR-017)
  - Log source failover events
- [x] Log levels defined (DEBUG/INFO/WARN/ERROR/FATAL)
  - INFO: Source priority changes, calibration updates
  - WARN: Calculation cycle overrun, outlier rejected
  - ERROR: All sources failed for a sensor type
- [x] Flash fallback for critical errors
  - Covered by existing WiFi management infrastructure

**Always-On Operation (Principle VI)**:
- [x] WiFi always-on requirement met
  - No sleep modes in calculation cycle
  - Web interface depends on WiFi (already always-on)
- [x] No deep sleep/light sleep modes used
  - ReactESP event loop runs continuously
  - 200ms timer always active
- [x] Designed for 24/7 operation
  - Static allocation prevents memory leaks
  - Diagnostic counters track uptime

**Fail-Safe Operation (Principle VII)**:
- [x] Watchdog timer enabled (production)
  - Calculation cycle bounded (200ms max)
  - Overrun triggers skip (not hang)
- [x] Safe mode/recovery mode implemented
  - Graceful degradation: missing sensor data → NaN/unavailable
  - All sources failed → mark data unavailable, continue operation
- [x] Graceful degradation for failures
  - Missing GPS → no current calculation, but wind still calculated
  - Missing compass → no TWA/WDIR, but TWS still calculated
  - Missing calibration file → use default values (K=1.0, offset=0.0)

**Technology Stack Compliance**:
- [x] Using approved libraries
  - ReactESP: calculation cycle timer (200ms repeat)
  - ESPAsyncWebServer: calibration web interface
  - LittleFS: calibration persistence (via existing IFileSystem)
- [x] File organization follows src/ structure
  - src/components/BoatData.h/.cpp
  - src/components/CalculationEngine.h/.cpp
  - src/components/SourcePrioritizer.h/.cpp
  - src/components/CalibrationManager.h/.cpp
  - src/utils/DataValidator.h/.cpp
  - test/unit/test_calculation_engine.cpp
  - test/unit/test_source_prioritizer.cpp
- [x] Conventional commits format
  - feat: add BoatData centralized data model
  - feat: add calculation engine for derived parameters
  - feat: add source prioritization with failover
  - feat: add calibration web interface

**Initial Constitution Check**: ✅ PASS

## Project Structure

### Documentation (this feature)
```
specs/003-boatdata-feature-as/
├── spec.md              # Feature specification (completed)
├── plan.md              # This file (/plan command output)
├── research.md          # Phase 0 output (/plan command)
├── data-model.md        # Phase 1 output (/plan command)
├── quickstart.md        # Phase 1 output (/plan command)
├── contracts/           # Phase 1 output (/plan command)
│   ├── IBoatDataStore.h # Data storage interface contract
│   ├── ISensorUpdate.h  # Sensor update interface contract
│   └── ICalibration.h   # Calibration interface contract
└── tasks.md             # Phase 2 output (/tasks command - NOT created by /plan)
```

### Source Code (repository root)
```
src/
├── components/
│   ├── BoatData.h                  # Central data structure (statically allocated)
│   ├── BoatData.cpp                # Data access methods
│   ├── CalculationEngine.h         # Derived parameter calculation functions
│   ├── CalculationEngine.cpp       # TWS, TWA, leeway, VMG, SOC, DOC calculations
│   ├── SourcePrioritizer.h         # Multi-source priority + failover logic
│   ├── SourcePrioritizer.cpp       # Automatic priority (frequency-based) + manual override
│   ├── CalibrationManager.h        # Calibration persistence + web API
│   ├── CalibrationManager.cpp      # LittleFS read/write, ESPAsyncWebServer endpoints
│   └── CalibrationWebServer.h/.cpp # HTTP endpoints for calibration UI
├── utils/
│   ├── DataValidator.h             # Range validation + outlier detection
│   ├── DataValidator.cpp           # Validate lat/lon, angles, speeds; detect outliers
│   └── AngleUtils.h                # Angle wraparound, conversion utilities
├── hal/
│   └── interfaces/
│       └── IBoatDataStore.h        # Abstract interface for data storage (for mocking)
└── mocks/
    └── MockBoatDataStore.h         # Mock implementation for testing

test/
├── unit/
│   ├── test_calculation_engine.cpp # Pure function tests (no hardware)
│   ├── test_source_prioritizer.cpp # Priority logic tests (mocked sources)
│   ├── test_data_validator.cpp     # Validation + outlier tests
│   └── test_calibration_manager.cpp# Persistence tests (mocked filesystem)
├── integration/
│   ├── test_sensor_update_flow.cpp # End-to-end: sensor update → calculation → read
│   └── test_calibration_flow.cpp   # End-to-end: web update → persist → calculation
└── test_boatdata_timing/
    └── test_main.cpp               # Hardware test: 200ms cycle timing validation
```

**Structure Decision**: Single embedded project structure following Poseidon2 constitutional file organization (src/ with hal/, components/, utils/, mocks/ subdirectories). This aligns with the existing WiFi management foundation and follows the modular component design principle.

## Phase 0: Outline & Research

**Unknowns Identified**: None remaining - all clarifications resolved in Session 2025-10-06.

**Research Tasks**:
1. ✅ **Concurrency Model Decision**:
   - **Question**: RTOS tasks vs ReactESP single-threaded event loop?
   - **Research needed**: Thread safety requirements for concurrent sensor updates

2. ✅ **Mathematical Formula Validation**:
   - **Question**: Verify calculation formulas from examples/Calculations/calc.cpp
   - **Research needed**: Confirm angle wraparound handling, singularity detection

3. ✅ **Outlier Detection Algorithm**:
   - **Question**: Define "significantly deviate" threshold for FR-016
   - **Research needed**: Marine sensor noise characteristics, acceptable deviation ranges

**Output**: research.md with decisions on concurrency model, validated formulas, outlier thresholds

## Phase 1: Design & Contracts
*Prerequisites: research.md complete*

### Entities (to data-model.md)
1. **BoatData Entity**:
   - Raw sensor fields (GPS, compass, wind, speed, attitude)
   - Derived parameter fields (TWS, TWA, STW, VMG, SOC, DOC, leeway, corrected AWA)
   - Calibration fields (K factor, wind offset)
   - Diagnostic fields (message counts, last update timestamp)
   - Data availability flags (per field)

2. **SensorSource Entity**:
   - Source ID, sensor type enum
   - Protocol type enum (NMEA0183/NMEA2000/1Wire)
   - Update frequency (Hz), last update timestamp
   - Priority level (automatic or manual override)
   - Availability status (active/stale/failed)

3. **CalibrationParameters Entity**:
   - K factor (double, > 0)
   - Wind angle offset (double, -2π to 2π)
   - Validation rules, persistence flag

### API Contracts (to /contracts/)
**No REST/GraphQL endpoints** - This is an embedded component, not a web service.

**Contracts are C++ interfaces**:

1. **IBoatDataStore.h** - Data storage interface:
   ```cpp
   // Methods: getGPSData(), setGPSData(), getWindData(), etc.
   // Purpose: Abstract data access for testing
   ```

2. **ISensorUpdate.h** - Sensor update interface:
   ```cpp
   // Methods: updateGPS(), updateCompass(), updateWind(), etc.
   // Purpose: Called by NMEA/1-Wire handlers to push sensor data
   ```

3. **ICalibration.h** - Calibration interface:
   ```cpp
   // Methods: getCalibration(), setCalibration(), validateCalibration()
   // Purpose: Web API and calculation engine access
   ```

### Contract Tests (to test/contract/)
1. **test_iboatdatastore_contract.cpp**:
   - Assert data retrieval returns expected structure
   - Assert data updates are reflected in subsequent reads
   - Assert concurrent updates don't corrupt data

2. **test_isensorupdate_contract.cpp**:
   - Assert sensor updates trigger data availability flags
   - Assert invalid data rejected by contract
   - Assert update timestamps recorded

3. **test_icalibration_contract.cpp**:
   - Assert calibration persistence (write → read)
   - Assert validation rejects out-of-range values
   - Assert atomic updates during calculations

### Integration Test Scenarios (to quickstart.md)
Extracted from acceptance scenarios:

1. **Single Source GPS** (Scenario 1):
   - Setup: Mock NMEA0183 GPS source
   - Action: Send position data (40.7128°N, -74.0060°W)
   - Assert: getGPSData() returns correct values

2. **Multi-Source Priority** (Scenario 2):
   - Setup: Mock GPS-A (1 Hz) + GPS-B (10 Hz)
   - Action: Both send data
   - Assert: Active source is GPS-B (higher frequency)

3. **Source Failover** (Scenario 3):
   - Setup: GPS-B active, GPS-A standby
   - Action: GPS-B stops (>5s no data)
   - Assert: Active source switches to GPS-A

4. **Derived Calculation** (Scenario 5):
   - Setup: Valid GPS, compass, wind, speed data
   - Action: Trigger 200ms calculation cycle
   - Assert: All derived parameters (TWS, TWA, STW, VMG, SOC, DOC, leeway) calculated

5. **Calibration Update** (Scenario 6):
   - Setup: K=0.5 persisted
   - Action: POST /api/calibration {"k": 0.65}
   - Assert: Next calculation uses K=0.65, value persisted to flash

6. **Outlier Rejection** (Scenario 7):
   - Setup: GPS reporting lat=40.7128°N
   - Action: Send invalid lat=200°N
   - Assert: Outlier rejected, last valid value retained, rejection logged

### Agent File Update
Run: `.specify/scripts/bash/update-agent-context.sh claude`

**New Technical Context to Add**:
- BoatData component: centralized sensor data storage
- CalculationEngine: 200ms cycle for derived sailing parameters
- SourcePrioritizer: frequency-based automatic prioritization
- CalibrationManager: LittleFS persistence + web API
- Data units: angles in radians, speeds in knots, coordinates in decimal degrees

**Output**: data-model.md, /contracts/*, contract test files (failing), quickstart.md, CLAUDE.md updated

## Phase 2: Task Planning Approach
*This section describes what the /tasks command will do - DO NOT execute during /plan*

**Task Generation Strategy**:
1. Load `.specify/templates/tasks-template.md` as base
2. Generate tasks from Phase 1 design docs:
   - Each contract → contract test task [P]
   - Each entity → model creation task [P]
   - Each user story → integration test task
   - Calculation engine → formula implementation tasks (sequential: AWA corrections → leeway → STW → TWS/TWA → VMG → WDIR → SOC/DOC)
   - Source prioritization → priority tracking tasks
   - Calibration → persistence + web API tasks

**Ordering Strategy**:
- **TDD Order**: Tests before implementation
  1. Contract interface definitions [P]
  2. Contract tests (failing) [P]
  3. Data structures (BoatData, SensorSource, CalibrationParameters) [P]
  4. Validation utilities (range checks, outlier detection) [P]
  5. Calculation engine (pure functions, sequential by dependency)
  6. Source prioritization logic
  7. Calibration persistence
  8. Web API endpoints
  9. ReactESP integration (200ms timer)
  10. Integration tests

- **Dependency Order**:
  - Data structures before calculation engine
  - Validation before sensor updates
  - Calibration persistence before web API
  - All components before ReactESP integration

- **Parallel Execution**: Mark [P] for independent files
  - Contract interfaces [P]
  - Contract tests [P]
  - Data structures [P]
  - Validation utilities [P]
  - Unit tests [P]

**Estimated Output**: 35-40 numbered, ordered tasks in tasks.md

**Categories**:
- Setup (1-2 tasks): Contract interfaces, data structures
- Validation (3-5 tasks): Range checks, outlier detection, unit tests
- Calculation Engine (10-15 tasks): Formula implementation (one task per derived parameter), unit tests
- Source Prioritization (5-7 tasks): Priority logic, failover, unit tests
- Calibration (5-7 tasks): Persistence, web API, integration tests
- Integration (5-8 tasks): ReactESP timer, sensor update flow, end-to-end tests
- QA Review (1 task): Constitutional compliance review

**IMPORTANT**: This phase is executed by the /tasks command, NOT by /plan

## Phase 3+: Future Implementation
*These phases are beyond the scope of the /plan command*

**Phase 3**: Task execution (/tasks command creates tasks.md)
**Phase 4**: Implementation (execute tasks.md following constitutional principles)
**Phase 5**: Validation (run tests, execute quickstart.md, performance validation)

## Complexity Tracking
*No constitutional violations - no entries required*

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| (none)    | N/A        | N/A                                  |

## Progress Tracking
*This checklist is updated during execution flow*

**Phase Status**:
- [x] Phase 0: Research complete (/plan command) ✅
  - research.md created with concurrency model, formula validation, outlier detection decisions
- [x] Phase 1: Design complete (/plan command) ✅
  - data-model.md created (BoatData, SensorSource, CalibrationParameters entities)
  - contracts/ created (IBoatDataStore, ISensorUpdate, ICalibration, ISourcePrioritizer interfaces)
  - quickstart.md created (7 integration test scenarios + 1 hardware timing test)
  - CLAUDE.md updated with BoatData technical context
- [x] Phase 2: Task planning complete (/plan command - describe approach only) ✅
  - Task generation strategy documented
  - Ordering strategy defined (TDD, dependency order, parallel execution)
  - Estimated 35-40 tasks
- [ ] Phase 3: Tasks generated (/tasks command) - **Next step**
- [ ] Phase 4: Implementation complete
- [ ] Phase 5: Validation passed

**Gate Status**:
- [x] Initial Constitution Check: PASS ✅
- [x] Post-Design Constitution Check: PASS ✅
  - All 7 constitutional principles satisfied
  - Static allocation confirmed (BoatData ~1400 bytes)
  - ReactESP pattern adopted (mandatory)
  - HAL abstraction maintained (no direct hardware access)
  - QA review checkpoints identified (calculation formulas, thread safety, angle wraparound)
- [x] All NEEDS CLARIFICATION resolved ✅
- [x] Complexity deviations documented (none) ✅

**Artifacts Generated**:
- ✅ /specs/003-boatdata-feature-as/plan.md
- ✅ /specs/003-boatdata-feature-as/research.md
- ✅ /specs/003-boatdata-feature-as/data-model.md
- ✅ /specs/003-boatdata-feature-as/contracts/README.md
- ✅ /specs/003-boatdata-feature-as/quickstart.md
- ✅ /CLAUDE.md (updated)

---
*Based on Constitution v1.1.0 - See `.specify/memory/constitution.md`*
