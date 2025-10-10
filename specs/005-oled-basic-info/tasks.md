# Tasks: OLED Basic Info Display

**Input**: Design documents from `/home/niels/Dev/Poseidon2/specs/005-oled-basic-info/`
**Prerequisites**: plan.md ✅, research.md ✅, data-model.md ✅, contracts/ ✅, quickstart.md ✅

## Execution Flow (main)
```
1. Load plan.md from feature directory
   → ✅ Loaded tech stack: C++14, Adafruit_SSD1306, ReactESP, FreeRTOS stats
   → ✅ Structure: HAL-based architecture, PlatformIO grouped tests
2. Load optional design documents:
   → ✅ data-model.md: DisplayMetrics, SubsystemStatus, DisplayPage entities
   → ✅ contracts/: IDisplayAdapter, ISystemMetrics interfaces
   → ✅ research.md: 7 technical decisions documented
3. Generate tasks by category:
   → Setup: HAL interfaces, types, mocks (T001-T008)
   → Tests: Contract, integration, unit tests (T009-T022)
   → Core: Components, hardware adapters (T023-T026)
   → Integration: main.cpp, ReactESP, config (T027-T029)
   → Polish: Hardware tests, docs, validation (T030-T033)
4. Apply task rules:
   → Different files marked [P] for parallel execution
   → TDD order: Tests before implementation
   → Dependencies tracked (interfaces → mocks → tests → impl)
5. Number tasks sequentially (T001-T033)
6. Generate dependency graph (below)
7. Create parallel execution examples (below)
8. Validate task completeness:
   → ✅ All contracts have tests (IDisplayAdapter, ISystemMetrics)
   → ✅ All entities have type definitions (DisplayMetrics, SubsystemStatus, DisplayPage)
   → ✅ All integration scenarios covered (5 scenarios from quickstart.md)
9. Return: SUCCESS (33 tasks ready for execution)
```

## Format: `[ID] [P?] Description`
- **[P]**: Can run in parallel (different files, no dependencies)
- Include exact file paths in descriptions

## Path Conventions
**Poseidon2 uses PlatformIO grouped test organization (ESP32 embedded system)**:
- **Source**: `src/hal/`, `src/components/`, `src/utils/`, `src/types/`, `src/mocks/`
- **Tests**: `test/test_oled_[type]/` where type is `contracts`, `integration`, `units`, or `hardware`
- **Test execution**:
  - Native tests: `pio test -e native -f test_oled_*`
  - Hardware tests: `pio test -e esp32dev_test -f test_oled_hardware`
- **Test groups**: Each test directory has `test_main.cpp` that runs all tests in that group using Unity framework

---

## Phase 3.1: Setup (HAL Interfaces, Types, Mocks)

### T001 [X] [P]: Create IDisplayAdapter HAL interface
**File**: `src/hal/interfaces/IDisplayAdapter.h`
**Description**: Create Hardware Abstraction Layer interface for OLED display operations. Define pure virtual methods: `init()`, `clear()`, `setCursor(x, y)`, `setTextSize(size)`, `print(text)`, `display()`, `isReady()`. Include doxygen-style documentation with usage examples.
**Contract Reference**: `/home/niels/Dev/Poseidon2/specs/005-oled-basic-info/contracts/IDisplayAdapter.h`
**Constitutional Compliance**: Principle I (Hardware Abstraction)
**Dependencies**: None (can start immediately)

### T002 [X] [P]: Create ISystemMetrics HAL interface
**File**: `src/hal/interfaces/ISystemMetrics.h`
**Description**: Create Hardware Abstraction Layer interface for ESP32 system metrics. Define pure virtual methods: `getFreeHeapBytes()`, `getSketchSizeBytes()`, `getFreeFlashBytes()`, `getCpuIdlePercent()`, `getMillis()`. Include doxygen documentation with FreeRTOS stats notes.
**Contract Reference**: `/home/niels/Dev/Poseidon2/specs/005-oled-basic-info/contracts/ISystemMetrics.h`
**Constitutional Compliance**: Principle I (Hardware Abstraction)
**Dependencies**: None (can start immediately)

### T003 [X] [P]: Create DisplayTypes.h with data structures
**File**: `src/types/DisplayTypes.h`
**Description**: Define core data structures for OLED display system: `DisplayMetrics` struct (freeRamBytes, sketchSizeBytes, freeFlashBytes, cpuIdlePercent, animationState, lastUpdate), `SubsystemStatus` struct (wifiStatus, wifiSSID, wifiIPAddress, fsStatus, webServerStatus, timestamps), `DisplayPage` struct (pageNumber, renderFunc, pageName). Define enums: `ConnectionStatus`, `FilesystemStatus`, `WebServerStatus`. Use efficient data types (uint8_t, uint16_t, uint32_t).
**Data Model Reference**: `/home/niels/Dev/Poseidon2/specs/005-oled-basic-info/data-model.md`
**Memory Target**: ~97 bytes total static allocation
**Dependencies**: None (can start immediately)

