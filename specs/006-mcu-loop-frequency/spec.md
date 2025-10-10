# Feature Specification: MCU Loop Frequency Display

**Feature Branch**: `006-mcu-loop-frequency`
**Created**: 2025-10-10
**Status**: Draft
**Input**: User description: "MCU Loop Frequency as per user_requirements/R006 - MCU utilization.md"

## Execution Flow (main)
```
1. Parse user description from Input
   → Feature description provided with reference code snippet
2. Extract key concepts from description
   → Actors: Device operator, system monitor
   → Actions: Calculate loop frequency, update display metric
   → Data: Loop iteration count, time interval
   → Constraints: 5-second measurement window, display format
3. For each unclear aspect:
   → All aspects clearly specified in reference document
4. Fill User Scenarios & Testing section
   → User flow: Monitor system performance via OLED display
5. Generate Functional Requirements
   → Each requirement testable via integration and hardware tests
6. Identify Key Entities
   → Performance metrics data structure
7. Run Review Checklist
   → No implementation details exposed to stakeholders
   → All requirements testable
8. Return: SUCCESS (spec ready for planning)
```

---

## ⚡ Quick Guidelines
- ✅ Focus on WHAT users need and WHY
- ❌ Avoid HOW to implement (no tech stack, APIs, code structure)
- 👥 Written for business stakeholders, not developers

---

## User Scenarios & Testing

### Primary User Story
As a **system operator**, I need to **monitor the real-time performance** of the device's main processing loop so that I can **detect performance degradation** and **ensure the system is running within expected parameters**.

**Current State**: The OLED display shows a static "CPU Idle: 85%" metric that does not reflect actual system performance.

**Desired State**: The OLED display shows a dynamic "MCU Loop Frequency: XXX Hz" metric that reflects the actual measured loop frequency averaged over 5-second intervals.

### Acceptance Scenarios

1. **Given** the device is powered on and operating normally, **When** I view the OLED display after 5 seconds, **Then** I see the MCU loop frequency displayed in Hz (e.g., "MCU Loop Frequency: 212 Hz").

2. **Given** the system is running with no additional load, **When** the display updates every 5 seconds, **Then** the loop frequency value changes to reflect the current measurement period.

3. **Given** the system experiences increased processing load, **When** the display updates, **Then** the loop frequency value decreases to reflect the slower loop rate.

4. **Given** the loop frequency is being calculated, **When** a measurement period completes, **Then** the displayed value represents the average frequency over the full 5-second window.

5. **Given** the display line is limited to 21 characters, **When** the loop frequency is displayed, **Then** the text fits within the line: "MCU Loop Frequency: XXX Hz" (where XXX is right-aligned with appropriate spacing).

### Edge Cases

- What happens when the loop frequency is very low (< 10 Hz)?
  - Display should show single/double-digit values without truncation

- What happens when the loop frequency is very high (> 1000 Hz)?
  - Display should show 4-digit values, may abbreviate if space limited (e.g., "1.2k Hz")

- What happens during the first 5 seconds after boot?
  - Display should show "---" or "calculating..." until first measurement completes

- How does the system handle measurement overflow?
  - Counter overflow protection must be implemented (wrap-around detection)

### Testing Strategy *(ESP32 embedded system)*

**Mock-First Approach** (Constitutional requirement):
- **Contract Tests**: Validate performance monitor interface contracts with mocked display adapter
- **Integration Tests**: Test complete measurement cycle with controlled loop timing simulation
- **Unit Tests**: Validate frequency calculation formulas, counter overflow handling, display formatting
- **Hardware Tests**: Validate actual loop frequency measurement on ESP32 hardware, verify timing accuracy

**Test Organization** (PlatformIO grouped tests):
- `test_performance_contracts/` - IPerformanceMonitor interface validation
- `test_performance_integration/` - End-to-end measurement scenarios with mocked hardware
- `test_performance_units/` - Frequency calculation, formatting, overflow detection tests
- `test_performance_hardware/` - Hardware timing validation (ESP32 only)

**Specific Test Cases**:
1. Frequency calculation accuracy (known loop count / 5.0 seconds)
2. Display format validation (21-character limit, right-aligned numeric value)
3. Measurement window timing (exactly 5 seconds between updates)
4. Counter reset after each report (no cumulative drift)
5. Overflow protection (micros() and millis() wrap-around at ~70 minutes and ~49 days)
6. Initial state handling (no display until first measurement completes)

## Requirements

### Functional Requirements

- **FR-041**: System MUST measure the main loop iteration count over a 5-second window.

- **FR-042**: System MUST calculate the average loop frequency as (iteration_count / 5.0) Hz.

- **FR-043**: System MUST update the OLED display with the calculated loop frequency every 5 seconds.

- **FR-044**: System MUST replace the current "CPU Idle: 85%" line with "MCU Loop Frequency: XXX Hz" format.

- **FR-045**: Display format MUST show frequency as an integer value (no decimal places) in Hz.

- **FR-046**: System MUST reset the loop counter after each 5-second measurement period completes.

- **FR-047**: System MUST NOT print performance metrics to the serial port (display-only output).

- **FR-048**: System MUST handle counter overflow conditions gracefully (micros() and millis() wrap-around).

- **FR-049**: System MUST display a placeholder value ("---" or similar) during the initial 5-second measurement period after boot.

- **FR-050**: System MUST maintain measurement accuracy within ±5 Hz of actual loop frequency.

### Non-Functional Requirements

- **NFR-007**: Measurement overhead MUST NOT significantly impact loop frequency (< 1% performance impact).

- **NFR-008**: Display update MUST complete within the existing 5-second display refresh cycle without additional delays.

- **NFR-009**: Memory footprint MUST remain within constitutional resource limits (static allocation preferred, no heap fragmentation).

### Key Entities

- **PerformanceMetrics**: Represents the real-time performance characteristics of the main loop
  - Loop iteration count (accumulated over measurement window)
  - Measurement window start time (timestamp)
  - Current loop frequency (calculated average in Hz)
  - Validity flag (indicates if first measurement completed)

---

## Review & Acceptance Checklist

### Content Quality
- [x] No implementation details (languages, frameworks, APIs)
- [x] Focused on user value and business needs (monitoring system performance)
- [x] Written for non-technical stakeholders (operator-focused language)
- [x] All mandatory sections completed

### Requirement Completeness
- [x] No [NEEDS CLARIFICATION] markers remain
- [x] Requirements are testable and unambiguous (specific Hz accuracy, timing windows)
- [x] Success criteria are measurable (±5 Hz accuracy, < 1% overhead)
- [x] Scope is clearly bounded (display only, no serial output, replace existing metric)
- [x] Dependencies and assumptions identified (existing OLED display, 5-second refresh cycle)

---

## Execution Status

- [x] User description parsed
- [x] Key concepts extracted (loop frequency measurement, 5-second window, display formatting)
- [x] Ambiguities marked (none - reference code provides clear implementation pattern)
- [x] User scenarios defined (operator monitoring, performance visibility)
- [x] Requirements generated (10 functional + 3 non-functional)
- [x] Entities identified (PerformanceMetrics)
- [x] Review checklist passed

---
