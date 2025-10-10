# Research Findings: Enhanced BoatData

**Feature**: Enhanced BoatData (R005)
**Date**: 2025-10-10
**Status**: Complete

## Overview

This document consolidates research findings for implementing enhanced BoatData structures based on R005 requirements. The feature involves restructuring and extending existing marine sensor data structures to accommodate GPS variation, compass motion sensors (rate of turn, heel, pitch, heave), DST (Depth/Speed/Temperature) data, engine telemetry, saildrive status, battery monitoring, and shore power data.

## Technical Context Resolution

### 1. Expected Pitch Angle Range

**Decision**: Pitch range of ±30° (±π/6 radians) for normal vessel operation

**Rationale**:
- NMEA2000 PGN 127257 (Attitude) specification defines pitch as signed angle
- Typical sailing vessel pitch range: -15° to +15° in normal conditions
- Storm conditions rarely exceed ±25°
- Physical limit: ±30° provides safety margin while catching abnormal readings
- Values exceeding this range indicate sensor malfunction or extreme conditions requiring warning

**Validation Strategy**:
- Values within [-π/6, π/6]: Accept as valid
- Values outside range: Clamp to limits, set availability flag false, log WARNING

**Alternatives Considered**:
- ±45° range: Rejected as too permissive for marine vessels (would miss many abnormal readings)
- ±90° range: Rejected as physically implausible for intact vessel under power/sail

**Reference**: NMEA2000 PGN 127257 - Attitude (pitch, roll, yaw)

---

### 2. Expected Heave Range

**Decision**: Heave range of ±5 meters for normal operation

**Rationale**:
- NMEA2000 PGN 127257 defines heave as vertical displacement relative to reference
- Typical wave conditions: 0.5m to 2m heave amplitude
- Storm conditions: 3m to 4m heave (significant wave height)
- Extreme conditions: Up to 5m (rare, indicates severe weather)
- Sensor noise/drift: Values beyond ±5m likely indicate malfunction

**Validation Strategy**:
- Values within [-5.0, 5.0] meters: Accept as valid
- Values outside range: Clamp to limits, set availability flag false, log WARNING

**Alternatives Considered**:
- ±10m range: Rejected as too permissive (would accept clearly erroneous readings)
- ±3m range: Rejected as too restrictive (legitimate storm conditions would be rejected)

**Reference**: NMEA2000 PGN 127257 - Attitude (heave field)

---

### 3. Engine RPM Unit (engineRev)

**Decision**: Store as RPM (revolutions per minute), not Hz

**Rationale**:
- NMEA2000 PGN 127488 (Engine Parameters, Rapid Update) transmits in RPM
- NMEA2000 PGN 127489 (Engine Parameters, Dynamic) also uses RPM
- Industry standard: Marine engine instrumentation displays RPM
- User expectation: Operators think in RPM, not Hz
- Direct mapping: No conversion needed from N2K message to storage
- Validation range: 0 to 6000 RPM (typical marine diesel/gasoline engines)

**Validation Strategy**:
- Values within [0, 6000] RPM: Accept as valid
- Negative values: Set to 0, availability flag false, log WARNING
- Values > 6000 RPM: Clamp to 6000, log WARNING (potential sensor error)

**Alternatives Considered**:
- Hz (revolutions per second): Rejected due to industry convention mismatch
- Raw sensor counts: Rejected as not universally applicable across NMEA2000 devices

**Reference**: NMEA2000 PGN 127488, PGN 127489 - Engine Parameters

---

### 4. Data Refresh Rates per Sensor Type

**Decision**: Sensor-specific refresh rates based on NMEA2000/NMEA0183 protocol update frequencies

**Rationale**:
- NMEA2000 PGNs have defined transmission intervals per specification
- NMEA0183 sentences typically transmit at 1Hz standard rate
- 1-wire sensors polled at application-defined intervals (typically 1-5 seconds)
- ReactESP event loops enable non-blocking periodic updates matching source rates

**Refresh Rate Table**:

