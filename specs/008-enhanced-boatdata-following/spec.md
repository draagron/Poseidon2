# Feature Specification: Enhanced Boatdata

**Feature Branch**: `008-enhanced-boatdata-following`
**Created**: 2025-10-10
**Status**: Draft
**Input**: User description: "Enhanced Boatdata - following the details in user_requirements/R005 - enhanced boatdata.md"

## Execution Flow (main)
```
1. Parse user description from Input
   → ✓ Feature requirements extracted from R005
2. Extract key concepts from description
   → ✓ Identified: data structure changes, new data structures, sensor sources
3. For each unclear aspect:
   → [NEEDS CLARIFICATION: Unit conversions and validation ranges]
   → [NEEDS CLARIFICATION: Data refresh rates and update intervals]
   → [NEEDS CLARIFICATION: Error handling for missing/invalid sensor data]
4. Fill User Scenarios & Testing section
   → ✓ Defined data collection, aggregation, and access scenarios
5. Generate Functional Requirements
   → ✓ Each requirement testable via data structure validation
6. Identify Key Entities (if data involved)
   → ✓ Entities: GPSData, CompassData, DSTData, EngineData, SaildriveData, BatteryData, ShorePowerData
7. Run Review Checklist
   → ⚠ WARN "Spec has uncertainties - needs clarification on validation and refresh rates"
8. Return: SUCCESS (spec ready for planning)
```

---

## ⚡ Quick Guidelines
- ✅ Focus on WHAT users need and WHY
- ❌ Avoid HOW to implement (no tech stack, APIs, code structure)
- 👥 Written for business stakeholders, not developers

---

## Clarifications

### Session 2025-10-10
- Q: What representation should be used for battery state of charge (stateOfChargeA/B)? → A: Percentage 0-100 (integer or float, e.g., 75.5% = 75.5)
- Q: What unit should be used for engine oil temperature (oilTemperature field)? → A: Celsius (consistent with existing seaTemperature in DSTData)
- Q: How should the system handle missing or unavailable sensor data (e.g., GPS variation not provided, depth sensor offline)? → A: Set field to zero with availability flag false
- Q: What sign convention should be used for battery amperage (positive/negative for charging/discharging)? → A: Positive = charging, negative = discharging
- Q: The user requirements specify converting measuredBoatSpeed from its current unit to m/s. What is the current unit? → A: Already in m/s - no conversion needed

---

## User Scenarios & Testing

### Primary User Story
As a boat operator, I need access to comprehensive boat sensor data through a centralized data model so that monitoring systems can display accurate vessel status including navigation (GPS, compass), motion (heel, pitch, heave, rate of turn), environment (depth, water temperature), propulsion (engine, saildrive), and electrical systems (batteries, shore power).

### Acceptance Scenarios

1. **Given** GPS system is receiving position data, **When** magnetic variation is available, **Then** variation value is stored in GPSData (not CompassData)

2. **Given** compass sensor is active, **When** vessel is turning, **Then** rate of turn is captured in radians/second

3. **Given** vessel is underway, **When** motion sensors detect heel and pitch, **Then** angles are recorded in CompassData with correct sign conventions (starboard/bow up positive)

4. **Given** depth sounder is operating, **When** water depth is measured, **Then** depth below waterline is stored in meters in DSTData

5. **Given** DST sensor is active, **When** water temperature is measured, **Then** temperature in Celsius is stored in DSTData

6. **Given** engine is running, **When** engine sensors report data, **Then** RPM, oil temperature, and alternator voltage are captured in EngineData

7. **Given** saildrive system has position sensor, **When** saildrive engagement changes, **Then** engagement state (true/false) is recorded in SaildriveData

8. **Given** dual battery system is installed, **When** battery monitors report, **Then** voltage, amperage, state of charge, and charger status are captured for both Battery A and Battery B

9. **Given** shore power is connected, **When** power consumption is measured, **Then** connection status and wattage are stored in ShorePowerData

### Edge Cases
- What happens when sensors provide data outside expected ranges? System MUST clamp values to valid ranges and log warnings
- How does system handle missing sensor data (e.g., GPS variation unavailable)? Field set to zero with availability flag false
- What happens when heel angle exceeds ±90°? Value clamped to [-π/2, π/2] range with warning logged
- How are negative depth readings handled (sensor above water)? Negative values set to zero with availability flag false
- What happens when battery state of charge readings are inconsistent? Accept sensor value if within 0-100 range, otherwise set to zero with availability flag false

