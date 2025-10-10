# Implementation Plan: Enhanced BoatData

**Branch**: `008-enhanced-boatdata-following` | **Date**: 2025-10-10 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/home/niels/Dev/Poseidon2/specs/008-enhanced-boatdata-following/spec.md`

## Execution Flow (/plan command scope)
```
1. Load feature spec from Input path
   → ✅ Loaded from specs/008-enhanced-boatdata-following/spec.md
2. Fill Technical Context (scan for NEEDS CLARIFICATION)
   → ✅ Completed - ESP32/PlatformIO embedded system context
3. Fill the Constitution Check section
   → ✅ Completed - All principles evaluated
4. Evaluate Constitution Check section below
   → ✅ PASS - No violations, no complexity deviations
5. Execute Phase 0 → research.md
   → ✅ Completed - All unknowns resolved (pitch/heave ranges, RPM unit, refresh rates, etc.)
6. Execute Phase 1 → contracts, data-model.md, quickstart.md, CLAUDE.md
   → ✅ Completed - data-model.md, contracts/IOneWireSensors.h, contracts/BoatDataTypes_v2.h, quickstart.md, CLAUDE.md updated
7. Re-evaluate Constitution Check section
   → ✅ PASS - Design maintains constitutional compliance
8. Plan Phase 2 → Describe task generation approach (DO NOT create tasks.md)
   → ✅ Completed - Task generation strategy documented below
9. STOP - Ready for /tasks command
   → ✅ Planning complete - Proceed to /tasks
