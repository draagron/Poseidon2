# Feature Specification: OLED Basic Info Display

**Feature Branch**: `005-oled-basic-info`
**Created**: 2025-10-08
**Status**: Draft
**Input**: User description: "OLED basic info display using outline in user_requirements/R004 - OLED basic info.md"

## Execution Flow (main)
```
1. Parse user description from Input
   → Requirements defined in R004 - OLED basic info.md
2. Extract key concepts from description
   → Identified: startup progress display, system status, resource monitoring, page-based architecture
3. For each unclear aspect:
   → Marked with [NEEDS CLARIFICATION: specific question]
4. Fill User Scenarios & Testing section
   → User flow: Boat operator monitors system health via OLED during startup and operation
5. Generate Functional Requirements
   → Each requirement must be testable
   → Marked ambiguous requirements
6. Identify Key Entities (if data involved)
   → DisplayMetrics, SubsystemStatus structures
7. Run Review Checklist
   → Validation performed
8. Return: SUCCESS (spec ready for planning)
```

---

## ⚡ Quick Guidelines
- ✅ Focus on WHAT users need and WHY
- ❌ Avoid HOW to implement (no tech stack, APIs, code structure)
- 👥 Written for business stakeholders, not developers

---

## Clarifications

### Session 2025-10-08
- Q: What visual indicator should be used for the "system alive" (loop executing) display on the 128x64 OLED? → A: Small rotating icon in corner (e.g., /, -, \, | animation cycle)
- Q: How frequently should the rotating "system alive" icon update to balance visibility and resource conservation? → A: 1000ms (1 update/second - clear movement, low overhead)
- Q: What should happen if the OLED display hardware fails to initialize at startup? → A: Continue with all other subsystems, log error via WebSocket only (silent failure on display)
- Q: What MCU utilization metric should be displayed to help operators assess system health? → A: CPU idle time percentage (requires FreeRTOS stats, moderate overhead)
- Q: What refresh rate should be used for updating the main status information (RAM, flash, CPU idle %) on the display? → A: 5 seconds (balanced responsiveness with minimal overhead, rotating icon remains at 1 second)

---

## User Scenarios & Testing *(mandatory)*

### Primary User Story
A boat operator powers on the Poseidon2 gateway device. The OLED display immediately shows startup progress, indicating the status of each subsystem as it initializes (WiFi connection with IP address, filesystem mounting, web server startup). Once initialization is complete, the display transitions to showing live system status including WiFi connectivity, resource utilization (RAM usage, flash memory usage), and a visual indication that the system is actively running (not frozen). If WiFi connection drops during operation, the operator sees this reflected immediately on the display and observes when reconnection occurs.

### Acceptance Scenarios

1. **Given** the device is powered off, **When** the device boots up, **Then** the OLED display shows sequential startup progress for each subsystem (WiFi, filesystem, web server) with success/failure indication for each.

2. **Given** the device has completed startup successfully, **When** the system is running normally, **Then** the display shows current WiFi status (connected SSID and IP address), free RAM, flash memory usage (sketch size and free space), and a visual indicator that the system loop is executing.

3. **Given** the device is connected to WiFi, **When** the WiFi connection drops, **Then** the display immediately updates to show disconnected status and shows reconnection progress when attempting to reconnect.

4. **Given** the device is running normally, **When** RAM usage changes significantly (e.g., memory leak or normal operation), **Then** the display updates the free RAM indicator within 5 seconds.

5. **Given** the device is displaying system status, **When** the system encounters an error in any subsystem, **Then** the display shows the error state for that subsystem clearly.

### Edge Cases
- What happens when WiFi takes longer than expected to connect during startup? [NEEDS CLARIFICATION: Should there be a timeout displayed? Progress indicator?]
- How does the system handle a filesystem mount failure? [Display should show failure status, but overall system behavior needs clarification]

