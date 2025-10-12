# Contract: NMEA2000 Handler Functions

**Version**: 1.0.0
**Date**: 2025-10-12
**Status**: Draft

## Overview

This contract defines the interface and behavior requirements for all NMEA2000 PGN handler functions. All handlers must follow this contract to ensure consistent error handling, logging, and data validation.

## Function Signature

```cpp
void HandleN2kPGN<NUMBER>(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger);
```

Where `<NUMBER>` is the PGN number (e.g., 129025, 127250).

## Parameters

### Input Parameters

- **N2kMsg**: `const tN2kMsg&` (read-only)
  - NMEA2000 message from CAN bus
  - Contains PGN number, data length, payload bytes
  - Passed by const reference (no copy, no modification)

- **boatData**: `BoatData*` (non-null pointer)
  - BoatData instance to update with parsed data
  - **Precondition**: Must not be nullptr
  - **Behavior**: If nullptr, function returns immediately without action

- **logger**: `WebSocketLogger*` (non-null pointer)
  - WebSocket logger for debug/warn/error output
  - **Precondition**: Must not be nullptr
  - **Behavior**: If nullptr, function returns immediately without action

## Behavior Requirements

### 1. Parse PGN Message

```cpp
if (ParseN2kPGN<NUMBER>(N2kMsg, field1, field2, ...) {
    // Parse succeeded - continue to step 2
} else {
    // Parse failed - log ERROR and return
    logger->broadcastLog(LogLevel::ERROR, "NMEA2000", "PGN<NUMBER>_PARSE_FAILED",
        F("{\"reason\":\"Failed to parse PGN <NUMBER>\"}"));
    return;
}
```

**Requirements**:
- ✅ Call NMEA2000 library `ParseN2kPGN<NUMBER>()` function
- ✅ Check return value (true = success, false = failure)
- ✅ On parse failure: Log ERROR level, return without updating BoatData

### 2. Check for Unavailable (NA) Values

```cpp
if (N2kIsNA(value)) {
    logger->broadcastLog(LogLevel::DEBUG, "NMEA2000", "PGN<NUMBER>_NA",
        F("{\"reason\":\"<Field name> not available\"}"));
    return;
}
```

**Requirements**:
- ✅ Check all critical fields using `N2kIsNA()` macro
- ✅ On NA value: Log DEBUG level, return without updating BoatData
- ✅ Preserve existing BoatData values (do not overwrite with NA)

### 3. Validate Data Ranges

```cpp
bool valid = DataValidation::isValid<Type>(value);
if (!valid) {
    logger->broadcastLog(LogLevel::WARN, "NMEA2000", "PGN<NUMBER>_OUT_OF_RANGE",
        String(F("{\"<field>\":")) + value + F(",\"clamped\":") +
        DataValidation::clamp<Type>(value) + F("}"));
    value = DataValidation::clamp<Type>(value);
}
```

**Requirements**:
- ✅ Validate all fields using `DataValidation::isValid<Type>()` functions
- ✅ On out-of-range: Clamp using `DataValidation::clamp<Type>()` function
- ✅ Log WARN level with original and clamped values
- ✅ Still update BoatData with clamped value (graceful degradation)

### 4. Update BoatData Structure

```cpp
<DataType> data = boatData->get<DataType>();
data.field = value;
data.available = true;
data.lastUpdate = millis();
boatData->set<DataType>(data);
```

**Requirements**:
- ✅ Retrieve current data structure using `boatData->get<DataType>()`
- ✅ Update relevant fields with parsed/validated values
- ✅ Set `available = true` to indicate valid data
- ✅ Set `lastUpdate = millis()` to record timestamp
- ✅ Store updated structure using `boatData->set<DataType>()`

### 5. Log Successful Update

```cpp
logger->broadcastLog(LogLevel::DEBUG, "NMEA2000", "PGN<NUMBER>_UPDATE",
    String(F("{\"<field>\":")) + value + F(",\"unit\":\"<unit>\"}")));
```

**Requirements**:
- ✅ Log DEBUG level (verbose logging for diagnostics)
- ✅ Include PGN number in log identifier
- ✅ Include all updated field values and units
- ✅ Use JSON format for structured logging

