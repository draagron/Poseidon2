# Feature Specification: NMEA 2000 Message Handling

**Feature Branch**: `010-nmea-2000-handling`
**Created**: 2025-10-12
**Status**: Draft
**Input**: User description: "NMEA 2000 handling as per user_requirements/R008 - NMEA 2000 data.md"

## User Scenarios & Testing *(mandatory)*

### User Story 1 - GPS Navigation Data via NMEA 2000 (Priority: P1)

A sailor wants to see their boat's position, speed, and course on the vessel's display system, receiving this data from the boat's NMEA 2000 GPS unit.

**Why this priority**: GPS data is the most fundamental navigation information needed for safe operation. Without accurate position, speed, and course data, the vessel's navigation system is essentially non-functional.

**Independent Test**: Can be fully tested by connecting an NMEA 2000 GPS unit and verifying that position (lat/lon), COG, SOG, and magnetic variation appear in the BoatData structure and are logged via WebSocket.

**Acceptance Scenarios**:

1. **Given** a NMEA 2000 GPS is broadcasting PGN 129025 (Position Rapid Update), **When** the system receives valid latitude/longitude, **Then** GPSData.latitude and GPSData.longitude are updated with correct decimal degree values
2. **Given** a NMEA 2000 GPS is broadcasting PGN 129026 (COG & SOG Rapid Update), **When** the system receives valid COG and SOG values, **Then** GPSData.cog (in radians) and GPSData.sog (in knots) are updated correctly
3. **Given** a NMEA 2000 compass is broadcasting PGN 129258 (Magnetic Variation), **When** the system receives valid variation data, **Then** GPSData.variation is updated in radians
4. **Given** PGN 129029 (GNSS Position Data) is received with ≥4 satellites visible, **When** GNSS Type is valid, **Then** GPSData structure is populated with position data
5. **Given** PGN 129029 is received with <4 satellites, **When** processed, **Then** GPSData is NOT updated and DEBUG log indicates insufficient satellite count

---

### User Story 2 - Compass and Vessel Attitude Monitoring (Priority: P1)

A sailor needs to monitor the vessel's heading, heel angle, pitch, and rate of turn for safe navigation and sail trim optimization.

**Why this priority**: Heading information is critical for navigation, and attitude data (heel/pitch) is essential for sail trim and stability monitoring. Rate of turn helps with collision avoidance.

**Independent Test**: Connect NMEA 2000 compass/attitude sensor and verify CompassData structure updates with true/magnetic heading, heel, pitch, rate of turn, and heave data via WebSocket logs.

**Acceptance Scenarios**:

1. **Given** PGN 127250 (Vessel Heading) is received, **When** reference is N2khr_true, **Then** CompassData.trueHeading is updated in radians
2. **Given** PGN 127250 (Vessel Heading) is received, **When** reference is N2khr_magnetic, **Then** CompassData.magneticHeading is updated in radians
3. **Given** PGN 127251 (Rate of Turn) provides rotation rate, **When** valid data is received, **Then** CompassData.rateOfTurn is updated (positive = starboard turn)
4. **Given** PGN 127257 (Attitude) provides heel and pitch angles, **When** received, **Then** CompassData.heelAngle (positive = starboard) and CompassData.pitchAngle (positive = bow up) are updated
5. **Given** PGN 127252 (Heave) provides vertical displacement, **When** received, **Then** CompassData.heave is updated (positive = upward motion)

---

### User Story 3 - Depth, Speed, and Water Temperature (DST) Data (Priority: P2)

A sailor wants to monitor water depth, boat speed through water, and sea temperature from NMEA 2000 instruments to ensure safe navigation and optimize performance.

**Why this priority**: DST data is critical for safe coastal navigation (depth), performance monitoring (speed through water), and environmental awareness (water temperature). This is secondary to GPS/heading but essential for complete situational awareness.

**Independent Test**: Connect NMEA 2000 depth sounder, paddle wheel speed sensor, and temperature sensor. Verify DSTData structure updates correctly and all unit conversions are applied.

**Acceptance Scenarios**:

1. **Given** PGN 128267 (Water Depth) is received, **When** depth below transducer and offset are valid, **Then** DSTData.depth is calculated as DepthBelowTransducer + Offset
2. **Given** PGN 128259 (Boat Speed, Water Referenced) is received, **When** water-referenced speed is valid, **Then** DSTData.measuredBoatSpeed is updated in m/s
3. **Given** PGN 130316 (Temperature Extended Range) is received with TempSource = N2kts_SeaTemperature, **When** temperature is valid in Kelvin, **Then** DSTData.seaTemperature is updated in Celsius (converted from Kelvin)
4. **Given** PGN 130316 is received with TempSource ≠ N2kts_SeaTemperature, **When** processed, **Then** message is silently ignored (no BoatData update, no log)
5. **Given** depth value is negative or exceeds 100m, **When** validation occurs, **Then** value is clamped and WARN log is generated

