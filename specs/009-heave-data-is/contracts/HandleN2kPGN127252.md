# API Contract: HandleN2kPGN127252

**Feature**: NMEA2000 PGN 127252 Heave Handler
**Function**: `HandleN2kPGN127252`
**File**: `src/components/NMEA2000Handlers.cpp` (implementation), `src/components/NMEA2000Handlers.h` (declaration)
**Date**: 2025-10-11

---

## Function Signature

```cpp
/**
 * @brief Handle PGN 127252 - Heave
 *
 * Parses NMEA2000 PGN 127252 (Heave) messages to extract vertical displacement data,
 * validates the value against the acceptable range (±5.0 meters), and updates the
 * CompassData.heave field in the BoatData structure. Logs all operations via WebSocket
 * for debugging and monitoring.
 *
 * Sign convention: positive = upward motion (vessel rising above reference plane)
 *
 * Validation:
 * - Valid range: [-5.0, 5.0] meters
 * - Out-of-range values: Clamped to limits with WARN log
 * - Unavailable (N2kDoubleNA): Logged at DEBUG, update skipped
 * - Parse failure: Logged at ERROR, availability set to false
 *
 * @param N2kMsg NMEA2000 message (const reference, not modified)
 * @param boatData BoatData instance to update (must not be nullptr)
 * @param logger WebSocket logger for debug output (must not be nullptr)
 *
 * @pre boatData != nullptr
 * @pre logger != nullptr
 * @post If valid heave parsed: CompassData.heave updated, availability=true, timestamp=millis()
 * @post WebSocket log entry created (DEBUG/WARN/ERROR depending on outcome)
 * @post NMEA2000 message counter incremented (if successful)
 *
 * @see ParseN2kPGN127252 (NMEA2000 library function)
 * @see DataValidation::clampHeave
 * @see DataValidation::isValidHeave
 */
void HandleN2kPGN127252(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger);
```

---

## Parameters

### Input Parameters

#### 1. `N2kMsg` (const tN2kMsg &)
- **Type**: Const reference to NMEA2000 message
- **Description**: NMEA2000 PGN 127252 message containing heave data
- **Constraints**: Must be valid NMEA2000 message (validated by library)
- **Modification**: NOT modified (const reference)
- **Lifetime**: Valid only during handler execution (passed by reference)

**Structure** (internal to NMEA2000 library):
```cpp
// tN2kMsg is opaque - parsed via ParseN2kPGN127252()
// Contains:
// - PGN: 127252 (Parameter Group Number)
// - Data length
// - Data bytes (encoded heave, delay, delaySource)
```

#### 2. `boatData` (BoatData*)
- **Type**: Pointer to BoatData instance
- **Description**: Central marine data repository to update with heave value
- **Constraints**:
  - **Must not be nullptr** (precondition checked in handler)
  - Must be initialized and ready for updates
  - Must support getCompassData() and setCompassData() methods
- **Modification**: Modified via setCompassData() call
- **Lifetime**: Outlives handler execution (global or long-lived object)

**Required Methods**:
```cpp
CompassData getCompassData();                      // Retrieve current compass data
void setCompassData(const CompassData& data);      // Update compass data
void incrementNMEA2000Count();                     // Increment message counter
```

#### 3. `logger` (WebSocketLogger*)
- **Type**: Pointer to WebSocketLogger instance
- **Description**: WebSocket logging interface for debug/warn/error messages
- **Constraints**:
  - **Must not be nullptr** (precondition checked in handler)
  - Must be initialized and connected (or buffering if WebSocket unavailable)
- **Modification**: Modified via broadcastLog() calls
- **Lifetime**: Outlives handler execution (global or long-lived object)

**Required Methods**:
```cpp
void broadcastLog(LogLevel level, const char* component, const char* event, const String& data);
```

**Log Levels Used**:
- `LogLevel::DEBUG` - Successful heave update, unavailable data (N2kDoubleNA)
- `LogLevel::WARN` - Out-of-range heave value (clamping applied)
- `LogLevel::ERROR` - Parse failure

### Output/Side Effects

