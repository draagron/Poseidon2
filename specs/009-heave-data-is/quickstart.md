# Quickstart Guide: NMEA2000 PGN 127252 Heave Handler

**Feature**: PGN 127252 Heave Handler
**Date**: 2025-10-11
**Purpose**: Step-by-step validation scenarios for testing the heave handler implementation

---

## Prerequisites

### Hardware Requirements
- **Optional**: ESP32 board with NMEA2000 transceiver (for hardware testing only)
- **Optional**: NMEA2000 motion sensor capable of transmitting PGN 127252
- **For Most Tests**: Native platform (no hardware required - uses mocked messages)

### Software Requirements
- PlatformIO installed
- Repository cloned and on branch `009-heave-data-is`
- All dependencies installed (`pio lib install`)

### Build Verification
```bash
# Verify project builds successfully
pio run
```

**Expected Output**: Build SUCCESS, no errors

---

## Scenario 1: Valid Heave Value

**Description**: NMEA2000 sensor transmits valid heave value (2.5m upward), handler parses and stores correctly.

**Preconditions**:
- BoatData initialized
- WebSocketLogger initialized
- NMEA2000 handler registered (or mock test environment)

**Test Steps**:
1. Create mock PGN 127252 message with heave = 2.5 meters
2. Call `HandleN2kPGN127252(message, boatData, logger)`
3. Retrieve CompassData via `boatData->getCompassData()`
4. Verify heave field updated

**Expected Results**:
```cpp
// Verify data stored correctly
CompassData compass = boatData->getCompassData();
ASSERT_EQUAL(2.5, compass.heave, 0.001);  // heave = 2.5m ±0.001m tolerance
ASSERT_TRUE(compass.available);            // availability flag set
ASSERT_TRUE(compass.lastUpdate > 0);       // timestamp set

// Verify WebSocket log (DEBUG level)
// Expected log entry:
{
  "level": "DEBUG",
  "component": "NMEA2000",
  "event": "PGN127252_UPDATE",
  "data": "{\"heave\":2.5,\"meters\":true}"
}
```

**Integration Test Code** (`test_boatdata_integration/test_heave_from_pgn127252.cpp`):
```cpp
void test_heave_valid_positive_value() {
    // Setup
    MockBoatData boatData;
    MockWebSocketLogger logger;
    tN2kMsg message = createMockPGN127252(
        /* SID */ 1,
        /* Heave */ 2.5,
        /* Delay */ N2kDoubleNA,
        /* DelaySource */ N2kDD374_DataNotAvailable
    );

    // Execute
    HandleN2kPGN127252(message, &boatData, &logger);

    // Verify
    CompassData compass = boatData.getCompassData();
    TEST_ASSERT_EQUAL_DOUBLE(2.5, compass.heave);
    TEST_ASSERT_TRUE(compass.available);
    TEST_ASSERT_TRUE(compass.lastUpdate > 0);

    // Verify log entry
    TEST_ASSERT_TRUE(logger.hasLogEntry(LogLevel::DEBUG, "PGN127252_UPDATE"));
}
```

**Pass Criteria**:
- ✅ Heave value stored as 2.5 meters
- ✅ Availability flag set to true
- ✅ Timestamp updated (millis())
- ✅ DEBUG log entry created
- ✅ NMEA2000 message counter incremented

---

## Scenario 2: Out-of-Range Heave (Too High)

**Description**: Sensor transmits heave value exceeding maximum (6.2m), handler clamps to 5.0m with warning.

**Preconditions**: Same as Scenario 1

**Test Steps**:
1. Create mock PGN 127252 message with heave = 6.2 meters (exceeds max 5.0m)
2. Call `HandleN2kPGN127252(message, boatData, logger)`
3. Retrieve CompassData
4. Verify heave clamped to 5.0m
5. Verify WARN log entry with original and clamped values

