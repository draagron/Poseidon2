# Implementation Plan: OLED Basic Info Display

**Branch**: `005-oled-basic-info` | **Date**: 2025-10-08 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `/home/niels/Dev/Poseidon2/specs/005-oled-basic-info/spec.md`

## Execution Flow (/plan command scope)
```
1. Load feature spec from Input path
   → ✅ Loaded spec.md with 27 functional requirements
2. Fill Technical Context (scan for NEEDS CLARIFICATION)
   → ✅ 2 deferred clarifications identified (WiFi timeout UX, filesystem failure handling)
   → Detect Project Type: ESP32 embedded system
   → Set Structure Decision: HAL-based architecture
3. Fill the Constitution Check section
   → ✅ Evaluated against 7 core principles
4. Evaluate Constitution Check section
   → ✅ PASS - All constitutional requirements addressed
   → Update Progress Tracking: Initial Constitution Check
5. Execute Phase 0 → research.md
   → Generate research.md with technology decisions
6. Execute Phase 1 → contracts, data-model.md, quickstart.md, CLAUDE.md
   → Generate HAL contracts for display interfaces
   → Document data model entities
   → Create quickstart verification steps
7. Re-evaluate Constitution Check section
   → Post-design validation
   → Update Progress Tracking: Post-Design Constitution Check
8. Plan Phase 2 → Describe task generation approach (DO NOT create tasks.md)
   → Document TDD task ordering strategy
9. STOP - Ready for /tasks command
```

## Summary
Implement OLED display system for Poseidon2 marine gateway showing startup progress and runtime system status. Display shows WiFi connectivity (SSID, IP), resource metrics (free RAM, flash usage, CPU idle %), and animated "system alive" indicator. Uses 128x64 SSD1306 OLED on I2C Bus 2 with 5-second status refresh and 1-second animation update. Hardware abstraction via HAL interfaces enables mock-first testing on native platform. Graceful degradation if display hardware fails (continue all other subsystems, log via WebSocket).

## Technical Context
**Language/Version**: C++ (C++11 minimum, C++14 preferred) with Arduino framework
**Primary Dependencies**:
- Adafruit_SSD1306 (OLED display driver)
- Adafruit_GFX (graphics library for text rendering)
- ReactESP (event-driven architecture for periodic updates)
- Wire (I2C communication)
- ESP32 Core (FreeRTOS stats, memory APIs)

**Storage**: LittleFS (persistent storage - not used for display feature, but monitored for status)
**Testing**: Unity test framework, PlatformIO native environment for mocks, esp32dev_test for hardware validation
**Target Platform**: ESP32 (SH-ESP32 board), 24/7 always-on marine application
**Project Type**: ESP32 embedded system with HAL architecture
**Performance Goals**:
- 1-second update for rotating icon animation (4 character cycle)
- 5-second update for status information (RAM, flash, CPU)
- <1-second response to WiFi state changes
- I2C communication latency <50ms per display update

**Constraints**:
- Display: 128x64 pixels, monochrome
- Memory: Minimize heap allocations, use PROGMEM for strings
- Refresh timing: Must not block main loop or other subsystems
- Resource-efficient: Static allocation preferred, optimize for low overhead
- I2C Bus 2: SDA=GPIO21, SCL=GPIO22 (shared with other devices)

**Scale/Scope**:
- Single SSD1306 OLED display
- 3 subsystems to monitor (WiFi, filesystem, web server)
- 5 metrics to display (RAM, flash sketch size, flash free space, CPU idle %, rotating icon)
- 1 page (Page 1: System Status) - architecture supports future multi-page expansion
- Foundation for future button-based page navigation (not implemented in this feature)

## Constitution Check
*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

**Hardware Abstraction (Principle I)**:
- [x] All hardware interactions use HAL interfaces
  - `IDisplayAdapter` interface for OLED hardware (I2C init, rendering, clear, display)
  - `ISystemMetrics` interface for ESP32 platform APIs (getFreeHeap, getSketchSize, etc.)
  - Mock implementations: `MockDisplayAdapter`, `MockSystemMetrics`
- [x] Mock implementations provided for testing
  - All display operations testable on native platform via mocks
  - No physical ESP32 required for unit/integration tests