---

### User Story 4 - Engine Monitoring and Diagnostics (Priority: P3)

A sailor with an engine wants to monitor engine RPM, oil temperature, and alternator voltage via NMEA 2000 to detect potential issues before they become critical failures.

**Why this priority**: Engine monitoring is important for power vessels but not critical for all users (sailboats may not have engines or may have minimal instrumentation). This is an enhancement for users with engine sensors.

**Independent Test**: Connect NMEA 2000 engine gateway and verify EngineData structure updates with RPM, oil temperature (Celsius), and alternator voltage with appropriate validation warnings.

**Acceptance Scenarios**:

1. **Given** PGN 127488 (Engine Parameters Rapid) is received with engine instance 0, **When** engine speed is valid, **Then** EngineData.engineRev is updated with RPM value [0, 6000]
2. **Given** PGN 127489 (Engine Parameters Dynamic) is received with engine instance 0, **When** oil temperature is valid in Kelvin, **Then** EngineData.oilTemperature is converted to Celsius and validated against [-10, 150]°C range
3. **Given** PGN 127489 provides alternator voltage for engine instance 0, **When** voltage is valid, **Then** EngineData.alternatorVoltage is updated and validated against [0, 30]V range with warnings for values outside [12, 15]V (12V system normal range)
4. **Given** oil temperature exceeds 120°C, **When** data is processed, **Then** WARN log is generated indicating high oil temperature
5. **Given** PGN 127488 or 127489 is received with engine instance ≠ 0, **When** processed, **Then** message is silently ignored (no BoatData update, no log)

---

### User Story 5 - Wind Data for Sail Trim (Priority: P3)

A sailor wants to receive apparent wind angle and speed from NMEA 2000 wind instruments to optimize sail trim and performance.

**Why this priority**: Wind data is important for sailing vessels but not critical for all users. This is an enhancement that completes the marine instrumentation suite.

**Independent Test**: Connect NMEA 2000 wind sensor and verify WindData structure updates with apparent wind angle (radians) and speed (knots) with proper unit conversions.

**Acceptance Scenarios**:

1. **Given** PGN 130306 (Wind Data) is received with WindReference = N2kWind_Apparent, **When** wind angle is valid, **Then** WindData.apparentWindAngle is updated in radians
2. **Given** PGN 130306 provides wind speed in m/s, **When** received, **Then** WindData.apparentWindSpeed is converted to knots and updated

---

### Edge Cases

- What happens when a PGN contains N2kDoubleNA or other unavailable value constants? → Handler skips update, logs DEBUG message, preserves existing BoatData values
- How does the system handle out-of-range sensor values? → Values are clamped to valid ranges, WARN log generated, availability flag set based on validity
- What happens when multiple NMEA 2000 devices broadcast the same PGN type? → All messages are processed; BoatData multi-source prioritization automatically selects the highest-frequency source
- What happens when multiple NMEA 2000 sources provide the same data type as NMEA 0183? → Highest frequency source wins automatically (NMEA 2000 typically 10 Hz vs NMEA 0183 1 Hz)
- How are unit conversions handled for temperature (Kelvin→Celsius) and speed (m/s→knots)? → Automatic conversion in handler using DataValidation utility functions
- What if a critical handler parse function fails? → ERROR log generated, BoatData availability flag set to false, existing values preserved
- How does the system handle excessive pitch angles (>30°) or heel angles (>45°)? → Values clamped, WARN log generated indicating vessel in extreme attitude
- What happens when GPS variation is missing from PGN 129029? → Existing variation value preserved (may come from other sources like PGN 127258 or NMEA 0183)

## Clarifications

### Session 2025-10-12

- Q: How should the Poseidon2 gateway handle NMEA 2000 address assignment on the CAN bus? → A: Dynamic address claiming using NMEA2000 library's built-in protocol (device announces preferred address, negotiates if conflict)
- Q: Which GPS quality indicators from PGN 129029 should determine data validity? → A: Only check GNSS Type and Number of SVs (satellites); update if ≥4 satellites visible
- Q: How should the system handle vessels with multiple engines (engine instance field in PGN 127488/127489)? → A: Store only engine instance 0 (primary engine); ignore all other engine instances
- Q: Should non-sea temperature sources from PGN 130316 be logged for debugging or completely ignored? → A: Silently ignore all non-sea temperature sources (no logging, no processing)
- Q: How should the system handle receiving duplicate PGN types from multiple devices on the bus? → A: Process all messages from all devices; BoatData multi-source prioritization handles frequency-based selection automatically

## Requirements *(mandatory)*

### Functional Requirements