### T004 [X] [P]: Create DisplayLayout utility header
**File**: `src/utils/DisplayLayout.h`
**Description**: Create header-only utility functions for 128x64 pixel layout calculations. Define constants: `SCREEN_WIDTH=128`, `SCREEN_HEIGHT=64`, `LINE_HEIGHT=10`, `CHAR_WIDTH=6`. Define inline functions: `getLineY(lineNum)` returns y-coordinate for line 0-5, `formatBytes(bytes, buffer)` converts bytes to KB string, `formatPercent(value, buffer)` formats percentage string. Use PROGMEM for label constants.
**Research Reference**: Display layout for 128x64 pixels (research.md section 5)
**Dependencies**: None (can start immediately)

### T005 [X] [P]: Implement MockDisplayAdapter
**Files**: `src/mocks/MockDisplayAdapter.h` and `src/mocks/MockDisplayAdapter.cpp`
**Description**: Implement mock display adapter for unit/integration tests. Track method calls (init, clear, setCursor, setTextSize, print, display) in internal state. Provide query methods: `wasCleared()`, `wasTextRendered(text)`, `getCursorPosition()`. Simulate init success/failure via `setInitResult(bool)`. No actual rendering, just state tracking.
**Interface**: Implements `IDisplayAdapter` (T001)
**Dependencies**: T001 (IDisplayAdapter interface must exist)

### T006 [X] [P]: Implement MockSystemMetrics
**Files**: `src/mocks/MockSystemMetrics.h` and `src/mocks/MockSystemMetrics.cpp`
**Description**: Implement mock system metrics for unit/integration tests. Provide setter methods: `setFreeHeapBytes(value)`, `setSketchSizeBytes(value)`, `setFreeFlashBytes(value)`, `setCpuIdlePercent(value)`, `setMillis(value)`. Return set values from getter methods. Enable test control over metric values.
**Interface**: Implements `ISystemMetrics` (T002)
**Dependencies**: T002 (ISystemMetrics interface must exist)

---

## Phase 3.2: Tests First (TDD) ⚠️ MUST COMPLETE BEFORE 3.3
**CRITICAL: These tests MUST be written and MUST FAIL before ANY implementation**

### Contract Tests (HAL Interface Validation)

### T007 [X]: Create test_oled_contracts test group
**Files**: `test/test_oled_contracts/test_main.cpp`
**Description**: Create Unity test runner for OLED contract tests. Include `<unity.h>`, declare test functions for IDisplayAdapter and ISystemMetrics, create `setUp()` and `tearDown()` functions, implement `main()` with `UNITY_BEGIN()`, `RUN_TEST()` calls for all contract tests, `UNITY_END()`.
**PlatformIO Config**: Ensure test discovery recognizes `test_oled_contracts/` directory
**Dependencies**: None (test infrastructure setup)

### T008 [X] [P]: Write IDisplayAdapter contract tests
**File**: `test/test_oled_contracts/test_idisplayadapter.cpp`
**Description**: Write contract validation tests for IDisplayAdapter interface using MockDisplayAdapter:
- `test_init_returns_true_on_success()`: Verify init() returns true on success
- `test_init_returns_false_on_failure()`: Inject I2C error, verify init() returns false
- `test_isReady_false_before_init()`: Verify isReady() returns false before init()
- `test_isReady_true_after_init()`: Verify isReady() returns true after successful init()
- `test_clear_does_not_crash_before_init()`: Call clear() before init(), verify no crash
- `test_setCursor_accepts_valid_coordinates()`: Call setCursor(0, 0) and setCursor(20, 7), verify accepted
- `test_setTextSize_accepts_valid_sizes()`: Call setTextSize(1) through setTextSize(4), verify accepted
- `test_print_renders_text()`: Call print("Hello"), verify text rendered via mock
- `test_display_pushes_buffer()`: Call display(), verify mock records display() call
**Mocks**: Uses MockDisplayAdapter (T005)
**Dependencies**: T001 (interface), T005 (mock), T007 (test runner)

### T009 [X] [P]: Write ISystemMetrics contract tests
**File**: `test/test_oled_contracts/test_isystemmetrics.cpp`
**Description**: Write contract validation tests for ISystemMetrics interface using MockSystemMetrics:
- `test_getFreeHeapBytes_returns_positive_value()`: Verify getFreeHeapBytes() > 0 and <= 320KB
- `test_getSketchSizeBytes_returns_positive_value()`: Verify getSketchSizeBytes() > 0
- `test_getFreeFlashBytes_returns_valid_value()`: Verify getFreeFlashBytes() >= 0
- `test_getCpuIdlePercent_returns_0_to_100()`: Verify getCpuIdlePercent() in range 0-100
- `test_getMillis_returns_increasing_value()`: Call getMillis() twice, verify second >= first
**Mocks**: Uses MockSystemMetrics (T006)
**Dependencies**: T002 (interface), T006 (mock), T007 (test runner)

