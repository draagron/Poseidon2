# Data Model: NMEA 2000 Message Handling

**Feature**: 010-nmea-2000-handling
**Date**: 2025-10-12
**Version**: 1.0.0

## Overview

This data model defines the NMEA 2000 handler architecture, message routing, and data flow from CAN bus to BoatData structure. The system processes 13 PGN (Parameter Group Number) types, updating GPS, compass, DST, engine, and wind data with automatic multi-source prioritization.

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                    NMEA 2000 CAN Bus                            │
│  (ISO 11783, 250 kbps, GPIO 34/32, 120Ω terminators)          │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│              tNMEA2000_esp32 (CAN Bus Driver)                   │
│  - Interrupt-driven message reception                           │
│  - Address claiming (ISO 11783)                                 │
│  - Message buffering (2KB)                                      │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│           N2kBoatDataHandler (Message Router)                   │
│  - switch(PGN) routes to handler functions                      │
│  - 13 PGN types supported                                       │
└────────────────────────┬────────────────────────────────────────┘
                         │
         ┌───────────────┼───────────────┬─────────────┐
         ▼               ▼               ▼             ▼
┌──────────────┐  ┌──────────────┐  ┌──────────┐  ┌──────────┐
│  GPS         │  │  Compass     │  │  DST     │  │  Engine  │
│  Handlers    │  │  Handlers    │  │  Handlers│  │  Handlers│
│              │  │              │  │          │  │          │
│ 129025       │  │ 127250       │  │ 128267   │  │ 127488   │
│ 129026       │  │ 127251       │  │ 128259   │  │ 127489   │
│ 129029       │  │ 127252       │  │ 130316   │  │          │
│ 127258       │  │ 127257       │  │          │  │          │
└──────┬───────┘  └──────┬───────┘  └────┬─────┘  └────┬─────┘
       │                 │                │             │
       └─────────────────┴────────────────┴─────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                    BoatData Structure                           │
│  - GPSData (lat, lon, cog, sog, variation)                     │
│  - CompassData (heading, rate of turn, heel, pitch, heave)     │
│  - DSTData (depth, speed, temperature)                         │
│  - EngineData (RPM, oil temp, voltage)                         │
│  - WindData (apparent angle, apparent speed)                   │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│              Multi-Source Prioritizer                           │
│  - Tracks update frequency per source                           │
│  - Automatic priority: NMEA2000 (10 Hz) > NMEA0183 (1 Hz)     │
│  - Failover to secondary source if primary stale (>5 sec)      │
└─────────────────────────────────────────────────────────────────┘
```

## Handler Function Signature

All handler functions follow this contract:

```cpp
/**
 * @brief Handle NMEA2000 PGN <NUMBER> - <Description>
 *
 * @param N2kMsg NMEA2000 message (const reference, read-only)
 * @param boatData BoatData instance to update (must not be nullptr)
 * @param logger WebSocket logger for debug output (must not be nullptr)
 *
 * @pre boatData != nullptr
 * @pre logger != nullptr
 * @post If valid data parsed: BoatData updated, availability=true, timestamp=millis()
 * @post If parse failed: availability=false, existing data preserved, ERROR logged
 * @post If data unavailable (NA): no update, existing data preserved, DEBUG logged
 */
