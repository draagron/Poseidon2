# Quickstart Guide: NMEA 2000 Message Handling

**Feature**: 010-nmea-2000-handling
**Date**: 2025-10-12
**Audience**: Developers implementing/testing/validating NMEA2000 handlers

## Quick Reference

**Goal**: Add comprehensive NMEA2000 PGN handling to populate BoatData with GPS, compass, DST, engine, and wind data.

**Estimated Time**: 16-24 hours (8-12 hours implementation + 8-12 hours testing)

**Prerequisites**:
- ✅ BoatData v2.0.0 implemented (complete)
- ✅ 9 of 13 handlers already implemented (complete)
- ✅ NMEA2000 library integrated (platformio.ini)
- ✅ Multi-source prioritization working (complete)

**Remaining Work**:
- ❌ Implement 5 missing PGN handlers (GPS, wind data)
- ❌ Add NMEA2000 CAN bus initialization in main.cpp
- ❌ Create N2kBoatDataHandler message router class
- ❌ Register NMEA2000 sources with BoatData
- ❌ Write comprehensive tests

## Implementation Checklist

### Phase 1: Missing Handler Functions (~4 hours)

Implement 5 missing handlers following the pattern in `src/components/NMEA2000Handlers.cpp`:

- [ ] **PGN 129025**: Position Rapid Update → GPSData lat/lon (~45 min)
- [ ] **PGN 129026**: COG/SOG Rapid Update → GPSData cog/sog (~45 min)
- [ ] **PGN 127250**: Vessel Heading → CompassData true/magnetic heading (~60 min)
- [ ] **PGN 127258**: Magnetic Variation → GPSData.variation (~30 min)
- [ ] **PGN 130306**: Wind Data → WindData (~60 min)

**Template** (copy from existing handlers, modify for new PGN):
```cpp
void HandleN2kPGN<NUMBER>(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger) {
    if (boatData == nullptr || logger == nullptr) return;

    // 1. Parse PGN
    // 2. Check N2kIsNA
    // 3. Validate ranges
    // 4. Update BoatData
    // 5. Log DEBUG
    // 6. Increment counter
}
```

**Files to Edit**:
- `src/components/NMEA2000Handlers.h` - Add function declarations
- `src/components/NMEA2000Handlers.cpp` - Add function implementations

**References**:
- See `contracts/HandlerFunctionContract.md` for detailed requirements
- Copy patterns from existing handlers (PGN 127251, 127252, etc.)

### Phase 2: Message Router Class (~2 hours)

Create N2kBoatDataHandler class to route PGN messages to handlers:

- [ ] **Create class** in `src/components/NMEA2000Handlers.cpp` (~60 min)
- [ ] **Implement HandleMsg()** with switch statement for all 13 PGNs (~30 min)
- [ ] **Update RegisterN2kHandlers()** to create and attach handler (~30 min)

**Files to Edit**:
- `src/components/NMEA2000Handlers.cpp` - Add N2kBoatDataHandler class
- `src/components/NMEA2000Handlers.cpp` - Update RegisterN2kHandlers()

**References**:
- See `contracts/N2kBoatDataHandlerContract.md` for class specification
- See `contracts/RegisterN2kHandlersContract.md` for registration function

### Phase 3: NMEA2000 Initialization (~2 hours)

Add CAN bus initialization and handler registration to main.cpp:

- [ ] **Add NMEA2000 includes** at top of main.cpp (~5 min)
- [ ] **Add NMEA2000 global variables** with other globals (~10 min)
- [ ] **Initialize CAN bus** in setup() step 5 (~45 min)
- [ ] **Register handlers** in setup() step 6 (~15 min)
- [ ] **Register sources** with BoatData (~15 min)
- [ ] **Test compilation** and fix errors (~30 min)

**Files to Edit**:
- `src/main.cpp` - Add initialization code in setup()
- `src/config.h` - Add CAN bus pin definitions (already exist)

**References**:
- See `data-model.md` section "Call Sequence in main.cpp"
- GPIO pins: CAN RX=34, CAN TX=32 (defined in CLAUDE.md)

### Phase 4: Testing (~10-14 hours)

Write comprehensive tests for all handlers and integration:

- [ ] **Contract tests** (~4 hours)
  - Create `test/test_nmea2000_contracts/` directory
  - Test each handler function contract compliance
  - Test N2kBoatDataHandler routing
  - Test RegisterN2kHandlers() behavior

