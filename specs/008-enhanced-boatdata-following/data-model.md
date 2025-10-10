# Data Model: Enhanced BoatData

**Feature**: Enhanced BoatData (R005)
**Date**: 2025-10-10
**Status**: Design Complete

## Overview

This document defines the complete data model for enhanced marine sensor data structures. The design refactors existing structures (SpeedData → DSTData, CompassData enhancements, GPSData enhancements) and introduces new structures for engine telemetry, saildrive status, battery monitoring, and shore power.

## Design Principles

1. **Static Allocation**: All structures use fixed-size fields (no dynamic memory)
2. **Availability Flags**: Every structure includes `bool available` and `unsigned long lastUpdate`
3. **Unit Consistency**: Follow established conventions from BoatDataTypes.h v1.0.0
4. **Validation at Boundary**: HAL implementations validate and clamp values before storing
5. **Sign Conventions**: Explicitly documented for all signed measurements

## Entity Definitions

### 1. GPSData (Enhanced)

**Purpose**: Navigation position data from NMEA0183/NMEA2000 GPS receivers

**Changes from Current**:
- **Added field**: `double variation` (magnetic variation from GPS, moved from CompassData)

**Rationale**: GPS receivers provide magnetic variation as part of position fix data (NMEA0183 RMC sentence, NMEA2000 PGN 129029). This field logically belongs with GPS data, not compass data.

```cpp
struct GPSData {
    // Position
    double latitude;           // Decimal degrees, positive = North, range [-90, 90]
    double longitude;          // Decimal degrees, positive = East, range [-180, 180]

    // Motion
    double cog;                // Course over ground, radians, range [0, 2π], true
    double sog;                // Speed over ground, knots, range [0, 100]

    // Magnetic variation (ADDED)
    double variation;          // Magnetic variation, radians, positive = East, negative = West

    // Metadata
    bool available;            // Data validity flag
    unsigned long lastUpdate;  // millis() timestamp of last update
};
```

**Validation Rules**:
- `latitude`: Clamp to [-π/2, π/2] radians ([-90°, 90°])
- `longitude`: Wrap to [-π, π] radians ([-180°, 180°])
- `cog`: Wrap to [0, 2π]
- `sog`: Clamp to [0, 100] knots
- `variation`: Clamp to [-π/4, π/4] radians ([-45°, 45°], typical global range)
- Set `available = false` if any validation fails

**Memory Footprint**: 56 bytes (7 doubles × 8B)

---

### 2. CompassData (Enhanced)

**Purpose**: Vessel heading and motion sensor data from NMEA2000 compass/attitude sensors

**Changes from Current**:
- **Removed field**: `double variation` (moved to GPSData)
- **Added field**: `double rateOfTurn` (radians/second)
- **Added field**: `double heelAngle` (radians, moved from SpeedData)
- **Added field**: `double pitchAngle` (radians, bow up/down)
- **Added field**: `double heave` (meters, vertical motion)

```cpp
struct CompassData {
    // Heading
    double trueHeading;        // Radians, range [0, 2π]
    double magneticHeading;    // Radians, range [0, 2π]

    // Rate of change (ADDED)
    double rateOfTurn;         // Radians/second, positive = turning right (starboard)

    // Vessel attitude (ADDED)
    double heelAngle;          // Radians, range [-π/2, π/2], positive = starboard tilt
    double pitchAngle;         // Radians, range [-π/6, π/6], positive = bow up
    double heave;              // Meters, range [-5.0, 5.0], positive = upward motion

    // Metadata
    bool available;            // Data validity flag
    unsigned long lastUpdate;  // millis() timestamp of last update
};
```

**Validation Rules**:
- `trueHeading`, `magneticHeading`: Wrap to [0, 2π]
- `rateOfTurn`: Clamp to [-π, π] rad/s (unlikely to exceed ±180°/s)
- `heelAngle`: Clamp to [-π/2, π/2], warn if exceeds ±π/4 (±45°)
- `pitchAngle`: Clamp to [-π/6, π/6] (±30°), warn if outside range
- `heave`: Clamp to [-5.0, 5.0] meters, warn if outside range
- Set `available = false` if critical validation fails