- [x] Business logic separable from hardware I/O
  - `DisplayManager` component orchestrates display logic (format, layout, update scheduling)
  - Hardware-specific code isolated to `ESP32DisplayAdapter`, `ESP32SystemMetrics`

**Resource Management (Principle II)**:
- [x] Static allocation preferred; heap usage justified
  - DisplayMetrics struct: static allocation (~20 bytes)
  - SubsystemStatus struct: static allocation (~100 bytes for strings)
  - Display buffer: managed by Adafruit_SSD1306 (1KB, justified for frame buffer)
  - No dynamic string allocation - use fixed char arrays with PROGMEM
- [x] Stack usage estimated and within 8KB per task
  - Display update functions: ~500 bytes estimated
  - ReactESP callbacks: minimal stack (< 200 bytes)
  - Total well within 8KB limit
- [x] Flash usage impact documented
  - Adafruit_SSD1306 + GFX: ~15KB flash
  - Display logic code: ~5KB estimated
  - PROGMEM strings: ~1KB
  - Total: ~21KB (1% of 1.9MB partition)
- [x] String literals use F() macro or PROGMEM
  - All display text stored in PROGMEM
  - F() macro for static strings passed to display library

**QA Review Process (Principle III - NON-NEGOTIABLE)**:
- [x] QA subagent review planned for all code changes
  - Memory safety validation (heap usage, stack depth)
  - Resource efficiency review (PROGMEM usage, static allocation)
  - Error handling completeness (OLED init failure, I2C errors)
- [x] Hardware-dependent tests minimized
  - Only `test_oled_hardware/` requires ESP32
  - All logic tested via mocks on native platform
- [x] Critical paths flagged for human review
  - I2C communication initialization
  - Display refresh timing integration with ReactESP
  - FreeRTOS stats access (CPU idle time)

**Modular Design (Principle IV)**:
- [x] Components have single responsibility
  - `DisplayManager`: Update scheduling, page management, layout orchestration
  - `MetricsCollector`: Gather system metrics (RAM, flash, CPU)
  - `DisplayFormatter`: Format metrics as display strings
  - `StartupProgressTracker`: Track and display subsystem initialization
- [x] Dependency injection used for hardware dependencies
  - DisplayManager receives IDisplayAdapter* and ISystemMetrics*
  - Enables mock injection for testing
- [x] Public interfaces documented
  - All HAL interfaces include doxygen-style comments
  - Usage examples in interface headers

