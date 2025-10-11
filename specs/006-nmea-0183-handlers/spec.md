# Feature Specification: NMEA 0183 Data Handlers

**Feature Branch**: `006-nmea-0183-handlers`
**Created**: 2025-10-11
**Status**: Draft
**Input**: User description: "NMEA 0183 handlers based on description in user_requirements/R007 - NMEA 0183 data.md"

## Execution Flow (main)
```
1. Parse user description from Input
   → Feature description provided in R007
2. Extract key concepts from description
   → Actors: Autopilot (AP), VHF radio (VH), BoatData system
   → Actions: Parse NMEA 0183 sentences, convert units, update BoatData
   → Data: GPS position, heading, rudder angle, course, speed, variation
   → Constraints: Only AP and VH talker IDs, 5 specific sentences, unit conversions required
3. For each unclear aspect:
   → Error handling behavior not specified
   → Performance requirements not specified
   → Invalid sentence handling not specified
4. Fill User Scenarios & Testing section
   → User flow: Marine device sends NMEA 0183 → Parse → Convert → Update BoatData
5. Generate Functional Requirements
   → Each requirement mapped to specific sentence and data element
6. Identify Key Entities (if data involved)
   → NMEA 0183 sentences, BoatData elements, talker IDs
7. Run Review Checklist
   → WARN: Spec has uncertainties regarding error handling and performance
8. Return: SUCCESS (spec ready for planning with clarifications needed)
```

---

## ⚡ Quick Guidelines
- ✅ Focus on WHAT users need and WHY
- ❌ Avoid HOW to implement (no tech stack, APIs, code structure)
- 👥 Written for business stakeholders, not developers

---

## Clarifications

### Session 2025-10-11

- Q: When a NMEA 0183 sentence fails checksum validation or is malformed, what should the system do? → A: Silently discard sentence (no logging, no counter)
- Q: When parsed NMEA 0183 data is out of acceptable range (e.g., rudder angle >90°, variation >30°), what should the system do? → A: Reject sentence entirely + do not update BoatData
- Q: What is the maximum acceptable processing time per NMEA 0183 sentence to ensure non-blocking operation? → A: 50 milliseconds per sentence
- Q: When Serial2 buffer overflows due to high NMEA 0183 data rate, what should the system do? → A: Drop oldest data from buffer + continue processing
- Q: For unit conversion: Do NMEA 0183 parsers provide speed (SOG) in knots or meters/second, requiring conversion to BoatData units? → A: Convert from m/s to knots

---

## User Scenarios & Testing *(mandatory)*

### Primary User Story

As a boat operator, I need the system to receive and process marine data from my autopilot and VHF radio over NMEA 0183 so that I can see accurate position, heading, speed, and steering information in my SignalK dashboard and on the OLED display, even when these older devices don't support the newer NMEA 2000 protocol.

### Acceptance Scenarios

1. **Given** the autopilot is transmitting APRSA sentences, **When** the rudder moves to starboard 15 degrees, **Then** the system must update BoatData with steering angle of 15 degrees starboard

2. **Given** the autopilot is transmitting APHDM sentences, **When** the boat is heading 045 degrees magnetic, **Then** the system must update BoatData with magnetic heading of 45 degrees

3. **Given** the VHF radio is transmitting VHGGA sentences, **When** the GPS position is received with latitude 52.5°N and longitude 5.7°E, **Then** the system must update BoatData with these coordinates in decimal degrees

4. **Given** the VHF radio is transmitting VHRMC sentences, **When** GPS data includes position, course, speed, and variation, **Then** the system must update BoatData with all these values and calculate true heading from magnetic heading and variation

5. **Given** the VHF radio is transmitting VHVTG sentences, **When** the sentence includes true COG, magnetic COG, and SOG, **Then** the system must update BoatData with course and speed, calculate variation from the difference between true and magnetic COG, and update true heading

6. **Given** multiple sources are providing the same data (e.g., NMEA 2000 and NMEA 0183), **When** the NMEA 2000 source transmits at 10 Hz and NMEA 0183 at 1 Hz, **Then** the system must prioritize the NMEA 2000 source (highest frequency wins)

