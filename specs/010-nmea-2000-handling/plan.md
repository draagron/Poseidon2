# Implementation Plan: NMEA 2000 Message Handling

**Branch**: `010-nmea-2000-handling` | **Date**: 2025-10-12 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `/home/niels/Dev/Poseidon2/specs/010-nmea-2000-handling/spec.md`

## Summary

This feature extends the existing NMEA2000 handler infrastructure to add comprehensive PGN (Parameter Group Number) message handling for GPS navigation, compass/attitude data, DST (Depth/Speed/Temperature) sensors, engine monitoring, and wind data. The system will process 13 PGN types from the NMEA2000 CAN bus, validate data ranges, convert units, and update the centralized BoatData structure with automatic multi-source prioritization (NMEA2000 10 Hz sources automatically preferred over NMEA0183 1 Hz sources).

**Key Technical Approach**:
- Leverage existing BoatData v2.0.0 structure (9 of 13 handlers already implemented)
- Implement 5 missing PGN handlers (GPS position/COG/SOG, compass heading, magnetic variation, wind data)
- Create N2kBoatDataHandler message router class (routes PGNs to handler functions)
- Initialize NMEA2000 CAN bus in main.cpp with address claiming protocol
- Register handlers with NMEA2000 library via RegisterN2kHandlers()
- Integrate with existing multi-source prioritization system

**Implementation Complexity**: LOW-MEDIUM
- Most infrastructure exists (BoatData structures, validation utilities, 9 handlers)
- Missing handlers follow existing patterns (~200 lines of code)
- NMEA2000 library handles CAN bus complexity (address claiming, message buffering)
- Test-driven development with mock-based testing (no hardware required for most tests)

**Estimated Effort**: 16-24 hours (8-12 hours implementation + 8-12 hours testing)

## Technical Context

**Language/Version**: C++ (C++14)
**Primary Dependencies**:
- Arduino framework for ESP32
- NMEA2000 library (ttlappalainen/NMEA2000 v4.x)
- NMEA2000_esp32 (CAN bus driver for ESP32)
- ReactESP v2.0.0 (event-driven architecture)
- ESPAsyncWebServer (WebSocket logging)

**Storage**:
- RAM: ~2.2KB static allocation (N2kBoatDataHandler, CAN buffers)
- Flash: ~12KB new code (5 handlers + router class + initialization)
- LittleFS: No persistent storage required for this feature

**Testing**:
- PlatformIO Unity framework (native environment for non-hardware tests)
- ESP32 hardware tests (optional, for CAN bus timing validation)
- Mock-based testing using NMEA2000 library's tN2kMsg constructor

**Target Platform**:
- ESP32 (espressif32 platform)
- SH-ESP32 board (CAN RX=GPIO34, CAN TX=GPIO32)
- 12V marine power supply
- NMEA2000 bus with 120Ω terminators

**Project Type**: Single embedded project (ESP32 firmware)

**Performance Goals**:
- Handler execution time <1ms per message
- System throughput: 1000 messages/second sustained (peak load)
- Latency: <2ms from CAN interrupt to BoatData update
- Memory footprint: <2.5KB RAM, <15KB flash

**Constraints**:
- Static memory allocation only (no heap, Principle II)
- Non-blocking operations (ReactESP compatible, Principle VI)
- WebSocket logging for all operations (serial ports reserved for devices, Principle V)
- Graceful degradation on errors (no crashes, Principle VII)

**Scale/Scope**:
- 13 PGN types supported (GPS, compass, DST, engine, wind)
- 5 new handler functions to implement
- 1 message router class to implement
- 4 test groups (contracts, integration, units, hardware)
- ~30 test scenarios across all test groups

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

### Principle I: Hardware Abstraction Layer (HAL)

✅ **COMPLIANT**: NMEA2000 library provides HAL via `tNMEA2000_esp32` class.
- CAN bus hardware abstraction provided by NMEA2000_esp32 library
- Handler functions operate on abstract `tN2kMsg` structures (no direct hardware access)
- Mock implementations possible via custom `tNMEA2000` subclass for testing
- Business logic (parse, validate, update) separated from hardware I/O

**Action Required**: None. Existing architecture satisfies HAL requirements.

### Principle II: Resource-Aware Development

✅ **COMPLIANT**: All memory statically allocated, within resource limits.
- N2kBoatDataHandler class: 16 bytes (2 pointers + vtable)
- tNMEA2000_esp32 instance: ~200 bytes
- CAN message buffers: ~2KB (NMEA2000 library internal)
- **Total RAM**: ~2.2KB (0.7% of ESP32 320KB RAM)
- **Total Flash**: ~12KB new code (0.6% of 1.9MB partition)
- Handler stack usage: ~150 bytes per call
- No heap allocation (malloc/new) in any handler or router code

