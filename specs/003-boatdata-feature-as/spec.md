# Feature Specification: BoatData - Centralized Marine Data Model

**Feature Branch**: `003-boatdata-feature-as`
**Created**: 2025-10-06
**Status**: Draft
**Input**: User description: "BoatData feature as described in the file user_requirements/R002 - boatdata.md"

## Execution Flow (main)
```
1. Parse user description from Input
   → Feature description parsed: centralized boat data model with sensor prioritization
2. Extract key concepts from description
   → Actors: Marine sensors (NMEA0183, NMEA2000, 1-Wire), system calculators, boat operators
   → Actions: Store sensor data, prioritize sources, calculate derived parameters, validate readings
   → Data: GPS, compass, wind, speed, rudder, calculated sailing parameters
   → Constraints: 200ms calculation cycle, multi-source prioritization, outlier detection
3. For each unclear aspect:
   → [NEEDS CLARIFICATION: Data persistence requirements - in-memory only or persistent storage?]
   → [NEEDS CLARIFICATION: Web interface authentication/authorization requirements]
   → [NEEDS CLARIFICATION: Expected data update rates for different sensor types]
   → [NEEDS CLARIFICATION: Historical data retention requirements]
4. Fill User Scenarios & Testing section
   ✓ User scenarios defined for sensor data updates, source prioritization, calibration
5. Generate Functional Requirements
   ✓ Requirements generated covering data model, prioritization, calculations, validation
6. Identify Key Entities
   ✓ Entities identified: Boat data elements, sensor sources, calibration parameters
7. Run Review Checklist
   → WARN "Spec has uncertainties - clarification markers present"
8. Return: SUCCESS (spec ready for planning after clarification)
```

---

## ⚡ Quick Guidelines
- ✅ Focus on WHAT users need and WHY
- ❌ Avoid HOW to implement (no tech stack, APIs, code structure)
- 👥 Written for business stakeholders, not developers

---

## Clarifications

### Session 2025-10-06
- Q: What data should persist across system reboots? → A: Only calibration parameters (K factor, wind angle offset)
- Q: What is the maximum age threshold for considering sensor data stale? → A: 5 seconds (conservative, avoids flapping between sources)
- Q: What authentication/authorization is required for the calibration web interface? → A: Open access (no authentication) - suitable for private boat network
- Q: What should happen if a calculation cycle takes longer than 200ms? → A: Skip the cycle, log warning, continue with next cycle
- Q: What are the valid ranges for calibration parameters? → A: K factor: any positive value, Wind offset: -2π to 2π radians

---

## User Scenarios & Testing *(mandatory)*

### Primary User Story
As a boat operator, I need a centralized system that collects data from multiple marine sensors (GPS, compass, wind vane, speed sensors), prioritizes data from the most reliable sources, calculates derived sailing parameters (true wind speed, leeway, VMG), and allows me to calibrate the system through a simple web interface so that I can make informed sailing decisions based on accurate real-time data.

### Acceptance Scenarios

#### Scenario 1: Single Source GPS Data
1. **Given** the system has one GPS sensor connected via NMEA0183
   **When** the GPS sensor sends position data (latitude/longitude)
   **Then** the system stores and makes available the current position data

#### Scenario 2: Multi-Source GPS with Automatic Prioritization
1. **Given** the system has:
   - GPS-A (NMEA0183) updating at 1 Hz (once per second)
   - GPS-B (NMEA2000) updating at 10 Hz (ten times per second)
   **When** both sensors are providing valid data
   **Then** the system uses GPS-B data (higher update frequency = higher priority)

#### Scenario 3: Source Failover
1. **Given** the system is using GPS-B (10 Hz) as the primary source
   **When** GPS-B stops sending data
   **Then** the system automatically switches to GPS-A (1 Hz) as the fallback source

#### Scenario 4: User Priority Override
1. **Given** the system has auto-prioritized GPS-B over GPS-A
   **When** the user manually sets GPS-A as the preferred source via configuration
   **Then** the system uses GPS-A regardless of update frequency