- [ ] **Integration tests** (~4 hours)
  - Create `test/test_nmea2000_integration/` directory
  - Test GPS data flow (4 PGNs → GPSData)
  - Test compass data flow (4 PGNs → CompassData)
  - Test DST data flow (3 PGNs → DSTData)
  - Test engine data flow (2 PGNs → EngineData)
  - Test wind data flow (1 PGN → WindData)
  - Test multi-source priority (NMEA2000 vs NMEA0183)

- [ ] **Unit tests** (~2 hours)
  - Create `test/test_nmea2000_units/` directory
  - Test data validation and clamping
  - Test unit conversions (Kelvin→Celsius, m/s→knots)
  - Test conditional logic (instance, reference type, source type)

- [ ] **Hardware tests** (~2-4 hours, optional)
  - Create `test/test_nmea2000_hardware/` directory
  - Test CAN bus initialization (ESP32 required)
  - Test message reception (NMEA2000 devices required)
  - Test address claiming protocol
  - Test handler execution timing

**Test Commands**:
```bash
# Run all NMEA2000 tests
pio test -e native -f test_nmea2000_*

# Run specific test groups
pio test -e native -f test_nmea2000_contracts
pio test -e native -f test_nmea2000_integration
pio test -e native -f test_nmea2000_units

# Run hardware tests (ESP32 required)
pio test -e esp32dev_test -f test_nmea2000_hardware
```

## Development Workflow

### 1. Start with Existing Code

**Review existing handlers** (already working):
```bash
# Read existing handler implementations
cat src/components/NMEA2000Handlers.cpp | grep -A 50 "PGN 127251"
```

**Understand pattern**:
- Parse → Check NA → Validate → Update BoatData → Log → Increment counter
- All handlers follow same structure
- Copy/paste/modify is fastest approach

### 2. Implement One Handler at a Time

**Workflow per handler**:
1. Copy existing handler function (e.g., PGN 127251)
2. Rename function and PGN number
3. Update parse function call (e.g., `ParseN2kPGN129025`)
4. Update field names and target BoatData structure
5. Update validation functions (if different ranges)
6. Update log messages with new PGN number and fields
7. Compile and fix syntax errors
8. Write unit test for handler
9. Run test and fix logic errors

**Example**:
```cpp
// 1. Copy PGN 127251 handler
// 2. Rename to HandleN2kPGN129025
// 3. Change ParseN2kPGN127251 → ParseN2kPGN129025
// 4. Change rateOfTurn → latitude, longitude
// 5. Change DataValidation::isValidRateOfTurn → isValidLatitude, isValidLongitude
// 6. Change log messages "PGN127251" → "PGN129025"
// 7. Compile: pio run
// 8. Write test: test/test_nmea2000_units/test_pgn129025.cpp
// 9. Run test: pio test -e native -f test_nmea2000_units
```

### 3. Create Message Router Class

**Implementation steps**:
1. Add class declaration in NMEA2000Handlers.cpp (before RegisterN2kHandlers)
2. Implement constructor (trivial initialization)
3. Implement HandleMsg() with switch statement for all 13 PGNs
4. Update RegisterN2kHandlers() to create static handler instance
5. Compile and test

**Verification**:
- All 13 PGNs routed to correct handler functions
- Unsupported PGNs silently ignored (default case)
- Handler lifetime managed correctly (static keyword)

### 4. Add NMEA2000 Initialization

**Implementation steps**:
1. Add includes at top of main.cpp
2. Add global variables with other globals
3. Add initialization code in setup() after Serial2 (step 5)
4. Add handler registration after initialization (step 6)
5. Add source registration with BoatData
6. Compile, upload, and test on ESP32

**Verification**:
- ESP32 boots without crashes
- WebSocket log shows "HANDLERS_REGISTERED" INFO message
- WebSocket log shows PGN updates (DEBUG level)

## Testing Strategy

### Test-Driven Development (TDD)

**Recommended approach**:
1. Write test BEFORE implementing handler
2. Run test (expect failure)
3. Implement handler
4. Run test (expect pass)
5. Refactor if needed

**Benefits**:
- Clarifies requirements before coding
- Catches bugs early
- Provides regression test suite
- Constitutional requirement (Principle III)

### Mock-Based Testing