### Integration Tests (End-to-End Scenarios with Mocked Hardware)

### T010 [X]: Create test_oled_integration test group
**Files**: `test/test_oled_integration/test_main.cpp`
**Description**: Create Unity test runner for OLED integration tests. Include `<unity.h>`, declare test functions for all 5 integration scenarios, create `setUp()` and `tearDown()` functions (initialize mocks), implement `main()` with `UNITY_BEGIN()`, `RUN_TEST()` calls for all scenario tests, `UNITY_END()`.
**Dependencies**: None (test infrastructure setup)

### T011 [X] [P]: Write startup sequence integration test
**File**: `test/test_oled_integration/test_startup_sequence.cpp`
**Description**: Test boot → display startup progress for WiFi, filesystem, web server (FR-001 to FR-006):
- Initialize DisplayManager with MockDisplayAdapter and MockSystemMetrics
- Set SubsystemStatus to CONN_CONNECTING, FS_MOUNTING, WS_STARTING
- Call DisplayManager::renderStartupProgress()
- Verify MockDisplayAdapter received calls: clear(), setCursor(0, 0), print("Poseidon2 Gateway"), print("Booting..."), print("WiFi: Connecting..."), print("Filesystem: Mounting"), print("WebServer: Starting"), display()
- Change statuses to CONN_CONNECTED, FS_MOUNTED, WS_RUNNING, render again
- Verify status indicators changed to success symbols
**Mocks**: MockDisplayAdapter (T005), MockSystemMetrics (T006)
**Components**: DisplayManager (to be implemented in T026)
**Dependencies**: T003 (types), T005 (mock display), T006 (mock metrics), T010 (test runner)

### T012 [X] [P]: Write status updates integration test
**File**: `test/test_oled_integration/test_status_updates.cpp`
**Description**: Test status refresh every 5 seconds (RAM, flash, CPU) (FR-016):
- Initialize DisplayManager with mocks
- Set initial metrics: freeRamBytes=250000, sketchSizeBytes=850000, freeFlashBytes=1000000, cpuIdlePercent=87
- Call DisplayManager::renderStatusPage()
- Verify display shows "RAM: 244KB", "Flash: 830/1920KB", "CPU Idle: 87%"
- Change metrics: freeRamBytes=240000, cpuIdlePercent=75
- Render again, verify updated values displayed
**Mocks**: MockDisplayAdapter (T005), MockSystemMetrics (T006)
**Dependencies**: T003 (types), T004 (layout utils), T005 (mock display), T006 (mock metrics), T010 (test runner)

### T013 [X] [P]: Write WiFi state changes integration test
**File**: `test/test_oled_integration/test_wifi_state_changes.cpp`
**Description**: Test WiFi connect → display SSID/IP; WiFi disconnect → display "Disconnected" (FR-007 to FR-010, FR-017):
- Initialize DisplayManager with mocks
- Set SubsystemStatus: wifiStatus=CONN_CONNECTED, wifiSSID="MyHomeNetwork", wifiIPAddress="192.168.1.100"
- Render status page
- Verify display shows "WiFi: MyHomeNetwork", "IP: 192.168.1.100"
- Change status: wifiStatus=CONN_DISCONNECTED, clear SSID and IP
- Render status page
- Verify display shows "WiFi: Disconnected", "IP: ---"
- Change status: wifiStatus=CONN_CONNECTING, elapsed time=5s
- Render status page
- Verify display shows "WiFi: Connecting...", "Attempting... 5s"
**Mocks**: MockDisplayAdapter (T005)
**Dependencies**: T003 (types), T005 (mock display), T010 (test runner)

### T014 [X] [P]: Write animation cycle integration test
**File**: `test/test_oled_integration/test_animation_cycle.cpp`
**Description**: Test rotating icon updates every 1 second (/, -, \, |) (FR-014, FR-016a):
- Initialize DisplayManager with mocks
- Set DisplayMetrics: animationState=0
- Call DisplayManager::updateAnimationIcon()
- Verify display shows "[ / ]" in corner
- Set animationState=1, render, verify "[ - ]"
- Set animationState=2, render, verify "[ \ ]"
- Set animationState=3, render, verify "[ | ]"
- Set animationState=0 again, render, verify cycles back to "[ / ]"
**Mocks**: MockDisplayAdapter (T005)
**Dependencies**: T003 (types), T005 (mock display), T010 (test runner)