#### Scenario 5: Derived Parameter Calculation
1. **Given** the system has valid data from:
   - GPS (latitude, longitude, COG, SOG)
   - Compass (heading, variation)
   - Wind vane (apparent wind angle, apparent wind speed)
   - Speed sensor (boat speed, heel angle)
   **When** the calculation cycle triggers (every 200ms)
   **Then** the system calculates and updates all derived parameters:
   - True wind speed (TWS)
   - True wind angle (TWA)
   - Speed through water (STW)
   - Velocity made good (VMG)
   - Wind direction (WDIR)
   - Current speed and direction (SOC, DOC)
   - Leeway angle

#### Scenario 6: Calibration Parameter Update
1. **Given** the user accesses the web calibration interface
   **When** the user updates the "Leeway Calibration Factor" from 0.5 to 0.65
   **Then** the system persists the new calibration value and uses it in subsequent calculations

#### Scenario 7: Outlier Detection
1. **Given** the GPS sensor has been reporting valid position data (latitude 40.7128°N)
   **When** the GPS sensor sends an invalid reading (latitude 200°N - outside valid range)
   **Then** the system rejects the outlier and continues using the last valid position

### Edge Cases
- **What happens when all sources for a sensor type fail?** System should mark that data type as unavailable and continue operating with degraded functionality.
- **What happens when sensor data is partially valid?** System should accept valid fields and ignore invalid fields within the same message.
- **How does the system handle initial startup with no data?** All data fields should have a clear "not available" state until first valid reading received.
- **What happens during calibration updates while calculations are running?** Calibration changes should be applied atomically to prevent calculation errors.
- **How does the system detect a "stale" data source?** Data is considered stale if no update received within 5 seconds, triggering automatic failover to next-priority source.
- **What happens when compass heading wraps around (359° → 0°)?** Calculations must handle angle wraparound correctly.
- **How are negative angles represented (port vs starboard)?** Consistently use radians with negative/positive for port/starboard.

## Requirements *(mandatory)*

### Functional Requirements

#### Data Storage and Access
- **FR-001**: System MUST store all raw sensor data elements:
  - GPS: latitude, longitude, COG (course over ground), SOG (speed over ground)
  - Compass: true heading, magnetic heading, magnetic variation
  - Wind vane: apparent wind angle (AWA), apparent wind speed (AWS)
  - Paddle wheel: heel angle, measured boat speed
  - Rudder: steering angle

- **FR-002**: System MUST store all calculated/derived sailing parameters:
  - Corrected apparent wind angles (AWA_Offset, AWA_Heel)
  - True wind measurements (TWS, TWA, WDIR)
  - Speed calculations (STW, VMG)
  - Current calculations (SOC, DOC)
  - Leeway angle

- **FR-003**: System MUST store calibration parameters:
  - Leeway calibration factor (K)
  - Wind angle offset (masthead misalignment correction)

- **FR-004**: System MUST store diagnostic information (RAM-only, resets on reboot):
  - NMEA0183 message count
  - NMEA2000 message count
  - Actisense message count
  - Timestamp of last calculation update

- **FR-005**: System MUST provide read access to all stored data elements for other components (e.g., display, logging, network transmission)

- **FR-006**: System MUST provide write access for interface message handlers to update raw sensor data without knowledge of internal storage details

#### Multi-Source Data Prioritization
- **FR-007**: System MUST support multiple data sources for GPS data (latitude, longitude, COG, SOG)

- **FR-008**: System MUST support multiple data sources for compass data (heading, variation)

- **FR-009**: System MUST automatically prioritize data sources based on update frequency (higher frequency = higher priority)

- **FR-010**: System MUST use data only from the highest-priority available source at any given time

- **FR-011**: System MUST automatically failover to next-priority source when the current source becomes unavailable (no updates received within 5 seconds)

- **FR-012**: System MUST allow users to override automatic prioritization and manually specify preferred sources

- **FR-013**: System MUST NOT persist user-configured source priorities (volatile, reset to automatic prioritization on reboot)