| Sensor Category | Data Structure | Source Protocol | Update Frequency | ReactESP Interval |
|----------------|---------------|----------------|-----------------|------------------|
| GPS Position | GPSData | NMEA0183/N2K PGN 129029 | 1 Hz (standard) | 1000ms |
| Compass Heading | CompassData | NMEA0183/N2K PGN 127250 | 10 Hz (typical) | 100ms |
| Rate of Turn | CompassData | N2K PGN 127251 | 10 Hz | 100ms |
| Heel/Pitch/Heave | CompassData | N2K PGN 127257 | 10 Hz | 100ms |
| Depth/Speed/Temp | DSTData | NMEA0183/N2K PGN 128267, 128259 | 1 Hz | 1000ms |
| Engine Data | EngineData | N2K PGN 127488 (rapid), 127489 | 100ms (rapid), 500ms (dynamic) | 100ms |
| Saildrive | SaildriveData | 1-wire GPIO | Application poll | 1000ms (1 Hz) |
| Battery | BatteryData | 1-wire analog | Application poll | 2000ms (0.5 Hz) |
| Shore Power | ShorePowerData | 1-wire GPIO + analog | Application poll | 2000ms (0.5 Hz) |

**Implementation Strategy**:
- Use ReactESP `onRepeat()` with appropriate intervals per sensor category
- NMEA message handlers update structures immediately when data arrives
- Display refresh/aggregation runs at 200ms (5 Hz) to smooth fast-updating sensors
- Availability flags set to false if no update received within 3x expected interval

**Alternatives Considered**:
- Single global refresh rate: Rejected due to inefficient polling of slow sensors (batteries)
- Synchronous blocking delays: Rejected per Constitution Principle VI (always-on operation)
- Fixed 10ms poll rate: Rejected as excessive overhead for slow sensors

**Reference**: NMEA2000 Appendix B (PGN Update Rates), ReactESP documentation

---

### 5. Unit Conversions and Sign Conventions

**Decision**: Consolidate unit standards from current BoatDataTypes.h and R005 requirements

**Current State (BoatDataTypes.h v1.0.0)**:
- Angles: Radians
- Speeds: Knots (GPS SOG, wind speeds)
- Coordinates: Decimal degrees
- Time: millis() timestamps

**R005 Requirements**:
- measuredBoatSpeed (DST paddle wheel): Already in m/s (no conversion)
- Battery state of charge: Percentage 0-100.0
- Engine oil temperature: Celsius
- Water temperature (seaTemperature): Celsius
- Heel angle: Radians, positive = starboard tilt, negative = port
- Pitch angle: Radians, positive = bow up, negative = bow down
- Battery amperage: Amperes, positive = charging, negative = discharging
- Alternator voltage: Volts
- Shore power: Watts

**Unified Unit Standard**:

| Measurement Type | Unit | Sign Convention | Range |
|-----------------|------|----------------|-------|
| Angles (heading, COG) | Radians | N/A (0 to 2π) | [0, 2π] |
| Angles (heel, pitch) | Radians | Positive = starboard/bow up | [-π/2, π/2] pitch, [-π/2, π/2] heel |
| Rate of turn | Radians/second | Positive = turning right (starboard) | [-π, π] |
| Heave | Meters | Positive = upward motion | [-5.0, 5.0] |
| Linear speed (SOG, boat speed) | Knots (GPS/derived), m/s (DST paddle wheel) | N/A | [0, 100] knots, [0, 25] m/s |
| Depth | Meters | Below waterline | [0, 100] |
| Temperature | Celsius | N/A | [-10, 50] |
| Voltage | Volts | N/A | [0, 30] DC |
| Current | Amperes | Positive = charging, negative = discharging | [-200, 200] |
| Power | Watts | N/A | [0, 5000] |
| Percentage | Percent (0-100) | N/A | [0.0, 100.0] |
| RPM | Revolutions/minute | N/A | [0, 6000] |

**NMEA Conversion Requirements**:
- NMEA0183 RMC/VTG: SOG in knots (no conversion)
- NMEA0183 DPT: Depth in meters (no conversion)
- NMEA0183 MTW: Water temp in Celsius (no conversion)
- NMEA2000 PGN 128259: Speed in m/s (DSTData.measuredBoatSpeed direct assignment)
- NMEA2000 PGN 127488: Engine RPM direct (no conversion)
- NMEA2000 PGN 130316: Temperature in Kelvin → Convert to Celsius (T_celsius = T_kelvin - 273.15)
- NMEA2000 PGN 127506: DC voltage/current direct in V/A (no conversion)