```

**IMPORTANT**: The /plan command STOPS at step 9. Phases 2-4 are executed by other commands:
- Phase 2: /tasks command creates tasks.md
- Phase 3-4: Implementation execution (manual or via tools)

## Summary

Enhanced BoatData (R005) extends the existing marine sensor data model to include:
- **GPS variation** (moved from CompassData to GPSData)
- **Compass motion sensors** (rate of turn, heel, pitch, heave added to CompassData)
- **DST sensor data** (SpeedData renamed to DSTData with depth and seaTemperature added)
- **Engine telemetry** (new EngineData structure for RPM, oil temp, alternator voltage)
- **Saildrive status** (new SaildriveData structure for engagement monitoring)
- **Battery monitoring** (new BatteryData structure for dual battery banks via 1-wire)
- **Shore power** (new ShorePowerData structure for connection status and power draw)

**Technical Approach**:
- Incremental refactoring with backward compatibility shim (SpeedData typedef → DSTData)
- HAL abstraction for 1-wire sensors (IOneWireSensors interface)
- NMEA2000 PGN handlers for new data sources (8 new PGNs, 2 enhanced PGNs)
- ReactESP event loops with sensor-specific refresh rates (100ms to 2000ms)
- Validation at HAL boundary (clamping, availability flags, WebSocket logging)
- Static allocation only (total +256 bytes RAM, ~0.08% of ESP32 capacity)

## Technical Context

**Language/Version**: C++ (C++11 minimum, C++14 preferred) with Arduino framework
**Primary Dependencies**: NMEA0183, NMEA2000, NMEA2000_esp32, ReactESP, ESPAsyncWebServer, Adafruit_SSD1306, OneWire, DallasTemperature
**Storage**: LittleFS for calibration persistence (existing), no new storage requirements
**Testing**: Unity test framework on PlatformIO native environment (mock-first), minimal ESP32 hardware tests
**Target Platform**: ESP32 family (ESP32, ESP32-S2, ESP32-C3, ESP32-S3) - SH-ESP32 board
**Project Type**: ESP32 embedded system using PlatformIO grouped test organization
**Performance Goals**: ReactESP event loop latency <10ms, calculation cycles <200ms, no blocking I/O
**Constraints**: Static allocation only, <8KB stack per task, <200 bytes RAM increase per feature, F() macros for string literals
**Scale/Scope**: 7 data structures (1 renamed, 2 enhanced, 4 new), 10 NMEA2000 PGN handlers, 1 HAL interface, ~30 test scenarios

## Constitution Check
*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

**Hardware Abstraction (Principle I)**:
- [x] All hardware interactions use HAL interfaces (IOneWireSensors for 1-wire sensors)
- [x] Mock implementations provided for testing (MockOneWireSensors planned)
- [x] Business logic separable from hardware I/O (data structures pure C structs, no hardware dependencies)

**Resource Management (Principle II)**:
- [x] Static allocation preferred; heap usage justified (all structures statically allocated, no dynamic memory)
- [x] Stack usage estimated and within 8KB per task (negligible stack impact, data structures 560 bytes total)
- [x] Flash usage impact documented (estimated +4KB, ~0.2% of partition)
- [x] String literals use F() macro or PROGMEM (WebSocket log messages will use F() macro)

**QA Review Process (Principle III - NON-NEGOTIABLE)**:
- [x] QA subagent review planned for all code changes (PR review before merge)
- [x] Hardware-dependent tests minimized (mock-first testing, 1-wire HAL abstraction)
- [x] Critical paths flagged for human review (NMEA2000 PGN handlers, validation logic)

**Modular Design (Principle IV)**:
- [x] Components have single responsibility (data structures separate from handlers, HAL separate from business logic)
- [x] Dependency injection used for hardware dependencies (IOneWireSensors interface injected into components)
- [x] Public interfaces documented (HAL interface contracts include usage examples)

**Network Debugging (Principle V)**:
- [x] WebSocket logging implemented (existing WebSocketLogger used for all sensor updates and validation warnings)
- [x] Log levels defined (DEBUG/INFO/WARN/ERROR/FATAL) (existing system)
- [x] Flash fallback for critical errors if WebSocket unavailable (existing system)

**Always-On Operation (Principle VI)**:
- [x] WiFi always-on requirement met (no changes to WiFi behavior)
- [x] No deep sleep/light sleep modes used (ReactESP event loops only)
- [x] Designed for 24/7 operation (all sensor polling non-blocking)

**Fail-Safe Operation (Principle VII)**:
- [x] Watchdog timer enabled (production) (no changes to existing watchdog)
- [x] Safe mode/recovery mode implemented (existing safe mode unchanged)
- [x] Graceful degradation for failures (availability flags, validation clamping, continue operation on sensor failure)

**Technology Stack Compliance**:
- [x] Using approved libraries (NMEA0183, NMEA2000, ReactESP, ESPAsyncWebServer, OneWire/DallasTemperature for 1-wire)
- [x] File organization follows src/ structure (hal/interfaces/, hal/implementations/, types/, components/)
- [x] Conventional commits format (will follow established pattern)

**Initial Constitution Check: PASS ✅**
**Post-Design Constitution Check: PASS ✅**

## Project Structure

### Documentation (this feature)
```
specs/008-enhanced-boatdata-following/
├── spec.md              # Feature specification (input)
├── plan.md              # This file (/plan command output)
├── research.md          # Phase 0 output (/plan command) ✅
├── data-model.md        # Phase 1 output (/plan command) ✅
├── quickstart.md        # Phase 1 output (/plan command) ✅
├── contracts/           # Phase 1 output (/plan command) ✅
│   ├── IOneWireSensors.h           # HAL interface contract
│   └── BoatDataTypes_v2.h          # Data structure contracts
└── tasks.md             # Phase 2 output (/tasks command - NOT created by /plan)
```

### Source Code (repository root)
**Poseidon2 uses ESP32/PlatformIO architecture with Hardware Abstraction Layer**

```
src/
├── main.cpp                         # Entry point and ReactESP event loops
│                                    # MODIFY: Add new event loops for 1-wire polling
├── config.h                         # Compile-time configuration
│                                    # MODIFY: Add 1-wire GPIO pin definitions
├── hal/                             # Hardware Abstraction Layer
│   ├── interfaces/                  # HAL interfaces (I[Feature].h)
│   │   ├── IOneWireSensors.h        # NEW: 1-wire sensor HAL interface
│   │   └── IBoatDataStore.h         # Existing (no changes)
│   └── implementations/             # ESP32-specific implementations
│       ├── ESP32OneWireSensors.cpp/h    # NEW: Hardware 1-wire adapter
│       └── ESP32DisplayAdapter.cpp/h    # Existing (no changes)
├── components/                      # Feature components (business logic)
│   └── NMEA2000Handlers.cpp/h       # MODIFY: Add new PGN handlers
├── utils/                           # Utility functions
│   └── WebSocketLogger.cpp/h        # Existing (use for sensor logging)
├── types/                           # Type definitions
│   └── BoatDataTypes.h              # MODIFY: Update to v2.0.0 schema
└── mocks/                           # Mock implementations for testing
    └── MockOneWireSensors.cpp/h     # NEW: Mock 1-wire adapter

