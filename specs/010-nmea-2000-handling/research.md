# Research: NMEA 2000 Message Handling

**Feature**: 010-nmea-2000-handling
**Date**: 2025-10-12
**Status**: Complete

## Executive Summary

This feature extends the existing NMEA2000 handler infrastructure to add comprehensive PGN (Parameter Group Number) message handling for GPS navigation, compass/attitude data, DST (Depth/Speed/Temperature) sensors, engine monitoring, and wind data. The architecture builds on the existing Enhanced BoatData v2.0.0 framework with 9 handler functions already implemented and tested.

**Key Finding**: Most infrastructure exists. This feature primarily adds:
1. Missing PGN handlers (GPS, wind data)
2. NMEA2000 CAN bus initialization in main.cpp
3. Handler registration and message routing
4. Multi-source prioritization integration

## Existing Infrastructure Analysis

### 1. BoatData Structure (src/types/BoatDataTypes.h)

**Status**: ✅ COMPLETE - Enhanced BoatData v2.0.0 fully implemented

All required data structures exist:
- `GPSData`: latitude, longitude, cog, sog, variation (v2.0.0 enhancement)
- `CompassData`: true/magnetic heading, rateOfTurn, heelAngle, pitchAngle, heave
- `DSTData`: depth, measuredBoatSpeed, seaTemperature
- `WindData`: apparentWindAngle, apparentWindSpeed
- `EngineData`: engineRev, oilTemperature, alternatorVoltage

Memory footprint: ~560 bytes (within constitutional limits)

### 2. NMEA2000 Handler Functions (src/components/NMEA2000Handlers.cpp)

**Status**: ✅ PARTIAL - 9 of 13 handlers implemented

**Existing Handlers** (tested and working):
- ✅ PGN 127251: Rate of Turn → CompassData.rateOfTurn
- ✅ PGN 127252: Heave → CompassData.heave
- ✅ PGN 127257: Attitude → CompassData heel/pitch
- ✅ PGN 129029: GNSS Position → GPSData.variation
- ✅ PGN 128267: Water Depth → DSTData.depth
- ✅ PGN 128259: Boat Speed → DSTData.measuredBoatSpeed
- ✅ PGN 130316: Temperature → DSTData.seaTemperature
- ✅ PGN 127488: Engine Rapid → EngineData.engineRev
- ✅ PGN 127489: Engine Dynamic → EngineData oil/voltage

**Missing Handlers** (must be implemented):
- ❌ PGN 129025: Position Rapid Update → GPSData lat/lon
- ❌ PGN 129026: COG/SOG Rapid Update → GPSData cog/sog
- ❌ PGN 127250: Vessel Heading → CompassData true/magnetic heading
- ❌ PGN 127258: Magnetic Variation → GPSData.variation (alternative to 129029)
- ❌ PGN 130306: Wind Data → WindData

### 3. Data Validation Utilities (src/utils/DataValidation.h)

**Status**: ✅ COMPLETE - All validation functions exist

- ✅ `kelvinToCelsius()` - Temperature conversion
- ✅ `mpsToKnots()` - Speed conversion
- ✅ `isValidRateOfTurn()`, `clampRateOfTurn()` - Rate of turn validation
- ✅ `isValidHeave()`, `clampHeave()` - Heave validation
- ✅ `isValidPitch()`, `clampPitch()` - Pitch angle validation
- ✅ `isValidHeel()`, `clampHeel()` - Heel angle validation
- ✅ `isValidDepth()`, `clampDepth()` - Depth validation

### 4. Multi-Source Prioritization (src/components/SourcePrioritizer.h)

**Status**: ✅ COMPLETE - Full implementation exists

The BoatData class already includes:
- `registerSource()` - Register NMEA 2000 sources
- Automatic frequency-based priority (10 Hz NMEA2000 > 1 Hz NMEA0183)
- Failover to lower-frequency source when primary becomes stale (>5 seconds)
- Source tracking with `SensorSource` metadata

### 5. NMEA2000 Library Integration