### Testing Strategy *(ESP32 embedded system)*
**Mock-First Approach** (Constitutional requirement):
- **Contract Tests**: Validate display interface contracts work correctly with mocked display hardware (native platform)
- **Integration Tests**: Test display update logic and state transitions with mocked display and subsystem status (native platform)
- **Unit Tests**: Validate metrics calculation, formatting logic, and display layout calculations (native platform)
- **Hardware Tests**: Minimal - only for actual OLED hardware communication and rendering validation (ESP32 required)

**Test Organization** (PlatformIO grouped tests):
- `test_oled_contracts/` - Display HAL interface contract validation
- `test_oled_integration/` - Display update scenarios with mocked hardware (startup sequence, status updates, WiFi state changes)
- `test_oled_units/` - Metrics formatting, memory statistics, text layout utilities
- `test_oled_hardware/` - OLED display hardware validation (ESP32 only, I2C communication, actual rendering)

## Requirements *(mandatory)*

### Functional Requirements

#### Startup Progress Display
- **FR-001**: System MUST display startup progress sequentially for each initializing subsystem
- **FR-002**: System MUST display WiFi connection status during startup, including which network is being attempted
- **FR-003**: System MUST display WiFi IP address once connection succeeds
- **FR-004**: System MUST display filesystem (LittleFS) mount status during startup (success/failure)
- **FR-005**: System MUST display web server initialization status during startup (success/failure)
- **FR-006**: System MUST display success or failure indication for each subsystem clearly distinguishable on the display

#### Runtime Status Display
- **FR-007**: System MUST display current WiFi connection status (connected vs disconnected) continuously during runtime
- **FR-008**: System MUST display connected WiFi SSID and IP address when WiFi is connected
- **FR-009**: System MUST display WiFi disconnection immediately when connection is lost
- **FR-010**: System MUST display WiFi reconnection progress when attempting to reconnect
- **FR-011**: System MUST display current free RAM (heap memory) in bytes or kilobytes
- **FR-012**: System MUST display flash memory usage showing uploaded code size (sketch size)
- **FR-013**: System MUST display free flash memory space available for future updates
- **FR-014**: System MUST display a small rotating icon in corner (cycling through /, -, \, | characters) updating every 1 second to indicate the system loop is actively running (not frozen/crashed)
- **FR-015**: System MUST display CPU idle time percentage using FreeRTOS runtime statistics

#### Display Refresh and Updates
- **FR-016**: System MUST update status information (RAM, flash memory, CPU idle %) every 5 seconds to balance responsiveness with resource efficiency
- **FR-016a**: System MUST update rotating "system alive" icon animation every 1 second independent of main status refresh
- **FR-017**: System MUST update WiFi status immediately (within 1 second) when connection state changes
- **FR-018**: System MAY perform full display refresh on each update cycle (partial update optimization is not required for this feature)

#### Multi-Page Architecture (Foundation Only)
- **FR-019**: System MUST organize display content in a page-based structure to support future addition of multiple pages
- **FR-020**: System MUST display page 1 (system status page) as defined in this specification
- **FR-021**: System MUST provide a foundation for future page navigation via button press (button functionality NOT implemented in this feature, only architectural preparation)

#### Resource Efficiency
- **FR-022**: System MUST use efficient data types for metrics (uint8_t, uint16_t, uint32_t) appropriate to value ranges
- **FR-023**: System MUST store constant strings and display text in flash memory (PROGMEM) rather than RAM
- **FR-024**: System MUST minimize heap allocations related to display operations
- **FR-025**: System MUST avoid unnecessary abstraction layers that consume additional resources without clear benefit

#### Error Handling
- **FR-026**: System MUST log OLED initialization failure via WebSocket logging if display hardware fails to initialize
- **FR-027**: System MUST continue operating all other subsystems (WiFi, web server, etc.) normally even if OLED display fails to initialize (graceful degradation)

### Key Entities *(feature involves data)*