test/
├── helpers/                         # Shared test utilities
│   ├── test_mocks.h                 # MODIFY: Add 1-wire mock helpers
│   ├── test_fixtures.h              # MODIFY: Add sensor data fixtures
│   └── test_utilities.h             # Existing (no changes)
├── test_boatdata_contracts/         # HAL interface contract tests (native)
│   ├── test_main.cpp                # Unity test runner
│   ├── test_ionewire.cpp            # NEW: IOneWireSensors contract tests
│   └── test_data_structures.cpp     # NEW: BoatDataTypes_v2 structure tests
├── test_boatdata_integration/       # Integration scenarios (native, mocked hardware)
│   ├── test_main.cpp                # Unity test runner
│   ├── test_gps_variation.cpp       # NEW: GPS variation field scenario
│   ├── test_compass_attitude.cpp    # NEW: Heel/pitch/heave scenario
│   ├── test_dst_sensors.cpp         # NEW: Depth/speed/temp scenario
│   ├── test_engine_telemetry.cpp    # NEW: Engine data scenario
│   └── test_battery_monitoring.cpp  # NEW: Battery/shore power scenario
├── test_boatdata_units/             # Unit tests (native, formulas/utilities)
│   ├── test_main.cpp                # Unity test runner
│   ├── test_validation.cpp          # NEW: Validation and clamping logic
│   └── test_unit_conversions.cpp    # NEW: Kelvin→Celsius conversion
└── test_boatdata_hardware/          # Hardware validation tests (ESP32 required)
    └── test_main.cpp                # Unity test runner with 1-wire bus tests
