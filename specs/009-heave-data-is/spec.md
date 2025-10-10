# Feature Specification: NMEA2000 PGN 127252 Heave Handler

**Feature Branch**: `009-heave-data-is`
**Created**: 2025-10-11
**Status**: Draft
**Input**: User description: "Heave Data is available in PGN 127252. Implement NMEA2000 handler as per https://github.com/ttlappalainen/NMEA2000/blob/master/src/N2kMessages.h and in the same style as other handler are implemented in src/components/NMEA2000Handlers.cpp"

## Execution Flow (main)
```
1. Parse user description from Input
   → Feature request: Implement NMEA2000 PGN 127252 handler for heave data ✓
2. Extract key concepts from description
   → PGN 127252 (Heave vertical displacement)
   → NMEA2000 message parsing
   → Consistent implementation style with existing handlers
   → Update BoatData structure
3. For each unclear aspect:
   → [No clarifications needed - technical specification is clear]
4. Fill User Scenarios & Testing section
   → Scenario: Marine sensor transmits heave data via NMEA2000
   → System parses, validates, and stores heave measurements
5. Generate Functional Requirements
   → Parse PGN 127252 messages
   → Validate heave data range
   → Update CompassData.heave field
   → Log updates via WebSocket
6. Identify Key Entities
   → Heave measurement (vertical displacement in meters)
   → CompassData structure (existing entity to be updated)
7. Run Review Checklist
   → No implementation details in spec (implementation referenced for style consistency)
   → Focus on WHAT (heave data capture) not HOW (code structure)
8. Return: SUCCESS (spec ready for planning)
```

---

## ⚡ Quick Guidelines
- ✅ Focus on WHAT users need and WHY
- ❌ Avoid HOW to implement (no tech stack, APIs, code structure)
- 👥 Written for business stakeholders, not developers

---

## User Scenarios & Testing *(mandatory)*

### Primary User Story
As a sailboat operator monitoring vessel stability and motion, I need the system to capture heave data (vertical displacement) from NMEA2000-compatible marine sensors so that I can:
- Monitor wave-induced vertical motion in real-time
- Track vessel stability during rough sea conditions
- Integrate heave measurements with other motion sensors (heel, pitch, rate of turn)
- Export complete vessel motion data for analysis and logging

### Acceptance Scenarios
1. **Given** a NMEA2000 marine sensor broadcasting PGN 127252 messages with heave data, **When** the system receives the message, **Then** the heave value (in meters) must be parsed, validated, and stored in the vessel's motion data structure
2. **Given** a PGN 127252 message with heave data outside the valid range (±5.0 meters), **When** the system processes the message, **Then** the value must be clamped to the valid range and a warning must be logged
3. **Given** a PGN 127252 message with invalid or unavailable heave data, **When** the system attempts to parse it, **Then** the system must log a debug message and skip the update without crashing
4. **Given** successfully parsed heave data, **When** the data is stored, **Then** the system must update the availability flag and timestamp for tracking data freshness

### Edge Cases
- What happens when heave value is N2kDoubleNA (not available)?
  → System logs DEBUG message, skips update, continues operation
- What happens when heave exceeds ±5.0 meters (typical marine sensor range)?
  → System clamps to valid range, logs WARNING with original and clamped values
- What happens when PGN 127252 parsing fails?
  → System logs ERROR, sets availability flag to false, continues operation
- How does system handle very high frequency heave updates (>10 Hz)?
  → System processes each update, ReactESP event loop ensures non-blocking operation

### Testing Strategy *(ESP32 embedded system)*
**Mock-First Approach** (Constitutional requirement):
- **Contract Tests**: Validate PGN 127252 parsing function interface works correctly (native platform)
- **Integration Tests**: Test complete heave data flow from PGN reception to BoatData storage with mocked NMEA2000 messages (native platform)
- **Unit Tests**: Validate heave validation/clamping functions in DataValidation.h (native platform)
- **Hardware Tests**: Minimal - only for actual NMEA2000 bus communication timing (ESP32 required)

**Test Organization** (PlatformIO grouped tests):
- Use existing `test_boatdata_contracts/` - Validate PGN 127252 handler signature
- Use existing `test_boatdata_integration/` - Test heave data scenario end-to-end
- Use existing `test_boatdata_units/` - Test heave validation functions
- Use existing `test_boatdata_hardware/` - Validate NMEA2000 timing (ESP32 only)

## Requirements *(mandatory)*

### Functional Requirements
- **FR-001**: System MUST parse NMEA2000 PGN 127252 messages to extract heave (vertical displacement) data in meters
- **FR-002**: System MUST validate heave values are within the valid range of ±5.0 meters (typical marine sensor operating range)
- **FR-003**: System MUST clamp out-of-range heave values to the valid range and log a warning with original and clamped values
- **FR-004**: System MUST update the vessel motion data structure (CompassData.heave field) with validated heave measurements
- **FR-005**: System MUST set the availability flag to true when heave data is successfully parsed and valid
- **FR-006**: System MUST update the timestamp field (lastUpdate) when heave data is stored to track data freshness
- **FR-007**: System MUST log all heave updates at DEBUG level via WebSocket for debugging and monitoring
- **FR-008**: System MUST log warnings at WARN level when heave values are out of range or clamped
- **FR-009**: System MUST log errors at ERROR level when PGN 127252 parsing fails
- **FR-010**: System MUST handle unavailable heave data (N2kDoubleNA) by logging DEBUG message and skipping update without error
- **FR-011**: System MUST increment the NMEA2000 message counter after successfully processing PGN 127252
- **FR-012**: System MUST follow the existing handler implementation pattern (null checks, validation, logging, data storage)