**Expected Results**:
```cpp
// Verify data clamped correctly
CompassData compass = boatData->getCompassData();
ASSERT_EQUAL(5.0, compass.heave, 0.001);  // Clamped to max limit
ASSERT_TRUE(compass.available);            // Still considered available
ASSERT_TRUE(compass.lastUpdate > 0);

// Verify WebSocket log (WARN level)
// Expected log entry:
{
  "level": "WARN",
  "component": "NMEA2000",
  "event": "PGN127252_OUT_OF_RANGE",
  "data": "{\"heave\":6.2,\"clamped\":5.0}"
}
```

**Integration Test Code**:
```cpp
void test_heave_out_of_range_too_high() {
    // Setup
    MockBoatData boatData;
    MockWebSocketLogger logger;
    tN2kMsg message = createMockPGN127252(
        /* SID */ 2,
        /* Heave */ 6.2,  // Exceeds max 5.0m
        /* Delay */ N2kDoubleNA,
        /* DelaySource */ N2kDD374_DataNotAvailable
    );

    // Execute
    HandleN2kPGN127252(message, &boatData, &logger);

    // Verify clamping
    CompassData compass = boatData.getCompassData();
    TEST_ASSERT_EQUAL_DOUBLE(5.0, compass.heave);  // Clamped to max
    TEST_ASSERT_TRUE(compass.available);

    // Verify WARN log
    TEST_ASSERT_TRUE(logger.hasLogEntry(LogLevel::WARN, "PGN127252_OUT_OF_RANGE"));
    TEST_ASSERT_TRUE(logger.logContains("heave\":6.2"));
    TEST_ASSERT_TRUE(logger.logContains("clamped\":5.0"));
}
```

**Pass Criteria**:
- ✅ Heave value clamped to 5.0 meters (max limit)
- ✅ Availability flag set to true (data still valid)
- ✅ Timestamp updated
- ✅ WARN log entry with original (6.2) and clamped (5.0) values
- ✅ NMEA2000 message counter incremented

---

## Scenario 3: Out-of-Range Heave (Too Low)

**Description**: Sensor transmits heave value below minimum (-7.5m), handler clamps to -5.0m with warning.

**Preconditions**: Same as Scenario 1

**Test Steps**:
1. Create mock PGN 127252 message with heave = -7.5 meters (below min -5.0m)
2. Call handler
3. Verify heave clamped to -5.0m
4. Verify WARN log entry

**Expected Results**:
```cpp
// Verify data clamped correctly
CompassData compass = boatData->getCompassData();
ASSERT_EQUAL(-5.0, compass.heave, 0.001);  // Clamped to min limit
ASSERT_TRUE(compass.available);

// Verify WebSocket log (WARN level)
{
  "level": "WARN",
  "component": "NMEA2000",
  "event": "PGN127252_OUT_OF_RANGE",
  "data": "{\"heave\":-7.5,\"clamped\":-5.0}"
}
```

**Integration Test Code**:
```cpp
void test_heave_out_of_range_too_low() {
    MockBoatData boatData;
    MockWebSocketLogger logger;
    tN2kMsg message = createMockPGN127252(
        /* SID */ 3,
        /* Heave */ -7.5,  // Below min -5.0m
        /* Delay */ N2kDoubleNA,
        /* DelaySource */ N2kDD374_DataNotAvailable
    );

    HandleN2kPGN127252(message, &boatData, &logger);

    CompassData compass = boatData.getCompassData();
    TEST_ASSERT_EQUAL_DOUBLE(-5.0, compass.heave);  // Clamped to min
    TEST_ASSERT_TRUE(compass.available);
    TEST_ASSERT_TRUE(logger.hasLogEntry(LogLevel::WARN, "PGN127252_OUT_OF_RANGE"));
}
```

**Pass Criteria**:
- ✅ Heave value clamped to -5.0 meters (min limit)
- ✅ Availability flag set to true
- ✅ WARN log entry with original (-7.5) and clamped (-5.0) values
- ✅ NMEA2000 message counter incremented

---

## Scenario 4: Negative Heave (Valid Range)

**Description**: Sensor transmits negative heave value within valid range (-3.2m downward), handler stores correctly.