**Library**: ttlappalainen/NMEA2000 (already in platformio.ini)

**Key Dependencies**:
```cpp
#include <NMEA2000.h>              // Base library
#include <N2kMessages.h>            // PGN parsing functions
#include <NMEA2000_esp32.h>         // ESP32 CAN bus implementation
```

**Parse Functions Available** (from NMEA2000 library):
- `ParseN2kPGN129025()` - Position Rapid Update
- `ParseN2kPGN129026()` - COG/SOG Rapid Update
- `ParseN2kPGN127250()` - Vessel Heading
- `ParseN2kPGN127258()` - Magnetic Variation
- `ParseN2kPGN130306()` - Wind Data
- All other parse functions already used by existing handlers

**Constants for Validation**:
- `N2kDoubleNA`, `N2kInt8NA`, `N2kUInt8NA`, `N2kUInt16NA`, `N2kUInt32NA` - Unavailable value markers
- `N2kIsNA()` macro - Check if value is unavailable

## NMEA 2000 CAN Bus Architecture

### Hardware Configuration (SH-ESP32 Board)

```
CAN RX: GPIO 34  (input from NMEA2000 bus)
CAN TX: GPIO 32  (output to NMEA2000 bus)
```

**Bus Requirements**:
- 120Ω termination resistors (both ends of backbone)
- 12V power supply (from boat electrical system)
- Twisted pair cabling (DeviceNet standard)
- Maximum cable length: 200m
- Maximum devices: 252 (including gateway)

### Address Claiming Protocol

NMEA2000 uses ISO 11783 address claiming:
1. Device requests preferred address (0-251)
2. If conflict detected, device with higher priority wins
3. Lower priority device claims alternate address
4. Gateway device class: 25 (Network device)

**Implementation**: NMEA2000 library handles automatically via `NMEA2000.SetMode(tNMEA2000::N2km_ListenAndNode)` and `NMEA2000.Open()`.

### Message Handler Registration

**Pattern** (from NMEA2000 library):
```cpp
// 1. Extend receive messages list
nmea2000->ExtendReceiveMessages(PGNList);

// 2. Create message handler class
class N2kBoatDataHandler : public tNMEA2000::tMsgHandler {
    void HandleMsg(const tN2kMsg &N2kMsg) override {
        // Route to handler based on N2kMsg.PGN
    }
};

// 3. Attach handler
nmea2000->AttachMsgHandler(&handler);
```

**RegisterN2kHandlers() Function** (already exists):
- Calls `ExtendReceiveMessages()` with all handled PGNs
- Creates `N2kBoatDataHandler` instance
- Attaches handler via `AttachMsgHandler()`

## PGN Handler Implementation Pattern

Each handler follows this structure:

```cpp
void HandleN2kPGN<NUMBER>(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger) {
    // 1. Parse PGN using NMEA2000 library function
    if (ParseN2kPGN<NUMBER>(N2kMsg, field1, field2, ...) {
        // 2. Check for N2kDoubleNA/N2kIntNA unavailable values
        if (N2kIsNA(value)) {
            logger->broadcastLog(LogLevel::DEBUG, "NMEA2000", "PGN<NUMBER>_NA", ...);
            return;
        }

        // 3. Validate data using DataValidation helpers
        bool valid = DataValidation::isValid<Type>(value);
        if (!valid) {
            logger->broadcastLog(LogLevel::WARN, "NMEA2000", "PGN<NUMBER>_OUT_OF_RANGE", ...);
            value = DataValidation::clamp<Type>(value);
        }

        // 4. Get current data structure from BoatData
        <DataType> data = boatData->get<DataType>();

        // 5. Update fields
        data.field = value;
        data.available = true;
        data.lastUpdate = millis();

        // 6. Store updated data
        boatData->set<DataType>(data);

        // 7. Log update (DEBUG level)
        logger->broadcastLog(LogLevel::DEBUG, "NMEA2000", "PGN<NUMBER>_UPDATE", ...);

        // 8. Increment message counter
        boatData->incrementNMEA2000Count();

    } else {
        // Parse failed - log ERROR and set availability to false
        logger->broadcastLog(LogLevel::ERROR, "NMEA2000", "PGN<NUMBER>_PARSE_FAILED", ...);
    }
}
```