**Source Mapping**:
- NMEA2000 PGN 127250: Vessel Heading → `trueHeading`, `magneticHeading`
- NMEA2000 PGN 127251: Rate of Turn → `rateOfTurn`
- NMEA2000 PGN 127257: Attitude → `heelAngle` (roll), `pitchAngle`, `heave`
- NMEA0183 HDM: Heading Magnetic → `magneticHeading`

**Memory Footprint**: 64 bytes (8 doubles × 8B)

---

### 3. DSTData (NEW - Replaces SpeedData)

**Purpose**: Depth, Speed, Temperature sensor data (marine "DST" triducer concept)

**Rationale**:
- Renames `SpeedData` to better reflect actual sensor measurements (DST = Depth/Speed/Temperature)
- Groups related underwater sensor readings (paddle wheel speed, depth sounder, water temp)
- Removes `heelAngle` (moved to CompassData where it logically belongs)

```cpp
struct DSTData {
    // Depth sounder
    double depth;              // Depth below waterline, meters, range [0, 100]

    // Paddle wheel speed sensor
    double measuredBoatSpeed;  // Speed through water, m/s, range [0, 25]

    // Water temperature sensor
    double seaTemperature;     // Water temperature, Celsius, range [-10, 50]

    // Metadata
    bool available;            // Data validity flag
    unsigned long lastUpdate;  // millis() timestamp of last update
};
```

**Validation Rules**:
- `depth`: Clamp to [0, 100] meters, reject negative values (sensor above water)
- `measuredBoatSpeed`: Clamp to [0, 25] m/s (~50 knots max)
- `seaTemperature`: Clamp to [-10, 50] Celsius (ice to tropical)
- Set `available = false` if any validation fails

**Source Mapping**:
- NMEA2000 PGN 128267: Water Depth → `depth`
- NMEA2000 PGN 128259: Speed (Water Referenced) → `measuredBoatSpeed` (in m/s)
- NMEA2000 PGN 130316: Temperature Extended Range → `seaTemperature` (convert from Kelvin)
- NMEA0183 DPT: Depth → `depth`
- NMEA0183 VHW: Speed Through Water → `measuredBoatSpeed`
- NMEA0183 MTW: Water Temperature → `seaTemperature`

**Migration from SpeedData**:
- **Removed**: `heelAngle` (now in CompassData)
- **Added**: `depth`, `seaTemperature`
- **Renamed structure**: `SpeedData` → `DSTData`

**Memory Footprint**: 40 bytes (5 doubles × 8B)

---

### 4. EngineData (NEW)

**Purpose**: Engine telemetry from NMEA2000 engine sensors

**Rationale**: Propulsion system monitoring requires dedicated structure for engine parameters separate from navigation/attitude data.

```cpp
struct EngineData {
    // Engine performance
    double engineRev;          // Engine RPM, range [0, 6000]

    // Engine temperature
    double oilTemperature;     // Oil temperature, Celsius, range [-10, 150]

    // Electrical system
    double alternatorVoltage;  // Alternator output, volts, range [0, 30]

    // Metadata
    bool available;            // Data validity flag
    unsigned long lastUpdate;  // millis() timestamp of last update
};
```

**Validation Rules**:
- `engineRev`: Clamp to [0, 6000] RPM, warn if exceeds redline (engine-specific)
- `oilTemperature`: Clamp to [-10, 150] Celsius, warn if exceeds 120°C (overheat)
- `alternatorVoltage`: Clamp to [0, 30] volts DC, warn if outside [12, 15] (12V system) or [24, 30] (24V system)
- Set `available = false` if sensor offline or data invalid

**Source Mapping**:
- NMEA2000 PGN 127488: Engine Parameters, Rapid Update → `engineRev`
- NMEA2000 PGN 127489: Engine Parameters, Dynamic → `oilTemperature`, `alternatorVoltage`

**Memory Footprint**: 40 bytes (5 doubles × 8B)

---

### 5. SaildriveData (NEW)

**Purpose**: Saildrive mechanism engagement status from 1-wire sensor

**Rationale**: Sailboats with saildrive propulsion need engagement monitoring to ensure drive is deployed before engine operation.