**Action Required**: None. Memory footprint well within constitutional limits.

### Principle III: QA-First Review Process

✅ **COMPLIANT**: TDD approach with comprehensive test suite.
- Contract tests validate handler function compliance
- Integration tests validate end-to-end scenarios
- Unit tests validate logic and conversions
- Hardware tests minimal (CAN timing only, optional)
- QA subagent review required before merge

**Action Required**:
- Write tests BEFORE implementing handlers (TDD)
- Request QA subagent review after implementation complete
- Ensure all tests passing before merge to main

### Principle IV: Modular Component Design

✅ **COMPLIANT**: Handler functions as focused components.
- Each handler function has single responsibility (one PGN type)
- N2kBoatDataHandler class has single responsibility (message routing)
- RegisterN2kHandlers() function has single responsibility (handler registration)
- Dependencies injected (BoatData, WebSocketLogger passed as parameters)
- Public interfaces documented with Doxygen-style comments

**Action Required**: None. Modular design already defined in contracts.

### Principle V: Network-Based Debugging

✅ **COMPLIANT**: WebSocket logging for all operations.
- Every handler includes DEBUG-level log on successful update
- WARN-level log for out-of-range values with original and clamped values
- ERROR-level log for parse failures
- INFO-level log for handler registration
- Log format: JSON with PGN number, field names, values, and units

**Action Required**: None. WebSocket logging integrated into all handler functions.

### Principle VI: Always-On Operation

✅ **COMPLIANT**: Non-blocking, interrupt-driven architecture.
- NMEA2000 library uses interrupt-driven CAN bus reception
- Handler functions execute quickly (<1ms typical)
- No blocking delays (delay(), I/O) in any handler
- ReactESP event-driven architecture maintained
- No sleep modes used

**Action Required**: None. Non-blocking architecture by design.

### Principle VII: Fail-Safe Operation

✅ **COMPLIANT**: Graceful degradation on all errors.
- Parse failures: ERROR log, availability=false, existing data preserved
- Out-of-range values: WARN log, clamped, data still usable
- Unavailable (NA) values: DEBUG log, skip update, existing data preserved
- CAN bus failures: Failover to NMEA0183 sources (multi-source prioritization)
- System continues normal operation without crashes

**Action Required**: None. Fail-safe error handling defined in contracts.

### Principle VIII: Workflow Selection

✅ **COMPLIANT**: Feature Development workflow (/specify → /plan → /tasks → /implement).
- This is new functionality (NMEA2000 handler expansion)
- Full specification completed (spec.md)
- Implementation planning completed (this document)
- Task breakdown to follow (/tasks command)
- TDD approach with comprehensive testing

**Action Required**:
- Run `/tasks` to generate dependency-ordered task breakdown
- Run `/implement` to execute tasks following TDD approach

## Project Structure

### Documentation (this feature)

```
specs/010-nmea-2000-handling/
├── spec.md                        # Feature specification (completed)
├── plan.md                        # This file (implementation plan)
├── research.md                    # Phase 0 research (completed)
├── data-model.md                  # Phase 1 data model (completed)
├── quickstart.md                  # Phase 1 quickstart guide (completed)
├── contracts/                     # Phase 1 contracts (completed)
│   ├── HandlerFunctionContract.md           # Handler function interface
│   ├── N2kBoatDataHandlerContract.md        # Message router class interface
│   └── RegisterN2kHandlersContract.md       # Registration function interface
└── tasks.md                       # Phase 2 task breakdown (to be generated)
```

### Source Code (repository root)