## Implementation Gaps Analysis

### Gap 1: Missing PGN Handlers

**Impact**: Critical - Without these handlers, GPS position, heading, and wind data will not be received.

**Required Implementations**:
1. `HandleN2kPGN129025()` - GPS Position Rapid Update
2. `HandleN2kPGN129026()` - COG/SOG Rapid Update
3. `HandleN2kPGN127250()` - Vessel Heading
4. `HandleN2kPGN127258()` - Magnetic Variation (alternative source)
5. `HandleN2kPGN130306()` - Wind Data

**Complexity**: LOW - Same pattern as existing handlers, parse functions exist in library.

### Gap 2: NMEA2000 CAN Bus Initialization

**Impact**: Critical - Without initialization, no NMEA2000 messages will be received.

**Required in main.cpp setup()**:
```cpp
// After WiFi initialization (step 1)
// After I2C/OLED initialization (steps 2-3)
// After Serial2 initialization (step 4)

// STEP 5: NMEA2000 CAN bus initialization
#include <NMEA2000_esp32.h>

tNMEA2000 *nmea2000 = new tNMEA2000_esp32(CAN_TX_PIN, CAN_RX_PIN);
nmea2000->SetProductInformation(...);  // Device info
nmea2000->SetDeviceInformation(...);   // Unique ID, device class
nmea2000->SetMode(tNMEA2000::N2km_ListenAndNode);
nmea2000->EnableForward(false);  // No forwarding to PC
nmea2000->Open();

// STEP 6: Register message handlers
RegisterN2kHandlers(nmea2000, boatData, &logger);

// STEP 7: ReactESP event loops (already exists)
```

**Complexity**: MEDIUM - Must follow initialization sequence, configure device info.

### Gap 3: Message Handler Class

**Impact**: Critical - Routes incoming PGN messages to handler functions.

**Required Implementation** (in NMEA2000Handlers.cpp):
```cpp
class N2kBoatDataHandler : public tNMEA2000::tMsgHandler {
private:
    BoatData* boatData;
    WebSocketLogger* logger;

public:
    N2kBoatDataHandler(BoatData* bd, WebSocketLogger* log) : boatData(bd), logger(log) {}

    void HandleMsg(const tN2kMsg &N2kMsg) override {
        switch (N2kMsg.PGN) {
            case 129025: HandleN2kPGN129025(N2kMsg, boatData, logger); break;
            case 129026: HandleN2kPGN129026(N2kMsg, boatData, logger); break;
            // ... all other PGNs
        }
    }
};
```

**Complexity**: LOW - Simple switch statement, calls existing/new handlers.

### Gap 4: Source Registration

**Impact**: LOW - Multi-source prioritization already works, just needs registration.

**Required** (in main.cpp after BoatData initialization):
```cpp
// Register NMEA 2000 sources (10 Hz typical)
boatData->registerSource("NMEA2000-GPS", SensorType::GPS, 10.0);
boatData->registerSource("NMEA2000-COMPASS", SensorType::COMPASS, 10.0);
```

**Complexity**: TRIVIAL - Two function calls.

## PGN Message Specifications

### GPS Messages

#### PGN 129025 - Position, Rapid Update (10 Hz)
- **Fields**: Latitude (decimal degrees), Longitude (decimal degrees)
- **Parse Function**: `ParseN2kPGN129025(N2kMsg, latitude, longitude)`
- **Target**: `GPSData.latitude`, `GPSData.longitude`
- **Validation**: Latitude [-90, 90], Longitude [-180, 180]

#### PGN 129026 - COG & SOG, Rapid Update (10 Hz)
- **Fields**: SID, COG Reference (true/magnetic), COG (radians), SOG (m/s)
- **Parse Function**: `ParseN2kPGN129026(N2kMsg, SID, COGReference, COG, SOG)`
- **Target**: `GPSData.cog` (radians), `GPSData.sog` (convert m/s → knots)
- **Validation**: COG [0, 2π], SOG [0, 100 knots]