```cpp
struct SaildriveData {
    // Engagement status
    bool saildriveEngaged;     // true = saildrive deployed/engaged, false = retracted/disengaged

    // Metadata
    bool available;            // Data validity flag (sensor responding)
    unsigned long lastUpdate;  // millis() timestamp of last update
};
```

**Validation Rules**:
- `saildriveEngaged`: Direct boolean from digital sensor (no validation required)
- Set `available = false` if 1-wire sensor CRC check fails or sensor not found

**Source Mapping**:
- 1-wire digital sensor on GPIO 4 bus (device address TBD during hardware integration)

**Memory Footprint**: 12 bytes (2 bools × 1B + unsigned long × 8B + padding)

---

### 6. BatteryData (NEW)

**Purpose**: Dual battery bank monitoring from 1-wire analog sensors

**Rationale**: Marine electrical systems require monitoring of house and starter batteries, charger status, and current flow direction.

```cpp
struct BatteryData {
    // Battery A (House Bank)
    double voltageA;           // Volts, range [0, 30]
    double amperageA;          // Amperes, range [-200, 200], positive = charging, negative = discharging
    double stateOfChargeA;     // Percent, range [0.0, 100.0]
    bool shoreChargerOnA;      // true = shore charger active for Battery A
    bool engineChargerOnA;     // true = alternator charging Battery A

    // Battery B (Starter Bank)
    double voltageB;           // Volts, range [0, 30]
    double amperageB;          // Amperes, range [-200, 200], positive = charging, negative = discharging
    double stateOfChargeB;     // Percent, range [0.0, 100.0]
    bool shoreChargerOnB;      // true = shore charger active for Battery B
    bool engineChargerOnB;     // true = alternator charging Battery B

    // Metadata
    bool available;            // Data validity flag
    unsigned long lastUpdate;  // millis() timestamp of last update
};
```

**Validation Rules**:
- `voltageA/B`: Clamp to [0, 30] volts, warn if outside [10, 15] (12V system undercharge/overcharge)
- `amperageA/B`: Clamp to [-200, 200] amperes
- `stateOfChargeA/B`: Clamp to [0.0, 100.0] percent
- `shoreChargerOnA/B`, `engineChargerOnA/B`: Boolean flags (no validation)
- Set `available = false` if 1-wire sensor read fails

**Source Mapping**:
- 1-wire analog sensors on GPIO 4 bus (DS2438 or similar battery monitor ICs)
- Alternative: NMEA2000 PGN 127506 (DC Detailed Status) if N2K battery monitor present

**Sign Convention**:
- **Positive amperage**: Current flowing INTO battery (charging)
- **Negative amperage**: Current flowing OUT of battery (discharging)

**Memory Footprint**: 80 bytes (6 doubles × 8B + 4 bools × 1B + metadata)

---

### 7. ShorePowerData (NEW)

**Purpose**: Shore power connection status and consumption from 1-wire sensor

**Rationale**: Monitor AC shore power connection and power draw for safety and energy management.

```cpp
struct ShorePowerData {
    // Shore power status
    bool shorePowerOn;         // true = shore power connected and available

    // Power consumption
    double power;              // Shore power draw, watts, range [0, 5000]

    // Metadata
    bool available;            // Data validity flag
    unsigned long lastUpdate;  // millis() timestamp of last update
};
```

**Validation Rules**:
- `shorePowerOn`: Boolean flag (no validation)
- `power`: Clamp to [0, 5000] watts, warn if exceeds 3000W (typical 30A circuit limit)
- Set `available = false` if sensor read fails

**Source Mapping**:
- 1-wire digital sensor (connection status) + analog sensor (power measurement) on GPIO 4 bus

**Memory Footprint**: 20 bytes (1 double × 8B + 2 bools × 1B + metadata)

---

## Composite BoatDataStructure (Updated)

**Purpose**: Central statically-allocated data repository

**Changes**:
- Replace `SpeedData speed` with `DSTData dst`
- Remove `variation` from `CompassData` (moved to `GPSData`)
- Add `EngineData engine`
- Add `SaildriveData saildrive`
- Add `BatteryData battery`
- Add `ShorePowerData shorePower`