#### Data Validation and Outlier Detection
- **FR-014**: System MUST validate all incoming sensor readings for correctness (e.g., latitude must be -90° to +90°)

- **FR-015**: System MUST ignore invalid sensor readings (out of valid range)

- **FR-016**: System MUST detect and ignore outlier readings that deviate significantly from recent valid data

- **FR-017**: System MUST log rejected readings for diagnostic purposes via WebSocket logger

#### Derived Parameter Calculations
- **FR-018**: System MUST calculate all derived sailing parameters every 200 milliseconds

- **FR-019**: System MUST calculate AWA_Offset by applying wind angle offset calibration to raw apparent wind angle

- **FR-020**: System MUST calculate AWA_Heel by correcting AWA_Offset for heel angle effect

- **FR-021**: System MUST calculate leeway angle using the formula: `Leeway = K * HeelAngle / (MeasuredBoatSpeed^2)` where K is the calibration factor

- **FR-022**: System MUST calculate speed through water (STW) by correcting measured boat speed for leeway

- **FR-023**: System MUST calculate true wind speed (TWS) using vector addition of apparent wind and boat motion

- **FR-024**: System MUST calculate true wind angle (TWA) relative to boat heading

- **FR-025**: System MUST calculate velocity made good (VMG) as the component of STW in the direction of TWA

- **FR-026**: System MUST calculate magnetic wind direction (WDIR) by combining heading and TWA

- **FR-027**: System MUST calculate current speed (SOC) and direction (DOC) by comparing GPS data (COG, SOG) with water-referenced data (heading+leeway, STW)

- **FR-028**: System MUST handle mathematical edge cases (divide by zero, singularities) gracefully without crashing

- **FR-029**: System MUST update the LastUpdate timestamp whenever calculations complete

#### Calibration Interface
- **FR-030**: System MUST provide a web interface for updating calibration parameters

- **FR-031**: Users MUST be able to update the leeway calibration factor (K) via the web interface

- **FR-032**: Users MUST be able to update the wind angle offset via the web interface

- **FR-033**: System MUST validate calibration parameter inputs (K factor must be positive, wind angle offset must be -2π to 2π radians)

- **FR-034**: System MUST persist calibration parameters so they survive system reboots

- **FR-035**: System MUST apply updated calibration parameters immediately in subsequent calculations

- **FR-036**: Calibration web interface MUST be accessible without authentication (assumes secure private boat network)

#### Integration Requirements
- **FR-037**: System MUST provide an interface for NMEA0183 message handlers to update boat data without coupling to internal implementation

- **FR-038**: System MUST provide an interface for NMEA2000 message handlers to update boat data without coupling to internal implementation

- **FR-039**: System MUST provide an interface for 1-Wire sensor handlers to update boat data without coupling to internal implementation

- **FR-040**: System MUST NOT depend on specific NMEA or 1-Wire interface implementations (those are separate features)

#### Units and Conventions
- **FR-041**: System MUST store all angles in radians (except where degrees specified in requirement)

- **FR-042**: System MUST use the following angle conventions:
  - Headings/bearings: 0 to 2π (0 to 360°), where 0 = North
  - Wind angles: -π to π (-180° to 180°), where positive = starboard, negative = port
  - Heel angle: positive = starboard, negative = port
  - Rudder angle: positive = starboard, negative = port

- **FR-043**: System MUST store all speeds in knots

- **FR-044**: System MUST store GPS coordinates in decimal degrees (latitude: positive = North, longitude: positive = East)

### Non-Functional Requirements

- **NFR-001**: Calculation cycle MUST complete within 200 milliseconds; if exceeded, skip that cycle, log warning via WebSocket logger, and continue with next scheduled cycle

- **NFR-002**: System MUST operate continuously (24/7) without memory leaks or performance degradation (use static memory allocation for boat data storage)

- **NFR-003**: System MUST maintain data consistency during concurrent updates from multiple sensor interfaces (concurrency model to be defined in planning phase)

