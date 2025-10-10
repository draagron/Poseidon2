# Technical Research: NMEA2000 PGN 127252 Heave Handler

**Feature**: PGN 127252 Heave Handler
**Date**: 2025-10-11
**Research Phase**: Complete ✓

## Research Questions & Answers

### 1. NMEA2000 Library Support for PGN 127252

**Question**: Does the NMEA2000 library provide a parsing function for PGN 127252?

**Answer**: YES ✓

**Details**:
- **Library**: ttlappalainen/NMEA2000 (https://github.com/ttlappalainen/NMEA2000)
- **File**: src/N2kMessages.h
- **Parse Function Signature**:
  ```cpp
  bool ParseN2kPGN127252(const tN2kMsg &N2kMsg,
                         unsigned char &SID,
                         double &Heave,
                         double &Delay,
                         tN2kDelaySource &DelaySource);
  ```
- **Return Value**: `bool` - true if parsing successful, false otherwise
- **Parameters**:
  - `SID`: Sequence identifier (ties related PGNs together)
  - `Heave`: Vertical displacement in meters (output parameter)
  - `Delay`: Optional delay added by calculations in seconds (output parameter)
  - `DelaySource`: Optional source of delay (output parameter, enum type)
- **Units**: Heave returned in meters (NMEA2000 native unit)
- **Sign Convention**: Not explicitly documented, but marine standard is positive = upward motion

**Source**: https://github.com/ttlappalainen/NMEA2000/blob/master/src/N2kMessages.h (lines ~1600-1620)

---

### 2. CompassData.heave Field Existence

**Question**: Does the `CompassData` structure already have a `heave` field?

**Answer**: YES ✓

**Details**:
- **File**: `src/types/BoatDataTypes.h`
- **Structure**: `struct CompassData`
- **Field Definition**:
  ```cpp
  struct CompassData {
      double trueHeading;        // Radians [0, 2π]
      double magneticHeading;    // Radians [0, 2π]
      double rateOfTurn;         // Radians/second, positive = starboard turn
      double heelAngle;          // Radians [-π/2, π/2], positive = starboard
      double pitchAngle;         // Radians [-π/6, π/6], positive = bow up
      double heave;              // Meters [-5.0, 5.0], positive = upward **ALREADY EXISTS**
      bool available;
      unsigned long lastUpdate;
  };
  ```
- **Valid Range**: ±5.0 meters
- **Sign Convention**: Positive = upward motion (vessel rising above reference)
- **Added In**: Enhanced BoatData v2.0.0 (R005, spec 008-enhanced-boatdata-following)
- **Note**: PGN 127257 (Attitude) handler currently does NOT populate heave field (commented as needing separate PGN)

**Source**: `src/types/BoatDataTypes.h` lines 45-54

---

### 3. DataValidation Helpers for Heave

**Question**: Do validation and clamping functions exist for heave data?

**Answer**: YES ✓

**Details**:
- **File**: `src/utils/DataValidation.h`
- **Functions**:
  ```cpp
  /**
   * @brief Clamp heave value to valid range [-5.0, 5.0] meters
   */
  static inline double clampHeave(double heave) {
      if (heave < -5.0) return -5.0;
      if (heave > 5.0) return 5.0;
      return heave;
  }

  /**
   * @brief Check if heave value is within valid range
   */
  static inline bool isValidHeave(double heave) {
      return (heave >= -5.0 && heave <= 5.0);
  }
  ```
- **Range**: [-5.0, 5.0] meters
- **Behavior**: Out-of-range values are clamped (not rejected)
- **Added In**: Enhanced BoatData v2.0.0 (R005)
- **Usage**: Already used in other handlers for consistent validation

**Source**: `src/utils/DataValidation.h` lines 120-135

---

### 4. Existing Handler Implementation Pattern

**Question**: What is the standard pattern for NMEA2000 PGN handlers?

**Answer**: Well-established pattern in `src/components/NMEA2000Handlers.cpp`

**Pattern Structure** (12 steps):
1. **Null Pointer Checks**: Return early if `boatData == nullptr || logger == nullptr`
2. **Parse PGN**: Call NMEA2000 library parse function (e.g., `ParseN2kPGN127252`)
3. **Check Availability**: Use `N2kIsNA()` to detect unavailable data (N2kDoubleNA)
4. **Log Unavailable**: If NA, log DEBUG message and return early
5. **Validate Range**: Call `DataValidation::isValid*()` function
6. **Log Out-of-Range**: If invalid, log WARN with original and clamped values
7. **Clamp Value**: Apply `DataValidation::clamp*()` function
8. **Get Current Data**: Retrieve current BoatData structure (e.g., `getCompassData()`)
9. **Update Field**: Modify the target field(s)
10. **Set Metadata**: Update `available = true` and `lastUpdate = millis()`
11. **Store Data**: Call BoatData setter (e.g., `setCompassData()`)
12. **Log Success**: Log DEBUG message with updated values
13. **Increment Counter**: Call `boatData->incrementNMEA2000Count()`
14. **Handle Parse Failure**: Else block logs ERROR if parse function returns false

**Reference Handlers**:
- **PGN 127251** (Rate of Turn): Lines 21-65 in NMEA2000Handlers.cpp
- **PGN 127257** (Attitude): Lines 71-128 in NMEA2000Handlers.cpp
- **PGN 129029** (GNSS Position): Lines 134-200 in NMEA2000Handlers.cpp

**Example Code Flow** (PGN 127251 as reference):
```cpp
void HandleN2kPGN127251(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger) {
    if (boatData == nullptr || logger == nullptr) return;  // Step 1

    unsigned char SID;
    double rateOfTurn;

    if (ParseN2kPGN127251(N2kMsg, SID, rateOfTurn)) {  // Step 2
        if (N2kIsNA(rateOfTurn)) {  // Step 3
            logger->broadcastLog(LogLevel::DEBUG, "NMEA2000", "PGN127251_NA",
                F("{\"reason\":\"Rate of turn not available\"}"));  // Step 4
            return;
        }

        bool valid = DataValidation::isValidRateOfTurn(rateOfTurn);  // Step 5
        if (!valid) {  // Step 6
            logger->broadcastLog(LogLevel::WARN, "NMEA2000", "PGN127251_OUT_OF_RANGE",
                String(F("{\"rateOfTurn\":")) + rateOfTurn + F(",\"clamped\":") +
                DataValidation::clampRateOfTurn(rateOfTurn) + F("}"));
            rateOfTurn = DataValidation::clampRateOfTurn(rateOfTurn);  // Step 7
        }

        CompassData compass = boatData->getCompassData();  // Step 8

        compass.rateOfTurn = rateOfTurn;  // Step 9
        compass.available = true;  // Step 10
        compass.lastUpdate = millis();

        boatData->setCompassData(compass);  // Step 11

        logger->broadcastLog(LogLevel::DEBUG, "NMEA2000", "PGN127251_UPDATE",
            String(F("{\"rateOfTurn\":")) + rateOfTurn + F(",\"rad_per_sec\":true}"));  // Step 12

        boatData->incrementNMEA2000Count();  // Step 13
    } else {  // Step 14
        logger->broadcastLog(LogLevel::ERROR, "NMEA2000", "PGN127251_PARSE_FAILED",
            F("{\"reason\":\"Failed to parse PGN 127251\"}"));
    }
}
```

**Source**: `src/components/NMEA2000Handlers.cpp` lines 21-65

---

### 5. Handler Registration Mechanism

**Question**: How are PGN handlers registered with the NMEA2000 library?

**Answer**: Via `RegisterN2kHandlers()` function

**Details**:
- **File**: `src/components/NMEA2000Handlers.cpp`
- **Function**: `RegisterN2kHandlers(tNMEA2000* nmea2000, BoatData* boatData, WebSocketLogger* logger)`
- **Mechanism**: Calls `SetN2kPGNHandler()` for each PGN
- **Integration Point**: `src/main.cpp` has placeholder comment for NMEA2000 initialization
- **Current Status**: NMEA2000 bus not yet initialized (placeholder ready)

**Example Registration**:
```cpp
void RegisterN2kHandlers(tNMEA2000* nmea2000, BoatData* boatData, WebSocketLogger* logger) {
    if (nmea2000 == nullptr || boatData == nullptr || logger == nullptr) return;

    // Register PGN 127251 (Rate of Turn)
    nmea2000->SetN2kPGNHandler(127251, [boatData, logger](const tN2kMsg &N2kMsg) {
        HandleN2kPGN127251(N2kMsg, boatData, logger);
    });

    // Register PGN 127257 (Attitude)
    nmea2000->SetN2kPGNHandler(127257, [boatData, logger](const tN2kMsg &N2kMsg) {
        HandleN2kPGN127257(N2kMsg, boatData, logger);
    });

    // ... more handlers ...

    // ADD: Register PGN 127252 (Heave) - TO BE ADDED
    nmea2000->SetN2kPGNHandler(127252, [boatData, logger](const tN2kMsg &N2kMsg) {
        HandleN2kPGN127252(N2kMsg, boatData, logger);
    });
}
```

**Placeholder in main.cpp**:
```cpp
// T040-T041: NMEA2000 initialization and PGN handler registration
// NOTE: NMEA2000 library initialization will be added in a future feature
// When NMEA2000 is initialized, call: RegisterN2kHandlers(&NMEA2000, boatData, &logger);
// This will register handlers for PGNs: 127251, 127257, 129029, 128267, 128259, 130316, 127488, 127489
Serial.println(F("NMEA2000 initialization placeholder - not yet implemented"));
logger.broadcastLog(LogLevel::INFO, "Main", "NMEA2000_PLACEHOLDER",
                    F("{\"status\":\"PGN handlers ready for registration when NMEA2000 is initialized\"}"));
```

**Source**: `src/components/NMEA2000Handlers.cpp` lines 550-600, `src/main.cpp` lines 428-434

---

### 6. Testing Infrastructure

**Question**: What test suites exist for BoatData features?

**Answer**: Comprehensive test infrastructure from Enhanced BoatData v2.0.0

**Test Suites**:
1. **Contract Tests** (`test_boatdata_contracts/`):
   - HAL interface validation
   - Data structure memory layout
   - Function signature checks
   - **Files**: test_main.cpp, test_ionewire.cpp, test_data_structures.cpp, test_memory_footprint.cpp
   - **Count**: 7 tests
   - **Platform**: Native (no hardware required)

2. **Integration Tests** (`test_boatdata_integration/`):
   - End-to-end scenarios with mocked hardware
   - Tests for each sensor type (GPS, compass, DST, engine, battery, saildrive, shore power)
   - **Files**: test_main.cpp, test_gps_variation.cpp, test_compass_attitude.cpp, test_compass_rate_of_turn.cpp, test_dst_sensors.cpp, test_engine_telemetry.cpp, test_battery_monitoring.cpp, test_saildrive_status.cpp, test_shore_power.cpp
   - **Count**: 15 tests
   - **Platform**: Native (mocked NMEA2000 messages)

3. **Unit Tests** (`test_boatdata_units/`):
   - Validation/clamping functions
   - Unit conversions (Kelvin → Celsius)
   - Sign conventions
   - **Files**: test_main.cpp, test_validation.cpp, test_unit_conversions.cpp, test_sign_conventions.cpp
   - **Count**: 13 tests
   - **Platform**: Native

4. **Hardware Tests** (`test_boatdata_hardware/`):
   - 1-wire bus communication (ESP32 required)
   - NMEA2000 timing placeholders (awaiting bus initialization)
   - **Files**: test_main.cpp
   - **Count**: 7 tests
   - **Platform**: ESP32 only

**New Tests for PGN 127252**:
- **Contract**: Add handler signature test to `test_boatdata_contracts/test_main.cpp`
- **Integration**: Create `test_heave_from_pgn127252.cpp` in `test_boatdata_integration/`
- **Unit**: Heave validation already tested (reuse existing)
- **Hardware**: Update placeholder in `test_boatdata_hardware/test_main.cpp`

**Source**: `test/test_boatdata_*/` directories

---

## Technical Decisions

### 1. Handler Function Signature
**Decision**: Follow existing pattern exactly
```cpp
void HandleN2kPGN127252(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger);
```
**Rationale**: Consistency with 8 existing handlers ensures maintainability

---

### 2. Heave Field Location
**Decision**: Store in `CompassData.heave`
**Rationale**:
- Field already exists (added in Enhanced BoatData v2.0.0)
- Heave is vessel motion data, logically grouped with heel/pitch/rate of turn
- No need for separate structure (avoids unnecessary complexity)

**Alternative Considered**: Separate `HeaveData` structure
**Rejected**: Adds complexity without benefit, heave is fundamentally motion data

---

### 3. Validation Range
**Decision**: Use existing ±5.0 meter range
**Rationale**:
- Matches typical marine sensor operating range
- Validation functions already implemented (clampHeave, isValidHeave)
- Consistent with Enhanced BoatData v2.0.0 design

---

### 4. Optional Parameters (Delay, DelaySource)
**Decision**: Parse but DO NOT store
**Rationale**:
- Delay parameters rarely used in practice
- Primary use case is heave value only
- Can add storage later if needed (future enhancement)
- Reduces memory footprint (no new fields)

**Future Enhancement**: If delay data needed, add `DelayData` structure:
```cpp
struct DelayData {
    double delay;  // seconds
    tN2kDelaySource source;
    bool available;
};
```

---

### 5. Test Strategy
**Decision**: TDD approach with 4 test types
1. **Contract Test**: Handler signature validation
2. **Integration Tests**: 5 scenarios (valid, out-of-range, negative, N/A, parse failure)
3. **Unit Tests**: Reuse existing validation tests
4. **Hardware Test**: Placeholder for NMEA2000 timing

**Rationale**:
- Follows Enhanced BoatData v2.0.0 test pattern
- Tests written BEFORE implementation (TDD gate)
- Native platform testing (no hardware required for most tests)
- Constitutional compliance (Principle III: QA-First Review)

---

## Dependencies

### Upstream Dependencies
- **NMEA2000 Library** (ttlappalainen/NMEA2000)
  - Provides ParseN2kPGN127252 function
  - Already integrated in project
  - No version changes required

### Horizontal Dependencies (Existing Components)
- **DataValidation.h** (`src/utils/DataValidation.h`)
  - clampHeave() function
  - isValidHeave() function
  - Already implemented in Enhanced BoatData v2.0.0

- **WebSocketLogger** (`src/utils/WebSocketLogger.h`)
  - broadcastLog() method
  - LogLevel enum (DEBUG/WARN/ERROR)
  - Already integrated

- **BoatData Component** (`src/components/BoatData.h`)
  - getCompassData() method
  - setCompassData() method
  - incrementNMEA2000Count() method
  - Already implemented

- **BoatDataTypes** (`src/types/BoatDataTypes.h`)
  - CompassData structure with heave field
  - Already defined in Enhanced BoatData v2.0.0

### Downstream Dependencies
**None** - Handler is a leaf component (no other components depend on it)

---

## Risks & Mitigations

### 1. NMEA2000 Bus Not Yet Initialized
**Risk Level**: LOW
**Description**: `src/main.cpp` has placeholder for NMEA2000 initialization, bus not yet active
**Mitigation**:
- Handler can be implemented and tested independently
- Integration tests use mocked NMEA2000 messages
- Placeholder in main.cpp ready for handler registration
- Handler will be automatically available when NMEA2000 bus is initialized
**Impact**: Minimal - handler ready for future integration

---

### 2. High-Frequency Heave Data (>10 Hz)
**Risk Level**: LOW
**Description**: Some marine motion sensors transmit heave data at high frequency
**Mitigation**:
- Handler is stateless and non-blocking (ReactESP event-driven)
- No loops or delays in handler code
- ReactESP event loop handles message queueing
- Heave updates are independent (no inter-message dependencies)
**Impact**: Minimal - ReactESP architecture designed for high-frequency events

---

### 3. Memory Footprint Increase
**Risk Level**: NEGLIGIBLE
**Description**: Adding handler function increases flash usage
**Mitigation**:
- No new RAM allocations (reuses existing CompassData structure)
- Flash increase minimal: estimated +2KB (~0.08% of partition)
- Current flash usage: 47.7% (plenty of headroom before 80% warning)
- Compile-time flash usage checks will detect if approaching limits
**Impact**: Negligible - well within flash budget

---

## Research Conclusions

### Implementation Feasibility: ✅ CONFIRMED

All prerequisites exist:
- ✅ NMEA2000 library provides ParseN2kPGN127252
- ✅ CompassData.heave field already defined
- ✅ Validation functions already implemented
- ✅ Handler pattern well-established (8 existing handlers)
- ✅ Test infrastructure in place (42 existing tests)
- ✅ WebSocket logging integrated
- ✅ BoatData component supports CompassData updates

### Implementation Complexity: LOW

Reasons for low complexity:
- Reuses 100% of existing infrastructure
- Single handler function (~60 lines)
- No new data structures required
- No new dependencies
- Follows well-established pattern
- High code reuse (validation, logging, data storage)

### Estimated Implementation Effort

**Code Changes**:
- `NMEA2000Handlers.h`: +15 lines (function declaration + comments)
- `NMEA2000Handlers.cpp`: +60 lines (handler implementation)
- `NMEA2000Handlers.cpp`: +3 lines (registration call)
- **Total**: ~78 lines of new code

**Test Changes**:
- Contract test: +15 lines
- Integration tests: +150 lines (5 scenarios)
- Hardware test placeholder: +10 lines
- **Total**: ~175 lines of test code

**Documentation Changes**:
- CLAUDE.md: +50 lines (PGN 127252 documentation)
- CHANGELOG.md: +30 lines (feature summary)
- **Total**: ~80 lines of documentation

**Grand Total**: ~333 lines (code + tests + docs)

### Recommended Next Steps

1. ✅ **Phase 0 Complete**: Research.md generated
2. **Next**: Generate Phase 1 artifacts (data-model.md, contracts/, quickstart.md)
3. **Then**: /tasks command to generate task breakdown
4. **Finally**: /implement command to execute implementation

---

**Research Status**: ✅ COMPLETE
**Phase 0 Gate**: ✅ PASS
**Ready for Phase 1**: ✅ YES
