---
**⚠️ LEGACY IMPLEMENTATION NOTICE**

This specification describes the initial WiFi management implementation which used UDP broadcast logging (port 4444). The system has since been migrated to WebSocket logging for improved reliability.

**Current Implementation**: WebSocket logging via `ws://<device-ip>/logs`
**Historical Implementation** (described below): UDP broadcast logging on port 4444

This document is preserved for historical reference and architectural decision context. For current logging setup, see [README.md](../../README.md) and [CLAUDE.md](../../CLAUDE.md).

---

# Tasks: WiFi Network Management Foundation

**Input**: Design documents from `/specs/001-create-feature-spec/`
**Prerequisites**: plan.md, research.md, data-model.md, contracts/, quickstart.md

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
- **Single project**: `src/`, `tests/` at repository root
- Paths shown below use absolute paths from `/Users/nielsnorgaard/Dev/Poseidon2/`

---

## Phase 3.1: Setup & Infrastructure

- [X] **T001** Create project directory structure (src/hal/interfaces/, src/hal/implementations/, src/components/, src/utils/, src/mocks/, tests/unit/, tests/integration/, tests/hardware/)
- [X] **T002** [P] Create src/config.h with compile-time WiFi configuration constants (WIFI_TIMEOUT_MS=30000, MAX_NETWORKS=3, CONFIG_FILE_PATH="/wifi.conf", UDP_DEBUG_PORT=4444, REBOOT_DELAY_MS=5000)
- [X] **T003** [P] Add PlatformIO dependencies to platformio.ini (lib_deps: ReactESP@^2.0.0, https://github.com/ESP32Async/ESPAsyncWebServer, LittleFS)
- [X] **T004** [P] Configure PlatformIO native test environment in platformio.ini for mocked tests

---

## Phase 3.2: HAL Interfaces & Mocks (TDD Setup)

- [X] **T005** [P] Define src/hal/interfaces/IWiFiAdapter.h interface (methods: begin, status, disconnect, onEvent callbacks)
- [X] **T006** [P] Define src/hal/interfaces/IFileSystem.h interface (methods: exists, open, read, write, close)
- [X] **T007** [P] Implement src/mocks/MockWiFiAdapter.cpp with simulated connection states for unit tests
- [X] **T008** [P] Implement src/mocks/MockFileSystem.cpp with in-memory file storage for unit tests

---

## Phase 3.3: Data Models (Tests First - MUST FAIL)

- [X] **T009** [P] Write tests/unit/test_wifi_credentials.cpp to validate WiFiCredentials struct (SSID 1-32 chars, password 0 or 8-63 chars, reject invalid formats) - **MUST FAIL INITIALLY**
- [X] **T010** [P] Write tests/unit/test_wifi_config_file.cpp to validate WiFiConfigFile struct (max 3 networks, priority ordering, duplicate SSIDs allowed) - **MUST FAIL INITIALLY**
- [X] **T011** [P] Write tests/unit/test_connection_state.cpp to validate WiFiConnectionState enum and state transitions (DISCONNECTED→CONNECTING→CONNECTED→FAILED) - **MUST FAIL INITIALLY**

---

## Phase 3.4: Data Models (Implementation)

- [X] **T012** [P] Create src/components/WiFiCredentials.h struct with ssid and password fields, validation methods
- [X] **T013** [P] Create src/components/WiFiConfigFile.h struct with WiFiCredentials array[3], count field, load/save methods
- [X] **T014** [P] Create src/components/WiFiConnectionState.h enum (DISCONNECTED, CONNECTING, CONNECTED, FAILED) and state struct (status, currentNetworkIndex, attemptStartTime, connectedSSID, retryCount)

---

## Phase 3.5: Config Parser (Tests First - MUST FAIL)

- [X] **T015** [P] Write tests/unit/test_config_parser.cpp for plain text parsing logic (comma-separated format, max 3 lines, SSID/password validation, malformed input handling) - **MUST FAIL INITIALLY**

---

## Phase 3.6: Config Parser (Implementation)

- [X] **T016** Implement src/components/ConfigParser.cpp with parseLine() method (split on comma, validate SSID length 1-32, validate password length 0 or 8-63)
- [X] **T017** Implement src/components/ConfigParser.cpp parseFile() method (read max 3 lines, call parseLine() for each, populate WiFiConfigFile, log errors via UDP for invalid lines)

---

## Phase 3.7: Connection State Machine (Tests First - MUST FAIL)

- [X] **T018** [P] Write tests/unit/test_state_machine.cpp for state transitions (DISCONNECTED→CONNECTING on begin(), CONNECTING→CONNECTED on success, CONNECTING→FAILED on timeout, FAILED→CONNECTING on next network, all failed→DISCONNECTED for reboot) - **MUST FAIL INITIALLY**

---

## Phase 3.8: Connection State Machine (Implementation)

- [X] **T019** Implement src/components/ConnectionStateMachine.cpp with transition() method (validate state changes, update timestamps, increment retryCount)
- [X] **T020** Implement src/components/ConnectionStateMachine.cpp shouldRetry() method (check if timeout exceeded using millis() - attemptStartTime >= 30000ms)
- [X] **T021** Implement src/components/ConnectionStateMachine.cpp getNextNetwork() method (increment currentNetworkIndex, wrap to reboot if >= networks.count)

---

## Phase 3.9: Utilities (Logging & Timeout)

- [X] **T022** [P] Implement src/utils/UDPLogger.cpp with broadcastLog() method (format JSON with timestamp, level, component, event, data; send to UDP port 4444)
- [X] **T023** [P] Implement src/utils/UDPLogger.cpp logConnectionEvent() helper (events: CONNECTION_ATTEMPT, CONNECTION_SUCCESS, CONNECTION_FAILED, CONNECTION_LOST, CONFIG_LOADED, CONFIG_SAVED, CONFIG_INVALID, REBOOT_SCHEDULED)
- [X] **T024** [P] Implement src/utils/TimeoutManager.cpp for ReactESP timeout tracking (register callback after 30s, cancel on success)

---

## Phase 3.10: WiFi Manager Core Logic (Tests First - MUST FAIL)

- [X] **T025** [P] Write tests/unit/test_wifi_manager_logic.cpp with MockWiFiAdapter for loadConfig() method (reads /wifi.conf via IFileSystem, parses with ConfigParser, populates WiFiConfigFile) - **MUST FAIL INITIALLY**
- [X] **T026** [P] Write tests/unit/test_wifi_manager_logic.cpp for saveConfig() method (writes WiFiConfigFile to /wifi.conf in plain text format, validates before write) - **MUST FAIL INITIALLY**
- [X] **T027** [P] Write tests/unit/test_wifi_manager_logic.cpp for connect() method (calls IWiFiAdapter.begin() with credentials[currentIndex], sets state to CONNECTING, registers timeout) - **MUST FAIL INITIALLY**
- [X] **T028** [P] Write tests/unit/test_wifi_manager_logic.cpp for handleDisconnect() method (sets state to DISCONNECTED, keeps currentIndex unchanged for retry, logs event) - **MUST FAIL INITIALLY**

---

## Phase 3.11: WiFi Manager Core Logic (Implementation)

- [X] **T029** Implement src/components/WiFiManager.cpp loadConfig() method using IFileSystem interface and ConfigParser
- [X] **T030** Implement src/components/WiFiManager.cpp saveConfig() method with validation before write
- [X] **T031** Implement src/components/WiFiManager.cpp connect() method with IWiFiAdapter.begin(), state update, timeout registration via ReactESP
- [X] **T032** Implement src/components/WiFiManager.cpp checkTimeout() method (calls ConnectionStateMachine.shouldRetry(), moves to next network or schedules reboot)
- [X] **T033** Implement src/components/WiFiManager.cpp handleDisconnect() method (WiFi disconnect event callback, state update, retry same network)
- [X] **T034** Implement src/components/WiFiManager.cpp handleConnectionSuccess() method (WiFi connected event callback, state update to CONNECTED, log success)

---

## Phase 3.12: HAL Implementations (ESP32-Specific)

- [X] **T035** [P] Implement src/hal/implementations/ESP32WiFiAdapter.cpp wrapping WiFi.h (begin, status, disconnect, onEvent registration)
- [X] **T036** [P] Implement src/hal/implementations/LittleFSAdapter.cpp wrapping LittleFS.h (mount, exists, open, read, write)

---

## Phase 3.13: Web Server API (Contract Tests First - MUST FAIL)

- [X] **T037** [P] Write tests/integration/test_upload_config_endpoint.cpp for POST /upload-wifi-config (valid 3-network file → 200 + reboot scheduled, invalid SSID length → 400 + error details, max networks exceeded → 400) - **MUST FAIL INITIALLY**
- [X] **T038** [P] Write tests/integration/test_get_config_endpoint.cpp for GET /wifi-config (returns SSIDs with passwords redacted, shows priority order) - **MUST FAIL INITIALLY**
- [X] **T039** [P] Write tests/integration/test_status_endpoint.cpp for GET /wifi-status (CONNECTED state → ssid + IP + signal, CONNECTING state → current attempt + time remaining, DISCONNECTED state → retry count + reboot countdown) - **MUST FAIL INITIALLY**

---

## Phase 3.14: Web Server API (Implementation)

- [X] **T040** Implement src/components/ConfigWebServer.cpp setupRoutes() to register ESPAsyncWebServer endpoints (/upload-wifi-config, /wifi-config, /wifi-status)
- [X] **T041** Implement src/components/ConfigWebServer.cpp handleUpload() for POST /upload-wifi-config (parse multipart/form-data, validate with ConfigParser, call WiFiManager.saveConfig(), schedule reboot after 5s)
- [X] **T042** Implement src/components/ConfigWebServer.cpp handleGetConfig() for GET /wifi-config (read WiFiConfigFile, redact passwords, return JSON with SSIDs and priorities)
- [X] **T043** Implement src/components/ConfigWebServer.cpp handleGetStatus() for GET /wifi-status (read WiFiConnectionState, return JSON with status, SSID, IP for CONNECTED; current attempt for CONNECTING; reboot countdown for DISCONNECTED)

---

## Phase 3.15: Main Application Integration

- [X] **T044** Implement src/main.cpp setup() function (mount LittleFS, create ESP32WiFiAdapter and LittleFSAdapter instances, create WiFiManager with HAL dependencies, register ReactESP event loops)
- [X] **T045** Implement src/main.cpp WiFi initialization sequence (loadConfig(), attempt first network, register disconnect/connect event handlers)
- [X] **T046** Implement src/main.cpp ReactESP loop integration (periodic checkTimeout() calls every 1s, process WiFi events)
- [X] **T047** Implement src/main.cpp ConfigWebServer initialization (start server after WiFi connected, register upload/config/status endpoints)
- [X] **T048** Implement src/main.cpp reboot logic (ESP.restart() with 5-second delay, log reboot reason via UDP before restart)

---

## Phase 3.16: Integration Tests (User Stories)

- [X] **T049** [P] Write tests/integration/test_first_time_config.cpp for User Story 1 (device boots with no config → logs error → user uploads wifi.conf → device reboots → connects to first network)
- [X] **T050** [P] Write tests/integration/test_network_failover.cpp for User Story 2 (first network unavailable → timeout after 30s → connect to second network → total time 30-60s)
- [X] **T051** [P] Write tests/integration/test_connection_loss_recovery.cpp for User Story 3 (connected → disconnect event → retry same network → reconnect when available, no failover to other networks)
- [X] **T052** [P] Write tests/integration/test_all_networks_unavailable.cpp for User Story 4 (all 3 networks fail after 30s each → reboot after 95s total → repeat on next boot)
- [X] **T053** [P] Write tests/integration/test_invalid_config_handling.cpp for User Story 5 (upload config with SSID > 32 chars → 400 error + details, password < 8 chars → 400 error, existing config unchanged)
- [X] **T054** [P] Write tests/integration/test_services_run_independently.cpp for Services Integration Test (disable WiFi → verify NMEA handlers, UDP logger, OLED display start within 2s, no blocking on WiFi)

---

## Phase 3.17: Hardware Tests (Minimal - Actual ESP32)

- [X] **T055** Write tests/hardware/test_wifi_connection.cpp for actual ESP32 WiFi connection (verify real WiFi.begin() works, actual network connection, timeout behavior on ESP32 hardware)

---

## Phase 3.18: Polish & Documentation

- [ ] **T056** [P] Add F() macro to all string literals in logging code (UDPLogger messages, error strings)
- [ ] **T057** [P] Add doxygen-style comments to all public interfaces (WiFiManager, ConfigParser, HAL interfaces)
- [X] **T058** [P] Create README.md in repository root with project overview, setup instructions, WiFi configuration file format, HTTP API documentation
- [X] **T059** [P] Update CLAUDE.md with WiFi management specifics (config file location, API endpoints, state machine, troubleshooting common issues)
- [ ] **T060** Run quickstart.md validation scenarios (execute all 5 user stories manually, verify performance benchmarks met, document results)
- [ ] **T061** QA subagent review of all WiFi management code (memory safety, resource usage, error handling, Arduino best practices)

---

## Dependencies

**Must complete before starting implementation**:
- T001-T008 (Setup & HAL) must complete before all other tasks

**TDD Enforcement**:
- Tests T009-T011 before models T012-T014
- Test T015 before parser T016-T017
- Test T018 before state machine T019-T021
- Tests T025-T028 before WiFi manager T029-T034
- Tests T037-T039 before web server T040-T043
- Tests T049-T054 before declaring feature complete

**Sequential Dependencies**:
- T016 depends on T012 (ConfigParser needs WiFiCredentials struct)
- T017 depends on T016 (parseFile uses parseLine)
- T029 depends on T016-T017 (loadConfig uses ConfigParser)
- T031 depends on T014 (connect uses WiFiConnectionState)
- T040 depends on T029-T034 (Web server uses WiFiManager methods)
- T044-T048 depend on ALL previous components

**Parallel Groups** (can execute simultaneously):
- T002, T003, T004 (setup files)
- T005, T006 (HAL interfaces)
- T007, T008 (mocks)
- T009, T010, T011 (model tests)
- T012, T013, T014 (model implementations - different files)
- T022, T023, T024 (utilities - different files)
- T025, T026, T027, T028 (WiFi manager tests)
- T035, T036 (HAL implementations)
- T037, T038, T039 (web server tests)
- T049-T054 (integration tests - different scenarios)
- T056, T057, T058, T059 (polish tasks)

---

## Parallel Execution Examples

### Example 1: Setup Phase (T002-T004)
```bash
# Launch setup tasks in parallel:
# Task 1: Create config.h
# Task 2: Add PlatformIO dependencies
# Task 3: Configure native test environment
```

### Example 2: Model Tests (T009-T011)
```bash
# All model tests can run in parallel (different test files):
# Task: test_wifi_credentials.cpp
# Task: test_wifi_config_file.cpp
# Task: test_connection_state.cpp
```

### Example 3: Integration Tests (T049-T054)
```bash
# All integration tests are independent (different scenarios):
# Task: test_first_time_config.cpp
# Task: test_network_failover.cpp
# Task: test_connection_loss_recovery.cpp
# Task: test_all_networks_unavailable.cpp
# Task: test_invalid_config_handling.cpp
# Task: test_services_run_independently.cpp
```

---

## Notes

- **[P] tasks** = different files, no dependencies, can run in parallel
- Verify ALL tests fail before implementing (T009-T011, T015, T018, T025-T028, T037-T039, T049-T055)
- Commit after each task completion
- QA review (T061) is final gate before merge
- Total estimated tasks: 61
- Parallelizable tasks: ~35 marked with [P]
- Sequential tasks: ~26 (dependencies or same file modifications)

---

## Validation Checklist

- [x] All entities from data-model.md have model tasks (WiFiCredentials, WiFiConfigFile, WiFiConnectionState)
- [x] All contract endpoints have test tasks (POST /upload-wifi-config, GET /wifi-config, GET /wifi-status)
- [x] All WiFiManager methods have test tasks (loadConfig, saveConfig, connect, handleDisconnect)
- [x] All user stories from quickstart.md have integration tests (5 stories → T049-T054)
- [x] HAL interfaces defined before implementations (T005-T006 before T035-T036)
- [x] Mocks created before unit tests (T007-T008 before T009+)
- [x] Tests written before implementation code (TDD enforced)
- [x] Parallel tasks marked with [P] (35 tasks parallelizable)
- [x] File paths specified for each task
- [x] Dependencies documented