#### 1. CompassData.heave Update
**Condition**: Valid heave value parsed and validated
**Action**: `boatData->setCompassData()` called with updated CompassData
**Fields Modified**:
```cpp
compass.heave = validatedHeave;  // Updated with clamped value
compass.available = true;        // Marks data as available
compass.lastUpdate = millis();   // Timestamp of update
// Other fields (trueHeading, magneticHeading, etc.) UNCHANGED
```

#### 2. WebSocket Log Entries
**Condition**: Always (varies by outcome)
**Log Messages**:

**Success (DEBUG)**:
```json
{"level":"DEBUG","component":"NMEA2000","event":"PGN127252_UPDATE","data":"{\"heave\":2.5,\"meters\":true}"}
```

**Out-of-Range (WARN)**:
```json
{"level":"WARN","component":"NMEA2000","event":"PGN127252_OUT_OF_RANGE","data":"{\"heave\":6.2,\"clamped\":5.0}"}
```

**Unavailable (DEBUG)**:
```json
{"level":"DEBUG","component":"NMEA2000","event":"PGN127252_NA","data":"{\"reason\":\"Heave not available\"}"}
```

**Parse Failure (ERROR)**:
```json
{"level":"ERROR","component":"NMEA2000","event":"PGN127252_PARSE_FAILED","data":"{\"reason\":\"Failed to parse PGN 127252\"}"}
```

#### 3. NMEA2000 Message Counter Increment
**Condition**: Valid heave value parsed and stored
**Action**: `boatData->incrementNMEA2000Count()` called
**Effect**: Increments internal counter for NMEA2000 message statistics

---

## Preconditions

### 1. Non-Null Pointers
```cpp
REQUIRE(boatData != nullptr);
REQUIRE(logger != nullptr);
```
**Check**: First statement in handler
**Violation**: Silent return (no error thrown, function exits immediately)

### 2. BoatData Initialized
```cpp
REQUIRE(boatData is initialized and ready for updates);
```
**Check**: Implicit (assumed by caller)
**Violation**: Undefined behavior if BoatData not initialized

### 3. WebSocketLogger Initialized
```cpp
REQUIRE(logger is initialized and ready for logging);
```
**Check**: Implicit (assumed by caller)
**Violation**: May crash if logger not initialized (but pointer check prevents nullptr dereference)

### 4. NMEA2000 Message Valid
```cpp
REQUIRE(N2kMsg is valid NMEA2000 message);
REQUIRE(N2kMsg.PGN == 127252);  // Implied by handler registration
```
**Check**: Performed by NMEA2000 library before calling handler
**Violation**: Handler may fail to parse (returns false, logs ERROR)

---

## Postconditions

### Success Case (Valid Heave)
```cpp
ENSURE(CompassData.heave == clampHeave(parsedHeave));
ENSURE(CompassData.available == true);
ENSURE(CompassData.lastUpdate == millis());  // Within ±10ms tolerance
ENSURE(NMEA2000 message counter incremented by 1);
ENSURE(WebSocket log entry created with level DEBUG);
```

### Out-of-Range Case
```cpp
ENSURE(CompassData.heave == clampedValue);  // Either -5.0 or 5.0
ENSURE(CompassData.available == true);      // Still considered available
ENSURE(WebSocket log entry created with level WARN, showing original and clamped values);
```

### Unavailable Case (N2kDoubleNA)
```cpp
ENSURE(CompassData.heave UNCHANGED);
ENSURE(CompassData.available UNCHANGED);
ENSURE(CompassData.lastUpdate UNCHANGED);
ENSURE(NMEA2000 message counter NOT incremented);
ENSURE(WebSocket log entry created with level DEBUG, reason "Heave not available");
```

### Parse Failure Case
```cpp
ENSURE(CompassData.heave UNCHANGED);
ENSURE(CompassData.available UNCHANGED);
ENSURE(CompassData.lastUpdate UNCHANGED);
ENSURE(NMEA2000 message counter NOT incremented);
ENSURE(WebSocket log entry created with level ERROR, reason "Failed to parse PGN 127252");
```

### Null Pointer Case (Precondition Violation)
```cpp
ENSURE(Function returns immediately);
ENSURE(No side effects);
ENSURE(No crash or exception);
```

---

## Error Handling