```
src/
├── main.cpp                       # Application entry point
│   └── [MODIFICATION REQUIRED]    # Add NMEA2000 initialization (step 5)
│                                  # Add handler registration (step 6)
│                                  # Add source registration with BoatData
│
├── config.h                       # Compile-time configuration
│   └── [ALREADY EXISTS]           # CAN_TX_PIN=32, CAN_RX_PIN=34
│
├── types/
│   └── BoatDataTypes.h            # [COMPLETE] BoatData v2.0.0 structures
│
├── components/
│   ├── BoatData.h                 # [COMPLETE] BoatData class interface
│   ├── BoatData.cpp               # [COMPLETE] BoatData implementation
│   ├── SourcePrioritizer.h        # [COMPLETE] Multi-source prioritization
│   ├── NMEA2000Handlers.h         # [MODIFICATION REQUIRED] Add 5 handler declarations
│   └── NMEA2000Handlers.cpp       # [MODIFICATION REQUIRED] Add:
│       ├── [EXISTING]             # 9 handlers already implemented
│       ├── [NEW]                  # 5 missing handler functions
│       ├── [NEW]                  # N2kBoatDataHandler class
│       └── [MODIFICATION REQUIRED]# Update RegisterN2kHandlers()
│
├── utils/
│   ├── DataValidation.h           # [COMPLETE] Validation helpers
│   └── WebSocketLogger.h          # [COMPLETE] WebSocket logging
│
└── hal/
    └── interfaces/
        └── ISourcePrioritizer.h   # [COMPLETE] Multi-source interface

test/
├── test_nmea2000_contracts/       # [NEW] Contract compliance tests
│   ├── test_main.cpp              # Test runner
│   ├── test_handler_contracts.cpp # Handler function contract tests
│   ├── test_router_contract.cpp   # N2kBoatDataHandler contract tests
│   └── test_register_contract.cpp # RegisterN2kHandlers contract tests
│
├── test_nmea2000_integration/     # [NEW] End-to-end integration tests
│   ├── test_main.cpp              # Test runner
│   ├── test_gps_data_flow.cpp     # GPS PGNs → GPSData
│   ├── test_compass_data_flow.cpp # Compass PGNs → CompassData
│   ├── test_dst_data_flow.cpp     # DST PGNs → DSTData
│   ├── test_engine_data_flow.cpp  # Engine PGNs → EngineData
│   ├── test_wind_data_flow.cpp    # Wind PGN → WindData
│   └── test_multi_source.cpp      # NMEA2000 vs NMEA0183 priority
│
├── test_nmea2000_units/           # [NEW] Unit tests for logic/conversions
│   ├── test_main.cpp              # Test runner
│   ├── test_gps_validation.cpp    # GPS range validation
│   ├── test_wind_validation.cpp   # Wind angle/speed validation
│   ├── test_conversions.cpp       # Kelvin→Celsius, m/s→knots
│   ├── test_instance_filtering.cpp# Engine instance filtering
│   ├── test_reference_filtering.cpp# Wind reference type filtering
│   └── test_source_filtering.cpp  # Temperature source filtering
│
└── test_nmea2000_hardware/        # [NEW] Hardware validation (optional)
    ├── test_main.cpp              # Test runner
    ├── test_can_init.cpp          # CAN bus initialization
    ├── test_address_claiming.cpp  # Address claiming protocol
    ├── test_message_reception.cpp # Message reception timing
    └── test_handler_timing.cpp    # Handler execution profiling
```

**Structure Decision**: Single embedded project structure with modular components. All NMEA2000 handler code consolidated in `src/components/NMEA2000Handlers.cpp` following existing patterns. Test organization uses PlatformIO grouped tests with `test_` prefix for automatic discovery.

**Key Design Decisions**:
1. **Consolidation**: All handlers in single file (NMEA2000Handlers.cpp) for cohesion
2. **Contracts**: Explicit interface contracts in `contracts/` directory for TDD
3. **Test Groups**: Four test groups (contracts, integration, units, hardware) for comprehensive coverage
4. **Incremental**: Builds on existing infrastructure (BoatData v2.0.0, 9 handlers)

## Complexity Tracking

*No complexity violations. All constitutional principles satisfied without exceptions.*

This feature follows standard patterns:
- Static memory allocation (Principle II compliant)
- HAL abstraction via NMEA2000 library (Principle I compliant)
- Single-responsibility components (Principle IV compliant)
- Non-blocking architecture (Principle VI compliant)
- Graceful error handling (Principle VII compliant)

No complexity justification required.

## Implementation Phases

### Phase 0: Research (COMPLETED)

**Status**: ✅ COMPLETE

**Artifacts Generated**:
- `research.md`: Comprehensive analysis of existing infrastructure, NMEA2000 library, PGN specifications, gaps, risks
- Identified 5 missing handlers, NMEA2000 initialization requirements, testing strategy
- Confirmed low risk, well-defined scope, 16-24 hour effort estimate

**Key Findings**:
- 9 of 13 handlers already implemented and tested
- NMEA2000 library provides all necessary parse functions
- Multi-source prioritization already working
- Validation utilities complete
- Clear implementation pattern from existing handlers

### Phase 1: Design (COMPLETED)

**Status**: ✅ COMPLETE

**Artifacts Generated**:
- `data-model.md`: Architecture diagram, handler signatures, PGN mappings, validation rules, memory layout
- `contracts/HandlerFunctionContract.md`: Detailed handler function interface contract
- `contracts/N2kBoatDataHandlerContract.md`: Message router class interface contract
- `contracts/RegisterN2kHandlersContract.md`: Registration function interface contract
- `quickstart.md`: Implementation guide, troubleshooting, validation checklist

**Design Decisions**:
- Handler functions follow 8-step pattern: parse → check NA → validate → update → log → increment
- N2kBoatDataHandler uses switch statement for PGN routing (simple, fast, maintainable)
- RegisterN2kHandlers() uses static handler instance for lifetime management
- All handlers share same WebSocketLogger and BoatData instances (dependency injection)