### Key Entities *(include if feature involves data)*
- **Heave Measurement**: Vertical displacement of the vessel perpendicular to the earth's surface (smooth, wave-free water reference), measured in meters, range ±5.0 meters, positive values indicate upward motion
- **CompassData Structure**: Existing vessel motion data container that stores heave alongside other motion sensors (heel, pitch, rate of turn), includes availability flag and timestamp for data freshness tracking
- **PGN 127252 Message**: NMEA2000 Parameter Group Number for heave data, includes Sequence ID (SID), heave value (meters), optional delay (seconds), and delay source enumeration

---

## Review & Acceptance Checklist
*GATE: Automated checks run during main() execution*

### Content Quality
- [X] No implementation details (languages, frameworks, APIs) - only reference to existing pattern for consistency
- [X] Focused on user value and business needs - marine operators monitoring vessel stability
- [X] Written for non-technical stakeholders - clear description of heave as vertical displacement
- [X] All mandatory sections completed

### Requirement Completeness
- [X] No [NEEDS CLARIFICATION] markers remain
- [X] Requirements are testable and unambiguous - each FR specifies exact behavior
- [X] Success criteria are measurable - validation ranges (±5.0m), log levels, availability flags
- [X] Scope is clearly bounded - single PGN handler following existing pattern
- [X] Dependencies and assumptions identified - assumes existing CompassData.heave field, DataValidation helpers, WebSocket logging

---

## Execution Status
*Updated by main() during processing*

- [X] User description parsed
- [X] Key concepts extracted
- [X] Ambiguities marked (none)
- [X] User scenarios defined
- [X] Requirements generated
- [X] Entities identified
- [X] Review checklist passed

---

## Dependencies & Assumptions

### Dependencies
- **Existing BoatData Structure**: CompassData must have `heave` field (currently exists per Enhanced BoatData v2.0.0)
- **NMEA2000 Library**: ParseN2kPGN127252 function must be available (confirmed in N2kMessages.h)
- **DataValidation Utilities**: Heave validation functions must exist (clampHeave, isValidHeave)
- **WebSocket Logger**: Required for DEBUG/WARN/ERROR logging
- **Existing Handler Pattern**: PGN 127251, 127257 handlers provide implementation reference

### Assumptions
- NMEA2000 bus initialization will be completed (currently pending integration)
- Heave data will be received at reasonable frequency (<10 Hz typical for motion sensors)
- Heave measurements represent vertical displacement from smooth water surface (not absolute altitude)
- Valid heave range is ±5.0 meters based on typical marine sensor specifications
- Positive heave values indicate upward motion (vessel rising above reference)
- CompassData.heave field already exists from Enhanced BoatData v2.0.0 implementation

---

## Success Metrics

### Functional Success
- PGN 127252 messages are parsed without errors (100% success rate for valid messages)
- Heave values within valid range are stored correctly (±5.0 meters)
- Out-of-range values are clamped and warnings are logged
- Unavailable data (N2kDoubleNA) is handled gracefully without errors
- Availability flags and timestamps are updated correctly on each successful parse

### Quality Metrics
- All tests pass (contract, integration, unit tests)
- WebSocket logs contain DEBUG entries for all heave updates
- WebSocket logs contain WARN entries for clamped values
- WebSocket logs contain ERROR entries for parse failures
- No memory leaks or crashes when processing heave data
- Handler follows existing code style and patterns (consistency with PGN 127251, 127257 handlers)

---

## Out of Scope

- **NMEA2000 Bus Initialization**: This feature only implements the PGN 127252 handler; NMEA2000 bus setup is handled separately
- **Delay and DelaySource Fields**: PGN 127252 includes optional delay parameters; this feature focuses on heave value only (delay fields may be added in future enhancement)
- **Heave Rate of Change**: Only instantaneous heave value is captured; derivative calculations (heave velocity/acceleration) are out of scope
- **Heave Data Filtering**: No smoothing or averaging; raw sensor values are stored directly
- **Heave Alarms**: No threshold-based alerting; monitoring applications will implement alarms based on stored heave data
- **Historical Heave Tracking**: No time-series storage; only current heave value is maintained in BoatData structure

---

## Notes

This feature completes the heave data integration that was partially addressed in Enhanced BoatData v2.0.0 (R005). The current PGN 127257 (Attitude) handler includes a comment noting that heave is not available in that PGN and would need to come from a different source. PGN 127252 is the correct NMEA2000 message for heave data.

The implementation should follow the exact pattern established in `src/components/NMEA2000Handlers.cpp` for consistency and maintainability:
1. Null pointer checks (boatData, logger)
2. Parse PGN using NMEA2000 library function
3. Check for N2kIsNA (not available)
4. Validate data range using DataValidation helpers
5. Clamp out-of-range values with warning log
6. Update BoatData structure
7. Log update at DEBUG level
8. Increment message counter
9. Log parse failures at ERROR level

The feature aligns with Constitutional Principle I (Hardware Abstraction) by using the NMEA2000 library for message parsing, Principle V (Network Debugging) by using WebSocket logging, and Principle VII (Fail-Safe Operation) by handling all error cases gracefully.