### Compass Messages

#### PGN 127250 - Vessel Heading (10 Hz)
- **Fields**: SID, Heading (radians), Deviation (radians), Variation (radians), Reference (true/magnetic)
- **Parse Function**: `ParseN2kPGN127250(N2kMsg, SID, Heading, Deviation, Variation, Reference)`
- **Target**: `CompassData.trueHeading` (if Reference == N2khr_true), `CompassData.magneticHeading` (if Reference == N2khr_magnetic)
- **Validation**: Heading [0, 2π]
- **Note**: Deviation and Variation may be N2kDoubleNA if not available

#### PGN 127258 - Magnetic Variation (1 Hz)
- **Fields**: SID, Source (auto/manual/chart), Days Since 1970, Variation (radians)
- **Parse Function**: `ParseN2kPGN127258(N2kMsg, SID, Source, DaysSince1970, Variation)`
- **Target**: `GPSData.variation`
- **Validation**: Variation [-30°, 30°] (≈ [-0.524, 0.524] radians)
- **Note**: Alternative to PGN 129029 variation field; both may be present

### Wind Messages

#### PGN 130306 - Wind Data (10 Hz)
- **Fields**: SID, Wind Speed (m/s), Wind Angle (radians), Reference (apparent/true ground/true water/true boat)
- **Parse Function**: `ParseN2kPGN130306(N2kMsg, SID, WindSpeed, WindAngle, Reference)`
- **Target**: `WindData.apparentWindAngle` (radians), `WindData.apparentWindSpeed` (convert m/s → knots)
- **Validation**: Only process if Reference == `N2kWind_Apparent`; silently ignore other reference types
- **Sign Convention**: Positive = starboard, negative = port

## NMEA 2000 vs NMEA 0183 Integration

### Multi-Source Scenario

**Example**: Boat has both NMEA 2000 GPS (10 Hz) and NMEA 0183 VHF GPS (1 Hz).

**Behavior**:
1. Both sources register with BoatData: "NMEA2000-GPS" (10 Hz), "NMEA0183-VH" (1 Hz)
2. BoatData automatically selects NMEA2000-GPS as primary (higher frequency)
3. If NMEA2000-GPS fails or becomes stale (>5 seconds), system failover to NMEA0183-VH
4. When NMEA2000-GPS recovers, system automatically switches back

**No Special Handling Required**: Multi-source prioritization layer handles automatically.

### Message Processing Strategy

**NMEA 2000**: Process ALL messages from ALL devices on bus. Do not filter by source address.

**Rationale**: Multiple devices may send same PGN type (e.g., two GPS units). BoatData prioritization layer selects active source based on update frequency, not handler logic.

**Example**: If two NMEA2000 GPS devices broadcast PGN 129025:
- Handler updates `GPSData` with every received message (from either device)
- BoatData tracks update frequency for each source
- Highest-frequency source automatically becomes primary
- Failover occurs if primary source stops sending

## Constitutional Compliance

### Principle I: HAL Abstraction

✅ **Compliant**: NMEA2000 library provides HAL via `tNMEA2000_esp32` class.
- Hardware dependency isolated to `NMEA2000_esp32.h` (CAN bus driver)
- Mock implementation possible via custom `tNMEA2000` subclass for testing
- Business logic (handlers) separated from hardware I/O

### Principle II: Resource Management

✅ **Compliant**: All memory statically allocated.
- Handler functions use stack variables only
- BoatData structures already validated (~560 bytes)
- `N2kBoatDataHandler` class: ~16 bytes (2 pointers + vtable)
- No heap allocation in handler code

### Principle V: Network Debugging

✅ **Compliant**: WebSocket logging for all operations.
- Every handler includes DEBUG-level log on successful update
- WARN-level log for out-of-range values with original and clamped values
- ERROR-level log for parse failures
- Log format includes PGN number, field names, and values