### T015 [X] [P]: Write graceful degradation integration test
**File**: `test/test_oled_integration/test_graceful_degradation.cpp`
**Description**: Test OLED init failure → log error, continue without display (FR-026, FR-027):
- Initialize DisplayManager with MockDisplayAdapter configured to fail init() (return false)
- Call DisplayManager::init()
- Verify init() returned false
- Call DisplayManager::renderStatusPage()
- Verify renderStatusPage() returns immediately without crashing (no display operations if not ready)
- Verify WebSocketLogger received ERROR log: "DisplayAdapter INIT_FAILED"
- Verify system can continue operating (mock other subsystems, verify they function normally)
**Mocks**: MockDisplayAdapter (T005) with init failure, MockSystemMetrics (T006)
**Dependencies**: T003 (types), T005 (mock display), T006 (mock metrics), T010 (test runner)

### Unit Tests (Formula/Utility Validation)

### T016 [X]: Create test_oled_units test group
**Files**: `test/test_oled_units/test_main.cpp`
**Description**: Create Unity test runner for OLED unit tests. Include `<unity.h>`, declare test functions for MetricsCollector, DisplayFormatter, DisplayLayout utils, create `setUp()` and `tearDown()` functions, implement `main()` with `UNITY_BEGIN()`, `RUN_TEST()` calls for all unit tests, `UNITY_END()`.
**Dependencies**: None (test infrastructure setup)

### T017 [X] [P]: Write MetricsCollector unit tests
**File**: `test/test_oled_units/test_metrics_collector.cpp`
**Description**: Test metrics gathering logic:
- `test_collectMetrics_gathers_all_values()`: Initialize MetricsCollector with MockSystemMetrics, set mock values, call collectMetrics(), verify DisplayMetrics populated correctly
- `test_collectMetrics_updates_timestamp()`: Call collectMetrics() twice with different mock millis values, verify lastUpdate timestamp advances
- `test_collectMetrics_handles_zero_values()`: Set mock metrics to 0 (edge case), verify no crash, values stored as 0
**Mocks**: MockSystemMetrics (T006)
**Dependencies**: T002 (interface), T003 (types), T006 (mock metrics), T016 (test runner)

### T018 [X] [P]: Write DisplayFormatter unit tests
**File**: `test/test_oled_units/test_display_formatter.cpp`
**Description**: Test string formatting (bytes → KB, percentages):
- `test_formatBytes_converts_to_KB()`: formatBytes(250000) returns "244KB" (250000 / 1024 = 244)
- `test_formatBytes_handles_small_values()`: formatBytes(500) returns "0KB"
- `test_formatBytes_handles_large_values()`: formatBytes(1920000) returns "1875KB"
- `test_formatPercent_formats_correctly()`: formatPercent(87) returns "87%"
- `test_formatPercent_handles_0_and_100()`: formatPercent(0) returns "0%", formatPercent(100) returns "100%"
- `test_formatIPAddress_formats_correctly()`: formatIPAddress("192.168.1.100") returns "IP: 192.168.1.100"
- `test_formatIPAddress_handles_empty()`: formatIPAddress("") returns "IP: ---"
**Dependencies**: T003 (types), T016 (test runner)

### T019 [X] [P]: Write DisplayLayout utility tests
**File**: `test/test_oled_units/test_display_layout.cpp`
**Description**: Test text positioning calculations:
- `test_getLineY_returns_correct_positions()`: getLineY(0) returns 0, getLineY(1) returns 10, getLineY(2) returns 20, ... getLineY(5) returns 50
- `test_getLineY_handles_invalid_line()`: getLineY(6) returns safe default or asserts
- `test_PROGMEM_strings_accessible()`: Verify PROGMEM label constants can be read via F() macro
**Dependencies**: T004 (layout utils), T016 (test runner)

---

## Phase 3.3: Core Implementation (ONLY after tests are failing)

### T020 [X] [P]: Implement MetricsCollector component
**Files**: `src/components/MetricsCollector.h` and `src/components/MetricsCollector.cpp`
**Description**: Implement component to gather system metrics via ISystemMetrics interface. Constructor accepts `ISystemMetrics*` (dependency injection). Method: `collectMetrics(DisplayMetrics* metrics)` calls `getFreeHeapBytes()`, `getSketchSizeBytes()`, `getFreeFlashBytes()`, `getCpuIdlePercent()`, `getMillis()` and populates DisplayMetrics struct. Update `lastUpdate` timestamp. Handle edge cases (0 values, overflow).
**Interface**: Uses ISystemMetrics (T002)
**Tests**: Makes T017 pass
**Dependencies**: T002 (interface), T003 (types)

### T021 [X] [P]: Implement DisplayFormatter component
**Files**: `src/components/DisplayFormatter.h` and `src/components/DisplayFormatter.cpp`
**Description**: Implement component to format metrics as display strings. Static methods (no state): `formatBytes(uint32_t bytes, char* buffer)` converts bytes to KB string, `formatPercent(uint8_t percent, char* buffer)` formats percentage, `formatIPAddress(const char* ip, char* buffer)` formats IP or "---" if empty. Use `snprintf()` for safe string formatting. Avoid heap allocations (use provided buffers).
**Types**: Uses DisplayMetrics, SubsystemStatus (T003)
**Tests**: Makes T018 pass
**Dependencies**: T003 (types)