**NMEA2000 library provides test message constructor**:
```cpp
// Create test message with PGN 129025
tN2kMsg testMsg;
testMsg.SetPGN(129025);
testMsg.Priority = 6;
testMsg.AddByte(0xFF);  // SID (unused)
testMsg.AddDouble(37.7749, 1e-7);  // Latitude (San Francisco)
testMsg.AddDouble(-122.4194, 1e-7); // Longitude

// Call handler
HandleN2kPGN129025(testMsg, &boatData, &logger);

// Verify BoatData updated
GPSData gps = boatData.getGPSData();
ASSERT_NEAR(gps.latitude, 37.7749, 0.0001);
ASSERT_NEAR(gps.longitude, -122.4194, 0.0001);
```

**Benefits**:
- No ESP32 hardware required
- Fast test execution (native platform)
- Deterministic results (no timing issues)

### Hardware Validation

**Minimal hardware tests required** (Constitutional Principle III):
- CAN bus initialization timing
- Address claiming protocol
- Message reception confirmation
- Handler execution profiling

**Hardware test requirements**:
- ESP32 with CAN transceivers (GPIO 34/32)
- NMEA2000 bus with 12V power and 120Ω terminators
- At least one NMEA2000 device broadcasting known PGNs

**Optional**: Use NMEA2000 simulator for testing without real devices.

## Validation Checklist

### Pre-Implementation Validation

- [ ] Read feature specification (`spec.md`)
- [ ] Read research document (`research.md`)
- [ ] Read data model document (`data-model.md`)
- [ ] Read all contract files (`contracts/`)
- [ ] Review existing handler implementations
- [ ] Understand NMEA2000 library API

### During Implementation Validation

- [ ] Each handler compiles without warnings
- [ ] Each handler follows HandlerFunctionContract.md
- [ ] Message router compiles without warnings
- [ ] Message router follows N2kBoatDataHandlerContract.md
- [ ] Registration function follows RegisterN2kHandlersContract.md
- [ ] main.cpp compiles without warnings
- [ ] All contract tests passing
- [ ] All integration tests passing
- [ ] All unit tests passing

### Post-Implementation Validation

- [ ] All 13 PGNs processed correctly
- [ ] BoatData structures populated with correct values
- [ ] Multi-source prioritization working (NMEA2000 > NMEA0183)
- [ ] WebSocket logs show handler activity (DEBUG level)
- [ ] Memory footprint within limits (~2.2KB RAM, ~12KB flash)
- [ ] Handler execution time <1ms per message
- [ ] No crashes or memory leaks during 24-hour stress test
- [ ] Hardware tests passing (if applicable)

## Troubleshooting

### Issue: Handlers not receiving messages

**Symptoms**: No WebSocket logs for PGN updates

**Causes**:
1. NMEA2000 not initialized (`Open()` not called)
2. Handlers not registered (`RegisterN2kHandlers()` not called)
3. CAN bus wiring incorrect (check GPIO 34/32)
4. CAN bus not powered (needs 12V)
5. NMEA2000 device not broadcasting (check with NMEA2000 monitor)

**Debugging**:
```cpp
// Add debug logs in setup()
logger.broadcastLog(LogLevel::INFO, "NMEA2000", "INIT_START", F("{}"));
nmea2000->Open();
logger.broadcastLog(LogLevel::INFO, "NMEA2000", "INIT_COMPLETE", F("{}"));
RegisterN2kHandlers(nmea2000, boatData, &logger);
// Check WebSocket for these log messages
```

### Issue: Parse failures (ERROR logs)

**Symptoms**: WebSocket logs show "PGN<NUMBER>_PARSE_FAILED"

**Causes**:
1. Wrong parse function called (check PGN number)
2. Message format changed (NMEA2000 library version mismatch)
3. Malformed message from device (check device firmware)

**Debugging**:
```cpp
// Add raw message hex dump
logger.broadcastLog(LogLevel::DEBUG, "NMEA2000", "RAW_MESSAGE",
    String(F("{\"pgn\":")) + N2kMsg.PGN + F(",\"len\":") + N2kMsg.DataLen + F("}"));
```

### Issue: Out-of-range values (WARN logs)

**Symptoms**: WebSocket logs show "PGN<NUMBER>_OUT_OF_RANGE"

**Causes**:
1. Sensor miscalibrated (check sensor documentation)
2. Sensor malfunctioning (check sensor wiring)
3. Validation range too strict (review DataValidation.h)

**Resolution**: Values are clamped automatically. WARN log alerts operator to sensor issue.

### Issue: Multi-source priority not working

**Symptoms**: NMEA0183 data used instead of NMEA2000