### Principle VI: Always-On Operation

✅ **Compliant**: ReactESP event-driven architecture.
- NMEA2000 library uses interrupt-driven CAN bus reception (non-blocking)
- Handler functions execute quickly (<1ms typical)
- No blocking delays in any handler
- No sleep modes used

### Principle VII: Fail-Safe Operation

✅ **Compliant**: Graceful degradation on errors.
- Parse failures do not crash system (ERROR log, availability=false)
- Out-of-range values clamped (WARN log, data still usable)
- Unavailable (NA) values skip update (DEBUG log, preserve existing data)
- CAN bus failures do not affect WiFi/web server operation

## Testing Strategy

### Contract Tests (test_nmea2000_contracts/)

**Purpose**: Validate handler function contracts and NMEA2000 library integration.

**Scenarios**:
1. Each handler correctly parses valid PGN message
2. Each handler detects N2kDoubleNA/N2kIntNA unavailable values
3. Each handler validates data ranges and clamps out-of-range values
4. Each handler logs errors on parse failures
5. RegisterN2kHandlers() extends receive message list with all PGNs
6. N2kBoatDataHandler routes messages to correct handler functions

**Mocking**: Use NMEA2000 library's `tN2kMsg` constructor to create test messages.

### Integration Tests (test_nmea2000_integration/)

**Purpose**: End-to-end scenarios with multiple PGNs and multi-source prioritization.

**Scenarios**:
1. GPS: Receive PGN 129025 + 129026 + 129029 → GPSData fully populated
2. Compass: Receive PGN 127250 + 127251 + 127257 → CompassData fully populated
3. DST: Receive PGN 128267 + 128259 + 130316 → DSTData fully populated
4. Engine: Receive PGN 127488 + 127489 → EngineData fully populated
5. Wind: Receive PGN 130306 (apparent reference) → WindData populated
6. Multi-source: NMEA2000 GPS (10 Hz) + NMEA0183 GPS (1 Hz) → NMEA2000 active
7. Failover: NMEA2000 GPS stops → automatic switch to NMEA0183 GPS

### Unit Tests (test_nmea2000_units/)

**Purpose**: Validate individual handler logic and data conversions.

**Scenarios**:
1. GPS validation: Latitude/longitude clamping, COG/SOG range checks
2. Wind validation: Angle sign convention, speed conversion (m/s → knots)
3. Temperature conversion: Kelvin → Celsius (130316 handler)
4. Engine instance filtering: Only engine instance 0 processed
5. Wind reference filtering: Only N2kWind_Apparent processed
6. Temperature source filtering: Only N2kts_SeaTemperature processed

### Hardware Tests (test_nmea2000_hardware/)

**Purpose**: Minimal validation with real NMEA2000 devices (ESP32 required).

**Scenarios**:
1. CAN bus initialization: GPIO 34/32 configuration, 250 kbps baud rate
2. Address claiming: Device successfully claims address (monitor via logger)
3. Message reception: Confirm PGN messages received via WebSocket logs
4. Timing validation: Handler execution time <1ms per message

**Requirements**:
- ESP32 with CAN transceivers (GPIO 34/32)
- NMEA2000 bus with 12V power and 120Ω terminators
- At least one NMEA2000 device broadcasting known PGNs

## Performance Characteristics

### Message Processing Budget

**Typical Load**:
- 10 PGN types at 10 Hz each = 100 messages/second
- Handler execution time: ~0.5ms average per message
- Total CPU time: ~5% (50ms/1000ms)

**Peak Load**:
- 20 devices * 5 PGNs each * 10 Hz = 1000 messages/second
- Total CPU time: ~50% (500ms/1000ms)
- Still acceptable with ReactESP event loop architecture

**Latency**:
- CAN interrupt to handler execution: <1ms (interrupt-driven)
- Handler execution to BoatData update: <0.5ms
- Total latency: <2ms (well under 10ms acceptable limit)

### Memory Footprint