**Preconditions**: Same as Scenario 1

**Test Steps**:
1. Create mock PGN 127252 message with heave = -3.2 meters (valid, downward motion)
2. Call handler
3. Verify heave stored as -3.2m (no clamping)
4. Verify DEBUG log (not WARN, since value is valid)

**Expected Results**:
```cpp
// Verify negative heave stored correctly
CompassData compass = boatData->getCompassData();
ASSERT_EQUAL(-3.2, compass.heave, 0.001);  // Negative value preserved
ASSERT_TRUE(compass.available);

// Verify WebSocket log (DEBUG level, not WARN)
{
  "level": "DEBUG",
  "component": "NMEA2000",
  "event": "PGN127252_UPDATE",
  "data": "{\"heave\":-3.2,\"meters\":true}"
}
```

**Integration Test Code**:
```cpp
void test_heave_valid_negative_value() {
    MockBoatData boatData;
    MockWebSocketLogger logger;
    tN2kMsg message = createMockPGN127252(
        /* SID */ 4,
        /* Heave */ -3.2,  // Valid negative value
        /* Delay */ N2kDoubleNA,
        /* DelaySource */ N2kDD374_DataNotAvailable
    );

    HandleN2kPGN127252(message, &boatData, &logger);

    CompassData compass = boatData.getCompassData();
    TEST_ASSERT_EQUAL_DOUBLE(-3.2, compass.heave);  // No clamping
    TEST_ASSERT_TRUE(compass.available);
    TEST_ASSERT_TRUE(logger.hasLogEntry(LogLevel::DEBUG, "PGN127252_UPDATE"));
    TEST_ASSERT_FALSE(logger.hasLogEntry(LogLevel::WARN, "PGN127252_OUT_OF_RANGE"));
}
```

**Pass Criteria**:
- ✅ Heave value stored as -3.2 meters (no clamping)
- ✅ Availability flag set to true
- ✅ DEBUG log entry (NOT WARN, since value is valid)
- ✅ Sign convention correct (negative = downward motion)

---

## Scenario 5: Unavailable Heave (N2kDoubleNA)

**Description**: Sensor indicates heave data not available (N2kDoubleNA), handler skips update and logs DEBUG.

**Preconditions**: Same as Scenario 1

**Test Steps**:
1. Initialize BoatData with pre-existing heave value (e.g., 1.5m from previous message)
2. Create mock PGN 127252 message with heave = N2kDoubleNA (not available)
3. Call handler
4. Verify CompassData.heave UNCHANGED (still 1.5m)
5. Verify availability and timestamp UNCHANGED
6. Verify DEBUG log entry explaining unavailable data

**Expected Results**:
```cpp
// Verify data NOT updated
CompassData compass = boatData->getCompassData();
ASSERT_EQUAL(1.5, compass.heave, 0.001);  // UNCHANGED from previous value
ASSERT_TRUE(compass.available);            // UNCHANGED (still true from previous)
// lastUpdate UNCHANGED

// Verify WebSocket log (DEBUG level)
{
  "level": "DEBUG",
  "component": "NMEA2000",
  "event": "PGN127252_NA",
  "data": "{\"reason\":\"Heave not available\"}"
}

// Verify NMEA2000 counter NOT incremented
```

**Integration Test Code**:
```cpp
void test_heave_unavailable_n2k_double_na() {
    // Setup with pre-existing heave value
    MockBoatData boatData;
    CompassData initialCompass;
    initialCompass.heave = 1.5;  // Previous value
    initialCompass.available = true;
    initialCompass.lastUpdate = 1000;
    boatData.setCompassData(initialCompass);

    MockWebSocketLogger logger;
    tN2kMsg message = createMockPGN127252(
        /* SID */ 5,
        /* Heave */ N2kDoubleNA,  // Not available
        /* Delay */ N2kDoubleNA,
        /* DelaySource */ N2kDD374_DataNotAvailable
    );

    unsigned long initialCounter = boatData.getNMEA2000Count();

    // Execute
    HandleN2kPGN127252(message, &boatData, &logger);

    // Verify NO update
    CompassData compass = boatData.getCompassData();
    TEST_ASSERT_EQUAL_DOUBLE(1.5, compass.heave);  // UNCHANGED
    TEST_ASSERT_TRUE(compass.available);            // UNCHANGED
    TEST_ASSERT_EQUAL_UINT32(1000, compass.lastUpdate);  // UNCHANGED

    // Verify counter NOT incremented
    TEST_ASSERT_EQUAL_UINT32(initialCounter, boatData.getNMEA2000Count());

    // Verify DEBUG log
    TEST_ASSERT_TRUE(logger.hasLogEntry(LogLevel::DEBUG, "PGN127252_NA"));
}
```