### Testing Strategy (ESP32 embedded system)
**Mock-First Approach** (Constitutional requirement):
- **Contract Tests**: Validate data structure field types, ranges, and units with mock sensor data
- **Integration Tests**: Test data aggregation across multiple sensor sources with mocked NMEA2000, NMEA0183, and 1-wire interfaces
- **Unit Tests**: Validate unit conversions (if needed), sign conventions, data migrations from old to new structure
- **Hardware Tests**: Validate actual sensor data parsing from NMEA2000, NMEA0183, and 1-wire devices on ESP32

**Test Organization** (PlatformIO grouped tests):
- `test_boatdata_contracts/` - Data structure validation, field presence, type checking
- `test_boatdata_integration/` - Multi-sensor data aggregation scenarios
- `test_boatdata_units/` - Unit conversion validation, sign convention checks
- `test_boatdata_hardware/` - Real sensor data parsing (NMEA/1-wire protocols)

## Requirements

### Functional Requirements

#### Data Structure Changes
- **FR-001**: System MUST relocate magnetic variation field from CompassData to GPSData
- **FR-002**: System MUST rename SpeedData structure to DSTData (Depth, Speed, Temperature)
- **FR-003**: System MUST relocate heelAngle field from DSTData (formerly SpeedData) to CompassData
- **FR-004**: System MUST maintain measuredBoatSpeed in m/s units in DSTData (no conversion required)

#### Compass Data Enhancements
- **FR-005**: CompassData MUST include rateOfTurn field in radians/second representing rate of change of heading
- **FR-006**: CompassData MUST include heelAngle field in radians (positive = starboard, negative = port) with range [-π/2, π/2]
- **FR-007**: CompassData MUST include pitchAngle field in radians (positive = bow up, negative = bow down) with range [NEEDS CLARIFICATION: expected pitch range]
- **FR-008**: CompassData MUST include heave field in meters representing vertical distance relative to average sea level with range [NEEDS CLARIFICATION: expected heave range]

#### GPS Data Enhancements
- **FR-009**: GPSData MUST include variation field in radians (positive = East, negative = West) representing magnetic variation

#### DST Sensor Data
- **FR-010**: DSTData MUST include depth field in meters representing depth below waterline
- **FR-011**: DSTData MUST include seaTemperature field in Celsius representing water temperature
- **FR-012**: DSTData MUST maintain measuredBoatSpeed field in m/s from paddle wheel sensor

#### Engine Data (New Structure)
- **FR-013**: System MUST provide EngineData structure capturing NMEA2000 engine sensor data
- **FR-014**: EngineData MUST include engineRev field representing engine RPM [NEEDS CLARIFICATION: unit - RPM or Hz?]
- **FR-015**: EngineData MUST include oilTemperature field in Celsius
- **FR-016**: EngineData MUST include alternatorVoltage field in volts

#### Saildrive Data (New Structure - 1-Wire)
- **FR-017**: System MUST provide SaildriveData structure capturing 1-wire saildrive sensor data
- **FR-018**: SaildriveData MUST include saildriveEngaged boolean field (true = engaged, false = disengaged)

#### Battery Data (New Structure - 1-Wire)
- **FR-019**: System MUST provide BatteryData structure capturing dual battery monitor data via 1-wire
- **FR-020**: BatteryData MUST include voltageA field in volts for Battery A
- **FR-021**: BatteryData MUST include amperageA field in amperes for Battery A (positive = charging, negative = discharging)
- **FR-022**: BatteryData MUST include stateOfChargeA field in percentage (range 0.0-100.0) representing Battery A charge level
- **FR-023**: BatteryData MUST include shoreChargerOnA boolean field (true = shore charger active, false = inactive)
- **FR-024**: BatteryData MUST include engineChargerOnA boolean field (true = alternator charging, false = not charging)
- **FR-025**: BatteryData MUST include voltageB, amperageB, stateOfChargeB, shoreChargerOnB, engineChargerOnB fields for Battery B with same conventions as Battery A