### T022 [X]: Implement StartupProgressTracker component
**Files**: `src/components/StartupProgressTracker.h` and `src/components/StartupProgressTracker.cpp`
**Description**: Implement component to track and display subsystem initialization progress. Constructor initializes SubsystemStatus with initial states (CONN_DISCONNECTED, FS_MOUNTING, WS_STARTING). Methods: `updateWiFiStatus(ConnectionStatus)`, `updateFilesystemStatus(FilesystemStatus)`, `updateWebServerStatus(WebServerStatus)`, each updates status and timestamp. Method: `getStatus()` returns const SubsystemStatus& for display rendering.
**Types**: Uses SubsystemStatus (T003)
**Tests**: Makes T011 pass (startup sequence)
**Dependencies**: T003 (types)

### T023 [X]: Implement DisplayManager component
**Files**: `src/components/DisplayManager.h` and `src/components/DisplayManager.cpp`
**Description**: Implement main display orchestration component. Constructor accepts `IDisplayAdapter*` and `ISystemMetrics*` (dependency injection). Create instances of MetricsCollector (T020), DisplayFormatter (T021), StartupProgressTracker (T022). Method: `init()` calls displayAdapter->init(), logs result via WebSocket, returns success. Method: `renderStartupProgress(const SubsystemStatus&)` renders boot screen with subsystem status. Method: `renderStatusPage()` renders runtime status (WiFi, RAM, flash, CPU, animation). Method: `updateAnimationIcon()` increments animationState, renders icon only. Handle graceful degradation if displayAdapter->isReady() is false.
**Interfaces**: Uses IDisplayAdapter (T001), ISystemMetrics (T002)
**Components**: Uses MetricsCollector (T020), DisplayFormatter (T021), StartupProgressTracker (T022)
**Tests**: Makes T011, T012, T013, T014, T015 pass
**Dependencies**: T001 (display interface), T002 (metrics interface), T003 (types), T020 (metrics collector), T021 (formatter), T022 (startup tracker)

### T024 [X] [P]: Implement ESP32DisplayAdapter hardware adapter
**Files**: `src/hal/implementations/ESP32DisplayAdapter.h` and `src/hal/implementations/ESP32DisplayAdapter.cpp`
**Description**: Implement hardware adapter for SSD1306 OLED using Adafruit_SSD1306 library. Constructor initializes `Adafruit_SSD1306 display(128, 64, &Wire, -1)`. Method: `init()` calls `Wire.begin(21, 22)`, `Wire.setClock(400000)`, `display.begin(SSD1306_SWITCHCAPVCC, 0x3C)`, logs result via WebSocket, returns success. Implement `clear()`, `setCursor()`, `setTextSize()`, `print()`, `display()`, `isReady()` by delegating to Adafruit_SSD1306 object. Store `_isReady` flag set by init().
**Interface**: Implements IDisplayAdapter (T001)
**Library**: Adafruit_SSD1306, Wire (I2C)
**Hardware**: I2C Bus 2 (SDA=GPIO21, SCL=GPIO22), I2C address 0x3C
**Dependencies**: T001 (interface)

### T025 [X] [P]: Implement ESP32SystemMetrics hardware adapter
**Files**: `src/hal/implementations/ESP32SystemMetrics.h` and `src/hal/implementations/ESP32SystemMetrics.cpp`
**Description**: Implement hardware adapter for ESP32 system metrics. Method: `getFreeHeapBytes()` returns `ESP.getFreeHeap()`. Method: `getSketchSizeBytes()` returns `ESP.getSketchSize()`. Method: `getFreeFlashBytes()` returns `ESP.getFreeSketchSpace()`. Method: `getCpuIdlePercent()` uses FreeRTOS `uxTaskGetSystemState()` to calculate idle task runtime percentage (see research.md section 2 for implementation pattern). Method: `getMillis()` returns `millis()`. Handle edge cases (division by zero in CPU idle calculation).
**Interface**: Implements ISystemMetrics (T002)
**Platform**: ESP32 FreeRTOS, ESP.h APIs
**Research**: FreeRTOS CPU idle time statistics (research.md section 2)
**Dependencies**: T002 (interface)

### T026 [X]: Add WebSocket logging for display events
**File**: `src/components/DisplayManager.cpp` (modify)
**Description**: Add WebSocket logging for key display events using existing WebSocketLogger. Log INFO level: "DisplayAdapter INIT_SUCCESS" when init() succeeds, "DisplayAdapter RENDER_STATUS_PAGE" when rendering status. Log ERROR level: "DisplayAdapter INIT_FAILED" when init() fails, "DisplayAdapter I2C_ERROR" for I2C communication errors. Use F() macro for log strings. Ensure logging does not block display operations.
**Logger**: Uses WebSocketLogger (existing utility)
**Constitutional Compliance**: Principle V (Network Debugging)
**Dependencies**: T023 (DisplayManager implementation)

