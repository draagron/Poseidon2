# Implementation Plan: WiFi Network Management Foundation

**Branch**: `001-create-feature-spec` | **Date**: 2025-10-06 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `/specs/001-create-feature-spec/spec.md`

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
6. Execute Phase 1 → contracts, data-model.md, quickstart.md, CLAUDE.md
7. Re-evaluate Constitution Check section
   → If new violations: Refactor design, return to Phase 1
   → Update Progress Tracking: Post-Design Constitution Check
8. Plan Phase 2 → Describe task generation approach (DO NOT create tasks.md)
9. STOP - Ready for /tasks command
```

**IMPORTANT**: The /plan command STOPS at step 8. Phases 2-4 are executed by other commands:
- Phase 2: /tasks command creates tasks.md
- Phase 3-4: Implementation execution (manual or via tools)

## Summary

Implement WiFi network management foundation for Poseidon Gateway ESP32 device. System must automatically connect to WiFi networks from a priority-ordered list stored in persistent flash storage (plain text format, max 3 networks). Connection attempts use 30-second timeouts with sequential fallback. When all networks fail, device reboots and retries. Connection loss triggers retry of current network only (no failover). Services run independently without blocking on WiFi status. Configuration managed via HTTP API using ESPAsyncWebServer.

**Technical Approach**: ReactESP event-driven architecture with WiFi async callbacks, LittleFS persistent storage, UDP broadcast logging, HAL abstraction for testability.

## Technical Context

**Language/Version**: C++ (C++14)
**Primary Dependencies**: WiFi.h (ESP32 Arduino Core), ReactESP v2.0+, ESPAsyncWebServer, LittleFS
**Storage**: LittleFS (flash filesystem) for WiFi configuration persistence (`/wifi.conf`)
**Testing**: PlatformIO native environment for business logic (mocked WiFi), minimal ESP32 hardware tests
**Target Platform**: ESP32 (ESP32, ESP32-S2, ESP32-C3, ESP32-S3) via PlatformIO/Arduino framework
**Project Type**: Single embedded system project
**Performance Goals**: Connection timeout 30s per network, parse time <50ms, file I/O <100ms
**Constraints**: Max 3 networks (~334 bytes RAM), non-blocking service init, 24/7 operation, no sleep modes
**Scale/Scope**: Single device, 3 WiFi credentials, plain text config file

## Constitution Check
*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

**Hardware Abstraction (Principle I)**:
- [x] All hardware interactions use HAL interfaces (WiFi via IWiFiAdapter interface)
- [x] Mock implementations provided for testing (MockWiFiAdapter for native tests)
- [x] Business logic separable from hardware I/O (connection state machine testable)

**Resource Management (Principle II)**:
- [x] Static allocation preferred; heap usage justified (WiFiConfigFile array-based, 334 bytes)
- [x] Stack usage estimated and within 8KB per task (single task, <2KB stack)
- [x] Flash usage impact documented (~200 bytes config file, ~15KB code)
- [x] String literals use F() macro or PROGMEM (log messages in PROGMEM)

**QA Review Process (Principle III - NON-NEGOTIABLE)**:
- [x] QA subagent review planned for all code changes
- [x] Hardware-dependent tests minimized (only connection validation on hardware)
- [x] Critical paths flagged for human review (reboot logic, config validation)

**Modular Design (Principle IV)**:
- [x] Components have single responsibility (WiFiManager, ConfigParser, ConnectionStateMachine)
- [x] Dependency injection used for hardware dependencies (IWiFiAdapter, IFileSystem interfaces)
- [x] Public interfaces documented (see contracts/wifi-config-api.md)

**Network Debugging (Principle V)**:
- [x] UDP broadcast logging implemented (all connection events via port 4444)
- [x] Log levels defined (DEBUG/INFO/WARN/ERROR/FATAL)
- [x] Flash fallback for critical errors (log to /error.log if UDP unavailable)

**Always-On Operation (Principle VI)**:
- [x] WiFi always-on requirement met (continuous retry, no sleep)
- [x] No deep sleep/light sleep modes used
- [x] Designed for 24/7 operation (resilient to connection loss)

**Fail-Safe Operation (Principle VII)**:
- [x] Watchdog timer enabled (production) - ESP32 default WDT, fed during connection attempts
- [x] Safe mode/recovery mode implemented (reboot loop until valid config)
- [x] Graceful degradation for failures (partial config parsing, service independence)

**Technology Stack Compliance**:
- [x] Using approved libraries (ReactESP, ESPAsyncWebServer - both pre-approved)
- [x] File organization follows src/ structure (hal/, components/, utils/, mocks/)
- [x] Conventional commits format

**✅ All constitutional requirements met. No violations.**

## Project Structure

### Documentation (this feature)
```
specs/001-create-feature-spec/
├── plan.md              # This file (/plan command output)
├── research.md          # Phase 0 output (/plan command) ✅
├── data-model.md        # Phase 1 output (/plan command) ✅
├── quickstart.md        # Phase 1 output (/plan command) ✅
├── contracts/           # Phase 1 output (/plan command) ✅
│   └── wifi-config-api.md
└── tasks.md             # Phase 2 output (/tasks command - NOT created by /plan)
```

### Source Code (repository root)
```
src/
├── main.cpp              # Entry point - WiFi manager initialization
├── config.h              # Compile-time configuration (WiFi defaults, timeouts)
├── hal/                  # Hardware Abstraction Layer
│   ├── interfaces/
│   │   ├── IWiFiAdapter.h        # WiFi hardware interface
│   │   └── IFileSystem.h         # Flash storage interface
│   └── implementations/
│       ├── ESP32WiFiAdapter.cpp  # ESP32-specific WiFi implementation
│       └── LittleFSAdapter.cpp   # LittleFS implementation
├── components/           # Feature components
│   ├── WiFiManager.cpp           # Main WiFi connection orchestrator
│   ├── ConfigParser.cpp          # Plain text config file parser
│   ├── ConnectionStateMachine.cpp # State: DISCONNECTED→CONNECTING→CONNECTED
│   └── ConfigWebServer.cpp       # HTTP upload/status endpoints
├── utils/                # Utility functions
│   ├── UDPLogger.cpp            # UDP broadcast logging
│   └── TimeoutManager.cpp       # Timeout tracking for ReactESP
└── mocks/                # Mock implementations for testing
    ├── MockWiFiAdapter.cpp      # Simulates WiFi for tests
    └── MockFileSystem.cpp       # In-memory filesystem for tests

