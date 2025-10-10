# Feature Specification: WebSocket Loop Frequency Logging

**Feature Branch**: `007-loop-frequency-should`
**Created**: 2025-10-10
**Status**: Draft
**Input**: User description: "Loop frequency should also be reported on the WebSocket logging channel, same 5 second interval as on the OLED display"

## Execution Flow (main)
```
1. Parse user description from Input ✅
   → Feature: Add WebSocket logging for loop frequency metric
2. Extract key concepts from description ✅
   → Actors: System, Operator/Developer
   → Actions: Report, Log
   → Data: Loop frequency (Hz)
   → Constraints: 5-second interval, WebSocket channel
3. For each unclear aspect:
   → Log level: [NEEDS CLARIFICATION: DEBUG, INFO, or other level?]
   → Log format: [NEEDS CLARIFICATION: JSON format, text format, or specific schema?]
   → Component name: [NEEDS CLARIFICATION: which component should emit the log?]
   → Event name: [NEEDS CLARIFICATION: specific event identifier?]
4. Fill User Scenarios & Testing section ✅
5. Generate Functional Requirements ✅
6. Identify Key Entities ✅
7. Run Review Checklist ⏳
   → WARN: Spec has 4 clarification points
8. Return: SUCCESS (spec ready for planning after clarifications)
```

---

## ⚡ Quick Guidelines
- ✅ Focus on WHAT users need and WHY
- ❌ Avoid HOW to implement (no tech stack, APIs, code structure)
- 👥 Written for business stakeholders, not developers

---

## Clarifications

### Session 2025-10-10
- Q: What log level should be used for normal loop frequency updates (10-2000 Hz)? → A: DEBUG
- Q: What format should the WebSocket log message use? → A: JSON
- Q: Which component should emit the WebSocket log message? → A: Performance
- Q: What should the event name/identifier be in the log message? → A: LOOP_FREQUENCY
- Q: What fields should be included in the JSON log message? → A: Minimal - only frequency value (e.g., `{"frequency": 212}`)

---

## User Scenarios & Testing *(mandatory)*

### Primary User Story
As an **operator or developer** monitoring the Poseidon2 marine gateway, I want to **see the main loop frequency via WebSocket logs** so that I can **monitor system performance remotely without needing to view the OLED display**, enabling troubleshooting and performance analysis from a development machine or remote monitoring tool.

### Acceptance Scenarios
1. **Given** the system is running and WebSocket logging is connected, **When** 5 seconds elapse, **Then** a log message containing the current loop frequency (in Hz) is broadcast via WebSocket
2. **Given** the loop frequency changes (e.g., system load increases), **When** the next 5-second interval completes, **Then** the updated loop frequency value is logged via WebSocket
3. **Given** the loop frequency measurement is unavailable (first 5 seconds after boot), **When** the 5-second interval elapses, **Then** the WebSocket log indicates no measurement yet (e.g., 0 Hz or placeholder value)
4. **Given** the WebSocket client is disconnected, **When** the client reconnects, **Then** the loop frequency logs resume at the next 5-second interval
5. **Given** multiple WebSocket clients are connected, **When** the 5-second interval elapses, **Then** all connected clients receive the loop frequency log message

### Edge Cases
- What happens when the loop frequency is extremely low (< 10 Hz)?
  - **Expected**: Log message should include the low frequency value and potentially a warning indicator
- What happens when the loop frequency is extremely high (> 1000 Hz)?
  - **Expected**: Log message should include the high frequency value (no abbreviation in logs, unlike display)
- What happens during the first 5 seconds after boot (before first measurement)?
  - **Expected**: Log message should indicate measurement not yet available (e.g., 0 Hz or "---")
- What happens if WebSocket logging is disabled or fails?
  - **Expected**: System continues normal operation, frequency still displayed on OLED (graceful degradation)

### Testing Strategy *(ESP32 embedded system)*
**Mock-First Approach** (Constitutional requirement):
- **Integration Tests**: Validate WebSocket log emission with mocked hardware (native platform)
  - Test log message format
  - Test 5-second interval timing
  - Test placeholder handling (before first measurement)
  - Test frequency value propagation

- **Unit Tests**: Validate log formatting and message construction (native platform)
  - Test JSON formatting (if JSON is used)
  - Test log level handling
  - Test frequency value serialization

- **Hardware Tests**: Minimal - only for timing validation (ESP32 required)
  - Verify WebSocket logs emit at 5-second intervals (±500ms tolerance)
  - Verify log content matches OLED display value

**Test Organization** (PlatformIO grouped tests):
- `test_websocket_logging_integration/` - WebSocket log emission scenarios with mocked hardware
- `test_websocket_logging_units/` - Log formatting and message construction
- `test_websocket_logging_hardware/` - Hardware timing validation (ESP32 only)