**Static Allocation** (one-time):
- N2kBoatDataHandler class: 16 bytes (2 pointers + vtable)
- tNMEA2000_esp32 instance: ~200 bytes
- CAN message buffers: ~2KB (NMEA2000 library internal)
- **Total RAM**: ~2.2KB (~0.7% of ESP32 320KB RAM)

**Per-Message** (stack):
- Handler local variables: ~50 bytes typical
- NMEA2000 library parse functions: ~100 bytes
- **Total stack per handler call**: ~150 bytes

**Flash Impact**:
- 5 new handler functions: ~10KB
- N2kBoatDataHandler class: ~2KB
- NMEA2000 library (already included): ~50KB
- **Total Flash**: ~12KB new (~0.6% of 1.9MB partition)

**Constitutional Compliance**: ✓ Well under resource limits (Principle II).

## Risk Assessment

### Risk 1: NMEA2000 Library API Changes

**Probability**: LOW - Library is mature (v4.x), API stable for years.
**Impact**: MEDIUM - Would require handler function updates.
**Mitigation**: Pin library version in platformio.ini, test before upgrades.

### Risk 2: Multiple Devices Same PGN Type

**Probability**: HIGH - Common scenario (multiple GPS, multiple depth sounders).
**Impact**: LOW - Multi-source prioritization handles automatically.
**Mitigation**: Already implemented in BoatData v2.0.0, tested in spec 008.

### Risk 3: CAN Bus Electrical Issues

**Probability**: MEDIUM - Marine environment, vibration, corrosion.
**Impact**: HIGH - No NMEA2000 data if bus fails.
**Mitigation**: Graceful degradation (failover to NMEA0183), WebSocket logging for diagnostics.

### Risk 4: Handler Execution Time Exceeds Budget

**Probability**: LOW - Handlers are simple parse + validate + update.
**Impact**: MEDIUM - Could cause message loss if ReactESP loop blocked.
**Mitigation**: Handlers profiled during hardware tests, no blocking operations.

## References

### NMEA2000 Library Documentation

- **API Reference**: https://ttlappalainen.github.io/NMEA2000/pg_lib_ref.html
- **GitHub Repository**: https://github.com/ttlappalainen/NMEA2000
- **Example Code**: `examples/poseidongw/src/` (working reference implementation)

### NMEA2000 Standard

- **ISO 11783**: CAN bus physical layer and addressing
- **IEC 61162-3**: NMEA2000 standard (proprietary, not freely available)
- **PGN Definitions**: Documented in NMEA2000 library `N2kMessages.h`

### Related Specifications

- **R008**: User requirements for NMEA 2000 data (`user_requirements/R008 - NMEA 2000 data.md`)
- **Spec 008**: Enhanced BoatData v2.0.0 (`specs/008-enhanced-boatdata-following/`)
- **Spec 006**: NMEA 0183 handlers (`specs/006-nmea-0183-handlers/`) - similar pattern

### Hardware Documentation

- **SH-ESP32 Board**: GPIO assignments (`CLAUDE.md` lines 85-95)
- **CAN Bus Wiring**: DeviceNet standard, 120Ω terminators, twisted pair
- **Power Requirements**: 12V from boat electrical system, isolated ground

## Conclusion

This feature is **LOW RISK** and **WELL-DEFINED**. Most infrastructure exists:
- ✅ BoatData structures complete (v2.0.0)
- ✅ 9 of 13 handlers already implemented and tested
- ✅ Multi-source prioritization working
- ✅ Validation utilities complete
- ✅ NMEA2000 library integrated

**Remaining Work**:
1. Implement 5 missing handlers (GPS, heading, wind) - ~200 lines of code
2. Add NMEA2000 CAN bus initialization in main.cpp - ~50 lines
3. Create N2kBoatDataHandler message router class - ~80 lines
4. Register NMEA2000 sources with BoatData - 2 lines
5. Write comprehensive tests (contract, integration, unit, hardware)

**Estimated Effort**: 8-12 hours implementation + 8-12 hours testing = 16-24 hours total.

**Constitutional Compliance**: ✅ All principles satisfied.