### 1. Null Pointer Parameters
**Error**: `boatData == nullptr` OR `logger == nullptr`
**Handling**: Silent return (function exits immediately)
**Rationale**: Defensive programming, prevents segfault
**Log**: None (logger may be nullptr, cannot log)
**Recovery**: Caller must ensure valid pointers

**Implementation**:
```cpp
if (boatData == nullptr || logger == nullptr) return;
```

### 2. Parse Failure
**Error**: `ParseN2kPGN127252()` returns `false`
**Handling**: Log ERROR, do not update BoatData
**Log**: `"PGN127252_PARSE_FAILED"` at ERROR level
**Recovery**: Skip this message, wait for next valid message

**Implementation**:
```cpp
if (!ParseN2kPGN127252(N2kMsg, SID, heave, delay, delaySource)) {
    logger->broadcastLog(LogLevel::ERROR, "NMEA2000", "PGN127252_PARSE_FAILED",
        F("{\"reason\":\"Failed to parse PGN 127252\"}"));
    return;
}
```

### 3. Unavailable Data (N2kDoubleNA)
**Error**: Heave value is `N2kDoubleNA` (NMEA2000 "not available" marker)
**Handling**: Log DEBUG, do not update BoatData
**Log**: `"PGN127252_NA"` at DEBUG level
**Recovery**: Skip this message, wait for next valid message

**Implementation**:
```cpp
if (N2kIsNA(heave)) {
    logger->broadcastLog(LogLevel::DEBUG, "NMEA2000", "PGN127252_NA",
        F("{\"reason\":\"Heave not available\"}"));
    return;
}
```

### 4. Out-of-Range Value
**Error**: Heave < -5.0 OR heave > 5.0
**Handling**: Clamp to range, log WARN, store clamped value
**Log**: `"PGN127252_OUT_OF_RANGE"` at WARN level with original and clamped values
**Recovery**: Use clamped value (data still considered valid)

**Implementation**:
```cpp
bool valid = DataValidation::isValidHeave(heave);
if (!valid) {
    logger->broadcastLog(LogLevel::WARN, "NMEA2000", "PGN127252_OUT_OF_RANGE",
        String(F("{\"heave\":")) + heave + F(",\"clamped\":") +
        DataValidation::clampHeave(heave) + F("}"));
    heave = DataValidation::clampHeave(heave);
}
```

---

## Dependencies

### External Functions

#### 1. ParseN2kPGN127252 (NMEA2000 Library)
```cpp
bool ParseN2kPGN127252(const tN2kMsg &N2kMsg,
                       unsigned char &SID,
                       double &Heave,
                       double &Delay,
                       tN2kDelaySource &DelaySource);
```
**Purpose**: Parse PGN 127252 message into individual fields
**Returns**: `true` if parsing successful, `false` otherwise
**Library**: ttlappalainen/NMEA2000 (already integrated)

#### 2. N2kIsNA (NMEA2000 Library)
```cpp
bool N2kIsNA(double value);
```
**Purpose**: Check if value is NMEA2000 "not available" marker
**Returns**: `true` if value is N2kDoubleNA, `false` otherwise
**Library**: ttlappalainen/NMEA2000 (already integrated)

#### 3. DataValidation::clampHeave (DataValidation Utility)
```cpp
static inline double clampHeave(double heave);
```
**Purpose**: Clamp heave value to [-5.0, 5.0] range
**Returns**: Clamped value
**File**: `src/utils/DataValidation.h` (already implemented)

#### 4. DataValidation::isValidHeave (DataValidation Utility)
```cpp
static inline bool isValidHeave(double heave);
```
**Purpose**: Check if heave value is within [-5.0, 5.0] range
**Returns**: `true` if valid, `false` if out-of-range
**File**: `src/utils/DataValidation.h` (already implemented)

#### 5. millis() (Arduino Core)
```cpp
unsigned long millis();
```
**Purpose**: Get milliseconds since ESP32 boot
**Returns**: Milliseconds (wraps after ~49 days)
**Library**: Arduino core (always available)

---

## Invariants

### 1. Heave Range Invariant
```cpp
INVARIANT: CompassData.heave is always within [-5.0, 5.0] meters
```
**Enforcement**: `clampHeave()` ensures value never exceeds limits
**Violation**: Impossible (enforced by validation)