### 6. Increment Message Counter

```cpp
boatData->incrementNMEA2000Count();
```

**Requirements**:
- ✅ Increment diagnostic counter for NMEA2000 messages
- ✅ Only increment on successful update (not on parse failure or NA)
- ✅ Counter used for system monitoring and diagnostics

## Special Handling Requirements

### Conditional Field Processing

Some PGNs require conditional logic based on field values:

#### Engine Instance Filtering (PGN 127488, 127489)

```cpp
unsigned char engineInstance;
// ... parse fields including engineInstance ...

if (engineInstance != 0) {
    // Silently ignore non-primary engine
    return;
}
// Continue processing engine instance 0
```

**Requirements**:
- ✅ Only process engine instance 0 (primary engine)
- ✅ Silently ignore other instances (no log, no update)

#### Wind Reference Type Filtering (PGN 130306)

```cpp
tN2kWindReference windReference;
// ... parse fields including windReference ...

if (windReference != N2kWind_Apparent) {
    // Silently ignore non-apparent wind references
    return;
}
// Continue processing apparent wind data
```

**Requirements**:
- ✅ Only process apparent wind reference type
- ✅ Silently ignore true wind, ground wind, etc. (no log, no update)

#### Temperature Source Filtering (PGN 130316)

```cpp
tN2kTempSource tempSource;
// ... parse fields including tempSource ...

if (tempSource != N2kts_SeaTemperature) {
    // Silently ignore non-sea temperature sources
    return;
}
// Continue processing sea temperature data
```

**Requirements**:
- ✅ Only process sea temperature source
- ✅ Silently ignore engine temp, cabin temp, etc. (no log, no update)

#### Heading Reference Type Routing (PGN 127250)

```cpp
tN2kHeadingReference headingReference;
// ... parse fields including headingReference ...

if (headingReference == N2khr_true) {
    compass.trueHeading = heading;
} else if (headingReference == N2khr_magnetic) {
    compass.magneticHeading = heading;
} else {
    // Unknown reference type - ignore
    return;
}
```

**Requirements**:
- ✅ Route to `trueHeading` if reference is `N2khr_true`
- ✅ Route to `magneticHeading` if reference is `N2khr_magnetic`
- ✅ Ignore other reference types

## Execution Time Requirements

**Maximum Handler Execution Time**: 1ms per message

**Rationale**:
- 100 messages/second typical load → 100ms/second total
- 1000 messages/second peak load → 1000ms/second total
- Must not block ReactESP event loop
- CAN interrupt must return quickly

**Prohibited Operations**:
- ❌ Blocking delays (`delay()`, `delayMicroseconds()`)
- ❌ Network I/O (HTTP requests, DNS lookups)
- ❌ File I/O (flash writes, SD card access)
- ❌ Complex calculations (matrix operations, trigonometry)

**Allowed Operations**:
- ✅ Simple arithmetic (+, -, *, /)
- ✅ Range checks (min/max comparisons)
- ✅ WebSocket logging (async, non-blocking)
- ✅ BoatData structure updates (in-memory, fast)

## Memory Requirements

**Stack Usage**: ≤150 bytes per handler call

**Components**:
- Handler local variables: ~50 bytes
- NMEA2000 parse function: ~100 bytes
- No heap allocation permitted (Principle II)

**Rationale**: ESP32 default task stack is 8KB; handlers must not consume excessive stack space.

## Error Recovery

### Parse Failure Recovery

```cpp
if (!ParseN2kPGN<NUMBER>(...)) {
    logger->broadcastLog(LogLevel::ERROR, ...);
    return;  // Do not update BoatData
}
```

**Behavior**:
- ✅ Existing BoatData values preserved
- ✅ `availability` flag unchanged (may still be true from previous update)
- ✅ `lastUpdate` timestamp unchanged
- ✅ System continues normal operation (graceful degradation)

### Out-of-Range Value Recovery

```cpp
if (!valid) {
    logger->broadcastLog(LogLevel::WARN, ...);
    value = DataValidation::clamp<Type>(value);
}
// Continue with clamped value
```