void HandleN2kPGN<NUMBER>(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger);
```

## PGN to BoatData Mapping

### GPS Data (4 PGNs → GPSData)

| PGN    | Name | Update Rate | Fields | Target | Notes |
|--------|------|-------------|--------|--------|-------|
| 129025 | Position, Rapid Update | 10 Hz | Latitude, Longitude | `GPSData.latitude`, `GPSData.longitude` | Primary position source |
| 129026 | COG & SOG, Rapid Update | 10 Hz | COG, SOG | `GPSData.cog`, `GPSData.sog` | Course over ground, speed over ground |
| 129029 | GNSS Position Data | 1 Hz | Lat, Lon, Altitude, Variation, Quality | `GPSData.*` (all fields) | Enhanced GPS with variation and quality metrics |
| 127258 | Magnetic Variation | 1 Hz | Variation, Source | `GPSData.variation` | Alternative variation source |

**Field Details**:
- `latitude`: Decimal degrees, range [-90, 90], positive = North
- `longitude`: Decimal degrees, range [-180, 180], positive = East
- `cog`: Radians, range [0, 2π], true course over ground
- `sog`: Knots, range [0, 100] (converted from m/s)
- `variation`: Radians, range [-0.524, 0.524] (±30°), positive = East

### Compass Data (4 PGNs → CompassData)

| PGN    | Name | Update Rate | Fields | Target | Notes |
|--------|------|-------------|--------|--------|-------|
| 127250 | Vessel Heading | 10 Hz | Heading, Deviation, Variation, Reference | `CompassData.trueHeading` or `.magneticHeading` | Reference type determines target field |
| 127251 | Rate of Turn | 10 Hz | Rate of Turn | `CompassData.rateOfTurn` | Radians/second, positive = starboard turn |
| 127252 | Heave | 10 Hz | Heave | `CompassData.heave` | Meters, positive = upward motion |
| 127257 | Attitude | 10 Hz | Yaw, Pitch, Roll | `CompassData.heelAngle`, `.pitchAngle` | Roll = heel, yaw ignored (redundant with heading) |

**Field Details**:
- `trueHeading`: Radians, range [0, 2π], true heading
- `magneticHeading`: Radians, range [0, 2π], magnetic heading
- `rateOfTurn`: Radians/second, range [-π, π], positive = starboard turn
- `heelAngle`: Radians, range [-π/2, π/2], positive = starboard tilt
- `pitchAngle`: Radians, range [-π/6, π/6], positive = bow up
- `heave`: Meters, range [-5.0, 5.0], positive = upward

**Special Handling**:
- PGN 127250: Route to `trueHeading` if Reference == `N2khr_true`, `magneticHeading` if Reference == `N2khr_magnetic`
- PGN 127257: Heave field is N/A in this PGN (use PGN 127252 for heave)

### DST Data (3 PGNs → DSTData)

| PGN    | Name | Update Rate | Fields | Target | Notes |
|--------|------|-------------|--------|--------|-------|
| 128267 | Water Depth | 10 Hz | Depth Below Transducer, Offset | `DSTData.depth` | Depth = DepthBelowTransducer + Offset |
| 128259 | Speed (Water Referenced) | 10 Hz | Water-Referenced Speed | `DSTData.measuredBoatSpeed` | Speed through water (paddle wheel) |
| 130316 | Temperature Extended Range | 10 Hz | Temperature, Source, Instance | `DSTData.seaTemperature` | Only process if Source == `N2kts_SeaTemperature` |

**Field Details**:
- `depth`: Meters, range [0, 100], depth below waterline
- `measuredBoatSpeed`: m/s, range [0, 25], speed through water
- `seaTemperature`: Celsius, range [-10, 50] (converted from Kelvin)

**Special Handling**:
- PGN 130316: Silently ignore if TempSource ≠ `N2kts_SeaTemperature` (no log, no update)

### Engine Data (2 PGNs → EngineData)

| PGN    | Name | Update Rate | Fields | Target | Notes |
|--------|------|-------------|--------|--------|-------|
| 127488 | Engine Parameters, Rapid Update | 10 Hz | Engine Instance, Engine Speed, Engine Boost Pressure, Engine Tilt/Trim | `EngineData.engineRev` | Only process engine instance 0 |
| 127489 | Engine Parameters, Dynamic | 1 Hz | Engine Instance, Oil Pressure, Oil Temperature, Engine Temperature, Alternator Voltage, Fuel Rate, Engine Hours | `EngineData.oilTemperature`, `.alternatorVoltage` | Only process engine instance 0 |

**Field Details**:
- `engineRev`: RPM, range [0, 6000], engine speed
- `oilTemperature`: Celsius, range [-10, 150] (converted from Kelvin)
- `alternatorVoltage`: Volts, range [0, 30], alternator output

**Special Handling**:
- Both PGNs: Silently ignore if Engine Instance ≠ 0 (no log, no update)
- Oil temperature: Generate WARN log if >120°C (approaching critical temperature)

### Wind Data (1 PGN → WindData)

| PGN    | Name | Update Rate | Fields | Target | Notes |
|--------|------|-------------|--------|--------|-------|
| 130306 | Wind Data | 10 Hz | Wind Speed, Wind Angle, Reference | `WindData.apparentWindAngle`, `.apparentWindSpeed` | Only process if Reference == `N2kWind_Apparent` |

**Field Details**:
- `apparentWindAngle`: Radians, range [-π, π], positive = starboard, negative = port
- `apparentWindSpeed`: Knots, range [0, 100] (converted from m/s)

**Special Handling**:
- Silently ignore if WindReference ≠ `N2kWind_Apparent` (no log, no update)

## Message Router Implementation

### N2kBoatDataHandler Class

```cpp
/**
 * @brief NMEA2000 message router class
 *
 * Routes incoming PGN messages to appropriate handler functions based on PGN number.
 * Implements tNMEA2000::tMsgHandler interface for NMEA2000 library integration.
 *
 * Memory footprint: 16 bytes (2 pointers + vtable)
 * Execution time: <0.1ms per message (simple switch statement)
 */