```

**Structure Decision**:
- **ESP32 embedded system** using PlatformIO grouped test organization
- **Test groups** organized by feature + type: `test_boatdata_[contracts|integration|units|hardware]/`
- **HAL pattern** required: All 1-wire access via IOneWireSensors interface
- **Mock-first testing**: All logic testable on native platform via MockOneWireSensors
- **Hardware tests minimal**: Only for 1-wire bus communication and NMEA2000 PGN reception

## Phase 0: Outline & Research

**Status**: ✅ Complete (research.md generated)

**Findings Summary**:
All technical unknowns from feature specification resolved:

1. **Pitch angle range**: ±30° (±π/6 radians) - validated against NMEA2000 PGN 127257 spec
2. **Heave range**: ±5 meters - based on typical/extreme wave conditions
3. **Engine RPM unit**: RPM (revolutions/minute), not Hz - industry standard, NMEA2000 PGN 127488/127489 native unit
4. **Data refresh rates**: Sensor-specific (100ms compass/engine, 1000ms GPS/DST, 2000ms batteries) using ReactESP event loops
5. **Unit conversions**: Minimal - only Kelvin→Celsius for NMEA2000 PGN 130316 temperature
6. **Refactoring strategy**: Incremental migration with backward compatibility typedef (SpeedData → DSTData)
7. **1-Wire sensor design**: HAL abstraction via IOneWireSensors, sequential polling with 50ms spacing, GPIO 4 bus
8. **NMEA2000 PGNs**: 10 total (2 existing enhanced, 8 new handlers) using ttlappalainen/NMEA2000 library parsers

**Memory footprint estimate**: +256 bytes RAM (~0.08%), +4KB flash (~0.2%)

**Output**: research.md with all NEEDS CLARIFICATION resolved

## Phase 1: Design & Contracts

**Status**: ✅ Complete (data-model.md, contracts/, quickstart.md generated)

### Data Model Summary

**7 Data Structures** (see data-model.md for full details):

1. **GPSData** (Enhanced): Added `variation` field (moved from CompassData)
2. **CompassData** (Enhanced): Added `rateOfTurn`, `heelAngle`, `pitchAngle`, `heave`; removed `variation`
3. **DSTData** (Renamed from SpeedData): Added `depth`, `seaTemperature`; removed `heelAngle` (moved to CompassData)
4. **EngineData** (NEW): `engineRev` (RPM), `oilTemperature` (Celsius), `alternatorVoltage` (volts)
5. **SaildriveData** (NEW): `saildriveEngaged` (boolean)
6. **BatteryData** (NEW): Dual banks A/B with `voltage`, `amperage` (signed), `stateOfCharge`, charger flags
7. **ShorePowerData** (NEW): `shorePowerOn` (boolean), `power` (watts)

### HAL Interface Contracts

**IOneWireSensors** (contracts/IOneWireSensors.h):
- `bool initialize()` - Initialize 1-wire bus and enumerate devices
- `bool readSaildriveStatus(SaildriveData&)` - Poll digital sensor
- `bool readBatteryA/B(BatteryMonitorData&)` - Poll analog battery monitors
- `bool readShorePower(ShorePowerData&)` - Poll connection status and power draw
- `bool isBusHealthy()` - Bus health check

**Validation Rules** (enforced at HAL boundary):
- Pitch: Clamp to [-π/6, π/6], warn if exceeded
- Heave: Clamp to [-5.0, 5.0] meters, warn if exceeded
- Engine RPM: Clamp to [0, 6000], warn if exceeded
- Battery voltage: Clamp to [0, 30] volts, warn if outside [10, 15] for 12V system
- All: Set `available=false` on validation failure

### Integration Test Scenarios (quickstart.md)

10 scenarios defined for acceptance testing:
1. GPS variation field migration (FR-001, FR-009)
2. Compass rate of turn (FR-005)
3. Heel/pitch/heave from PGN 127257 (FR-006-008)
4. DSTData rename and extension (FR-002, FR-010-012)
5. Engine data from PGN 127488/127489 (FR-013-016)
6. Saildrive status from 1-wire (FR-017-018)
7. Battery monitoring from 1-wire (FR-019-025)
8. Shore power monitoring from 1-wire (FR-026-028)
9. Validation and clamping (FR-033-034)
10. Memory footprint validation

### CLAUDE.md Update

**Status**: ✅ Updated via `.specify/scripts/bash/update-agent-context.sh claude`

**Output**: data-model.md, contracts/, quickstart.md, CLAUDE.md

## Phase 2: Task Planning Approach
*This section describes what the /tasks command will do - DO NOT execute during /plan*

**Task Generation Strategy**:

The /tasks command will generate tasks in TDD (Test-Driven Development) order:

1. **Contract Tests** (Phase 2a - All [P]arallel):
   - Test IOneWireSensors interface contract
   - Test BoatDataTypes_v2 structure definitions
   - Test data structure memory footprint

2. **Data Model Migration** (Phase 2b - Sequential dependencies):
   - Update BoatDataTypes.h to v2.0.0 schema
   - Add backward compatibility typedef (SpeedData → DSTData)
   - Update BoatDataStructure composite

3. **HAL Implementation** (Phase 2c - [P]arallel):
   - Implement ESP32OneWireSensors (hardware adapter)
   - Implement MockOneWireSensors (test mock)

4. **NMEA2000 PGN Handlers** (Phase 2d - [P]arallel):
   - Add PGN 127251 handler (Rate of Turn → CompassData)
   - Add PGN 127257 handler (Attitude → CompassData heel/pitch/heave)
   - Enhance PGN 129029 handler (GPS → add variation field)
   - Add PGN 128267 handler (Water Depth → DSTData)
   - Add PGN 128259 handler (Speed → DSTData)
   - Add PGN 130316 handler (Temperature → DSTData with Kelvin conversion)
   - Add PGN 127488 handler (Engine Rapid → EngineData RPM)
   - Add PGN 127489 handler (Engine Dynamic → EngineData temp/voltage)

5. **1-Wire Polling Integration** (Phase 2e - Sequential):
   - Add ReactESP event loop for saildrive polling (1000ms)
   - Add ReactESP event loop for battery polling (2000ms)
   - Add ReactESP event loop for shore power polling (2000ms)
   - Wire IOneWireSensors into main.cpp initialization

6. **Integration Tests** (Phase 2f - [P]arallel):
   - Test GPS variation scenario
   - Test compass attitude scenario
   - Test DST sensors scenario
   - Test engine telemetry scenario
   - Test battery monitoring scenario

7. **Unit Tests** (Phase 2g - [P]arallel):
   - Test validation and clamping logic
   - Test Kelvin→Celsius conversion
   - Test sign convention (battery amperage, heel/pitch angles)

8. **Hardware Tests** (Phase 2h - Sequential, ESP32 required):
   - Test 1-wire bus communication
   - Test NMEA2000 PGN reception
   - Test ReactESP event loop timing

9. **Validation** (Phase 2i - Sequential):
   - Run quickstart.md scenarios on hardware
   - Verify memory footprint
   - Performance profiling (event loop latency, no calculation overruns)

**Ordering Strategy**:
- **TDD order**: Contract tests → Mock implementation → Integration tests → Hardware implementation
- **Dependency order**: Data structures → HAL interfaces → PGN handlers → Event loops
- **Parallelization**: Mark [P] for independent files (tests, handlers, mocks)

**Estimated Output**: 40-50 numbered, ordered tasks in tasks.md

**IMPORTANT**: This phase is executed by the /tasks command, NOT by /plan

## Phase 3+: Future Implementation
*These phases are beyond the scope of the /plan command*

**Phase 3**: Task execution (/tasks command creates tasks.md)
**Phase 4**: Implementation (execute tasks.md following TDD approach)
**Phase 5**: Validation (run quickstart.md, hardware tests, performance profiling)
**Phase 6**: QA Review (QA subagent + human review for critical paths)
**Phase 7**: Merge to main (after all tests pass and QA approval)

## Complexity Tracking
*Fill ONLY if Constitution Check has violations that must be justified*

**No violations identified** - All constitutional principles satisfied without deviations.

## Progress Tracking
*This checklist is updated during execution flow*

**Phase Status**:
- [x] Phase 0: Research complete (/plan command) ✅
- [x] Phase 1: Design complete (/plan command) ✅
- [x] Phase 2: Task planning approach described (/plan command) ✅
- [ ] Phase 3: Tasks generated (/tasks command)
- [ ] Phase 4: Implementation complete
- [ ] Phase 5: Validation passed

**Gate Status**:
- [x] Initial Constitution Check: PASS ✅
- [x] Post-Design Constitution Check: PASS ✅
- [x] All NEEDS CLARIFICATION resolved ✅
- [x] Complexity deviations documented (N/A - no deviations) ✅

**Deliverables**:
- [x] research.md generated ✅
- [x] data-model.md generated ✅
- [x] contracts/ directory with HAL interfaces ✅
- [x] quickstart.md validation guide ✅
- [x] CLAUDE.md updated ✅

**Ready for /tasks command** ✅

---

## Implementation Notes

### Migration Path (SpeedData → DSTData)

**Backward Compatibility Shim** (temporary):
```cpp
// In BoatDataTypes.h v2.0.0 (during migration phase)
typedef DSTData SpeedData;  // Allows legacy code to compile