---

## Phase 3.4: Integration (Main Application)

### T027 [X]: Add display initialization to main.cpp setup()
**File**: `src/main.cpp` (modify)
**Description**: Add OLED display initialization to setup() function after WiFi initialization. Create global instances: `ESP32DisplayAdapter* displayAdapter = nullptr`, `ESP32SystemMetrics* systemMetrics = nullptr`, `DisplayManager* displayManager = nullptr`. In setup(), after WiFi init (line ~295), initialize display hardware adapters: `displayAdapter = new ESP32DisplayAdapter()`, `systemMetrics = new ESP32SystemMetrics()`. Create DisplayManager: `displayManager = new DisplayManager(displayAdapter, systemMetrics)`. Call `displayManager->init()`, log result via WebSocketLogger. If init fails, log error but continue (graceful degradation, FR-027). Add `Serial.println(F("OLED display initialized"))`.
**Hardware Init Sequence**: After WiFi, before NMEA (follows constitutional sequence)
**Dependencies**: T023 (DisplayManager), T024 (ESP32DisplayAdapter), T025 (ESP32SystemMetrics)

### T028 [X]: Add ReactESP event loops for display updates
**File**: `src/main.cpp` (modify)
**Description**: Add ReactESP periodic callbacks for display updates. After existing event loops (line ~370), add: `app.onRepeat(1000, []() { if (displayManager) displayManager->updateAnimationIcon(); })` for 1-second animation update (FR-016a). Add: `app.onRepeat(5000, []() { if (displayManager) displayManager->renderStatusPage(); })` for 5-second status refresh (FR-016). Ensure callbacks check `displayManager != nullptr` before calling methods. Add comment: "Display refresh loops - 1s animation, 5s status".
**Event Loop**: Uses ReactESP (existing architecture)
**Timing**: 1 second animation, 5 seconds status (per clarifications)
**Dependencies**: T027 (display initialization in main.cpp)

### T029 [X]: Add config.h constants for OLED display
**File**: `src/config.h` (modify)
**Description**: Add OLED display configuration constants at end of file. Define: `#define OLED_I2C_ADDRESS 0x3C` (I2C address), `#define OLED_SCREEN_WIDTH 128` (pixels), `#define OLED_SCREEN_HEIGHT 64` (pixels), `#define OLED_SDA_PIN 21` (GPIO), `#define OLED_SCL_PIN 22` (GPIO), `#define OLED_I2C_CLOCK 400000` (400kHz fast mode), `#define DISPLAY_ANIMATION_INTERVAL_MS 1000` (1 second), `#define DISPLAY_STATUS_INTERVAL_MS 5000` (5 seconds). Add comment block: "// OLED Display Configuration".
**Constitutional Compliance**: Compile-time configuration (Principle II)
**Dependencies**: None (can be done in parallel with other tasks)

---

## Phase 3.5: Hardware Validation & Polish

### T030 [X]: Create test_oled_hardware test group
**Files**: `test/test_oled_hardware/test_main.cpp`
**Description**: Create Unity test runner for OLED hardware validation tests (ESP32 required). Include `<unity.h>`, include `<Wire.h>`, include `ESP32DisplayAdapter.h`, include `ESP32SystemMetrics.h`. Declare hardware test functions: `test_i2c_communication()`, `test_display_rendering()`, `test_timing_validation()`. Implement `setUp()` to initialize I2C and display adapter. Implement `tearDown()` to clean up. Implement `main()` with `UNITY_BEGIN()`, `RUN_TEST()` calls, `UNITY_END()`.
**Platform**: ESP32 hardware required (cannot run on native)
**Dependencies**: T024 (ESP32DisplayAdapter), T025 (ESP32SystemMetrics)

### T031 [X]: Write hardware timing validation test
**File**: `test/test_oled_hardware/test_main.cpp` (add test function)
**Description**: Write hardware test to validate display timing on actual ESP32:
- `test_display_timing_under_50ms()`: Initialize ESP32DisplayAdapter, call init(), measure time for renderStatusPage() using micros(), verify duration < 50ms (performance goal from plan.md). Test full display update cycle (clear, render 6 lines, display).
- `test_i2c_communication_successful()`: Initialize display, call init(), verify returns true. Call isReady(), verify true. Attempt to render text, verify no I2C errors.
- `test_cpu_idle_calculation()`: Initialize ESP32SystemMetrics, call getCpuIdlePercent(), verify returns value 0-100. Verify calculation doesn't crash or timeout.
**Hardware**: Requires ESP32 with OLED connected (SDA=GPIO21, SCL=GPIO22)
**Run Command**: `pio test -e esp32dev_test -f test_oled_hardware`
**Dependencies**: T024 (ESP32DisplayAdapter), T025 (ESP32SystemMetrics), T030 (hardware test runner)