## Requirements *(mandatory)*

### Functional Requirements
- **FR-051**: System MUST emit a WebSocket log message containing the current loop frequency every 5 seconds
- **FR-052**: WebSocket log message MUST include the loop frequency value in Hz (same value displayed on OLED)
- **FR-053**: WebSocket log message MUST be emitted at the same 5-second interval as the OLED display update
- **FR-054**: System MUST emit WebSocket log message even if loop frequency is 0 Hz (before first measurement)
- **FR-055**: WebSocket log message MUST use DEBUG log level for normal frequency updates (10-2000 Hz) and WARN log level for abnormal frequencies (< 10 Hz or > 2000 Hz)
- **FR-056**: WebSocket log message MUST use "Performance" as the component identifier
- **FR-057**: WebSocket log message MUST use "LOOP_FREQUENCY" as the event identifier
- **FR-058**: WebSocket log message MUST use JSON format with minimal schema containing only the frequency field (e.g., `{"frequency": 212}`)
- **FR-059**: System MUST continue normal operation if WebSocket logging fails (graceful degradation)
- **FR-060**: System MUST broadcast loop frequency log to all connected WebSocket clients

### Non-Functional Requirements
- **NFR-010**: WebSocket log emission overhead MUST be < 1ms per message (minimal impact on loop performance)
- **NFR-011**: WebSocket log message size MUST be < 200 bytes (efficient bandwidth usage)
- **NFR-012**: WebSocket log emission timing MUST be synchronized with OLED display update (no separate timer)

### Key Entities *(data involved)*
- **Loop Frequency Log Message**: JSON-formatted message with minimal schema `{"frequency": <Hz value>}` sent via WebSocket logger with component "Performance" and event "LOOP_FREQUENCY"
- **WebSocket Logging Channel**: Existing WebSocket connection used for system debugging and monitoring

---

## Review & Acceptance Checklist
*GATE: Automated checks run during main() execution*

### Content Quality
- [X] No implementation details (languages, frameworks, APIs)
- [X] Focused on user value and business needs
- [X] Written for non-technical stakeholders
- [X] All mandatory sections completed

### Requirement Completeness
- [X] No [NEEDS CLARIFICATION] markers remain ✅
- [X] Requirements are testable and unambiguous
- [X] Success criteria are measurable
- [X] Scope is clearly bounded
- [X] Dependencies and assumptions identified

---

## Execution Status
*Updated by main() during processing*

- [X] User description parsed
- [X] Key concepts extracted
- [X] Ambiguities marked and resolved (5 clarifications completed)
- [X] User scenarios defined
- [X] Requirements generated
- [X] Entities identified
- [X] Review checklist passed ✅

---

## Dependencies & Assumptions

### Dependencies
- **Existing Loop Frequency Feature** (R006): This feature extends R006 by adding WebSocket logging
- **WebSocket Logging Infrastructure**: Assumes WebSocketLogger utility exists and is functional
- **ReactESP Event Loop**: Assumes existing 5-second event loop for OLED display update

### Assumptions
- WebSocket logging channel is already established and functional
- Loop frequency measurement (from R006) is accessible to logging subsystem
- 5-second interval timing is managed by existing ReactESP event loop (no new timer needed)
- WebSocket logging follows existing logging patterns in the codebase

### Out of Scope
- Creating new WebSocket logging infrastructure (assumed to exist)
- Modifying loop frequency measurement logic (R006 already implemented)
- Adding new timing mechanisms (reuses existing 5-second interval)
- Creating new display formats (only adds logging, display unchanged)

---

## Success Metrics

### Acceptance Criteria
1. ✅ Loop frequency logged via WebSocket every 5 seconds
2. ✅ Log value matches OLED display value (validated via test)
3. ✅ Log emitted even when frequency is 0 Hz (first 5 seconds)
4. ✅ All connected WebSocket clients receive log messages
5. ✅ System continues operation if WebSocket logging fails
6. ✅ Log emission overhead < 1ms per message

### Validation Method
- **Integration Tests**: Mock WebSocket logger, verify log emission timing and content
- **Unit Tests**: Validate log message formatting and serialization
- **Hardware Tests**: Verify timing on ESP32, compare log value to OLED display

---

**Specification Status**: ✅ **READY FOR PLANNING**

**Next Steps**:
1. ✅ All clarifications answered (5/5 completed)
2. ✅ All [NEEDS CLARIFICATION] markers removed
3. ⏳ Run `/plan` command to generate implementation plan
4. ⏳ Run `/tasks` command to generate task breakdown
5. ⏳ Run `/implement` command to execute implementation

---