### Phase 2: Task Breakdown (PENDING)

**Status**: ⏳ PENDING - Run `/tasks` command to generate

**Expected Artifacts**:
- `tasks.md`: Dependency-ordered task list with estimates, acceptance criteria, references

**Expected Task Groups**:
1. Implement 5 missing handler functions (~4 hours)
2. Implement N2kBoatDataHandler class (~2 hours)
3. Add NMEA2000 initialization in main.cpp (~2 hours)
4. Write contract tests (~4 hours)
5. Write integration tests (~4 hours)
6. Write unit tests (~2 hours)
7. Write hardware tests (~2 hours, optional)
8. Documentation updates (~1 hour)

**Dependencies**:
- Phase 1 design artifacts (complete)
- Existing BoatData v2.0.0 (complete)
- Existing NMEA2000 library integration (complete)

### Phase 3: Implementation (PENDING)

**Status**: ⏳ PENDING - Run `/implement` command to execute tasks

**Approach**: Test-Driven Development (TDD)
1. Write test for handler function
2. Run test (expect failure)
3. Implement handler function
4. Run test (expect pass)
5. Refactor if needed
6. Move to next handler

**Quality Gates**:
- All contract tests passing
- All integration tests passing
- All unit tests passing
- No compiler warnings
- QA subagent review passed
- Hardware tests passing (if applicable)

## Progress Tracking

| Phase | Status | Completion | Notes |
|-------|--------|------------|-------|
| **Phase 0: Research** | ✅ Complete | 100% | research.md generated |
| **Phase 1: Design** | ✅ Complete | 100% | data-model.md, contracts/, quickstart.md generated |
| **Phase 2: Task Breakdown** | ⏳ Pending | 0% | Run `/tasks` to generate tasks.md |
| **Phase 3: Implementation** | ⏳ Pending | 0% | Run `/implement` to execute tasks |

### Phase 1 Checklist (COMPLETED)

- [x] data-model.md generated with architecture diagram
- [x] data-model.md includes all PGN mappings and validation rules
- [x] HandlerFunctionContract.md defines handler interface
- [x] N2kBoatDataHandlerContract.md defines router class interface
- [x] RegisterN2kHandlersContract.md defines registration function interface
- [x] quickstart.md provides implementation guide and troubleshooting
- [x] Constitution check passed (all principles compliant)
- [x] Project structure defined with file paths and modification requirements

### Next Steps

1. **Run `/tasks`** command to generate dependency-ordered task breakdown
2. **Review tasks.md** to understand implementation sequence
3. **Run `/implement`** command to execute tasks following TDD approach
4. **Validate** using quickstart.md checklist
5. **Request QA review** before merge to main branch

## Risk Mitigation

### Risk 1: NMEA2000 Library API Changes
- **Mitigation**: Pin library version in platformio.ini (`ttlappalainen/NMEA2000@^4.0.0`)
- **Fallback**: Test before upgrades, maintain version compatibility matrix

### Risk 2: Multiple Devices Same PGN Type
- **Mitigation**: Multi-source prioritization handles automatically (already implemented)
- **Validation**: Integration test verifies correct behavior

### Risk 3: CAN Bus Electrical Issues
- **Mitigation**: Graceful degradation (failover to NMEA0183), WebSocket logging for diagnostics
- **Detection**: Monitor WARN/ERROR logs, CAN bus error counters

### Risk 4: Handler Execution Time Exceeds Budget
- **Mitigation**: Profile handlers during hardware tests, no blocking operations allowed
- **Prevention**: Simple parse-validate-update pattern keeps execution time <1ms

## References

### Implementation Artifacts
- **Feature Spec**: [spec.md](./spec.md)
- **Research**: [research.md](./research.md)
- **Data Model**: [data-model.md](./data-model.md)
- **Quickstart**: [quickstart.md](./quickstart.md)
- **Contracts**: [contracts/](./contracts/)

### External Documentation
- **NMEA2000 Library API**: https://ttlappalainen.github.io/NMEA2000/pg_lib_ref.html
- **NMEA2000 GitHub**: https://github.com/ttlappalainen/NMEA2000
- **Reference Implementation**: `examples/poseidongw/src/` (working code)
- **Constitution**: `.specify/memory/constitution.md`
- **Project Guidance**: `CLAUDE.md`

### Related Features
- **R008**: User requirements for NMEA 2000 data
- **Spec 008**: Enhanced BoatData v2.0.0
- **Spec 006**: NMEA 0183 handlers (similar pattern)

---

**Plan Version**: 1.0.0
**Last Updated**: 2025-10-12
**Next Action**: Run `/tasks` to generate implementation task breakdown