// Example legacy code that continues working:
double speed = boatData.speed.measuredBoatSpeed;  // 'speed' is alias for 'dst'
```

**Deprecation Timeline**:
- **Release 2.0.0**: Introduce DSTData, add typedef alias, deprecation warnings in docs
- **Release 2.1.0**: Remove typedef, require all code to use DSTData
- **Release 2.2.0**: Full removal of legacy references

### NMEA2000 PGN Handler Pattern

**Example Implementation** (from research.md):
```cpp
void HandleN2kPGN127257(const tN2kMsg &N2kMsg) {
    unsigned char SID;
    double yaw, pitch, roll;

    if (ParseN2kPGN127257(N2kMsg, SID, yaw, pitch, roll)) {
        // Validate pitch
        if (fabs(pitch) > M_PI/6) {  // ±30°
            remotelog(WARN, F("Pitch out of range: %.2f rad"), pitch);
            pitch = constrain(pitch, -M_PI/6, M_PI/6);
            boatData.compass.available = false;
        }

        // Store validated data
        boatData.compass.pitchAngle = pitch;
        boatData.compass.heelAngle = roll;  // Roll = heel in marine context
        boatData.compass.available = true;
        boatData.compass.lastUpdate = millis();
    } else {
        remotelog(ERROR, F("Failed to parse PGN 127257"));
        boatData.compass.available = false;
    }
}
```

### ReactESP Event Loop Pattern

**Example Integration**:
```cpp
// In main.cpp setup():
IOneWireSensors* oneWireSensors = new ESP32OneWireSensors();
oneWireSensors->initialize();