tests/
├── unit/
│   ├── test_config_parser.cpp       # Plain text parsing logic
│   ├── test_state_machine.cpp       # Connection state transitions
│   └── test_wifi_manager_logic.cpp  # Business logic (mocked WiFi)
├── integration/
│   └── test_config_upload_flow.cpp  # Web server + parser integration
└── hardware/
    └── test_wifi_connection.cpp     # Actual ESP32 WiFi (minimal)
```

**Structure Decision**: Single embedded system project. All source in `src/` with HAL abstraction for WiFi and filesystem. Mock implementations enable native environment testing without ESP32 hardware.

## Phase 0: Outline & Research

**Status**: ✅ Complete - research.md generated

**Key Decisions Made**:
1. **Storage**: LittleFS for persistent config (robust, power-fail safe)
2. **WiFi Pattern**: ReactESP event-driven + WiFi async events
3. **Config Format**: Line-by-line state machine parser (no regex/JSON overhead)
4. **Reboot**: ESP.restart() with 5-second delay (prevents rapid loops)
5. **Status**: Shared enum + UDP broadcast + optional OLED
6. **Upload**: HTTP POST via ESPAsyncWebServer
7. **Error Handling**: Fail-safe mode with manual recovery

**Unknowns Resolved**:
- ✅ Connection timeout: 30 seconds (from clarifications)
- ✅ Max networks: 3 (from clarifications)
- ✅ File format: Plain text comma-separated (from clarifications)
- ✅ Service dependencies: Independent, non-blocking (from clarifications)
- ✅ Connection loss behavior: Retry same network (from clarifications)

**Output**: research.md with technology stack, best practices, decision rationale

## Phase 1: Design & Contracts

**Status**: ✅ Complete - data-model.md, contracts/, quickstart.md generated

### Data Model (data-model.md)
**Entities**:
1. **WiFiCredentials**: (ssid: String[1-32], password: String[0|8-63])
2. **WiFiConfigFile**: (networks: WiFiCredentials[max 3], count: int)
3. **WiFiConnectionState**: (status: Enum, currentIndex: int, attemptStart: ulong, connectedSSID: String, retryCount: int)

**State Transitions**:
```
DISCONNECTED → CONNECTING → CONNECTED
              ↓ (timeout)
           FAILED → [next network] → CONNECTING
                  ↓ (all failed)
              DISCONNECTED → [reboot]