- **NFR-004**: System MUST persist only calibration parameters (K factor, wind angle offset) to flash storage; all other data (sensor readings, diagnostic counters, source priorities) is RAM-only and resets on reboot

### Key Entities *(data involved)*

#### BoatData Entity
**Purpose**: Central repository for all marine sensor data, calculated parameters, and diagnostic information.

**Attributes** (logical grouping, not implementation):
- **GPS Data**: latitude, longitude, COG, SOG
- **Compass Data**: true heading, magnetic heading, variation
- **Wind Data**: apparent wind angle, apparent wind speed, true wind angle, true wind speed, wind direction
- **Speed Data**: measured boat speed, speed through water, velocity made good
- **Attitude Data**: heel angle, rudder angle
- **Derived Parameters**: corrected wind angles, leeway, current speed/direction
- **Calibration Parameters**: leeway K factor, wind angle offset
- **Diagnostics**: message counts, last update timestamp

**Relationships**:
- Updated by NMEA0183 message handlers
- Updated by NMEA2000 message handlers
- Updated by 1-Wire sensor handlers
- Read by display components
- Read by logging components
- Read by network transmission components
- Read by calculation engine

#### SensorSource Entity
**Purpose**: Represents a data source for a specific sensor type (e.g., GPS-A via NMEA0183, GPS-B via NMEA2000).

**Attributes**:
- Source identifier
- Sensor type (GPS, compass)
- Protocol type (NMEA0183, NMEA2000, 1-Wire)
- Update frequency (Hz)
- Priority level (automatic or user-configured)
- Last update timestamp
- Availability status

**Relationships**:
- Multiple sources can provide the same sensor type
- One source is active (highest priority available) at any time per sensor type
- Sources are ranked by priority for failover

#### CalibrationParameters Entity
**Purpose**: Stores user-configurable calibration values used in calculations.

**Attributes**:
- Leeway calibration factor (K) - any positive value
- Wind angle offset (radians) - range: -2π to 2π

**Constraints**:
- K factor must be > 0
- Wind offset must be -2π ≤ offset ≤ 2π

**Relationships**:
- Persisted to non-volatile storage (flash filesystem)
- Read by calculation engine every 200ms cycle
- Updated via web interface (no authentication required)

---

## Review & Acceptance Checklist
*GATE: Automated checks run during main() execution*

### Content Quality
- [x] No implementation details (languages, frameworks, APIs)
- [x] Focused on user value and business needs
- [x] Written for non-technical stakeholders
- [x] All mandatory sections completed

### Requirement Completeness
- [x] No [NEEDS CLARIFICATION] markers remain - **All critical clarifications resolved**
- [x] Requirements are testable and unambiguous
- [x] Success criteria are measurable
- [x] Scope is clearly bounded
- [x] Dependencies and assumptions identified

**Clarifications Resolved (Session 2025-10-06):**
1. ✅ Data persistence: Only calibration parameters (K factor, wind offset)
2. ✅ Stale data threshold: 5 seconds
3. ✅ Web interface auth: Open access (no authentication)
4. ✅ Calculation overrun: Skip cycle, log warning
5. ✅ Calibration ranges: K > 0, wind offset -2π to 2π

**Deferred to Planning Phase:**
- Concurrency model (RTOS tasks vs single-threaded event loop)
- Expected sensor update rates (implementation detail)
- Additional calibration parameters (out of scope for v1)

---

## Execution Status
*Updated by main() during processing*

- [x] User description parsed
- [x] Key concepts extracted
- [x] Ambiguities marked (8 clarification items)
- [x] User scenarios defined
- [x] Requirements generated (44 functional, 4 non-functional)
- [x] Entities identified (3 entities)
- [x] Review checklist passed (with clarification warnings)

---

## Next Steps

1. **Clarification Phase** (`/clarify`): Address the 8 marked uncertainties before proceeding to implementation planning
2. **Planning Phase** (`/plan`): Once clarifications resolved, generate technical implementation plan
3. **Task Generation** (`/tasks`): Create dependency-ordered task breakdown
4. **Implementation** (`/implement`): Execute tasks with QA review gates