```cpp
struct BoatDataStructure {
    // Sensor Data
    GPSData gps;                   // GPS position and variation (enhanced)
    CompassData compass;           // Heading and attitude (enhanced)
    WindData wind;                 // Apparent wind (unchanged)
    DSTData dst;                   // Depth/Speed/Temperature (replaces SpeedData)
    RudderData rudder;             // Rudder angle (unchanged)
    EngineData engine;             // Engine telemetry (NEW)
    SaildriveData saildrive;       // Saildrive status (NEW)
    BatteryData battery;           // Dual battery monitoring (NEW)
    ShorePowerData shorePower;     // Shore power status (NEW)

    // Derived/Calculated Data
    CalibrationData calibration;   // Calibration parameters (unchanged)
    DerivedData derived;           // Calculated sailing params (unchanged)

    // Diagnostics
    DiagnosticData diagnostics;    // System counters (unchanged)
};
```

**Memory Footprint**:
- **Current (v1.0.0)**: ~304 bytes
- **Enhanced (v2.0.0)**:
  - GPSData: 56 bytes (+8 bytes for variation)
  - CompassData: 64 bytes (+32 bytes for rateOfTurn, heelAngle, pitchAngle, heave, -8 bytes for removed variation)
  - DSTData: 40 bytes (replaces SpeedData at 32 bytes, +8 bytes)
  - EngineData: 40 bytes (NEW)
  - SaildriveData: 12 bytes (NEW)
  - BatteryData: 80 bytes (NEW)
  - ShorePowerData: 20 bytes (NEW)
  - **Subtotal new structures**: 192 bytes
  - **Existing structures**: ~160 bytes (WindData, RudderData, CalibrationData, DerivedData, DiagnosticData)
  - **Total**: ~560 bytes (~0.17% of ESP32 RAM)

**Delta**: +256 bytes from v1.0.0 to v2.0.0 (acceptable per Constitution Principle II)

---

## Relationships and Dependencies

### Data Flow Diagram

```
[NMEA0183 Serial2] ──→ [NMEA0183Handlers] ──→ [BoatDataStructure.gps/compass/dst]
                                           ↓
[NMEA2000 CAN Bus] ──→ [N2K PGN Handlers] ──→ [BoatDataStructure.engine/compass/dst]
                                           ↓
[1-Wire GPIO 4]    ──→ [OneWireSensors]   ──→ [BoatDataStructure.saildrive/battery/shorePower]
                                           ↓
                            [BoatDataStructure] (global instance)
                                           ↓
                     ┌─────────────────────┼─────────────────────┐
                     ↓                     ↓                     ↓
              [SailingCalculations]  [DisplayManager]  [WebSocketLogger]
                     ↓                     ↓                     ↓
           [DerivedData updates]    [OLED display]     [Remote monitoring]
```

### Update Frequencies (from research.md)

| Structure | Primary Source | Update Interval | ReactESP Event |
|-----------|---------------|----------------|---------------|
| GPSData | NMEA0183/N2K | 1000ms (1 Hz) | `onRepeat(1000, ...)` |
| CompassData | NMEA0183/N2K | 100ms (10 Hz) | `onRepeat(100, ...)` |
| DSTData | NMEA0183/N2K | 1000ms (1 Hz) | `onRepeat(1000, ...)` |
| EngineData | NMEA2000 | 100ms (rapid PGN) | `onRepeat(100, ...)` |
| SaildriveData | 1-Wire | 1000ms (polled) | `onRepeat(1000, ...)` |
| BatteryData | 1-Wire | 2000ms (polled) | `onRepeat(2000, ...)` |
| ShorePowerData | 1-Wire | 2000ms (polled) | `onRepeat(2000, ...)` |

### Migration Strategy

**Backward Compatibility Shim** (temporary during transition):

```cpp
// In BoatDataTypes.h (during migration phase)
typedef DSTData SpeedData;  // Alias for backward compatibility

// Adapter accessors
inline double getHeelAngle(const BoatDataStructure& boat) {
    return boat.compass.heelAngle;  // Redirect from old SpeedData location
}
```