#### Shore Power Data (New Structure - 1-Wire)
- **FR-026**: System MUST provide ShorePowerData structure capturing 1-wire shore power sensor data
- **FR-027**: ShorePowerData MUST include shorePowerOn boolean field (true = shore power connected, false = disconnected)
- **FR-028**: ShorePowerData MUST include power field in watts representing shore power consumption

#### Data Source Requirements
- **FR-029**: System MUST source GPS and compass data from NMEA0183 or NMEA2000 protocols
- **FR-030**: System MUST source DST sensor data from NMEA0183 or NMEA2000 protocols
- **FR-031**: System MUST source engine data from NMEA2000 protocol
- **FR-032**: System MUST source saildrive, battery, and shore power data from 1-wire sensors

#### Data Validation Requirements
- **FR-033**: System MUST set unavailable sensor data fields to zero with availability flag false
- **FR-034**: System MUST clamp out-of-range values to valid ranges and log warnings
- **FR-035**: System MUST include availability flag with each data structure to indicate data validity

#### Data Refresh Requirements
- **FR-036**: System MUST update sensor data at intervals appropriate for each sensor type [NEEDS CLARIFICATION: specific refresh rates for each sensor category]
- **FR-037**: System MUST handle sensor data arriving at different rates without blocking

### Key Entities

- **GPSData**: Navigation sensor data including position, speed over ground, course over ground, and magnetic variation (relocated from CompassData). Source: NMEA0183/NMEA2000.

- **CompassData**: Vessel heading and motion data including heading, rate of turn, heel angle (relocated from DSTData), pitch angle, and heave. Source: NMEA0183/NMEA2000.

- **DSTData**: Depth/Speed/Temperature sensor data (renamed from SpeedData) including depth below waterline, water temperature, and measured boat speed from paddle wheel. Source: NMEA0183/NMEA2000.

- **EngineData**: Propulsion system data including engine RPM, oil temperature (Celsius), and alternator voltage (volts). Source: NMEA2000.

- **SaildriveData**: Saildrive mechanism status including engagement state. Source: 1-wire sensors.

- **BatteryData**: Dual battery monitor data including voltage (volts), amperage (amperes, positive=charging, negative=discharging), state of charge (percentage 0-100), and charger status for both Battery A and Battery B. Source: 1-wire sensors.

- **ShorePowerData**: Shore power connection and consumption data including connection status and power draw in watts. Source: 1-wire sensors.

---

## Review & Acceptance Checklist
*GATE: Automated checks run during main() execution*

### Content Quality
- [x] No implementation details (languages, frameworks, APIs)
- [x] Focused on user value and business needs
- [x] Written for non-technical stakeholders
- [x] All mandatory sections completed

### Requirement Completeness
- [ ] No [NEEDS CLARIFICATION] markers remain - **3 clarifications deferred**
- [x] Requirements are testable and unambiguous (critical clarifications resolved)
- [x] Success criteria are measurable (data structure validation)
- [x] Scope is clearly bounded (data model changes only)
- [x] Dependencies and assumptions identified (NMEA protocols, 1-wire sensors)

**Resolved Clarifications (5)**:
1. ✅ StateOfCharge representation: Percentage 0-100
2. ✅ OilTemperature unit: Celsius
3. ✅ Error handling for missing/invalid sensor data: Set to zero with availability flag false
4. ✅ Sign convention for battery amperage: Positive = charging, negative = discharging
5. ✅ Current unit for measuredBoatSpeed: Already m/s (no conversion)

**Deferred to Planning Phase (3)**:
1. Expected pitch angle range (validation threshold)
2. Expected heave range (validation threshold)
3. EngineRev unit (RPM or Hz)
4. Data refresh rates and update intervals per sensor type

---

## Execution Status
*Updated by main() during processing*

- [x] User description parsed
- [x] Key concepts extracted
- [x] Ambiguities marked
- [x] User scenarios defined
- [x] Requirements generated
- [x] Entities identified
- [x] Review checklist passed (with warnings)

---

## Next Steps

**Ready for Planning Phase** ✅

Critical clarifications resolved. Proceed with `/plan` to:
1. Determine pitch/heave validation ranges from NMEA2000 PGN specifications
2. Confirm engineRev unit from NMEA2000 Engine Parameters PGN (RPM standard)
3. Define sensor-specific refresh rates based on protocol update frequencies
4. Design HAL interfaces for new data structures
5. Plan data migration strategy from SpeedData to DSTData