**Pass Criteria**:
- ✅ Heave value UNCHANGED (previous value preserved)
- ✅ Availability flag UNCHANGED
- ✅ Timestamp UNCHANGED
- ✅ DEBUG log entry with reason "Heave not available"
- ✅ NMEA2000 message counter NOT incremented

---

## Scenario 6: Parse Failure

**Description**: Malformed or corrupt PGN 127252 message, ParseN2kPGN127252 returns false, handler logs ERROR.

**Preconditions**: Same as Scenario 1

**Test Steps**:
1. Initialize BoatData with pre-existing heave value
2. Create malformed/corrupt PGN 127252 message (invalid data bytes)
3. Call handler
4. Verify CompassData UNCHANGED
5. Verify ERROR log entry

**Expected Results**:
```cpp
// Verify data NOT updated
CompassData compass = boatData->getCompassData();
// heave, available, lastUpdate all UNCHANGED

// Verify WebSocket log (ERROR level)
{
  "level": "ERROR",
  "component": "NMEA2000",
  "event": "PGN127252_PARSE_FAILED",
  "data": "{\"reason\":\"Failed to parse PGN 127252\"}"
}

// Verify NMEA2000 counter NOT incremented
```

**Integration Test Code**:
```cpp
void test_heave_parse_failure() {
    // Setup with pre-existing heave value
    MockBoatData boatData;
    CompassData initialCompass;
    initialCompass.heave = 2.0;
    initialCompass.available = true;
    initialCompass.lastUpdate = 2000;
    boatData.setCompassData(initialCompass);

    MockWebSocketLogger logger;
    tN2kMsg message = createCorruptPGN127252();  // Invalid message

    unsigned long initialCounter = boatData.getNMEA2000Count();

    // Execute
    HandleN2kPGN127252(message, &boatData, &logger);

    // Verify NO update
    CompassData compass = boatData.getCompassData();
    TEST_ASSERT_EQUAL_DOUBLE(2.0, compass.heave);  // UNCHANGED
    TEST_ASSERT_TRUE(compass.available);            // UNCHANGED
    TEST_ASSERT_EQUAL_UINT32(2000, compass.lastUpdate);  // UNCHANGED

    // Verify counter NOT incremented
    TEST_ASSERT_EQUAL_UINT32(initialCounter, boatData.getNMEA2000Count());

    // Verify ERROR log
    TEST_ASSERT_TRUE(logger.hasLogEntry(LogLevel::ERROR, "PGN127252_PARSE_FAILED"));
}
```

**Pass Criteria**:
- ✅ Heave value UNCHANGED
- ✅ Availability flag UNCHANGED
- ✅ Timestamp UNCHANGED
- ✅ ERROR log entry with reason "Failed to parse PGN 127252"
- ✅ NMEA2000 message counter NOT incremented

---

## Running Integration Tests

### Native Platform (No Hardware Required)
```bash
# Run all BoatData integration tests
pio test -e native -f test_boatdata_integration

# Run only heave handler tests
pio test -e native -f test_boatdata_integration --verbose

# Expected output:
# test_heave_valid_positive_value                    PASSED
# test_heave_out_of_range_too_high                   PASSED
# test_heave_out_of_range_too_low                    PASSED
# test_heave_valid_negative_value                    PASSED
# test_heave_unavailable_n2k_double_na               PASSED
# test_heave_parse_failure                           PASSED
```