### T032 [X]: Memory footprint validation
**File**: Create new test `test/test_oled_units/test_memory_footprint.cpp`
**Description**: Write compile-time and runtime validation of memory usage:
- `test_static_allocation_under_target()`: Verify `sizeof(DisplayMetrics)` ≈ 21 bytes, `sizeof(SubsystemStatus)` ≈ 66 bytes, `sizeof(DisplayPage)` ≈ 10 bytes. Total ≈ 97 bytes (target from data-model.md).
- `test_no_dynamic_allocation_in_formatter()`: Call DisplayFormatter methods, verify no heap allocations (use heap tracking if available).
- `test_progmem_strings_used()`: Verify F() macro usage via code inspection (manual check, document in test comments).
**Constitutional Compliance**: Principle II (Resource-Aware Development)
**Target**: ~1.1KB total RAM (97 bytes structs + 1KB framebuffer)
**Dependencies**: T003 (types), T016 (unit test runner), T021 (formatter)

### T033 [X] [P]: Update CLAUDE.md with OLED feature integration guide
**File**: `CLAUDE.md` (modify)
**Description**: Update project documentation with OLED display feature. Update "Hardware Initialization Sequence" section (line ~143): Change "3. OLED display" to "3. OLED display (I2C Bus 2, 128x64 SSD1306)" with details. Add new section under "Key Implementation Patterns": "### OLED Display Management" with subsections: "Display Initialization", "ReactESP Display Loops", "Graceful Degradation". Document HAL interfaces (IDisplayAdapter, ISystemMetrics) in architecture diagram. Add display refresh timing (1s animation, 5s status) to ReactESP event loops section. Add notes on graceful degradation (OLED init failure handling).
**Script**: Already updated via `.specify/scripts/bash/update-agent-context.sh claude` during planning
**Manual**: Add detailed integration examples and troubleshooting notes
**Dependencies**: None (documentation task, can be done anytime after T027-T029)

---

## Dependencies

**Phase 3.1 (Setup)**:
- T001, T002, T003, T004: No dependencies (all parallel)
- T005: Requires T001 (IDisplayAdapter interface)
- T006: Requires T002 (ISystemMetrics interface)

**Phase 3.2 (Tests)**:
- T007: No dependencies (test infrastructure)
- T008: Requires T001, T005, T007
- T009: Requires T002, T006, T007
- T010: No dependencies (test infrastructure)
- T011: Requires T003, T005, T006, T010
- T012: Requires T003, T004, T005, T006, T010
- T013: Requires T003, T005, T010
- T014: Requires T003, T005, T010
- T015: Requires T003, T005, T006, T010
- T016: No dependencies (test infrastructure)
- T017: Requires T002, T003, T006, T016
- T018: Requires T003, T016
- T019: Requires T004, T016

**Phase 3.3 (Implementation)**:
- T020: Requires T002, T003 (makes T017 pass)
- T021: Requires T003 (makes T018 pass)
- T022: Requires T003 (makes T011 pass)
- T023: Requires T001, T002, T003, T020, T021, T022 (makes T011-T015 pass)
- T024: Requires T001 (independent from T023)
- T025: Requires T002 (independent from T023)
- T026: Requires T023 (adds logging to DisplayManager)

**Phase 3.4 (Integration)**:
- T027: Requires T023, T024, T025
- T028: Requires T027 (display initialized first)
- T029: No dependencies (config constants, can be done anytime)

**Phase 3.5 (Hardware & Polish)**:
- T030: Requires T024, T025
- T031: Requires T024, T025, T030
- T032: Requires T003, T016, T021
- T033: No dependencies (documentation, can be done anytime)

---

## Parallel Execution Examples

### Setup Phase (T001-T006)
```bash
# Create all interfaces, types, and mocks in parallel (T001-T004 have no dependencies):
# T001: IDisplayAdapter.h
# T002: ISystemMetrics.h
# T003: DisplayTypes.h
# T004: DisplayLayout.h
# (Create these 4 files simultaneously)

# After interfaces exist, create mocks in parallel:
# T005: MockDisplayAdapter (requires T001)
# T006: MockSystemMetrics (requires T002)
```

### Test Infrastructure Phase (T007, T010, T016)
```bash
# Create all test runners in parallel (no dependencies between them):
# T007: test_oled_contracts/test_main.cpp
# T010: test_oled_integration/test_main.cpp
# T016: test_oled_units/test_main.cpp
```

### Contract Tests Phase (T008-T009)
```bash
# Write contract tests in parallel (after T007 runner exists):
# T008: test_idisplayadapter.cpp (requires T001, T005, T007)
# T009: test_isystemmetrics.cpp (requires T002, T006, T007)
```

