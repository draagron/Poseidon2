
# Implementation Plan: NMEA 0183 Data Handlers

**Branch**: `006-nmea-0183-handlers` | **Date**: 2025-10-11 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `/home/niels/Dev/Poseidon2/specs/006-nmea-0183-handlers/spec.md`

## Execution Flow (/plan command scope)
```
1. Load feature spec from Input path
   → If not found: ERROR "No feature spec at {path}"
2. Fill Technical Context (scan for NEEDS CLARIFICATION)
   → Detect Project Type from file system structure or context (web=frontend+backend, mobile=app+api)
   → Set Structure Decision based on project type
3. Fill the Constitution Check section based on the content of the constitution document.
4. Evaluate Constitution Check section below
   → If violations exist: Document in Complexity Tracking
   → If no justification possible: ERROR "Simplify approach first"
   → Update Progress Tracking: Initial Constitution Check
5. Execute Phase 0 → research.md
   → If NEEDS CLARIFICATION remain: ERROR "Resolve unknowns"
6. Execute Phase 1 → contracts, data-model.md, quickstart.md, agent-specific template file (e.g., `CLAUDE.md` for Claude Code, `.github/copilot-instructions.md` for GitHub Copilot, `GEMINI.md` for Gemini CLI, `QWEN.md` for Qwen Code, or `AGENTS.md` for all other agents).
7. Re-evaluate Constitution Check section
   → If new violations: Refactor design, return to Phase 1
   → Update Progress Tracking: Post-Design Constitution Check
8. Plan Phase 2 → Describe task generation approach (DO NOT create tasks.md)
9. STOP - Ready for /tasks command
```

**IMPORTANT**: The /plan command STOPS at step 7. Phases 2-4 are executed by other commands:
- Phase 2: /tasks command creates tasks.md
- Phase 3-4: Implementation execution (manual or via tools)

## Summary

Parse NMEA 0183 sentences from autopilot (AP) and VHF radio (VH) devices, convert units to BoatData format, and update the centralized BoatData repository. Support 5 sentence types: RSA (rudder angle), HDM (magnetic heading), GGA (GPS position), RMC (GPS with COG/SOG/variation), and VTG (true/magnetic COG with calculated variation). Integrate with existing BoatData multi-source prioritization system (NMEA 2000 sources with higher frequency automatically take precedence). Implement custom RSA parser (not in NMEA0183 library). Gracefully handle malformed sentences and out-of-range values with silent discard. Process each sentence within 50ms to ensure non-blocking operation.

## Technical Context
**Language/Version**: C++ (C++11 minimum, C++14 preferred) - Arduino framework for ESP32
**Primary Dependencies**: NMEA0183 library (ttlappalainen), BoatData (existing R005 feature), ReactESP event loops
**Storage**: N/A (real-time data processing only, no persistence for NMEA 0183 data)
**Testing**: Unity framework via PlatformIO native environment (mock-first), ESP32 hardware tests minimal
**Target Platform**: ESP32 (ESP32, ESP32-S2, ESP32-C3, ESP32-S3) running 24/7 in marine environment
**Project Type**: ESP32 embedded system with HAL architecture
**Performance Goals**: ≤50ms per sentence processing time, 4800 baud Serial2 reception without data loss
**Constraints**: Static memory allocation preferred, 8KB stack limit per task, no blocking operations in main loop
**Scale/Scope**: 5 sentence types (RSA, HDM, GGA, RMC, VTG), 2 talker IDs (AP, VH), ~1Hz typical update rate for NMEA 0183 sources

## Constitution Check
*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

**Hardware Abstraction (Principle I)**:
- [x] All hardware interactions use HAL interfaces - Serial2 access abstracted via ISerialPort interface
- [x] Mock implementations provided for testing - MockSerialPort for native tests
- [x] Business logic separable from hardware I/O - Sentence parsing logic separate from Serial2 reading