### ESP32 Platform (Hardware Required)
```bash
# Run hardware tests (requires ESP32 + NMEA2000 transceiver)
pio test -e esp32dev_test -f test_boatdata_hardware

# Expected: PGN 127252 timing placeholder test passes
```

---

## Verification Checklist

### Code Implementation
- [ ] `HandleN2kPGN127252` function added to `src/components/NMEA2000Handlers.cpp`
- [ ] Function declaration added to `src/components/NMEA2000Handlers.h`
- [ ] Handler registered in `RegisterN2kHandlers()` function
- [ ] Doxygen comments added

### Testing
- [ ] Contract test passes (handler signature validation)
- [ ] Integration test Scenario 1 passes (valid heave)
- [ ] Integration test Scenario 2 passes (out-of-range high)
- [ ] Integration test Scenario 3 passes (out-of-range low)
- [ ] Integration test Scenario 4 passes (valid negative)
- [ ] Integration test Scenario 5 passes (unavailable N2kDoubleNA)
- [ ] Integration test Scenario 6 passes (parse failure)
- [ ] Hardware test placeholder added

### Build & Memory
- [ ] Build succeeds with no errors
- [ ] Flash usage <50% (currently 47.7%)
- [ ] RAM usage <20% (currently 13.5%)
- [ ] No compiler warnings

### Documentation
- [ ] CLAUDE.md updated with PGN 127252 handler documentation
- [ ] CHANGELOG.md updated with feature summary
- [ ] README.md updated (if applicable)

### Constitutional Compliance
- [ ] Hardware abstraction maintained (NMEA2000 library)
- [ ] Resource management verified (no new allocations)
- [ ] WebSocket logging implemented (DEBUG/WARN/ERROR)
- [ ] Graceful degradation ensured (N2kDoubleNA handling)
- [ ] QA review completed (PR review process)

---

## Troubleshooting

### Problem: Handler Not Called
**Symptoms**: Heave data not updating despite NMEA2000 messages on bus
**Causes**:
1. Handler not registered with NMEA2000 library
2. NMEA2000 bus not initialized
3. Wrong PGN number in message

**Solutions**:
1. Verify `RegisterN2kHandlers()` called in main.cpp
2. Check NMEA2000 initialization status
3. Use NMEA2000 analyzer to verify PGN 127252 messages

### Problem: Values Always Clamped
**Symptoms**: WARN logs for every message, all heave values at ±5.0m
**Causes**:
1. Sensor transmitting out-of-range values
2. Unit conversion error (wrong units)
3. Sensor calibration issue

**Solutions**:
1. Check sensor specifications and configuration
2. Verify NMEA2000 library returns meters (not mm or ft)
3. Recalibrate sensor or adjust validation range

### Problem: Unavailable Data (N2kDoubleNA)
**Symptoms**: DEBUG logs, heave never updates
**Causes**:
1. Sensor not ready (startup period)
2. Sensor fault or disconnected
3. Sensor transmitting N/A values

**Solutions**:
1. Wait for sensor initialization (typically <30 seconds)
2. Check sensor power and connections
3. Verify sensor is functioning (check other PGNs from same sensor)

---

## Summary

This quickstart guide provides 6 validation scenarios covering all handler behaviors:
1. ✅ Valid heave (positive)
2. ✅ Out-of-range (too high, clamped)
3. ✅ Out-of-range (too low, clamped)
4. ✅ Valid heave (negative)
5. ✅ Unavailable data (N2kDoubleNA)
6. ✅ Parse failure

**All scenarios** include:
- Clear preconditions and test steps
- Expected results with verification code
- Integration test implementation
- Pass criteria

**Next Steps**:
1. Review scenarios with stakeholders
2. Implement handler following contract
3. Write integration tests (TDD approach)
4. Verify all scenarios pass
5. Update documentation

**Status**: ✅ COMPLETE
**Ready for**: Task generation (/tasks command)