**Behavior**:
- ✅ Clamped value still usable (better than no data)
- ✅ WARN log alerts operator to sensor issue
- ✅ System continues normal operation (graceful degradation)

## Testing Requirements

### Unit Tests (test_nmea2000_units/)

- ✅ Parse valid PGN message → BoatData updated correctly
- ✅ Parse PGN with N2kDoubleNA → BoatData unchanged, DEBUG log
- ✅ Parse PGN with out-of-range value → BoatData updated with clamped value, WARN log
- ✅ Parse invalid PGN message → BoatData unchanged, ERROR log
- ✅ Handler called with nullptr boatData → No crash, immediate return
- ✅ Handler called with nullptr logger → No crash, immediate return

### Integration Tests (test_nmea2000_integration/)

- ✅ Multiple PGNs update same data structure correctly
- ✅ Handler execution time <1ms per message
- ✅ Handler does not block ReactESP event loop

## Example Implementation

```cpp
void HandleN2kPGN129025(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger) {
    // Precondition checks
    if (boatData == nullptr || logger == nullptr) return;

    // 1. Parse PGN message
    double latitude, longitude;
    if (ParseN2kPGN129025(N2kMsg, latitude, longitude)) {

        // 2. Check for unavailable values
        if (N2kIsNA(latitude) || N2kIsNA(longitude)) {
            logger->broadcastLog(LogLevel::DEBUG, "NMEA2000", "PGN129025_NA",
                F("{\"reason\":\"Position not available\"}"));
            return;
        }

        // 3. Validate data ranges
        bool validLat = DataValidation::isValidLatitude(latitude);
        bool validLon = DataValidation::isValidLongitude(longitude);
        if (!validLat || !validLon) {
            logger->broadcastLog(LogLevel::WARN, "NMEA2000", "PGN129025_OUT_OF_RANGE",
                String(F("{\"latitude\":")) + latitude + F(",\"longitude\":") + longitude + F("}"));
            latitude = DataValidation::clampLatitude(latitude);
            longitude = DataValidation::clampLongitude(longitude);
        }

        // 4. Update BoatData structure
        GPSData gps = boatData->getGPSData();
        gps.latitude = latitude;
        gps.longitude = longitude;
        gps.available = true;
        gps.lastUpdate = millis();
        boatData->setGPSData(gps);

        // 5. Log successful update
        logger->broadcastLog(LogLevel::DEBUG, "NMEA2000", "PGN129025_UPDATE",
            String(F("{\"latitude\":")) + latitude + F(",\"longitude\":") + longitude + F("}"));

        // 6. Increment message counter
        boatData->incrementNMEA2000Count();

    } else {
        // Parse failed
        logger->broadcastLog(LogLevel::ERROR, "NMEA2000", "PGN129025_PARSE_FAILED",
            F("{\"reason\":\"Failed to parse PGN 129025\"}"));
    }
}
```

## Compliance Checklist

Before marking a handler function as complete, verify:

- [ ] Follows function signature `void HandleN2kPGN<NUMBER>(const tN2kMsg&, BoatData*, WebSocketLogger*)`
- [ ] Checks for nullptr parameters (boatData, logger)
- [ ] Calls NMEA2000 library `ParseN2kPGN<NUMBER>()` function
- [ ] Handles parse failure with ERROR log
- [ ] Checks all critical fields for N2kIsNA values
- [ ] Validates all fields using `DataValidation` helpers
- [ ] Clamps out-of-range values with WARN log
- [ ] Updates BoatData with correct fields
- [ ] Sets `available=true` and `lastUpdate=millis()`
- [ ] Logs successful update with DEBUG level
- [ ] Increments NMEA2000 message counter
- [ ] Handles conditional logic (instance, reference type, source type) if applicable
- [ ] Execution time <1ms per message
- [ ] No blocking operations (delay, I/O, complex calculations)
- [ ] Stack usage ≤150 bytes
- [ ] No heap allocation
- [ ] Unit tests written and passing
- [ ] Integration tests written and passing

## Version History

- **1.0.0** (2025-10-12): Initial contract definition