class N2kBoatDataHandler : public tNMEA2000::tMsgHandler {
private:
    BoatData* boatData;         ///< BoatData instance to update
    WebSocketLogger* logger;    ///< Logger for debug output

public:
    /**
     * @brief Constructor
     *
     * @param bd BoatData instance (must not be nullptr)
     * @param log WebSocket logger (must not be nullptr)
     */
    N2kBoatDataHandler(BoatData* bd, WebSocketLogger* log)
        : boatData(bd), logger(log) {}

    /**
     * @brief Handle incoming NMEA2000 message
     *
     * Routes message to appropriate handler based on PGN number.
     * Unhandled PGNs are silently ignored (no log, no error).
     *
     * @param N2kMsg NMEA2000 message
     */
    void HandleMsg(const tN2kMsg &N2kMsg) override {
        switch (N2kMsg.PGN) {
            // GPS handlers
            case 129025: HandleN2kPGN129025(N2kMsg, boatData, logger); break;
            case 129026: HandleN2kPGN129026(N2kMsg, boatData, logger); break;
            case 129029: HandleN2kPGN129029(N2kMsg, boatData, logger); break;
            case 127258: HandleN2kPGN127258(N2kMsg, boatData, logger); break;

            // Compass handlers
            case 127250: HandleN2kPGN127250(N2kMsg, boatData, logger); break;
            case 127251: HandleN2kPGN127251(N2kMsg, boatData, logger); break;
            case 127252: HandleN2kPGN127252(N2kMsg, boatData, logger); break;
            case 127257: HandleN2kPGN127257(N2kMsg, boatData, logger); break;

            // DST handlers
            case 128267: HandleN2kPGN128267(N2kMsg, boatData, logger); break;
            case 128259: HandleN2kPGN128259(N2kMsg, boatData, logger); break;
            case 130316: HandleN2kPGN130316(N2kMsg, boatData, logger); break;

            // Engine handlers
            case 127488: HandleN2kPGN127488(N2kMsg, boatData, logger); break;
            case 127489: HandleN2kPGN127489(N2kMsg, boatData, logger); break;

            // Wind handlers
            case 130306: HandleN2kPGN130306(N2kMsg, boatData, logger); break;

            // All other PGNs silently ignored
            default: break;
        }
    }
};
```

## Handler Registration

### RegisterN2kHandlers() Function

```cpp
/**
 * @brief Register all PGN handlers with NMEA2000 library
 *
 * 1. Extends receive message list with all handled PGNs
 * 2. Creates N2kBoatDataHandler instance
 * 3. Attaches handler to NMEA2000 library
 * 4. Logs INFO message with PGN count and list
 *
 * @param nmea2000 NMEA2000 instance (must not be nullptr)
 * @param boatData BoatData instance for handlers (must not be nullptr)
 * @param logger WebSocket logger (must not be nullptr)
 *
 * @pre nmea2000->Open() has been called (CAN bus initialized)
 * @post All 13 PGNs added to receive message list
 * @post N2kBoatDataHandler attached to NMEA2000 library
 */