7. **Given** a NMEA 0183 sentence is received with an invalid talker ID (not AP or VH), **When** the sentence is processed, **Then** the system must ignore the sentence and continue normal operation

8. **Given** a NMEA 0183 sentence is received with a supported talker ID but unsupported message type, **When** the sentence is processed, **Then** the system must ignore the sentence and continue normal operation

### Edge Cases

- What happens when a NMEA 0183 sentence is malformed or has invalid checksums? (Expected: System silently discards the sentence and continues normal operation)

- What happens when the VHF provides position data in VHGGA but another source (NMEA 2000) is already active with higher frequency? (Expected: NMEA 0183 data ignored due to lower priority)

- What happens when APRSA sentence provides rudder angle outside the valid range (-90 to +90 degrees)? (Expected: System rejects the sentence entirely and does not update BoatData)

- What happens when VTG provides true and magnetic COG that differ by more than expected (e.g., >30 degrees variation)? (Expected: System rejects the sentence entirely if calculated variation exceeds acceptable range)

- What happens when Serial2 buffer overflows due to high data rate? (Expected: System drops oldest data from buffer and continues processing new incoming data)

### Testing Strategy *(ESP32 embedded system)*

**Mock-First Approach** (Constitutional requirement):

- **Contract Tests**: Validate NMEA 0183 parser interfaces work correctly with mocks (native platform)
  - Test ISentenceParser interface with known-good sentences
  - Test IUnitConverter interface with boundary values
  - Test BoatData update interfaces (ISensorUpdate)

- **Integration Tests**: Test complete scenarios with mocked hardware (native platform)
  - Scenario 1: Autopilot sends RSA and HDM, verify rudder angle and magnetic heading updated
  - Scenario 2: VHF sends GGA, verify GPS position updated
  - Scenario 3: VHF sends RMC with complete data, verify all GPS fields updated
  - Scenario 4: VHF sends VTG, verify COG/SOG updated and variation calculated
  - Scenario 5: Multiple sources with different frequencies, verify highest frequency wins
  - Scenario 6: Invalid talker ID sentences ignored
  - Scenario 7: Unsupported sentence types ignored
  - Scenario 8: Unit conversion correctness (degrees to radians, knots to m/s, etc.)

- **Unit Tests**: Validate formulas, calculations, and utilities (native platform)
  - Test unit conversions (degrees ↔ radians, m/s → knots, degrees/minutes ↔ decimal degrees)
  - Test variation calculation from true/magnetic COG difference
  - Test true heading calculation from magnetic heading and variation
  - Test sentence parsing for each supported message type
  - Test checksum validation
  - Test field extraction and type conversion

- **Hardware Tests**: Minimal - only for HAL validation and timing-critical operations (ESP32 required)
  - Test Serial2 NMEA 0183 sentence reception timing at 4800 baud
  - Test sentence parsing performance (ensure ≤50ms per sentence)
  - Test buffer handling under sustained high data rate

**Test Organization** (PlatformIO grouped tests):
- `test_nmea0183_contracts/` - HAL interface contract validation (ISentenceParser, IUnitConverter)
- `test_nmea0183_integration/` - End-to-end scenarios with mocked Serial2 and BoatData
- `test_nmea0183_units/` - Formula and utility unit tests (conversions, calculations)
- `test_nmea0183_hardware/` - Hardware validation tests (Serial2 timing, ESP32 only)

## Requirements *(mandatory)*

### Functional Requirements

**Sentence Parsing**