```

**Memory Footprint**: ~334 bytes RAM total (well within ESP32 limits)

### API Contracts (contracts/wifi-config-api.md)
**HTTP Endpoints**:
1. `POST /upload-wifi-config` - Upload config file (multipart/form-data)
2. `GET /wifi-config` - Retrieve SSIDs (passwords redacted)
3. `GET /wifi-status` - Current connection state

**Internal Interfaces**:
1. `WiFiManager::loadConfig()` - Read from LittleFS
2. `WiFiManager::saveConfig()` - Write to LittleFS
3. `WiFiManager::connect()` - Initiate connection attempt
4. `WiFiManager::checkTimeout()` - Verify 30s timeout
5. `WiFiManager::handleDisconnect()` - Connection loss handler

**UDP Logging Protocol**: JSON events (CONNECTION_ATTEMPT, CONNECTION_SUCCESS, etc.)

### Contract Test Scenarios
- Valid config upload → 200, config saved, reboot scheduled
- Invalid SSID length → 400, error details
- Max networks exceeded → 400, reject
- Connection status query → accurate state
- Password redaction on config retrieval

### Quickstart (quickstart.md)
**User Stories Covered**:
1. First-time configuration (upload wifi.conf, verify connection)
2. Network failover (first unavailable, connect to second)
3. Connection loss recovery (retry same network)
4. All networks unavailable (reboot loop)
5. Invalid configuration handling (validation, rejection)
6. Services run independently (integration test)

**Performance Validation**:
- First network available: <30s
- First fail, second success: 30-60s
- All fail → reboot: ~95s
- Memory usage: <500 bytes delta

### Agent-Specific Template
**Output**: CLAUDE.md already exists at repository root (created during /init)

## Phase 2: Task Planning Approach
*This section describes what the /tasks command will do - DO NOT execute during /plan*

**Task Generation Strategy**:

From **data-model.md**:
- WiFiCredentials struct → model creation task [P]
- WiFiConfigFile struct → model creation task [P]
- WiFiConnectionState enum + struct → model creation task [P]

From **contracts/wifi-config-api.md**:
- POST /upload-wifi-config → contract test task [P]
- GET /wifi-config → contract test task [P]
- GET /wifi-status → contract test task [P]
- WiFiManager interface methods → contract test tasks [P]

From **quickstart.md**:
- User Story 1 (first-time config) → integration test [P]
- User Story 2 (failover) → integration test [P]
- User Story 3 (connection loss) → integration test [P]
- User Story 4 (all unavailable) → integration test [P]
- User Story 5 (invalid config) → integration test [P]

From **research.md** + **constitution**:
- HAL interfaces (IWiFiAdapter, IFileSystem) → interface definition tasks [P]
- Mock implementations → mock creation tasks [P]
- ReactESP integration → event loop setup task
- UDP logger → utility implementation task [P]
- LittleFS initialization → setup task

**Ordering Strategy**:
1. **Setup Phase**: Project structure, HAL interfaces, mocks (parallel)
2. **Test-First Phase**: Contract tests, integration tests (must fail initially) [P]
3. **Core Implementation**: Models, config parser, state machine [P]
4. **WiFi Integration**: WiFiManager, connection logic (sequential - depends on models)
5. **Web Server**: Upload endpoint, status endpoints [P]
6. **Logging & Observability**: UDP logger, error handling [P]
7. **Polish**: README, quickstart validation, QA review

**TDD Enforcement**:
- All contract tests written first (failing tests)
- Models implemented to pass contract tests
- Integration tests written before connection logic
- Hardware tests minimal (connection validation only)

**Parallelization** ([P] marked tasks):
- Different files = parallel execution
- Same file/component = sequential
- Tests before implementation (strict)

**Estimated Output**: ~40-50 numbered, ordered tasks in tasks.md

**IMPORTANT**: This phase is executed by the /tasks command, NOT by /plan

## Phase 3+: Future Implementation
*These phases are beyond the scope of the /plan command*

**Phase 3**: Task execution (/tasks command creates tasks.md)
**Phase 4**: Implementation (execute tasks.md following constitutional principles)
**Phase 5**: Validation (run tests, execute quickstart.md, QA review)

## Complexity Tracking
*No constitutional violations detected - this section intentionally empty*

## Progress Tracking

**Phase Status**:
- [x] Phase 0: Research complete (/plan command)
- [x] Phase 1: Design complete (/plan command)
- [x] Phase 2: Task planning approach described (/plan command)
- [ ] Phase 3: Tasks generated (/tasks command)
- [ ] Phase 4: Implementation complete
- [ ] Phase 5: Validation passed

**Gate Status**:
- [x] Initial Constitution Check: PASS (all 7 principles + tech stack compliance)
- [x] Post-Design Constitution Check: PASS (re-evaluated after Phase 1)
- [x] All NEEDS CLARIFICATION resolved (5 clarifications from session 2025-10-06)
- [x] No complexity deviations (no violations to document)

**Artifacts Generated**:
- [x] research.md (Phase 0)
- [x] data-model.md (Phase 1)
- [x] contracts/wifi-config-api.md (Phase 1)
- [x] quickstart.md (Phase 1)
- [x] plan.md (this file)

---
*Based on Constitution v1.1.0 - See `.specify/memory/constitution.md`*
