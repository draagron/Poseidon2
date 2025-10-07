# Tasks: [FEATURE NAME]

**Input**: Design documents from `/specs/[###-feature-name]/`
**Prerequisites**: plan.md (required), research.md, data-model.md, contracts/

## Execution Flow (main)
```
1. Load plan.md from feature directory
   → If not found: ERROR "No implementation plan found"
   → Extract: tech stack, libraries, structure
2. Load optional design documents:
   → data-model.md: Extract entities → model tasks
   → contracts/: Each file → contract test task
   → research.md: Extract decisions → setup tasks
3. Generate tasks by category:
   → Setup: project init, dependencies, linting
   → Tests: contract tests, integration tests
   → Core: models, services, CLI commands
   → Integration: DB, middleware, logging
   → Polish: unit tests, performance, docs
4. Apply task rules:
   → Different files = mark [P] for parallel
   → Same file = sequential (no [P])
   → Tests before implementation (TDD)
5. Number tasks sequentially (T001, T002...)
6. Generate dependency graph
7. Create parallel execution examples
8. Validate task completeness:
   → All contracts have tests?
   → All entities have models?
   → All endpoints implemented?
9. Return: SUCCESS (tasks ready for execution)
```

## Format: `[ID] [P?] Description`
- **[P]**: Can run in parallel (different files, no dependencies)
- Include exact file paths in descriptions

## Path Conventions
**Poseidon2 uses PlatformIO grouped test organization (ESP32 embedded system)**:
- **Source**: `src/hal/`, `src/components/`, `src/utils/`, `src/types/`, `src/mocks/`
- **Tests**: `test/test_[feature]_[type]/` where type is `contracts`, `integration`, `units`, or `hardware`
- **Test execution**:
  - Native tests: `pio test -e native -f test_[feature]_*`
  - Hardware tests: `pio test -e esp32dev_test -f test_[feature]_hardware`
- **Test groups**: Each test directory has `test_main.cpp` that runs all tests in that group using Unity framework

## Phase 3.1: Setup
- [ ] T001 Create HAL interface in src/hal/interfaces/I[Feature].h
- [ ] T002 [P] Create types in src/types/[Feature]Types.h (if needed)
- [ ] T003 [P] Create mock in src/mocks/Mock[Feature].cpp/h

## Phase 3.2: Tests First (TDD) ⚠️ MUST COMPLETE BEFORE 3.3
**CRITICAL: These tests MUST be written and MUST FAIL before ANY implementation**

### Contract Tests (HAL interface validation)
- [ ] T004 Create test_[feature]_contracts/ directory with test_main.cpp
- [ ] T005 [P] Contract test for I[Feature] interface in test_[feature]_contracts/test_i[feature].cpp

### Integration Tests (End-to-end scenarios with mocked hardware)
- [ ] T006 Create test_[feature]_integration/ directory with test_main.cpp
- [ ] T007 [P] Integration test scenario 1 in test_[feature]_integration/test_[scenario1].cpp
- [ ] T008 [P] Integration test scenario 2 in test_[feature]_integration/test_[scenario2].cpp

### Unit Tests (Formula/utility validation)
- [ ] T009 Create test_[feature]_units/ directory with test_main.cpp (if needed)
- [ ] T010 [P] Unit test for utilities in test_[feature]_units/test_[component].cpp (if needed)

## Phase 3.3: Core Implementation (ONLY after tests are failing)
- [ ] T011 [P] Component implementation in src/components/[Feature].cpp/h
- [ ] T012 [P] ESP32 hardware adapter in src/hal/implementations/ESP32[Feature].cpp/h
- [ ] T013 [P] Utility functions in src/utils/[Utility].h (if needed)
- [ ] T014 Integrate component into src/main.cpp (setup, event loops, callbacks)
- [ ] T015 WebSocket logging for key events (ConnectionEvent, errors)
- [ ] T016 Error handling and validation

## Phase 3.4: Integration
- [ ] T017 Register ReactESP event loops in main.cpp
- [ ] T018 Configure component dependencies via dependency injection
- [ ] T019 LittleFS integration for persistent storage (if needed)
- [ ] T020 Calibration/configuration API endpoints (if needed)

## Phase 3.5: Hardware Validation & Polish
- [ ] T021 Create test_[feature]_hardware/ directory with test_main.cpp (ESP32 required)
- [ ] T022 Hardware timing test in test_[feature]_hardware/test_main.cpp
- [ ] T023 Memory footprint validation (static allocation check)
- [ ] T024 [P] Update CLAUDE.md with feature integration guide
- [ ] T025 [P] Update README.md feature status
- [ ] T026 Constitutional compliance validation (all 7 principles)

## Dependencies
- HAL interface (T001) before all tests and implementation
- Tests (T004-T010) before implementation (T011-T016)
- Component (T011) before ESP32 adapter (T012)
- Core implementation (T011-T016) before integration (T017-T020)
- Integration complete before hardware tests (T021-T023)
- Implementation before documentation polish (T024-T026)

## Parallel Execution Examples
```bash
# Setup phase - create interface, types, and mocks in parallel:
pio test -e native -f test_[feature]_contracts &  # T005 (after T004)
# Create types and mocks while contract tests run

# Test phase - write all test groups in parallel:
# T007, T008, T010 can all be written simultaneously (different files)

# Core implementation - component and adapter in parallel:
# T011 (component) and T012 (ESP32 adapter) are independent

# Documentation - update CLAUDE.md and README.md in parallel:
# T024 and T025 are independent
```

## Test Execution Commands
```bash
# Run all tests for this feature
pio test -e native -f test_[feature]_*

# Run specific test types
pio test -e native -f test_[feature]_contracts    # Contract tests only
pio test -e native -f test_[feature]_integration  # Integration tests only
pio test -e native -f test_[feature]_units        # Unit tests only

# Run hardware tests (ESP32 required)
pio test -e esp32dev_test -f test_[feature]_hardware
```

## Notes
- [P] tasks = different files, no dependencies
- Verify tests fail before implementing
- Commit after each task
- Avoid: vague tasks, same file conflicts

## Task Generation Rules
*Applied during main() execution*

1. **From HAL Interfaces**:
   - Each interface → contract test task in test_[feature]_contracts/ [P]
   - Each interface → mock implementation in src/mocks/ [P]
   - Each interface → ESP32 hardware adapter in src/hal/implementations/

2. **From Data Model**:
   - Each entity → types file in src/types/ [P]
   - Each entity → component in src/components/ [P]

3. **From User Stories**:
   - Each scenario → integration test in test_[feature]_integration/ [P]
   - Each scenario → component method implementation

4. **From Calculations/Utilities**:
   - Each formula → unit test in test_[feature]_units/ [P]
   - Each utility → implementation in src/utils/

5. **Hardware Validation**:
   - Timing-critical features → hardware test in test_[feature]_hardware/
   - Memory-critical features → static allocation validation task

6. **Ordering** (TDD + Constitutional):
   - HAL interfaces → Mocks → Tests → Implementation → Integration → Hardware validation → Documentation
   - All tests must fail before implementation starts
   - Dependencies block parallel execution
   - Constitutional compliance validation at end

## Validation Checklist
*GATE: Checked by main() before returning*

- [ ] All contracts have corresponding tests
- [ ] All entities have model tasks
- [ ] All tests come before implementation
- [ ] Parallel tasks truly independent
- [ ] Each task specifies exact file path
- [ ] No task modifies same file as another [P] task