// Saildrive polling (1 Hz)
app.onRepeat(1000, [oneWireSensors]() {
    SaildriveData data;
    if (oneWireSensors->readSaildriveStatus(data)) {
        boatData.saildrive = data;
        remotelog(DEBUG, F("Saildrive: %s"), data.saildriveEngaged ? "engaged" : "disengaged");
    }
});

// Battery polling (0.5 Hz)
app.onRepeat(2000, [oneWireSensors]() {
    BatteryMonitorData battA, battB;
    if (oneWireSensors->readBatteryA(battA) && oneWireSensors->readBatteryB(battB)) {
        boatData.battery.voltageA = battA.voltage;
        boatData.battery.amperageA = battA.amperage;
        // ... copy remaining fields
        boatData.battery.available = true;
        boatData.battery.lastUpdate = millis();
        remotelog(DEBUG, F("Batt A: %.1fV, %.1fA, %.0f%%"),
                  battA.voltage, battA.amperage, battA.stateOfCharge);
    }
});
```

### Memory Budget Verification

**Compile-Time Check**:
```cpp
static_assert(sizeof(BoatDataStructure) <= 600, "BoatDataStructure exceeds 600 byte budget");
```

**Runtime Monitoring**:
```cpp
void logMemoryUsage() {
    size_t structSize = sizeof(BoatDataStructure);
    uint32_t freeHeap = ESP.getFreeHeap();
    UBaseType_t stackWatermark = uxTaskGetStackHighWaterMark(NULL);

    remotelog(INFO, F("Memory: BoatData=%dB, Heap=%dB, Stack=%dB"),
              structSize, freeHeap, stackWatermark * 4);
}
```

---

*Based on Constitution v1.2.0 - See `.specify/memory/constitution.md`*
*Feature Specification: specs/008-enhanced-boatdata-following/spec.md*
*Research Findings: specs/008-enhanced-boatdata-following/research.md*
*Data Model: specs/008-enhanced-boatdata-following/data-model.md*