**Deprecation Timeline**:
1. **Release 2.0.0**: Introduce DSTData, add deprecation warnings for SpeedData accessors
2. **Release 2.1.0**: Remove SpeedData typedef, require consumers to use DSTData
3. **Release 2.2.0**: Full removal of legacy compatibility layer

---

## Validation and Error Handling

### Validation Levels

1. **HAL Layer** (hardware adapters):
   - Range checking and clamping
   - CRC/checksum validation (1-wire, NMEA)
   - Physical plausibility checks (e.g., RPM > 0)
   - Sets `available = false` on failure

2. **Business Logic Layer** (components):
   - Cross-sensor consistency checks (e.g., engine RPM vs. saildrive engagement)
   - Timeout detection (no updates within 3x expected interval)
   - Historical trend validation (sudden jumps indicate sensor failure)

3. **Display Layer** (formatters):
   - Graceful fallback to "---" or "N/A" when `available = false`
   - Visual warnings for out-of-range values

### Error Propagation

```cpp
// Example: PGN handler with validation
void HandleN2kPGN127488(const tN2kMsg &N2kMsg) {
    unsigned char engineInstance;
    double rpm, ...;

    if (ParseN2kPGN127488(N2kMsg, engineInstance, rpm, ...)) {
        // Validate RPM
        if (rpm < 0 || rpm > 6000) {
            remotelog(WARN, "Engine RPM out of range: %.0f", rpm);
            boatData.engine.available = false;
            return;
        }

        // Store valid data
        boatData.engine.engineRev = rpm;
        boatData.engine.available = true;
        boatData.engine.lastUpdate = millis();
    } else {
        // Parse failure
        remotelog(ERROR, "Failed to parse PGN 127488");
        boatData.engine.available = false;
    }
}
```

---

## Testing Strategy

### Contract Tests (HAL Interface Validation)

```cpp
// test_boatdata_contracts/test_idst_sensor.cpp
TEST(IDSTSensor, returns_valid_depth_range) {
    MockDSTSensor sensor;
    sensor.setDepth(10.5);  // meters

    DSTData data = sensor.readDepth();

    ASSERT_TRUE(data.available);
    ASSERT_GE(data.depth, 0.0);
    ASSERT_LE(data.depth, 100.0);
}

TEST(IDSTSensor, rejects_negative_depth) {
    MockDSTSensor sensor;
    sensor.setDepth(-5.0);  // Invalid (sensor above water)

    DSTData data = sensor.readDepth();

    ASSERT_FALSE(data.available);  // Should reject
}
```

### Integration Tests (Multi-Sensor Scenarios)

```cpp
// test_boatdata_integration/test_engine_saildrive_consistency.cpp
TEST(EngineSaildrive, warns_if_engine_running_but_saildrive_disengaged) {
    BoatDataStructure boat;
    boat.engine.engineRev = 1500;  // RPM
    boat.engine.available = true;
    boat.saildrive.saildriveEngaged = false;
    boat.saildrive.available = true;

    // System should log warning
    auto warnings = checkConsistency(boat);
    ASSERT_TRUE(warnings.contains("Engine running with saildrive disengaged"));
}
```

### Unit Tests (Validation Logic)

```cpp
// test_boatdata_units/test_validation.cpp
TEST(Validation, clamps_pitch_angle_to_range) {
    CompassData compass;
    compass.pitchAngle = M_PI / 2;  // 90° (too steep)

    validateCompassData(compass);

    ASSERT_LE(compass.pitchAngle, M_PI / 6);  // Clamped to 30°
    ASSERT_FALSE(compass.available);  // Marked invalid
}
```

---

## Open Questions for Implementation Phase

1. **1-Wire device addressing**: Need actual device addresses after hardware installation
2. **Battery SOC algorithm**: Use vendor-provided SOC or calculate from voltage curve?
3. **Multi-engine support**: Structure currently assumes single engine - extend to array if twin engines?
4. **NMEA2000 battery monitors**: Fallback to PGN 127506 if 1-wire sensors not available?

These will be resolved during Phase 2 (Task Planning) and Phase 4 (Implementation).

---

## Version History

- **v2.0.0** (2025-10-10): Enhanced BoatData specification for R005 implementation
- **v1.0.0** (2025-10-06): Initial BoatData centralized model (R003)