**Resource Management (Principle II)**:
- [x] Static allocation preferred; heap usage justified - Sentence buffers statically allocated (82 bytes per NMEA sentence max)
- [x] Stack usage estimated and within 8KB per task - Parser stack usage ~500 bytes (well within limit)
- [x] Flash usage impact documented - Estimated +15KB for parsers and handlers
- [x] String literals use F() macro or PROGMEM - All logging and constants use F() macro

**QA Review Process (Principle III - NON-NEGOTIABLE)**:
- [x] QA subagent review planned for all code changes - Standard QA review before merge
- [x] Hardware-dependent tests minimized - Only Serial2 timing tests on ESP32, all parsing logic tested on native
- [x] Critical paths flagged for human review - Custom RSA parser implementation requires human review

**Modular Design (Principle IV)**:
- [x] Components have single responsibility - SentenceParser (parsing), UnitConverter (conversions), NMEA0183Handler (coordination)
- [x] Dependency injection used for hardware dependencies - ISerialPort injected into handler
- [x] Public interfaces documented - All public methods documented with Doxygen comments

**Network Debugging (Principle V)**:
- [x] WebSocket logging implemented (ws://<device-ip>/logs) - Use existing WebSocketLogger for sentence processing events
- [x] Log levels defined (DEBUG/INFO/WARN/ERROR/FATAL) - DEBUG for valid sentences, WARN for rejected data, ERROR for unexpected failures
- [x] Flash fallback for critical errors if WebSocket unavailable - Handled by existing WebSocketLogger infrastructure

**Always-On Operation (Principle VI)**:
- [x] WiFi always-on requirement met - No WiFi state changes in this feature
- [x] No deep sleep/light sleep modes used - Real-time sentence processing requires always-on operation
- [x] Designed for 24/7 operation - ReactESP event loop pattern for non-blocking continuous operation

**Fail-Safe Operation (Principle VII)**:
- [x] Watchdog timer enabled (production) - No watchdog changes required (global watchdog already enabled)
- [x] Safe mode/recovery mode implemented - Graceful degradation: malformed sentences silently discarded, system continues
- [x] Graceful degradation for failures - Invalid checksums/out-of-range values silently discarded per FR-024, FR-025, FR-026

**Technology Stack Compliance**:
- [x] Using approved libraries (NMEA0183, NMEA2000, ReactESP, ESPAsyncWebServer, Adafruit_SSD1306) - NMEA0183 library already approved
- [x] File organization follows src/ structure (hal/, components/, utils/, mocks/) - Follows standard structure
- [x] Conventional commits format - Will use "feat(nmea0183): " prefix for commits

## Project Structure

### Documentation (this feature)
```
specs/[###-feature]/
├── plan.md              # This file (/plan command output)
├── research.md          # Phase 0 output (/plan command)
├── data-model.md        # Phase 1 output (/plan command)
├── quickstart.md        # Phase 1 output (/plan command)
├── contracts/           # Phase 1 output (/plan command)
└── tasks.md             # Phase 2 output (/tasks command - NOT created by /plan)
```

### Source Code (repository root)
**Poseidon2 uses ESP32/PlatformIO architecture with Hardware Abstraction Layer**

```
src/
├── main.cpp                         # Entry point and ReactESP event loops
├── config.h                         # Compile-time configuration
├── hal/                             # Hardware Abstraction Layer
│   ├── interfaces/                  # HAL interfaces (I[Feature].h)
│   │   └── I[Feature].h             # New interface for this feature
│   └── implementations/             # ESP32-specific implementations
│       └── ESP32[Feature].cpp/h     # Hardware implementation
├── components/                      # Feature components (business logic)
│   └── [Feature].cpp/h              # New component for this feature
├── utils/                           # Utility functions
│   └── [Utility].h                  # New utilities if needed
├── types/                           # Type definitions
│   └── [Feature]Types.h             # New types if needed
└── mocks/                           # Mock implementations for testing
    └── Mock[Feature].cpp/h          # Mock for unit tests

test/
├── helpers/                         # Shared test utilities
│   ├── test_mocks.h                 # Test mock implementations
│   ├── test_fixtures.h              # Test data fixtures
│   └── test_utilities.h             # Common test helpers
├── test_[feature]_contracts/        # HAL interface contract tests (native)
│   ├── test_main.cpp                # Unity test runner
│   └── test_i[feature].cpp          # Interface contract tests
├── test_[feature]_integration/      # Integration scenarios (native, mocked hardware)
│   ├── test_main.cpp                # Unity test runner
│   └── test_[scenario].cpp          # One file per scenario
├── test_[feature]_units/            # Unit tests (native, formulas/utilities)
│   ├── test_main.cpp                # Unity test runner
│   └── test_[component].cpp         # One file per component
└── test_[feature]_[hardware]/       # Hardware validation tests (ESP32 required)
    └── test_main.cpp                # Unity test runner with hardware tests
```

**Structure Decision**:
- **ESP32 embedded system** using PlatformIO grouped test organization
- **Test groups** organized by feature + type: `test_[feature]_[contracts|integration|units|hardware]/`
- **HAL pattern** required: All hardware via interfaces in `hal/interfaces/`, ESP32 implementations in `hal/implementations/`
- **Mock-first testing**: All logic testable on native platform via mocks in `src/mocks/`
- **Hardware tests minimal**: Only for HAL validation and timing-critical operations on ESP32

## Phase 0: Outline & Research
1. **Extract unknowns from Technical Context** above:
   - For each NEEDS CLARIFICATION → research task
   - For each dependency → best practices task
   - For each integration → patterns task

2. **Generate and dispatch research agents**:
   ```
   For each unknown in Technical Context:
     Task: "Research {unknown} for {feature context}"
   For each technology choice:
     Task: "Find best practices for {tech} in {domain}"
   ```

3. **Consolidate findings** in `research.md` using format:
   - Decision: [what was chosen]
   - Rationale: [why chosen]
   - Alternatives considered: [what else evaluated]

**Output**: research.md with all NEEDS CLARIFICATION resolved

## Phase 1: Design & Contracts
*Prerequisites: research.md complete*

1. **Extract entities from feature spec** → `data-model.md`:
   - Entity name, fields, relationships
   - Validation rules from requirements
   - State transitions if applicable

2. **Generate API contracts** from functional requirements:
   - For each user action → endpoint
   - Use standard REST/GraphQL patterns
   - Output OpenAPI/GraphQL schema to `/contracts/`

3. **Generate contract tests** from contracts:
   - One test file per endpoint
   - Assert request/response schemas
   - Tests must fail (no implementation yet)

4. **Extract test scenarios** from user stories:
   - Each story → integration test scenario
   - Quickstart test = story validation steps

5. **Update agent file incrementally** (O(1) operation):
   - Run `.specify/scripts/bash/update-agent-context.sh claude`
     **IMPORTANT**: Execute it exactly as specified above. Do not add or remove any arguments.
   - If exists: Add only NEW tech from current plan
   - Preserve manual additions between markers
   - Update recent changes (keep last 3)
   - Keep under 150 lines for token efficiency
   - Output to repository root

**Output**: data-model.md, /contracts/*, failing tests, quickstart.md, agent-specific file

## Phase 2: Task Planning Approach
*This section describes what the /tasks command will do - DO NOT execute during /plan*

**Task Generation Strategy**:
The /tasks command will generate a dependency-ordered task list using TDD (Test-Driven Development) approach:

1. **HAL Contracts First** (test infrastructure):
   - ISerialPort interface definition
   - Contract test suite for ISerialPort
   - ESP32SerialPort implementation (HAL)
   - MockSerialPort implementation (testing)

2. **Utility Foundations** (no dependencies):
   - UnitConverter utility class
   - Unit tests for conversions (degrees↔radians, DDMM↔decimal, variation calc)
   - NMEA0183Parsers utility (custom RSA parser)
   - Unit tests for RSA parser

3. **Handler Component** (depends on HAL + utilities):
   - NMEA0183Handler class structure
   - Integration test: RSA → BoatData.RudderData
   - Integration test: HDM → BoatData.CompassData
   - Integration test: GGA → BoatData.GPSData
   - Integration test: RMC → BoatData.GPSData + CompassData
   - Integration test: VTG → BoatData.GPSData + variation calculation
   - Integration test: Talker ID filtering (ignore non-AP/VH)
   - Integration test: Multi-source priority (NMEA 2000 wins)
   - Integration test: Invalid sentence handling

4. **main.cpp Integration** (depends on handler):
   - ReactESP loop integration (10ms polling)
   - Serial2 initialization sequence
   - Handler registration and startup

5. **Hardware Validation** (requires ESP32):
   - Serial2 4800 baud timing test
   - 50ms processing budget validation
   - Buffer overflow handling test

**Ordering Strategy**:
- **TDD Order**: Contract tests → utility tests → integration tests → implementation → hardware validation
- **Dependency Order**:
  - HAL interfaces first (ISerialPort)
  - Utilities second (UnitConverter, Parsers - no dependencies)
  - Handler third (depends on HAL + utilities)
  - main.cpp integration fourth (depends on handler)
  - Hardware tests last (validates complete system)
- **Parallelization**: Mark [P] for tasks with no dependencies:
  - Contract test + HAL implementation can run parallel
  - UnitConverter tests + Parsers tests can run parallel
  - Integration test scenarios (8 tests) can run parallel after handler structure exists

**Estimated Task Count**: 35-40 tasks
- HAL layer: 5 tasks (interface, contract tests, 2 implementations)
- Utilities: 6 tasks (2 classes × 3 tasks each: implementation, unit tests, edge cases)
- Handler: 15 tasks (structure + 8 integration test scenarios + 6 sentence handlers)
- Main integration: 4 tasks (ReactESP loop, Serial2 init, handler registration, startup sequence)
- Hardware validation: 4 tasks (Serial2 timing, processing budget, buffer overflow, full system test)
- Documentation: 2 tasks (code comments, quickstart validation)

**Test Coverage Goals**:
- Contract tests: 5 behavioral contracts for ISerialPort
- Unit tests: 15+ test cases for conversions and parsers
- Integration tests: 8 end-to-end scenarios (acceptance criteria from spec.md)
- Hardware tests: 3 timing/performance validations on ESP32

**IMPORTANT**: This phase is executed by the /tasks command, NOT by /plan

## Phase 3+: Future Implementation
*These phases are beyond the scope of the /plan command*

**Phase 3**: Task execution (/tasks command creates tasks.md)  
**Phase 4**: Implementation (execute tasks.md following constitutional principles)  
**Phase 5**: Validation (run tests, execute quickstart.md, performance validation)

## Complexity Tracking
*Fill ONLY if Constitution Check has violations that must be justified*

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| [e.g., 4th project] | [current need] | [why 3 projects insufficient] |
| [e.g., Repository pattern] | [specific problem] | [why direct DB access insufficient] |


## Progress Tracking
*This checklist is updated during execution flow*

**Phase Status**:
- [x] Phase 0: Research complete (/plan command) - research.md created
- [x] Phase 1: Design complete (/plan command) - data-model.md, contracts/, quickstart.md, CLAUDE.md updated
- [x] Phase 2: Task planning complete (/plan command - describe approach only) - 35-40 tasks estimated
- [x] Phase 3: Tasks generated (/tasks command) - 44 tasks created in tasks.md
- [ ] Phase 4: Implementation complete
- [ ] Phase 5: Validation passed

**Gate Status**:
- [x] Initial Constitution Check: PASS - All 7 principles compliant, no violations
- [x] Post-Design Constitution Check: PASS - Design maintains constitutional compliance
- [x] All NEEDS CLARIFICATION resolved - Technical Context has no NEEDS CLARIFICATION markers
- [x] Complexity deviations documented - No violations, Complexity Tracking table empty

---
*Based on Constitution v1.0.0 - See `.specify/memory/constitution.md`*