### 2. Availability Invariant
```cpp
INVARIANT: If CompassData.available == true, then at least one field has been updated
```
**Enforcement**: availability flag set to true after successful update
**Violation**: Not possible (flag set atomically with data update)

### 3. Timestamp Invariant
```cpp
INVARIANT: If CompassData.available == true, then lastUpdate >= (time of first update)
```
**Enforcement**: lastUpdate set to millis() on each successful update
**Violation**: Not possible (millis() is monotonically increasing, ignoring wraparound)

---

## Performance Characteristics

### Time Complexity
**Best Case**: O(1) - All operations constant time
**Worst Case**: O(1) - No loops or recursion
**Average Case**: O(1)

**Breakdown**:
- Null pointer checks: O(1)
- Parse function: O(1) (fixed message size)
- Validation: O(1) (simple comparison)
- Clamping: O(1) (simple min/max)
- Data copy: O(1) (fixed struct size, 69 bytes)
- Logging: O(1) (buffered, non-blocking)
- Counter increment: O(1)

### Space Complexity
**Stack**: ~200 bytes (local variables + function call overhead)
**Heap**: 0 bytes (no dynamic allocation)

**Stack Variables**:
- `SID` (1 byte)
- `heave` (8 bytes)
- `delay` (8 bytes)
- `delaySource` (4 bytes)
- `compass` (69 bytes)
- Function overhead (~100 bytes)

### Execution Time
**Target**: <1ms per message (non-blocking requirement)
**Estimated**:
- Parse: <100μs
- Validation: <10μs
- Logging: <50μs (buffered)
- Data update: <20μs
- **Total**: <200μs (well within <1ms target)

**Verification**: Hardware test will measure actual execution time

---

## Thread Safety

### Concurrency Model
**Single-threaded**: ReactESP event loop (Arduino framework)
**Handler Execution**: Sequential (one message at a time)
**No Locks Required**: Arduino framework does not use preemptive threading

### Reentrancy
**Not Reentrant**: Handler should not be called recursively
**Protection**: ReactESP event queue ensures sequential execution
**Assumption**: NMEA2000 library does not call handlers from interrupt context

### Shared State
**Read**: CompassData structure (via getCompassData())
**Write**: CompassData structure (via setCompassData())
**Protection**: Sequential execution ensures no race conditions

---

## Testing Requirements

### Contract Test (test_boatdata_contracts/)
**Purpose**: Validate handler signature and calling convention
**Tests**:
1. Function exists and is callable
2. Null pointer checks prevent crashes
3. Function returns void (no return value)
4. Parameters passed correctly

### Integration Tests (test_boatdata_integration/)
**Purpose**: Test complete heave data flow with mocked messages
**Test Scenarios**:
1. **Valid heave (2.5m)**: Value stored correctly
2. **Out-of-range heave (6.0m)**: Clamped to 5.0m, WARN logged
3. **Negative heave (-3.2m)**: Stored correctly (valid range)
4. **Unavailable (N2kDoubleNA)**: No update, DEBUG logged
5. **Parse failure**: ERROR logged, availability unchanged

### Unit Tests (test_boatdata_units/)
**Purpose**: Test validation functions
**Status**: Already tested in Enhanced BoatData v2.0.0 (reuse existing tests)
**Functions Tested**:
- `DataValidation::clampHeave()`
- `DataValidation::isValidHeave()`

### Hardware Tests (test_boatdata_hardware/)
**Purpose**: Validate NMEA2000 bus timing with real hardware
**Test**: Placeholder for PGN 127252 reception timing
**Status**: Pending NMEA2000 bus initialization

---

## Contract Status

**Status**: ✅ COMPLETE

**Preconditions**: 3 (null checks, initialization)

**Postconditions**: 4 outcome cases (success, out-of-range, unavailable, parse failure)

**Error Handling**: 4 error cases defined

**Dependencies**: 5 external functions identified

**Invariants**: 3 invariants enforced

**Performance**: <1ms execution time target

**Thread Safety**: Single-threaded, sequential execution

**Testing**: 4 test types planned

**Ready for**: Quickstart scenarios (Phase 1 continued)