#### GPS Data Handling
- **FR-001**: System MUST parse PGN 129025 (Position, Rapid Update) and update GPSData.latitude and GPSData.longitude with values in decimal degrees
- **FR-002**: System MUST parse PGN 129026 (COG & SOG, Rapid Update) and update GPSData.cog in radians and GPSData.sog in knots (convert from m/s)
- **FR-003**: System MUST parse PGN 129258 (Magnetic Variation) and update GPSData.variation in radians
- **FR-004**: System MUST parse PGN 129029 (GNSS Position Data) as an alternative/enhanced GPS source providing latitude, longitude, and quality metrics
- **FR-044**: System MUST validate PGN 129029 by checking GNSS Type field is valid and Number of SVs (satellites) is ≥4 before updating GPSData
- **FR-045**: System MUST log DEBUG message with GNSS Type and Number of SVs when processing PGN 129029

#### Compass and Attitude Data
- **FR-005**: System MUST parse PGN 127250 (Vessel Heading) and update CompassData.trueHeading when reference type is N2khr_true
- **FR-006**: System MUST parse PGN 127250 (Vessel Heading) and update CompassData.magneticHeading when reference type is N2khr_magnetic
- **FR-007**: System MUST parse PGN 127251 (Rate of Turn) and update CompassData.rateOfTurn in radians/second with sign convention: positive = starboard turn
- **FR-008**: System MUST parse PGN 127257 (Attitude) and update CompassData.heelAngle (roll) and CompassData.pitchAngle with values in radians
- **FR-009**: System MUST parse PGN 127252 (Heave) and update CompassData.heave in meters with sign convention: positive = upward motion
- **FR-010**: System MUST validate rate of turn against [-π, π] rad/s range and clamp if exceeded
- **FR-011**: System MUST validate pitch angle against [-π/6, π/6] radians (±30°) and clamp if exceeded
- **FR-012**: System MUST validate heel angle against [-π/2, π/2] radians (±90°) and generate WARN if exceeds ±45° (π/4)
- **FR-013**: System MUST validate heave against [-5.0, 5.0] meters and clamp if exceeded

#### DST (Depth/Speed/Temperature) Data
- **FR-014**: System MUST parse PGN 128267 (Water Depth) and calculate depth below waterline as DepthBelowTransducer + Offset
- **FR-015**: System MUST validate depth is non-negative and within [0, 100] meters range
- **FR-016**: System MUST parse PGN 128259 (Boat Speed, Water Referenced) and update DSTData.measuredBoatSpeed in m/s (native NMEA2000 unit)
- **FR-017**: System MUST validate boat speed against [0, 25] m/s range and clamp if exceeded
- **FR-018**: System MUST parse PGN 130316 (Temperature Extended Range) and process only when TempSource = N2kts_SeaTemperature; silently ignore all other temperature sources
- **FR-019**: System MUST convert sea temperature from Kelvin to Celsius using DataValidation::kelvinToCelsius()
- **FR-020**: System MUST validate water temperature against [-10, 50]°C range and clamp if exceeded

#### Engine Data
- **FR-021**: System MUST parse PGN 127488 (Engine Parameters Rapid) and update EngineData.engineRev with RPM value for engine instance 0 only
- **FR-022**: System MUST validate engine RPM against [0, 6000] range and clamp if exceeded
- **FR-023**: System MUST parse PGN 127489 (Engine Parameters Dynamic) and extract oil temperature and alternator voltage for engine instance 0 only
- **FR-024**: System MUST convert oil temperature from Kelvin to Celsius and validate against [-10, 150]°C range
- **FR-025**: System MUST validate alternator voltage against [0, 30]V hard range and WARN if outside [12, 15]V normal range for 12V systems
- **FR-026**: System MUST generate WARN log when oil temperature exceeds 120°C
- **FR-046**: System MUST ignore PGN 127488 and PGN 127489 messages where engine instance field is not 0 (primary engine only)

#### Wind Data
- **FR-027**: System MUST parse PGN 130306 (Wind Data) and update WindData when WindReference = N2kWind_Apparent
- **FR-028**: System MUST update WindData.apparentWindAngle in radians
- **FR-029**: System MUST convert WindData.apparentWindSpeed from m/s to knots

#### Data Availability and Error Handling
- **FR-030**: System MUST check for N2kDoubleNA, N2kInt8NA, N2kUInt8NA, N2kUInt16NA, N2kUInt32NA constants before using data values
- **FR-031**: System MUST skip BoatData updates and log DEBUG message when unavailable (NA) values are encountered
- **FR-032**: System MUST log ERROR and set availability flag to false when PGN parse function fails
- **FR-033**: System MUST preserve existing BoatData values when parse fails or data is unavailable
- **FR-034**: System MUST update lastUpdate timestamp to millis() for each successful data update
- **FR-035**: System MUST increment NMEA2000 message counter via boatData->incrementNMEA2000Count() for each successful update