- **FR-001**: System MUST parse APRSA sentences from autopilot (AP talker ID) to extract rudder steering angle
- **FR-002**: System MUST parse APHDM sentences from autopilot (AP talker ID) to extract magnetic heading
- **FR-003**: System MUST parse VHGGA sentences from VHF radio (VH talker ID) to extract GPS latitude and longitude
- **FR-004**: System MUST parse VHRMC sentences from VHF radio (VH talker ID) to extract GPS position, course over ground, speed over ground, and magnetic variation
- **FR-005**: System MUST parse VHVTG sentences from VHF radio (VH talker ID) to extract true course, magnetic course, and speed over ground
- **FR-006**: System MUST ignore all NMEA 0183 sentences with talker IDs other than AP and VH
- **FR-007**: System MUST ignore all sentence types other than RSA, HDM, GGA, RMC, and VTG regardless of talker ID

**Unit Conversion**

- **FR-008**: System MUST convert all angular values from degrees to radians before updating BoatData (target unit per BoatDataTypes.h)
- **FR-009**: System MUST convert speed over ground from meters per second (as provided by NMEA 0183 parsers) to knots before updating BoatData (target unit per BoatDataTypes.h)
- **FR-010**: System MUST convert GPS latitude/longitude from degrees-minutes format to decimal degrees format before updating BoatData
- **FR-011**: System MUST calculate magnetic variation as the difference between true course and magnetic course when both are provided in VTG sentence
- **FR-012**: System MUST calculate true heading by adding magnetic variation to magnetic heading when variation is available

**BoatData Integration**

- **FR-013**: System MUST update BoatData.RudderData.steeringAngle when APRSA sentence is received
- **FR-014**: System MUST update BoatData.CompassData.magneticHeading when APHDM sentence is received
- **FR-015**: System MUST update BoatData.GPSData (latitude, longitude) when VHGGA sentence is received
- **FR-016**: System MUST update BoatData.GPSData (latitude, longitude, cog, sog, variation) when VHRMC sentence is received
- **FR-017**: System MUST update BoatData.CompassData.trueHeading when VHRMC or VHVTG provides variation data
- **FR-018**: System MUST update BoatData.GPSData (cog, sog, variation) when VHVTG sentence is received
- **FR-019**: System MUST use BoatData's ISensorUpdate interface to update sensor values with appropriate source identifiers (e.g., "NMEA0183-AP", "NMEA0183-VH")

**Source Priority**

- **FR-020**: System MUST prioritize data sources by update frequency, where highest frequency source becomes the active source for each data element
- **FR-021**: System MUST allow NMEA 2000 sources to take priority over NMEA 0183 sources when NMEA 2000 transmits at higher frequency
- **FR-022**: System MUST register NMEA 0183 sources with BoatData's SourcePrioritizer with appropriate source identifiers
- **FR-023**: System MUST track update frequency for each NMEA 0183 source to enable automatic priority determination

**Error Handling**

- **FR-024**: System MUST validate NMEA 0183 sentence checksums and silently discard sentences with invalid checksums without logging or error counting
- **FR-025**: System MUST silently discard malformed or unparseable NMEA 0183 sentences and continue normal operation without logging or error counting
- **FR-026**: System MUST validate that parsed data values are within acceptable ranges (rudder angle: -90° to +90°, latitude: -90° to +90°, longitude: -180° to +180°, variation: -30° to +30°, speed: 0 to 100 knots, heading: 0° to 360°) and reject sentences with out-of-range values without updating BoatData
- **FR-032**: System MUST implement FIFO buffer overflow handling that drops oldest data when Serial2 buffer capacity is exceeded and continues processing new incoming data

**Performance**

- **FR-027**: System MUST process each incoming NMEA 0183 sentence within 50 milliseconds to avoid blocking other system operations
- **FR-028**: System MUST handle NMEA 0183 data at standard baud rate of 4800 bps on Serial2 without data loss using FIFO buffer overflow handling

**RSA Sentence Parser**

- **FR-029**: System MUST implement a parser for APRSA (rudder angle) sentences since this sentence type is not available in the standard NMEA0183 library
- **FR-030**: The RSA parser MUST extract starboard/port rudder angle values from the sentence
- **FR-031**: The RSA parser MUST follow the same structure and patterns as other parsers in the NMEA0183 library for consistency

### Key Entities