void RegisterN2kHandlers(tNMEA2000* nmea2000, BoatData* boatData, WebSocketLogger* logger) {
    if (nmea2000 == nullptr || boatData == nullptr || logger == nullptr) {
        return;
    }

    // Define list of all handled PGNs
    const unsigned long PGNList[] = {
        // GPS (4 PGNs)
        129025, 129026, 129029, 127258,
        // Compass (4 PGNs)
        127250, 127251, 127252, 127257,
        // DST (3 PGNs)
        128267, 128259, 130316,
        // Engine (2 PGNs)
        127488, 127489,
        // Wind (1 PGN)
        130306,
        // Terminator
        0
    };

    // Extend receive message list
    nmea2000->ExtendReceiveMessages(PGNList);

    // Create and attach message handler
    static N2kBoatDataHandler handler(boatData, logger);
    nmea2000->AttachMsgHandler(&handler);

    // Log registration success
    logger->broadcastLog(LogLevel::INFO, "NMEA2000", "HANDLERS_REGISTERED",
        F("{\"count\":13,\"pgns\":[129025,129026,129029,127258,127250,127251,127252,127257,128267,128259,130316,127488,127489,130306]}"));
}
```

## Data Validation Rules

### GPS Validation

| Field | Valid Range | Clamping | Warning Threshold |
|-------|-------------|----------|-------------------|
| Latitude | [-90, 90] | Clamp to limits | N/A |
| Longitude | [-180, 180] | Clamp to limits | N/A |
| COG | [0, 2π] | Wrap to [0, 2π] | N/A |
| SOG | [0, 100 knots] | Clamp to limits | >50 knots (unrealistic for sailboat) |
| Variation | [-0.524, 0.524] (±30°) | Clamp to limits | N/A |

### Compass Validation

| Field | Valid Range | Clamping | Warning Threshold |
|-------|-------------|----------|-------------------|
| Heading | [0, 2π] | Wrap to [0, 2π] | N/A |
| Rate of Turn | [-π, π] rad/s | Clamp to limits | N/A |
| Heel Angle | [-π/2, π/2] | Clamp to limits | >π/4 (45°) - extreme heel |
| Pitch Angle | [-π/6, π/6] (±30°) | Clamp to limits | >π/6 (30°) - extreme pitch |
| Heave | [-5.0, 5.0] m | Clamp to limits | N/A |

### DST Validation

| Field | Valid Range | Clamping | Warning Threshold |
|-------|-------------|----------|-------------------|
| Depth | [0, 100] m | Clamp to limits, reject negative | <2m (grounding risk) |
| Boat Speed | [0, 25] m/s | Clamp to limits | N/A |
| Sea Temperature | [-10, 50] °C | Clamp to limits | N/A |

### Engine Validation

| Field | Valid Range | Clamping | Warning Threshold |
|-------|-------------|----------|-------------------|
| Engine RPM | [0, 6000] | Clamp to limits | >5000 RPM (over-revving) |
| Oil Temperature | [-10, 150] °C | Clamp to limits | >120°C (approaching critical) |
| Alternator Voltage | [0, 30] V | Clamp to limits | <12V or >15V (12V system) |

### Wind Validation

| Field | Valid Range | Clamping | Warning Threshold |
|-------|-------------|----------|-------------------|
| Wind Angle | [-π, π] | Wrap to [-π, π] | N/A |
| Wind Speed | [0, 100 knots] | Clamp to limits | >50 knots (storm conditions) |

## Unit Conversions

### Temperature: Kelvin → Celsius

```cpp
double celsius = kelvin - 273.15;
```

**Used in**: PGN 130316 (sea temperature), PGN 127489 (oil temperature)

### Speed: m/s → Knots

```cpp
double knots = mps * 1.9438444924406047516198704103672;
```

**Used in**: PGN 129026 (SOG), PGN 130306 (wind speed)

### Angle: Degrees → Radians

```cpp
double radians = degrees * DEG_TO_RAD;  // DEG_TO_RAD = π/180
```

**Note**: NMEA2000 PGNs already use radians natively; no conversion needed.

## Error Handling States

### State 1: Parse Success, Data Valid

```
Flow: Parse → Check N2kIsNA → Validate → Update BoatData → Log DEBUG → Increment counter
Result: BoatData updated, availability=true, lastUpdate=millis()
Log: DEBUG level with PGN number and field values
```

### State 2: Parse Success, Data Unavailable (N2kDoubleNA)

```
Flow: Parse → Check N2kIsNA → Skip update → Log DEBUG
Result: BoatData unchanged, existing values preserved
Log: DEBUG level with "not available" reason
```

### State 3: Parse Success, Data Out of Range

```
Flow: Parse → Check N2kIsNA → Validate FAIL → Clamp → Update BoatData → Log WARN
Result: BoatData updated with clamped value, availability=true
Log: WARN level with original and clamped values
```

### State 4: Parse Failure

```
Flow: Parse FAIL → Set availability=false → Log ERROR
Result: BoatData availability=false, existing values preserved
Log: ERROR level with PGN number and failure reason
```

## Multi-Source Integration

### Source Registration

```cpp
// In main.cpp setup(), after BoatData initialization:
boatData->registerSource("NMEA2000-GPS", SensorType::GPS, 10.0);
boatData->registerSource("NMEA2000-COMPASS", SensorType::COMPASS, 10.0);
```

### Priority Calculation

**Automatic Priority** (frequency-based):
1. Measure update rate for each source (Hz)
2. Highest frequency source becomes active
3. If frequencies equal, first-registered source wins

**Example**:
- NMEA2000-GPS: 10 Hz → Priority 1 (active)
- NMEA0183-VH: 1 Hz → Priority 2 (standby)

### Failover Logic

**Condition**: Active source stale (no updates for >5 seconds)

**Action**:
1. Mark active source as `available=false`
2. Select next-highest priority source
3. Log INFO message with new active source

**Recovery**: When original source resumes, system automatically switches back (higher frequency).

## Memory Layout

### BoatData Structure (static allocation)

```
GPSData:        88 bytes  (7 doubles * 8 + bool + unsigned long)
CompassData:    104 bytes (8 doubles * 8 + bool + unsigned long)
DSTData:        48 bytes  (3 doubles * 8 + bool + unsigned long)
EngineData:     48 bytes  (3 doubles * 8 + bool + unsigned long)
WindData:       40 bytes  (2 doubles * 8 + bool + unsigned long)
...other fields...
─────────────────────────
Total: ~560 bytes (BoatDataStructure)
```

### Handler Overhead

```
N2kBoatDataHandler class:  16 bytes (2 pointers + vtable)
tNMEA2000_esp32 instance:  ~200 bytes
CAN message buffers:       ~2KB (NMEA2000 library)
─────────────────────────────────────
Total RAM: ~2.2KB (0.7% of ESP32 RAM)
```

### Per-Handler Stack Usage

```
Handler local variables:   ~50 bytes
NMEA2000 parse function:   ~100 bytes
─────────────────────────────────────
Total stack per call: ~150 bytes
```

**Constitutional Compliance**: ✓ Static allocation only, well under memory limits.

## Performance Characteristics

### Typical Load

- **Message rate**: 100 messages/second (10 PGNs * 10 Hz)
- **Handler execution**: 0.5ms average per message
- **CPU utilization**: ~5% (50ms/1000ms)
- **Latency**: <2ms (CAN interrupt to BoatData update)

### Peak Load

- **Message rate**: 1000 messages/second (20 devices * 5 PGNs * 10 Hz)
- **Handler execution**: 0.5ms average per message
- **CPU utilization**: ~50% (500ms/1000ms)
- **Latency**: <2ms (still acceptable)

**Constitutional Compliance**: ✓ Non-blocking, interrupt-driven, ReactESP compatible.

## References

- **NMEA2000 Library API**: https://ttlappalainen.github.io/NMEA2000/pg_lib_ref.html
- **PGN Definitions**: `N2kMessages.h` in NMEA2000 library
- **BoatData v2.0.0**: `src/types/BoatDataTypes.h`
- **Data Validation**: `src/utils/DataValidation.h`
- **Multi-Source Prioritization**: `src/components/SourcePrioritizer.h`
- **Feature Spec**: `specs/010-nmea-2000-handling/spec.md`
- **Research**: `specs/010-nmea-2000-handling/research.md`