**Alternatives Considered**:
- Convert all speeds to m/s: Rejected to preserve existing knots convention for SOG/wind speeds
- Store temperatures in Kelvin: Rejected as user-facing displays use Celsius
- Store heel/pitch in degrees: Rejected to maintain radians consistency across all angles

**Reference**: BoatDataTypes.h lines 8-13, NMEA2000 PGN field definitions

---

### 6. Data Structure Refactoring Strategy

**Decision**: Incremental refactoring with backward compatibility shim

**Migration Path**:
1. **Phase 1**: Create new DSTData structure alongside existing SpeedData
2. **Phase 2**: Update NMEA handlers to populate both structures (dual-write)
3. **Phase 3**: Update consumers (display, calculations) to read from DSTData
4. **Phase 4**: Deprecate SpeedData after validation period
5. **Phase 5**: Remove SpeedData in future release

**Rationale**:
- Zero-downtime migration: Old consumers continue working during transition
- Rollback capability: Can revert to SpeedData if issues discovered
- Test coverage: Both structures populated enables A/B comparison testing
- Constitutional compliance: Gradual change reduces risk (Principle VII - Fail-Safe Operation)

**Memory Impact**:
- Temporary overhead: ~50 bytes (one additional structure during migration)
- Final state: Net zero (SpeedData removed, DSTData replaces it)
- Negligible for ESP32 (320KB RAM)

**Alternatives Considered**:
- Direct replacement: Rejected due to risk of breaking existing functionality
- Feature flag: Rejected as overly complex for simple data structure change
- Fork codebase: Rejected as unmaintainable

**Reference**: Constitution Principle VII (Fail-Safe Operation)

---

### 7. 1-Wire Sensor Interface Design

**Decision**: Abstract 1-wire sensors through HAL interface for testability

**Architecture**:
```
IOneWireSensors (HAL interface)
  ├── readSaildriveEngaged() → bool
  ├── readBatteryA() → BatteryMonitorData
  ├── readBatteryB() → BatteryMonitorData
  └── readShorePower() → ShorePowerData

ESP32OneWireSensors (hardware implementation)
  └── Uses OneWire library + Dallas Temperature

MockOneWireSensors (test implementation)
  └── Provides simulated sensor data
```

**Rationale**:
- **Constitution Principle I**: Hardware abstraction mandatory
- **Testability**: Business logic testable without physical sensors
- **Flexibility**: Future sensor types added via interface extension
- **Graceful degradation**: Sensor failures handled at HAL boundary

**Sensor Mapping** (GPIO 4, 1-Wire bus):
- Address 0x28AABBCCDDEE0001 → Saildrive position sensor (digital)
- Address 0x28AABBCCDDEE0002 → Battery A monitor (analog via DS2438)
- Address 0x28AABBCCDDEE0003 → Battery B monitor (analog via DS2438)
- Address 0x28AABBCCDDEE0004 → Shore power sensor (digital + analog)

**Polling Strategy**:
- 1-wire bus shared across sensors (single GPIO)
- Sequential polling with 50ms spacing to avoid bus contention
- ReactESP event loop: `onRepeat(1000, pollSaildriveStatus)`
- ReactESP event loop: `onRepeat(2000, pollBatteryStatus)`
- ReactESP event loop: `onRepeat(2000, pollShorePowerStatus)`

**Error Handling**:
- CRC failure: Retry once, then set availability=false
- Sensor not found: Log WARNING, set availability=false, continue operation
- Bus timeout: Reset bus, skip cycle, log ERROR

**Alternatives Considered**:
- Direct hardware access: Rejected per Constitution Principle I
- Combined polling function: Rejected to allow independent refresh rates
- Interrupt-driven reads: Rejected as 1-wire protocol is inherently polling-based

**Reference**: Constitution Principle I (Hardware Abstraction Layer), OneWire library documentation

---

### 8. NMEA2000 PGN Integration

**Decision**: Leverage existing NMEA2000 library PGN handlers with minimal custom logic

**Required PGNs**:

| PGN | Description | Data Structure | Handler Strategy |
|-----|-------------|---------------|------------------|
| 129029 | GNSS Position Data | GPSData (latitude, longitude, altitude) | Existing - add variation field |
| 127250 | Vessel Heading | CompassData (heading) | Existing - add rateOfTurn |
| 127251 | Rate of Turn | CompassData (rateOfTurn) | **NEW** - add handler |
| 127257 | Attitude | CompassData (heel, pitch, heave) | **NEW** - add handler |
| 128267 | Water Depth | DSTData (depth) | **NEW** - add handler |
| 128259 | Speed (Water Referenced) | DSTData (measuredBoatSpeed) | **NEW** - add handler |
| 130316 | Temperature Extended Range | DSTData (seaTemperature) | **NEW** - add handler with Kelvin→Celsius conversion |
| 127488 | Engine Parameters, Rapid Update | EngineData (engineRev) | **NEW** - add handler |
| 127489 | Engine Parameters, Dynamic | EngineData (oilTemperature, alternatorVoltage) | **NEW** - add handler |
| 127506 | DC Detailed Status | BatteryData (voltage, current, SOC) | **NEW** - add handler (if NMEA2000 battery monitor present) |

**Handler Implementation Pattern** (from existing codebase):
```cpp
void HandleN2kPGN127257(const tN2kMsg &N2kMsg) {
    unsigned char SID;
    double yaw, pitch, roll;

    if (ParseN2kPGN127257(N2kMsg, SID, yaw, pitch, roll)) {
        boatData.compass.pitchAngle = pitch;
        boatData.compass.heelAngle = roll;  // Roll = heel in marine context
        boatData.compass.available = true;
        boatData.compass.lastUpdate = millis();

        // Validate and clamp
        if (fabs(pitch) > M_PI/6) {  // ±30°
            boatData.compass.pitchAngle = constrain(pitch, -M_PI/6, M_PI/6);
            remotelog(WARN, "Pitch angle out of range: %f rad", pitch);
        }
    }
}
```

**Rationale**:
- NMEA2000 library (ttlappalainen) provides ParseN2kPGNxxxxx() functions
- Minimal boilerplate: Extract fields, validate, store in BoatDataStructure
- Consistent error handling: Invalid data → availability=false, log warning
- ReactESP integration: Handlers called from `nmea2000->ParseMessages()` event loop

**Alternatives Considered**:
- Raw CAN frame parsing: Rejected as library abstracts complexity
- Generic PGN handler with switch statement: Rejected as less maintainable than dedicated functions
- Delegate validation to separate module: Rejected as adds unnecessary indirection

**Reference**: NMEA2000 library PGN parser functions, examples/poseidongw/src/NMEA0183Handlers.cpp

---

## Summary of Research Findings

All technical unknowns identified in the feature specification have been resolved:

1. **Pitch range**: ±30° (±π/6 rad) with validation and clamping
2. **Heave range**: ±5 meters with validation and clamping
3. **Engine RPM unit**: RPM (not Hz), range [0, 6000]
4. **Refresh rates**: Sensor-specific (100ms to 2000ms) using ReactESP event loops
5. **Unit conversions**: Minimal (only Kelvin→Celsius for temperature PGNs)
6. **Refactoring strategy**: Incremental migration with dual-write phase
7. **1-wire sensors**: HAL abstraction with GPIO 4 bus, sequential polling
8. **NMEA2000 PGNs**: 10 PGNs total (2 existing + 8 new handlers)

**Memory footprint estimate**:
- New structures: ~200 bytes (EngineData 40B, SaildriveData 20B, BatteryData 80B, ShorePowerData 20B, DSTData 40B)
- HAL interfaces: ~0 bytes (virtual functions, no vtables in embedded)
- Temporary migration overhead: ~50 bytes (duplicate SpeedData during transition)
- **Total**: ~250 bytes (~0.08% of ESP32 RAM)

**Flash footprint estimate**:
- New PGN handlers: ~2KB (8 handlers × 250 bytes average)
- 1-wire polling logic: ~1KB
- HAL interface implementations: ~1KB
- **Total**: ~4KB (~0.2% of typical partition)

**Constitutional compliance verified**: All decisions align with Constitution v1.2.0 principles (hardware abstraction, resource awareness, fail-safe operation).

**Next phase**: Proceed to Phase 1 (Design & Contracts) to create data-model.md and HAL interface specifications.