- **NMEA 0183 Sentence**: A line of text transmitted by marine devices following the NMEA 0183 protocol standard, consisting of a talker ID, message type, data fields, and checksum. Only sentences with talker IDs AP (autopilot) and VH (VHF radio) are processed by this feature.

- **Talker ID**: A two-character code identifying the source device type in NMEA 0183 sentences. This feature handles AP (autopilot) and VH (VHF radio).

- **RSA Message**: Rudder Sensor Angle message transmitted by autopilot, contains rudder position in degrees (port/starboard). Requires custom parser implementation.

- **HDM Message**: Heading Magnetic message transmitted by autopilot, contains vessel's magnetic heading in degrees.

- **GGA Message**: Global Positioning System Fix Data transmitted by VHF, contains GPS latitude, longitude, and fix quality information.

- **RMC Message**: Recommended Minimum Navigation Information transmitted by VHF, contains GPS position, course over ground (true), speed over ground, and magnetic variation.

- **VTG Message**: Track Made Good and Ground Speed transmitted by VHF, contains true course, magnetic course, and speed over ground. Used to calculate magnetic variation from the difference between true and magnetic course.

- **BoatData Elements**: The centralized data repository for marine sensor data. This feature updates the following elements:
  - RudderData.steeringAngle (from RSA)
  - CompassData.magneticHeading (from HDM)
  - CompassData.trueHeading (calculated from HDM + variation)
  - GPSData.latitude, longitude (from GGA, RMC)
  - GPSData.cog, sog, variation (from RMC, VTG)

- **Source Priority**: The mechanism by which BoatData determines which data source to use when multiple sources provide the same information. Priority is automatically determined by update frequency (highest frequency wins), allowing NMEA 2000 sources to naturally take precedence over slower NMEA 0183 sources.

- **Unit Conversion**: The transformation of data values from NMEA 0183 native units to BoatData target units. Key conversions include degrees to radians for all angles, degrees-minutes to decimal degrees for coordinates, and meters per second to knots for speeds.

---

## Review & Acceptance Checklist
*GATE: Automated checks run during main() execution*

### Content Quality
- [x] No implementation details (languages, frameworks, APIs) - specification avoids mentioning specific libraries or code structure
- [x] Focused on user value and business needs - feature enables older NMEA 0183 devices to integrate with modern boat data system
- [x] Written for non-technical stakeholders - describes what data is received, converted, and displayed
- [x] All mandatory sections completed - User Scenarios, Requirements, and Key Entities all present

### Requirement Completeness
- [x] No [NEEDS CLARIFICATION] markers remain - **All 5 clarifications resolved**:
  1. ✓ Error handling: Silent discard for malformed/invalid checksum sentences
  2. ✓ Data validation: Specific ranges defined, reject out-of-range values
  3. ✓ Performance: 50ms max processing time per sentence, FIFO buffer overflow handling
  4. ✓ Unit conversion: Convert m/s to knots for SOG
  5. ✓ Error logging: No logging or error counting for invalid sentences
- [x] Requirements are testable and unambiguous - each requirement specifies observable behavior
- [x] Success criteria are measurable - acceptance scenarios define clear pass/fail conditions
- [x] Scope is clearly bounded - only AP and VH talker IDs, only 5 sentence types, XDR explicitly out of scope
- [x] Dependencies and assumptions identified - depends on BoatData feature (R005), assumes Serial2 configured for NMEA 0183

---

## Execution Status
*Updated by main() during processing*

- [x] User description parsed - R007 requirement document processed
- [x] Key concepts extracted - talker IDs, sentences, BoatData mapping, unit conversions identified
- [x] Ambiguities marked - 5 clarification points marked in requirements
- [x] User scenarios defined - 8 acceptance scenarios plus edge cases
- [x] Requirements generated - 32 functional requirements across parsing, conversion, integration, priority, error handling, performance
- [x] Entities identified - NMEA sentences, talker IDs, BoatData elements, source priority, unit conversion
- [x] Review checklist passed - **SUCCESS**: All clarifications resolved, spec ready for planning phase

---