**Causes**:
1. NMEA2000 sources not registered (`registerSource()` not called)
2. Update frequency measured incorrectly
3. NMEA2000 messages not being received (see "Handlers not receiving messages")

**Debugging**:
```cpp
// Check active source
String activeGPS = boatData->getActiveSource(SensorType::GPS);
logger.broadcastLog(LogLevel::INFO, "BoatData", "ACTIVE_GPS",
    String(F("{\"source\":\"")) + activeGPS + F("\"}"));
```

## Performance Monitoring

### WebSocket Log Monitoring

**Connect to WebSocket logs**:
```bash
# Activate Python virtual environment
source src/helpers/websocket_env/bin/activate

# Connect to WebSocket
python3 src/helpers/ws_logger.py <ESP32_IP>

# Filter for NMEA2000 events
python3 src/helpers/ws_logger.py <ESP32_IP> --filter NMEA2000
```

**Expected log output** (DEBUG level):
```json
{"level":"DEBUG","component":"NMEA2000","event":"PGN129025_UPDATE","data":{"latitude":37.7749,"longitude":-122.4194}}
{"level":"DEBUG","component":"NMEA2000","event":"PGN129026_UPDATE","data":{"cog":1.5708,"sog":5.2}}
{"level":"DEBUG","component":"NMEA2000","event":"PGN127250_UPDATE","data":{"heading":1.5708,"reference":"true"}}
```

### Handler Execution Timing

**Add timing measurement to handlers**:
```cpp
unsigned long startMicros = micros();
// ... handler code ...
unsigned long durationMicros = micros() - startMicros;

if (durationMicros > 1000) {  // >1ms
    logger->broadcastLog(LogLevel::WARN, "NMEA2000", "HANDLER_SLOW",
        String(F("{\"pgn\":")) + N2kMsg.PGN + F(",\"duration_us\":") + durationMicros + F("}"));
}
```

**Expected timing**: <500μs per handler (well under 1ms budget)

### Memory Usage Monitoring

**Check RAM usage**:
```cpp
logger->broadcastLog(LogLevel::INFO, "System", "MEMORY",
    String(F("{\"free_heap\":")) + ESP.getFreeHeap() + F(",\"min_free_heap\":") + ESP.getMinFreeHeap() + F("}"));
```

**Expected values**:
- Free heap: >250KB (ESP32 has ~300KB usable heap)
- Min free heap: >200KB (lowest point during operation)

**If memory low**: Check for memory leaks (heap allocation in handlers).

## Success Criteria

### Functional Requirements

- ✅ All 13 PGNs processed correctly
- ✅ BoatData structures populated with validated data
- ✅ Out-of-range values clamped with WARN logs
- ✅ Unavailable (NA) values skipped with DEBUG logs
- ✅ Parse failures logged with ERROR level
- ✅ Multi-source prioritization working (NMEA2000 > NMEA0183)

### Performance Requirements

- ✅ Handler execution time <1ms per message
- ✅ Memory footprint <2.5KB RAM, <15KB flash
- ✅ System handles 1000 messages/second without overruns
- ✅ No blocking operations in handler code

### Quality Requirements

- ✅ All contract tests passing
- ✅ All integration tests passing
- ✅ All unit tests passing
- ✅ Hardware tests passing (if applicable)
- ✅ No compiler warnings
- ✅ Code follows constitutional principles
- ✅ WebSocket logs provide sufficient debugging information

## Next Steps

After completing this feature:

1. **Run `/tasks`** to generate task breakdown for implementation
2. **Run `/implement`** to execute tasks in dependency order
3. **Validate** using this quickstart guide
4. **Document** any deviations or lessons learned
5. **Update** CLAUDE.md with NMEA2000 usage patterns
6. **Tag release** with firmware version

## References

- **Feature Spec**: `specs/010-nmea-2000-handling/spec.md`
- **Research**: `specs/010-nmea-2000-handling/research.md`
- **Data Model**: `specs/010-nmea-2000-handling/data-model.md`
- **Contracts**: `specs/010-nmea-2000-handling/contracts/`
- **NMEA2000 Library**: https://github.com/ttlappalainen/NMEA2000
- **NMEA2000 API**: https://ttlappalainen.github.io/NMEA2000/pg_lib_ref.html
- **Constitution**: `.specify/memory/constitution.md`
- **CLAUDE.md**: Project guidance and patterns

---

**Questions?** Review the specification documents above or consult the NMEA2000 library documentation.