#### Handler Registration
- **FR-036**: System MUST register all implemented PGN handlers via RegisterN2kHandlers() function during setup()
- **FR-037**: System MUST call nmea2000->ExtendReceiveMessages() with all handled PGNs before attaching handler
- **FR-038**: System MUST create N2kBoatDataHandler instance and attach via nmea2000->AttachMsgHandler()
- **FR-039**: System MUST log INFO level message listing count and PGN numbers of all registered handlers
- **FR-043**: System MUST use NMEA2000 library's dynamic address claiming protocol with automatic conflict resolution

#### Multi-Source Prioritization
- **FR-040**: System MUST use BoatData source registration to enable automatic frequency-based prioritization
- **FR-041**: NMEA 2000 sources (typically 10 Hz) MUST automatically take precedence over NMEA 0183 sources (typically 1 Hz) when both are available
- **FR-042**: System MUST support automatic failover to lower-frequency source when higher-frequency source becomes stale (>5 seconds)
- **FR-047**: System MUST process all NMEA 2000 messages regardless of source address; BoatData prioritization layer selects active source based on update frequency

### Key Entities

- **NMEA 2000 PGN (Parameter Group Number)**: Standardized message identifier in the NMEA 2000 protocol that defines the data content and format. Each PGN contains specific sensor data (e.g., PGN 129025 = GPS position).

- **BoatData Structure**: Centralized repository for all vessel sensor data, organized into logical groups (GPSData, CompassData, DSTData, EngineData, WindData, etc.). Each group has availability flag and lastUpdate timestamp.

- **Handler Functions**: Individual functions that parse specific PGNs using NMEA2000 library functions, validate data, convert units, and update BoatData structure. Each handler includes WebSocket logging for debugging.

- **N2kBoatDataHandler Class**: Message handler class that implements tNMEA2000::tMsgHandler interface, routing incoming PGN messages to appropriate handler functions based on PGN number.

- **DataValidation Utilities**: Static helper functions for validating sensor ranges, clamping out-of-range values, and performing unit conversions (Kelvin→Celsius, m/s→knots, etc.).

- **WebSocketLogger**: Network-based logging system for debugging since serial ports are used for device communication. Provides DEBUG, INFO, WARN, ERROR, and FATAL log levels.

- **Source Prioritization System**: BoatData multi-source management that automatically selects highest-frequency data source for each sensor type, enabling seamless integration of NMEA 2000 (10 Hz) and NMEA 0183 (1 Hz) sources with automatic failover.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: All GPS PGN handlers (129025, 129026, 129258, 129029) correctly parse and update GPSData structure with values matching NMEA2000 library test cases
- **SC-002**: All Compass/Attitude PGN handlers (127250, 127251, 127252, 127257) correctly update CompassData with validated angular and displacement values
- **SC-003**: DST PGN handlers (128267, 128259, 130316) correctly parse, convert units (Kelvin→Celsius), and validate depth/speed/temperature data
- **SC-004**: Engine PGN handlers (127488, 127489) correctly parse, convert units, and validate RPM/temperature/voltage with appropriate warning thresholds
- **SC-005**: Wind PGN handler (130306) correctly parses and converts wind angle (radians) and speed (m/s→knots) for apparent wind reference type
- **SC-006**: All handlers correctly detect and skip N2kDoubleNA/N2kInt8NA/etc. unavailable values without updating BoatData
- **SC-007**: All handlers correctly clamp out-of-range values and generate WARN logs with original and clamped values
- **SC-008**: All handlers correctly log ERROR when parse functions fail and preserve existing BoatData values
- **SC-009**: RegisterN2kHandlers() successfully registers all handlers and logs INFO message with PGN count and list
- **SC-010**: NMEA 2000 sources (10 Hz) automatically take precedence over NMEA 0183 sources (1 Hz) when both provide same data type
- **SC-011**: System correctly failover to NMEA 0183 source when NMEA 2000 source becomes unavailable or stale (>5 seconds)
- **SC-012**: All unit conversions are mathematically correct: Kelvin→Celsius (−273.15), m/s→knots (×1.9438444924406047516198704103672)
- **SC-013**: WebSocket logs provide sufficient detail for debugging (includes PGN number, parsed values, validation results, and units)
- **SC-014**: Memory footprint for all handlers remains within constitutional limits (static allocation only, no heap)
- **SC-015**: All critical ranges validated: pitch ±30°, heel ±90° (warn >±45°), heave ±5m, depth 0-100m, engine RPM 0-6000, battery voltage 0-30V (warn outside 12-15V), water temp -10-50°C, oil temp -10-150°C (warn >120°C)