**Network Debugging (Principle V)**:
- [x] WebSocket logging implemented (ws://<device-ip>/logs)
  - OLED init success/failure logged
  - Display update events logged (DEBUG level)
  - I2C communication errors logged (ERROR level)
- [x] Log levels defined (DEBUG/INFO/WARN/ERROR/FATAL)
  - INFO: Display initialized, page changes
  - WARN: Display update overruns, I2C retries
  - ERROR: OLED init failure, I2C persistent errors
- [x] Flash fallback for critical errors if WebSocket unavailable
  - OLED init failure stored to LittleFS if WebSocket not ready

**Always-On Operation (Principle VI)**:
- [x] WiFi always-on requirement met
  - Display shows WiFi status, does not control WiFi
  - No impact on always-on operation
- [x] No deep sleep/light sleep modes used
  - Display updates via ReactESP periodic callbacks
  - No power management modes introduced
- [x] Designed for 24/7 operation
  - Continuous display updates (5-second cycle for status, 1-second for animation)
  - No timeout or idle states

**Fail-Safe Operation (Principle VII)**:
- [x] Watchdog timer enabled (production)
  - Display updates designed to complete quickly (<50ms)
  - No blocking operations that could trigger watchdog
- [x] Safe mode/recovery mode implemented
  - OLED init failure: graceful degradation (log error, continue without display)
  - I2C errors: retry logic with fallback to WebSocket logging
- [x] Graceful degradation for failures
  - FR-026, FR-027: Continue all subsystems if OLED fails to initialize
  - Missing metrics: Display "N/A" or placeholder values

**Technology Stack Compliance**:
- [x] Using approved libraries
  - Adafruit_SSD1306 (approved OLED driver)
  - ReactESP (approved event loop)
  - Wire (I2C, Arduino core)
  - ESP32 Core APIs (FreeRTOS, memory stats)
- [x] File organization follows src/ structure
  - `src/hal/interfaces/IDisplayAdapter.h`, `ISystemMetrics.h`
  - `src/hal/implementations/ESP32DisplayAdapter.cpp/h`, `ESP32SystemMetrics.cpp/h`
  - `src/components/DisplayManager.cpp/h`, `MetricsCollector.cpp/h`, `DisplayFormatter.cpp/h`
  - `src/mocks/MockDisplayAdapter.cpp/h`, `MockSystemMetrics.cpp/h`
  - `src/types/DisplayTypes.h` (DisplayMetrics, SubsystemStatus, DisplayPage)
- [x] Conventional commits format
  - feat(oled): add display HAL interfaces
  - feat(oled): implement metrics collection
  - test(oled): add display contract tests

## Project Structure

### Documentation (this feature)
```
specs/005-oled-basic-info/
├── spec.md              # Feature specification (input)
├── plan.md              # This file (/plan command output)
├── research.md          # Phase 0 output (/plan command)
├── data-model.md        # Phase 1 output (/plan command)
├── quickstart.md        # Phase 1 output (/plan command)
├── contracts/           # Phase 1 output (/plan command)
│   ├── IDisplayAdapter.h     # Display HAL contract
│   └── ISystemMetrics.h      # System metrics HAL contract
└── tasks.md             # Phase 2 output (/tasks command - NOT created by /plan)
```

### Source Code (repository root)
**Poseidon2 uses ESP32/PlatformIO architecture with Hardware Abstraction Layer**

```
src/
├── main.cpp                         # Entry point - add display initialization
├── config.h                         # Add OLED config (I2C address, screen size, refresh rates)
├── hal/
│   ├── interfaces/
│   │   ├── IDisplayAdapter.h         # NEW: Display hardware abstraction
│   │   └── ISystemMetrics.h          # NEW: System metrics abstraction
│   └── implementations/
│       ├── ESP32DisplayAdapter.cpp/h # NEW: SSD1306 I2C implementation
│       └── ESP32SystemMetrics.cpp/h  # NEW: ESP32 platform metrics
├── components/
│   ├── DisplayManager.cpp/h          # NEW: Display orchestration & page management
│   ├── MetricsCollector.cpp/h        # NEW: Gather system metrics
│   ├── DisplayFormatter.cpp/h        # NEW: Format metrics as strings
│   └── StartupProgressTracker.cpp/h  # NEW: Track subsystem initialization
├── utils/
│   ├── DisplayLayout.h               # NEW: Text positioning helpers (128x64 layout)
│   └── StringUtils.h                 # NEW: PROGMEM string utilities
├── types/
│   └── DisplayTypes.h                # NEW: DisplayMetrics, SubsystemStatus, DisplayPage
└── mocks/
    ├── MockDisplayAdapter.cpp/h      # NEW: Mock for unit tests
    └── MockSystemMetrics.cpp/h       # NEW: Mock for unit tests

test/
├── helpers/
│   ├── test_mocks.h                  # Add display-specific test mocks
│   └── test_fixtures.h               # Add display test fixtures
├── test_oled_contracts/              # NEW: HAL interface contract tests (native)
│   ├── test_main.cpp                 # Unity test runner
│   ├── test_idisplayadapter.cpp      # IDisplayAdapter contract validation
│   └── test_isystemmetrics.cpp       # ISystemMetrics contract validation
├── test_oled_integration/            # NEW: Integration scenarios (native, mocked hardware)
│   ├── test_main.cpp                 # Unity test runner
│   ├── test_startup_sequence.cpp     # Boot → display startup progress
│   ├── test_status_updates.cpp       # Status refresh every 5 seconds
│   ├── test_wifi_state_changes.cpp   # WiFi connect/disconnect handling
│   ├── test_animation_cycle.cpp      # Rotating icon 1-second updates
│   └── test_graceful_degradation.cpp # OLED init failure handling
├── test_oled_units/                  # NEW: Unit tests (native, formulas/utilities)
│   ├── test_main.cpp                 # Unity test runner
│   ├── test_metrics_collector.cpp    # Metrics gathering logic
│   ├── test_display_formatter.cpp    # String formatting (bytes → KB, percentages)
│   ├── test_display_layout.cpp       # Text positioning calculations
│   └── test_string_utils.cpp         # PROGMEM utilities
└── test_oled_hardware/               # NEW: Hardware validation tests (ESP32 required)
    └── test_main.cpp                 # I2C communication, actual OLED rendering
```

**Structure Decision**:
- **ESP32 embedded system** using PlatformIO grouped test organization
- **Test groups** organized by feature + type: `test_oled_[contracts|integration|units|hardware]/`
- **HAL pattern** required: All hardware via interfaces in `hal/interfaces/`, ESP32 implementations in `hal/implementations/`
- **Mock-first testing**: All logic testable on native platform via mocks in `src/mocks/`
- **Hardware tests minimal**: Only for HAL validation and I2C communication on ESP32

## Phase 0: Outline & Research

**Unknowns from Technical Context** (2 deferred clarifications from spec):
1. WiFi connection timeout display behavior during startup
2. Filesystem mount failure system behavior

**Research Tasks**:
1. **Adafruit_SSD1306 API best practices**:
   - Decision: Use Adafruit_SSD1306 class with 128x64 SCREEN_HEIGHT/SCREEN_WIDTH constants
   - Rationale: Mature library, widely used, proven on ESP32, good documentation
   - Display initialization: `begin(SSD1306_SWITCHCAPVCC, 0x3C)` (0x3C = common I2C address)
   - Text rendering: `setCursor()`, `setTextSize()`, `setTextColor()`, `print()`/`println()`
   - Update pattern: Clear buffer → draw content → `display()` to push to hardware
   - Alternatives considered: U8g2 library (more features but larger flash footprint), direct I2C (too low-level)

2. **FreeRTOS CPU idle time statistics**:
   - Decision: Use `esp_get_free_heap_size()` for RAM, `ESP.getSketchSize()`/`ESP.getFreeSketchSpace()` for flash
   - CPU idle time: Access via `vTaskGetRunTimeStats()` or calculate from idle task runtime
   - Rationale: ESP32 FreeRTOS provides runtime stats, requires `CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS=y` in sdkconfig
   - Format: Display as percentage (0-100%)
   - Alternatives considered: Loop iteration counting (less accurate), micros() delta (overhead too high)

3. **I2C Bus 2 configuration on ESP32**:
   - Decision: Use `Wire.begin(21, 22)` for SDA=GPIO21, SCL=GPIO22
   - Rationale: I2C Bus 2 dedicated to OLED per SH-ESP32 board pinout (Bus 1 may be used for other sensors)
   - Clock speed: 400kHz (fast mode) for responsive updates
   - Alternatives considered: Software I2C (slower, more CPU overhead)

4. **ReactESP timer integration**:
   - Decision: Use `app.onRepeat(1000, updateAnimation)` for 1-second animation, `app.onRepeat(5000, updateStatus)` for 5-second status
   - Rationale: Aligns with existing WiFi/BoatData ReactESP patterns, non-blocking
   - Separate callbacks allow independent timing for animation vs status
   - Alternatives considered: FreeRTOS tasks (overkill, more complex), millis() polling (already using ReactESP)

5. **Display layout for 128x64 pixels**:
   - Decision: 6-line text layout with font size 1 (5x7 pixels per char, 21 chars/line max)
   - Line allocation:
     - Line 0: WiFi status (SSID or "Disconnected")
     - Line 1: IP address (e.g., "192.168.1.100")
     - Line 2: RAM (e.g., "RAM: 245KB")
     - Line 3: Flash (e.g., "Flash: 850KB/1920KB")
     - Line 4: CPU idle (e.g., "CPU Idle: 87%")
     - Line 5: Rotating icon in right corner (e.g., "[ | ]")
   - Rationale: Fits all required metrics, readable at font size 1, allows room for status indicators
   - Alternatives considered: Larger font (fewer lines), graphical bars (more complex, flash overhead)

6. **WiFi timeout display (deferred clarification)**:
   - Decision: Show elapsed time during connection attempt (e.g., "Connecting... 10s")
   - Rationale: Provides feedback during 30-second timeout window, user knows system is responsive
   - Update every second during connection attempt
   - Alternative: Static "Connecting..." (less informative, user may think system frozen)

7. **Filesystem mount failure (deferred clarification)**:
   - Decision: Display "FS: FAILED" on startup screen, continue operation
   - Rationale: Consistent with graceful degradation principle (FR-027), user informed of issue via display + WebSocket
   - Does not block display initialization (filesystem failure is independent of OLED hardware)
   - Alternative: Reboot on filesystem failure (too aggressive, loses visibility into other subsystems)

**Output**: research.md with all decisions documented

## Phase 1: Design & Contracts

### Data Model (data-model.md)

**DisplayMetrics**:
```cpp
struct DisplayMetrics {
    uint32_t freeRamBytes;          // Free heap memory (ESP.getFreeHeap())
    uint32_t sketchSizeBytes;       // Uploaded code size (ESP.getSketchSize())
    uint32_t freeFlashBytes;        // Free flash space (ESP.getFreeSketchSpace())
    uint8_t  cpuIdlePercent;        // CPU idle time 0-100% (FreeRTOS stats)
    uint8_t  animationState;        // Rotating icon state: 0=/, 1=-, 2=\, 3=|
    unsigned long lastUpdate;       // millis() timestamp of last update
};
```

**SubsystemStatus**:
```cpp
enum ConnectionStatus {
    CONN_CONNECTING,
    CONN_CONNECTED,
    CONN_DISCONNECTED,
    CONN_FAILED
};

enum FilesystemStatus {
    FS_MOUNTING,
    FS_MOUNTED,
    FS_FAILED
};

enum WebServerStatus {
    WS_STARTING,
    WS_RUNNING,
    WS_FAILED
};

struct SubsystemStatus {
    ConnectionStatus wifiStatus;
    char wifiSSID[33];              // Max SSID length = 32 + null terminator
    char wifiIPAddress[16];         // "255.255.255.255" + null terminator
    FilesystemStatus fsStatus;
    WebServerStatus webServerStatus;
    unsigned long wifiTimestamp;    // millis() when WiFi state last changed
    unsigned long fsTimestamp;      // millis() when FS state last changed
    unsigned long wsTimestamp;      // millis() when web server state last changed
};
```

**DisplayPage** (foundation for future multi-page support):
```cpp
typedef void (*PageRenderFunction)(IDisplayAdapter* display, const DisplayMetrics& metrics, const SubsystemStatus& status);

struct DisplayPage {
    uint8_t pageNumber;             // Page identifier (1 = system status, 2+ reserved for future)
    PageRenderFunction renderFunc;  // Function pointer to render this page
    const char* pageName;           // Short name (e.g., "Status", "NMEA", "Sensors")
};
```

**Validation Rules**:
- `freeRamBytes` must be > 0 and <= total ESP32 RAM (320KB for ESP32)
- `cpuIdlePercent` must be 0-100
- `animationState` must be 0-3
- `wifiSSID` must be null-terminated
- `wifiIPAddress` must be valid IP format or empty string

**State Transitions**:
- WiFi: DISCONNECTED → CONNECTING → CONNECTED (or FAILED after timeout)
- WiFi: CONNECTED → DISCONNECTED (connection lost) → CONNECTING (retry)
- Filesystem: MOUNTING → MOUNTED (or FAILED)
- WebServer: STARTING → RUNNING (or FAILED)
- Animation: 0 → 1 → 2 → 3 → 0 (cyclic)

### API Contracts (contracts/)

**IDisplayAdapter** (HAL interface for OLED hardware):
```cpp
class IDisplayAdapter {
public:
    virtual ~IDisplayAdapter() = default;

    // Initialize display hardware (I2C, power on, configure)
    // Returns: true on success, false on failure
    virtual bool init() = 0;

    // Clear display buffer (does not update screen until display() called)
    virtual void clear() = 0;

    // Set cursor position for text (x = column 0-20, y = row 0-7)
    virtual void setCursor(uint8_t x, uint8_t y) = 0;

    // Set text size (1 = 5x7 pixels, 2 = 10x14 pixels, etc.)
    virtual void setTextSize(uint8_t size) = 0;

    // Print string to display buffer at current cursor position
    virtual void print(const char* text) = 0;

    // Push display buffer to physical hardware (I2C transaction)
    virtual void display() = 0;

    // Check if display is initialized and ready
    virtual bool isReady() const = 0;
};
```

**ISystemMetrics** (HAL interface for ESP32 platform metrics):
```cpp
class ISystemMetrics {
public:
    virtual ~ISystemMetrics() = default;

    // Get free heap memory in bytes
    virtual uint32_t getFreeHeapBytes() = 0;

    // Get sketch (uploaded code) size in bytes
    virtual uint32_t getSketchSizeBytes() = 0;

    // Get free flash space in bytes
    virtual uint32_t getFreeFlashBytes() = 0;

    // Get CPU idle time percentage (0-100)
    // Requires FreeRTOS runtime stats enabled
    virtual uint8_t getCpuIdlePercent() = 0;

    // Get current millis() timestamp
    virtual unsigned long getMillis() = 0;
};
```

### Contract Tests (test_oled_contracts/)

**test_idisplayadapter.cpp** (validates IDisplayAdapter contract):
```cpp
// Test: init() returns true on success
// Test: init() returns false on I2C failure (mock error injection)
// Test: isReady() returns false before init(), true after successful init()
// Test: clear() does not crash when called before init()
// Test: setCursor() accepts valid coordinates (0-20, 0-7)
// Test: setTextSize() accepts valid sizes (1-4)
// Test: print() renders text at cursor position
// Test: display() pushes buffer to hardware (verify I2C transaction)
```

**test_isystemmetrics.cpp** (validates ISystemMetrics contract):
```cpp
// Test: getFreeHeapBytes() returns value > 0 and <= 320KB
// Test: getSketchSizeBytes() returns value > 0
// Test: getFreeFlashBytes() returns value >= 0
// Test: getCpuIdlePercent() returns value 0-100
// Test: getMillis() returns monotonically increasing value
```

### Integration Test Scenarios (test_oled_integration/)

1. **test_startup_sequence.cpp**: Boot → display startup progress for WiFi, FS, web server
2. **test_status_updates.cpp**: Verify status refresh every 5 seconds (RAM, flash, CPU)
3. **test_wifi_state_changes.cpp**: WiFi connect → display SSID/IP; WiFi disconnect → display "Disconnected"
4. **test_animation_cycle.cpp**: Verify rotating icon updates every 1 second (/, -, \, |)
5. **test_graceful_degradation.cpp**: OLED init failure → log error, continue without display

### Quickstart Validation (quickstart.md)

**Manual Test Steps**:
1. Flash firmware to ESP32 via `pio run -t upload`
2. Observe OLED display shows "WiFi: Connecting..."
3. Wait for WiFi connection (≤30 seconds)
4. Verify display shows SSID and IP address
5. Verify rotating icon in corner updates every 1 second (/, -, \, |)
6. Wait 5 seconds, verify RAM/flash/CPU values update
7. Disconnect WiFi AP, verify display shows "Disconnected" within 1 second
8. Reconnect WiFi, verify display shows connected status

**Automated Quickstart Test** (test_oled_integration/test_quickstart.cpp):
- Mock WiFi state changes (connecting → connected → disconnected)
- Mock system metrics updates (RAM, flash, CPU)
- Verify display renders expected text at correct positions
- Verify animation state transitions 0 → 1 → 2 → 3 → 0

### Agent Context Update

Update CLAUDE.md with OLED feature context:
- Add OLED display initialization sequence (step 3 in Hardware Initialization)
- Add display HAL interfaces to architecture diagram
- Add display refresh timing (1s animation, 5s status) to ReactESP event loops
- Add graceful degradation notes (OLED init failure handling)

**Output**: data-model.md, /contracts/*, failing tests, quickstart.md, CLAUDE.md updated

## Phase 2: Task Planning Approach
*This section describes what the /tasks command will do - DO NOT execute during /plan*

**Task Generation Strategy**:
1. Load `.specify/templates/tasks-template.md` as base structure
2. Generate tasks from Phase 1 design docs in TDD order:

**HAL Contract Tasks** (parallel execution):
- T001 [P]: Create IDisplayAdapter interface in src/hal/interfaces/
- T002 [P]: Create ISystemMetrics interface in src/hal/interfaces/
- T003 [P]: Write contract test for IDisplayAdapter (test_oled_contracts/)
- T004 [P]: Write contract test for ISystemMetrics (test_oled_contracts/)

**Data Model Tasks** (parallel execution):
- T005 [P]: Create DisplayTypes.h with DisplayMetrics, SubsystemStatus, DisplayPage structs
- T006 [P]: Create DisplayLayout.h with text positioning utilities

**Mock Implementation Tasks** (parallel execution):
- T007 [P]: Implement MockDisplayAdapter (src/mocks/)
- T008 [P]: Implement MockSystemMetrics (src/mocks/)

**Component Implementation Tasks** (sequential dependencies):
- T009: Implement MetricsCollector component (depends on ISystemMetrics)
- T010: Implement DisplayFormatter component (depends on DisplayMetrics)
- T011: Implement StartupProgressTracker component (depends on SubsystemStatus)
- T012: Implement DisplayManager component (depends on IDisplayAdapter + all components)

**Hardware Implementation Tasks** (parallel after HAL interfaces ready):
- T013 [P]: Implement ESP32DisplayAdapter (src/hal/implementations/)
- T014 [P]: Implement ESP32SystemMetrics (src/hal/implementations/)

**Integration Test Tasks** (parallel after component implementation):
- T015 [P]: Write test_startup_sequence.cpp
- T016 [P]: Write test_status_updates.cpp
- T017 [P]: Write test_wifi_state_changes.cpp
- T018 [P]: Write test_animation_cycle.cpp
- T019 [P]: Write test_graceful_degradation.cpp

**Unit Test Tasks** (parallel after components ready):
- T020 [P]: Write test_metrics_collector.cpp
- T021 [P]: Write test_display_formatter.cpp
- T022 [P]: Write test_display_layout.cpp

**Main Integration Tasks** (sequential, final steps):
- T023: Add display initialization to main.cpp setup()
- T024: Add ReactESP event loops for 1s animation, 5s status refresh
- T025: Add config.h constants (I2C address, screen dimensions, refresh intervals)

**Hardware Validation Task** (ESP32 required):
- T026: Write test_oled_hardware/test_main.cpp (I2C communication, actual rendering)

**Documentation Tasks** (parallel, can be done anytime):
- T027 [P]: Update CLAUDE.md with OLED feature
- T028 [P]: Write quickstart.md manual test steps

**Ordering Strategy**:
- TDD order: Tests before implementation (contracts first, then integration tests, then implementation to pass tests)
- Dependency order: Interfaces → Mocks → Components → Hardware implementations → Main integration
- Mark [P] for parallel execution (independent files, can be developed concurrently)

**Estimated Output**: 28 numbered, ordered tasks in tasks.md

**IMPORTANT**: This phase is executed by the /tasks command, NOT by /plan

## Phase 3+: Future Implementation
*These phases are beyond the scope of the /plan command*

**Phase 3**: Task execution (/tasks command creates tasks.md with 28 tasks)
**Phase 4**: Implementation (execute tasks.md following constitutional principles, TDD approach)
**Phase 5**: Validation
- Run all tests: `pio test -e native -f test_oled_*` (contracts, integration, units)
- Run hardware test: `pio test -e esp32dev_test -f test_oled_hardware` (requires ESP32)
- Execute quickstart.md manual validation steps
- Performance validation: Verify 1s animation timing, 5s status timing using oscilloscope or timing logs

## Complexity Tracking
*Fill ONLY if Constitution Check has violations that must be justified*

No constitutional violations identified. All principles addressed:
- Hardware abstraction via IDisplayAdapter and ISystemMetrics
- Resource-aware: static allocation, PROGMEM for strings, minimal heap usage
- QA review planned for all code
- Modular design: DisplayManager, MetricsCollector, DisplayFormatter, StartupProgressTracker
- WebSocket logging for all display events
- Always-on operation (no sleep modes)
- Graceful degradation (OLED init failure handling)

## Progress Tracking
*This checklist is updated during execution flow*

**Phase Status**:
- [x] Phase 0: Research complete (/plan command)
- [x] Phase 1: Design complete (/plan command)
- [x] Phase 2: Task planning complete (/plan command - describe approach only)
- [ ] Phase 3: Tasks generated (/tasks command)
- [ ] Phase 4: Implementation complete
- [ ] Phase 5: Validation passed

**Gate Status**:
- [x] Initial Constitution Check: PASS
- [x] Post-Design Constitution Check: PASS
- [x] All NEEDS CLARIFICATION resolved (2 deferred items addressed in research)
- [x] Complexity deviations documented (none)

---
*Based on Constitution v1.2.0 - See `.specify/memory/constitution.md`*