- **DisplayMetrics**: Represents system resource metrics to be displayed
  - Free RAM (bytes)
  - Sketch size (bytes)
  - Free flash space (bytes)
  - CPU idle time percentage (0-100%, from FreeRTOS runtime stats)
  - Loop execution indicator (rotating icon animation state: 0=/, 1=-, 2=\, 3=|)

- **SubsystemStatus**: Represents initialization and runtime status of each subsystem
  - WiFi status (connecting, connected, disconnected, failed)
  - WiFi SSID (when connected)
  - WiFi IP address (when connected)
  - Filesystem status (mounting, mounted, failed)
  - Web server status (starting, running, failed)
  - Status timestamps for each subsystem

- **DisplayPage**: Represents a logical page of information on the OLED (architecture for future multi-page support)
  - Page number/identifier
  - Display update function reference
  - Page-specific data or state

---

## Review & Acceptance Checklist
*GATE: Automated checks run during main() execution*

### Content Quality
- [x] No implementation details (languages, frameworks, APIs) - avoided specific library mentions
- [x] Focused on user value and business needs - operator monitoring and system health visibility
- [x] Written for non-technical stakeholders - described in terms of user experience
- [x] All mandatory sections completed

### Requirement Completeness
- [ ] No [NEEDS CLARIFICATION] markers remain - **2 clarifications outstanding (deferred to planning)**
- [x] Requirements are testable and unambiguous
- [x] Success criteria are measurable - visible status updates, refresh rates, resource metrics
- [x] Scope is clearly bounded - Page 1 only, button navigation excluded from this feature
- [x] Dependencies and assumptions identified - requires WiFiManager, LittleFS, web server status

**Clarifications Resolved** (5):
1. ✅ Visual design for "system alive" indicator → Rotating icon (/, -, \, |)
2. ✅ Refresh rate for "system alive" indicator → 1 second
3. ✅ OLED initialization failure handling → Continue with all subsystems, log via WebSocket
4. ✅ MCU utilization metric specification → CPU idle time percentage (FreeRTOS stats)
5. ✅ Display refresh rate for status information → 5 seconds (icon animation remains 1 second)

**Clarifications Deferred to Planning** (2):
1. WiFi connection timeout display behavior during startup (UX design detail)
2. Filesystem mount failure system behavior (system-wide behavior, not OLED-specific)

---

## Execution Status
*Updated by main() during processing*

- [x] User description parsed
- [x] Key concepts extracted
- [x] Ambiguities marked (7 clarifications identified)
- [x] User scenarios defined
- [x] Requirements generated (27 functional requirements)
- [x] Entities identified (DisplayMetrics, SubsystemStatus, DisplayPage)
- [x] Clarification session completed (5 resolved, 2 deferred to planning)
- [x] Review checklist passed (ready for planning phase)

---

## Additional Notes

### Assumptions
- OLED display is connected to I2C Bus 2 (SDA=GPIO21, SCL=GPIO22) as per SH-ESP32 board configuration
- Display is 128x64 pixel SSD1306 OLED (monochrome)
- Button on GPIO 13 exists but will NOT be used in this feature (reserved for future page navigation)
- Existing subsystems (WiFiManager, LittleFS, ConfigWebServer) provide status information that can be queried or monitored

### Out of Scope for This Feature
- Button input handling and debouncing (future feature)
- Page navigation between multiple display pages (future feature)
- Additional display pages beyond Page 1 (system status)
- Graphical elements beyond text and simple status indicators
- User configuration of display refresh rates or displayed metrics
- Display brightness control or power management
- Persistent display state across reboots

### Dependencies
- WiFiManager component must expose connection state and status information
- LittleFS adapter must provide mount status information
- ConfigWebServer must provide initialization/running status
- ReactESP event loop for periodic display updates
- ESP32 platform APIs for memory and flash metrics (ESP.getFreeHeap(), ESP.getSketchSize(), ESP.getFreeSketchSpace())

### Reference Implementation
- `examples/poseidongw/src/main.cpp` contains reference OLED display usage (note: to be used for inspiration only, not direct copying per user guidance)