### Integration Tests Phase (T011-T015)
```bash
# Write all integration tests in parallel (after T010 runner exists, independent test files):
# T011: test_startup_sequence.cpp
# T012: test_status_updates.cpp
# T013: test_wifi_state_changes.cpp
# T014: test_animation_cycle.cpp
# T015: test_graceful_degradation.cpp
```

### Unit Tests Phase (T017-T019)
```bash
# Write all unit tests in parallel (after T016 runner exists):
# T017: test_metrics_collector.cpp
# T018: test_display_formatter.cpp
# T019: test_display_layout.cpp
```

### Component Implementation Phase (T020-T021)
```bash
# Implement MetricsCollector and DisplayFormatter in parallel (independent components):
# T020: MetricsCollector.cpp/h
# T021: DisplayFormatter.cpp/h
# (Both only depend on types, can be developed simultaneously)
```

### Hardware Adapters Phase (T024-T025)
```bash
# Implement hardware adapters in parallel (independent implementations):
# T024: ESP32DisplayAdapter.cpp/h
# T025: ESP32SystemMetrics.cpp/h
```

### Documentation Phase (T033)
```bash
# Update documentation in parallel with other polish tasks:
# T033: CLAUDE.md (can be done while T030-T032 run)
```

---

## Test Execution Commands

### Run All Native Tests (Contracts + Integration + Units)
```bash
cd /home/niels/Dev/Poseidon2

# Run all OLED tests on native platform (no ESP32 required)
pio test -e native -f test_oled_*

# Expected output:
# - test_oled_contracts: All contract tests pass
# - test_oled_integration: All 5 integration scenarios pass
# - test_oled_units: All unit tests pass
```

### Run Specific Test Types
```bash
# Contract tests only (HAL interface validation)
pio test -e native -f test_oled_contracts

# Integration tests only (end-to-end scenarios)
pio test -e native -f test_oled_integration

# Unit tests only (formula/utility validation)
pio test -e native -f test_oled_units
```

### Run Hardware Tests (ESP32 Required)
```bash
# Hardware validation tests (requires ESP32 with OLED connected)
pio test -e esp32dev_test -f test_oled_hardware

# Expected: I2C communication works, display renders, timing < 50ms
```

### Verify All Tests Pass Before Implementation
```bash
# CRITICAL: Run this before starting T020-T026 (implementation tasks)
# All tests MUST FAIL at this point (TDD requirement)

pio test -e native -f test_oled_*

# Expected failures:
# - test_idisplayadapter.cpp: MockDisplayAdapter not fully implemented
# - test_isystemmetrics.cpp: MockSystemMetrics not fully implemented
# - test_startup_sequence.cpp: DisplayManager not implemented
# - test_status_updates.cpp: DisplayManager not implemented
# - test_wifi_state_changes.cpp: DisplayManager not implemented
# - test_animation_cycle.cpp: DisplayManager not implemented
# - test_graceful_degradation.cpp: DisplayManager not implemented
# - test_metrics_collector.cpp: MetricsCollector not implemented
# - test_display_formatter.cpp: DisplayFormatter not implemented
# - test_display_layout.cpp: DisplayLayout utils not implemented
```

---

## Notes

- **[P] tasks**: Different files, no dependencies, can be developed concurrently
- **TDD Critical**: Verify all tests fail (T008-T019, T032) before starting implementation (T020-T026)
- **Commit Strategy**: Commit after each task completion (e.g., "feat(oled): add IDisplayAdapter interface [T001]")
- **Constitutional Validation**: After T027-T029, verify compliance with all 7 principles before merge
- **Hardware Testing**: T030-T031 require physical ESP32 with OLED connected (SDA=GPIO21, SCL=GPIO22, I2C address 0x3C)
- **Memory Target**: Total RAM impact ~1.1KB (0.34% of 320KB ESP32 RAM) - validate in T032
- **Performance Target**: Display update <50ms - validate in T031

---

## Validation Checklist
*GATE: Checked before tasks.md completion*

- [x] All contracts have corresponding tests (IDisplayAdapter → T008, ISystemMetrics → T009)
- [x] All entities have type definitions (DisplayMetrics, SubsystemStatus, DisplayPage → T003)
- [x] All tests come before implementation (T007-T019 before T020-T026)
- [x] Parallel tasks truly independent (verified [P] tasks use different files)
- [x] Each task specifies exact file path (all tasks include file paths)
- [x] No task modifies same file as another [P] task (verified no conflicts)
- [x] TDD order enforced (Phase 3.2 tests MUST complete before Phase 3.3 implementation)
- [x] Constitutional compliance addressed (Principle I-VII requirements in tasks)
- [x] Total task count matches estimate (33 tasks generated vs 28 estimated - 5 additional infrastructure tasks)

---

*Tasks generated: 2025-10-08 | Total: 33 tasks | Parallel groups: 13 | Ready for execution*
