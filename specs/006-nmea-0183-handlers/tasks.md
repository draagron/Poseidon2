# Tasks: NMEA 0183 Data Handlers

**Feature**: 006-nmea-0183-handlers
**Branch**: `006-nmea-0183-handlers`
**Input**: Design documents from `/home/niels/Dev/Poseidon2/specs/006-nmea-0183-handlers/`
**Prerequisites**: plan.md ✓, research.md ✓, data-model.md ✓, contracts/ ✓

---

## Task Overview

**Total Tasks**: 40 tasks organized in 5 phases
- **Phase 3.1 Setup**: 6 tasks (HAL interfaces, types, mocks)
- **Phase 3.2 Tests First (TDD)**: 18 tasks (contract, integration, unit tests)
- **Phase 3.3 Core Implementation**: 8 tasks (handlers, parsers, utilities)
- **Phase 3.4 Integration**: 4 tasks (main.cpp, ReactESP, logging)
- **Phase 3.5 Polish**: 4 tasks (hardware tests, docs, validation)

**Parallel Opportunities**: 21 tasks marked [P] can run in parallel with their peers
**Estimated Time**: 12-16 hours (assuming 20-30 min per task average)

---

## Phase 3.1: Setup (HAL Interfaces, Types, Mocks)

### T001: Create ISerialPort HAL Interface
**File**: `src/hal/interfaces/ISerialPort.h`
**Description**: Define ISerialPort interface for Serial2 hardware abstraction
**Prerequisites**: None
**Details**:
- Create interface with methods: `int available()`, `int read()`, `void begin(unsigned long baud)`
- Add Doxygen comments describing behavioral contracts (non-blocking, -1 on empty, byte consumption)
- Include virtual destructor
- Reference: specs/006-nmea-0183-handlers/contracts/ISerialPort.contract.md

**Acceptance**:
- [X] Interface compiles without errors
- [X] All methods are pure virtual (= 0)
- [X] Doxygen comments for all public methods

---

### T002 [P]: Create Mock Serial Port Implementation
**File**: `src/mocks/MockSerialPort.h`, `src/mocks/MockSerialPort.cpp`
**Description**: Implement MockSerialPort for testing NMEA sentence processing
**Prerequisites**: T001 (ISerialPort interface)
**Details**:
- Implement ISerialPort interface
- Add `setMockData(const char* data)` method for test setup
- Track position in mock data buffer
- `available()` returns `strlen(mockData) - position`
- `read()` returns next byte and increments position, or -1 if empty
- `begin()` is no-op (mock doesn't need initialization)

**Acceptance**:
- [X] Implements ISerialPort interface
- [X] Can load predefined NMEA sentence strings
- [X] Correctly simulates byte-by-byte reading
- [X] Compiles for native platform (no Arduino dependencies)

---

### T003 [P]: Create ESP32 Serial Port Adapter
**File**: `src/hal/implementations/ESP32SerialPort.h`, `src/hal/implementations/ESP32SerialPort.cpp`
**Description**: Implement ESP32SerialPort wrapper for HardwareSerial (Serial2)
**Prerequisites**: T001 (ISerialPort interface)
**Details**:
- Implement ISerialPort interface
- Constructor takes `HardwareSerial*` pointer (e.g., &Serial2)
- `available()` delegates to `serial_->available()`
- `read()` delegates to `serial_->read()`
- `begin(baud)` delegates to `serial_->begin(baud)`
- Store HardwareSerial pointer as private member

**Acceptance**:
- [X] Implements ISerialPort interface
- [X] Compiles for ESP32 platform
- [X] No memory leaks (pointer stored, not owned)
- [X] Can be instantiated with Serial2

---

### T004 [P]: Create UnitConverter Utility Class
**File**: `src/utils/UnitConverter.h`
**Description**: Implement static utility functions for NMEA→BoatData unit conversions
**Prerequisites**: None (no dependencies)
**Details**:
- `static double degreesToRadians(double degrees)` - multiply by DEG_TO_RAD
- `static double nmeaCoordinateToDecimal(double degreesMinutes, char hemisphere)` - convert DDMM.MMMM to DD.dddd, apply sign based on N/S/E/W
- `static double calculateVariation(double trueCOG, double magCOG)` - return trueCOG - magCOG, normalize to [-180, 180]
- `static double normalizeAngle(double radians)` - normalize to [0, 2π]
- All functions header-only (inline or constexpr if possible)

**Acceptance**:
- [X] All functions are static (no instance needed)
- [X] Header-only implementation (no .cpp file)
- [X] Compiles for native and ESP32 platforms
- [X] Uses Arduino constants (DEG_TO_RAD, M_PI)

---

### T005 [P]: Create NMEA0183Parsers Utility
**File**: `src/utils/NMEA0183Parsers.h`, `src/utils/NMEA0183Parsers.cpp`
**Description**: Implement custom RSA parser (not in NMEA0183 library)
**Prerequisites**: None (uses NMEA0183 library types)
**Details**:
- `bool NMEA0183ParseRSA(const tNMEA0183Msg& msg, double& rudderAngle)` - parse RSA sentence
- Validate talker ID is "AP" using `strcmp(msg.Sender(), "AP") == 0`
- Validate message code is "RSA" using `strcmp(msg.MessageCode(), "RSA") == 0`
- Extract rudder angle from Field(0) using `atof(msg.Field(0))`
- Validate status field Field(1) is "A" (valid)
- Return true if successfully parsed, false otherwise
- Reference: examples/poseidongw/src/NMEA0183Handlers.cpp:78-86

**Acceptance**:
- [X] Function signature matches library parser pattern
- [X] Returns bool (true = success, false = failure)
- [X] Validates talker ID, message code, and status field
- [X] Compiles with NMEA0183 library dependency

---

### T006: Create Test Helpers Directory
**File**: `test/helpers/nmea0183_test_fixtures.h`
**Description**: Create shared test utilities for NMEA 0183 sentence fixtures
**Prerequisites**: None
**Details**:
- Define valid NMEA 0183 sentence constants for each type:
  - `const char* VALID_APRSA = "$APRSA,15.0,A*3C\r\n";`
  - `const char* VALID_APHDM = "$APHDM,045.5,M*2F\r\n";`
  - `const char* VALID_VHGGA = "$VHGGA,123519,5230.5000,N,00507.0000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n";`
  - `const char* VALID_VHRMC = "$VHRMC,123519,A,5230.5000,N,00507.0000,E,5.5,054.7,230394,003.1,W*6A\r\n";`
  - `const char* VALID_VHVTG = "$VHVTG,054.7,T,057.9,M,5.5,N,10.2,K*48\r\n";`
- Define invalid sentence fixtures (bad checksum, out-of-range values, wrong talker ID)
- Define mock BoatData factory for integration tests

**Acceptance**:
- [X] All sentences have valid checksums
- [X] Fixtures usable from all test files
- [X] Header-only (no .cpp file needed)

---

## Phase 3.2: Tests First (TDD) ⚠️ MUST COMPLETE BEFORE PHASE 3.3

**CRITICAL**: These tests MUST be written and MUST FAIL before ANY implementation in Phase 3.3

### Contract Tests (HAL Interface Validation)

### T007: Create Contract Test Directory
**File**: `test/test_nmea0183_contracts/test_main.cpp`
**Description**: Create Unity test runner for ISerialPort contract tests
**Prerequisites**: T002 (MockSerialPort), T003 (ESP32SerialPort)
**Details**:
- Include Unity framework headers
- Create `setUp()` and `tearDown()` functions
- Call RUN_TEST() for each contract test (T008-T012)
- Implement `main()` to run UNITY_BEGIN() and UNITY_END()

**Acceptance**:
- [ ] Compiles for native platform
- [ ] Runs Unity test framework
- [ ] test_main.cpp structure matches existing test patterns

---

### T008 [P]: Contract Test - ISerialPort available() Non-Blocking
**File**: `test/test_nmea0183_contracts/test_iserialport.cpp`
**Description**: Test ISerialPort.available() returns immediately and accurately
**Prerequisites**: T007 (test directory)
**Details**:
- Test with MockSerialPort containing "ABC"
- Assert `available()` returns 3
- Measure execution time < 1ms (use millis() if available, or skip timing on native)
- Call `available()` multiple times, should return same value (non-destructive)
- Test with empty mock, assert `available()` returns 0

**Expected Result**: ❌ Test MUST FAIL (MockSerialPort not yet implemented)

**Acceptance**:
- [ ] Test compiles and runs on native platform
- [ ] Test fails before implementation
- [ ] Test covers empty and non-empty buffer cases

---

### T009 [P]: Contract Test - ISerialPort read() Consumes Bytes
**File**: `test/test_nmea0183_contracts/test_iserialport.cpp` (add to existing file)
**Description**: Test ISerialPort.read() consumes bytes and returns -1 when empty
**Prerequisites**: T007 (test directory)
**Details**:
- Load mock with "ABC"
- Assert `read()` returns 'A' (0x41)
- Assert `available()` now returns 2 (byte consumed)
- Assert `read()` returns 'B', then 'C'
- Assert `read()` returns -1 (empty buffer)
- Assert `available()` returns 0

**Expected Result**: ❌ Test MUST FAIL

**Acceptance**:
- [ ] Test verifies byte consumption
- [ ] Test verifies -1 return on empty
- [ ] Test verifies FIFO order (A, B, C)

---

### T010 [P]: Contract Test - ISerialPort begin() Idempotent
**File**: `test/test_nmea0183_contracts/test_iserialport.cpp` (add to existing file)
**Description**: Test ISerialPort.begin() can be called multiple times safely
**Prerequisites**: T007 (test directory)
**Details**:
- Call `begin(4800)` on MockSerialPort
- Load mock data with "TEST"
- Call `begin(4800)` again
- Assert mock still functional (`available()` returns 4)
- Test with ESP32SerialPort in hardware test (just verify no crash)

**Expected Result**: ❌ Test MUST FAIL

**Acceptance**:
- [ ] Test calls begin() 3 times
- [ ] No crashes or exceptions
- [ ] Mock remains usable after multiple begin() calls

---

### T011 [P]: Unit Test - UnitConverter Degree/Radian Conversion
**File**: `test/test_nmea0183_units/test_main.cpp` and `test/test_nmea0183_units/test_unit_conversions.cpp`
**Description**: Test UnitConverter::degreesToRadians() and normalizeAngle()
**Prerequisites**: T004 (UnitConverter class)
**Details**:
- Create test directory `test/test_nmea0183_units/` with test_main.cpp (Unity runner)
- Test `degreesToRadians(0.0)` returns 0.0
- Test `degreesToRadians(90.0)` returns π/2 (≈1.5708)
- Test `degreesToRadians(180.0)` returns π
- Test `degreesToRadians(360.0)` returns 2π
- Test `normalizeAngle(3π)` returns π
- Test `normalizeAngle(-π/2)` returns 3π/2

**Expected Result**: ❌ Test MUST FAIL (UnitConverter not yet implemented)

**Acceptance**:
- [ ] Tests use floating-point comparison with epsilon (0.0001)
- [ ] Covers boundary cases (0°, 90°, 180°, 360°)
- [ ] Compiles for native platform

---

### T012 [P]: Unit Test - UnitConverter NMEA Coordinate Conversion
**File**: `test/test_nmea0183_units/test_unit_conversions.cpp` (add to existing file)
**Description**: Test UnitConverter::nmeaCoordinateToDecimal()
**Prerequisites**: T004 (UnitConverter class)
**Details**:
- Test `nmeaCoordinateToDecimal(5230.5000, 'N')` returns 52.508333 (52°30.5' North)
- Test `nmeaCoordinateToDecimal(5230.5000, 'S')` returns -52.508333 (South is negative)
- Test `nmeaCoordinateToDecimal(00507.0000, 'E')` returns 5.116667 (5°7' East)
- Test `nmeaCoordinateToDecimal(00507.0000, 'W')` returns -5.116667 (West is negative)
- Test boundary: `nmeaCoordinateToDecimal(9000.0000, 'N')` returns 90.0 (90°0' = max latitude)

**Expected Result**: ❌ Test MUST FAIL

**Acceptance**:
- [ ] Tests all 4 hemispheres (N, S, E, W)
- [ ] Uses epsilon comparison for floating-point results
- [ ] Covers boundary cases (90°, 180°)

---

### T013 [P]: Unit Test - UnitConverter Variation Calculation
**File**: `test/test_nmea0183_units/test_unit_conversions.cpp` (add to existing file)
**Description**: Test UnitConverter::calculateVariation()
**Prerequisites**: T004 (UnitConverter class)
**Details**:
- Test `calculateVariation(54.7, 57.9)` returns -3.2 (West variation)
- Test `calculateVariation(100.0, 95.0)` returns 5.0 (East variation)
- Test `calculateVariation(10.0, 350.0)` returns 20.0 (wraparound case: 10° - 350° = 20° not -340°)
- Test `calculateVariation(350.0, 10.0)` returns -20.0 (reverse wraparound)
- Test normalization to [-180, 180] range

**Expected Result**: ❌ Test MUST FAIL

**Acceptance**:
- [ ] Tests wraparound cases (crossing 0°/360°)
- [ ] Tests both East (positive) and West (negative) variation
- [ ] Verifies result in [-180, 180] range

---

### T014 [P]: Unit Test - NMEA0183ParseRSA Custom Parser
**File**: `test/test_nmea0183_units/test_parsers.cpp`
**Description**: Test custom RSA parser extracts rudder angle correctly
**Prerequisites**: T005 (NMEA0183ParseRSA function), T006 (test fixtures)
**Details**:
- Create mock tNMEA0183Msg from VALID_APRSA fixture
- Call `NMEA0183ParseRSA(msg, rudderAngle)`
- Assert returns true
- Assert rudderAngle == 15.0
- Test invalid talker ID "VH" (should return false)
- Test invalid message code "HDM" (should return false)
- Test invalid status "V" (should return false)
- Test out-of-range angle 120.0° (parser extracts, handler validates later)

**Expected Result**: ❌ Test MUST FAIL (parser not yet implemented)

**Acceptance**:
- [ ] Tests valid sentence returns true and correct angle
- [ ] Tests talker ID validation
- [ ] Tests message code validation
- [ ] Tests status field validation

---

### Integration Tests (End-to-End Scenarios with Mocked Hardware)

### T015: Create Integration Test Directory
**File**: `test/test_nmea0183_integration/test_main.cpp`
**Description**: Create Unity test runner for integration scenarios
**Prerequisites**: T002 (MockSerialPort), T006 (test fixtures)
**Details**:
- Include Unity framework headers
- Create `setUp()` to initialize MockSerialPort, BoatData, NMEA0183Handler
- Create `tearDown()` to clean up instances
- Call RUN_TEST() for each integration test (T016-T023)
- Implement `main()` with UNITY_BEGIN() and UNITY_END()

**Acceptance**:
- [ ] Compiles for native platform
- [ ] Sets up test environment with mocked dependencies
- [ ] Tears down cleanly after each test

---

### T016 [P]: Integration Test - RSA Sentence to BoatData.RudderData
**File**: `test/test_nmea0183_integration/test_rsa_to_boatdata.cpp`
**Description**: Test autopilot RSA sentence updates rudder angle in BoatData
**Prerequisites**: T015 (integration test directory)
**Details**:
- Load MockSerialPort with VALID_APRSA ("$APRSA,15.0,A*3C\r\n")
- Call NMEA0183Handler.processSentences()
- Assert BoatData.getRudderData().steeringAngle == 0.2618 radians (15° in radians)
- Assert BoatData.getRudderData().available == true
- Assert BoatData.getRudderData().lastUpdate > 0 (timestamp updated)

**Expected Result**: ❌ Test MUST FAIL (handler not yet implemented)

**Acceptance**:
- [ ] Test loads mock sentence data
- [ ] Test verifies BoatData updated with correct radians value
- [ ] Test checks availability flag and timestamp

**Maps to Acceptance Scenario 1** (spec.md:61)

---

### T017 [P]: Integration Test - HDM Sentence to BoatData.CompassData
**File**: `test/test_nmea0183_integration/test_hdm_to_boatdata.cpp`
**Description**: Test autopilot HDM sentence updates magnetic heading in BoatData
**Prerequisites**: T015 (integration test directory)
**Details**:
- Load MockSerialPort with VALID_APHDM ("$APHDM,045.5,M*2F\r\n")
- Call NMEA0183Handler.processSentences()
- Assert BoatData.getCompassData().magneticHeading == 0.7941 radians (45.5° in radians)
- Assert BoatData.getCompassData().available == true

**Expected Result**: ❌ Test MUST FAIL

**Acceptance**:
- [ ] Test verifies magnetic heading conversion to radians
- [ ] Test checks availability flag

**Maps to Acceptance Scenario 2** (spec.md:63)

---

### T018 [P]: Integration Test - GGA Sentence to BoatData.GPSData
**File**: `test/test_nmea0183_integration/test_gga_to_boatdata.cpp`
**Description**: Test VHF GGA sentence updates GPS position in BoatData
**Prerequisites**: T015 (integration test directory)
**Details**:
- Load MockSerialPort with VALID_VHGGA
- Call NMEA0183Handler.processSentences()
- Assert BoatData.getGPSData().latitude == 52.508333 (52°30.5' in decimal)
- Assert BoatData.getGPSData().longitude == 5.116667 (5°7' in decimal)
- Assert BoatData.getGPSData().available == true

**Expected Result**: ❌ Test MUST FAIL

**Acceptance**:
- [ ] Test verifies DDMM.MMMM to decimal degree conversion
- [ ] Test checks both latitude and longitude
- [ ] Test uses epsilon comparison for floating-point (0.0001)

**Maps to Acceptance Scenario 3** (spec.md:65)

---

### T019 [P]: Integration Test - RMC Sentence to BoatData (GPS + Variation)
**File**: `test/test_nmea0183_integration/test_rmc_to_boatdata.cpp`
**Description**: Test VHF RMC sentence updates GPS data and magnetic variation
**Prerequisites**: T015 (integration test directory)
**Details**:
- Load MockSerialPort with VALID_VHRMC
- Call NMEA0183Handler.processSentences()
- Assert BoatData.getGPSData().latitude == 52.508333
- Assert BoatData.getGPSData().longitude == 5.116667
- Assert BoatData.getGPSData().cog == 0.9548 radians (54.7° in radians)
- Assert BoatData.getGPSData().sog == 5.5 knots (no conversion needed)
- Assert BoatData.getCompassData().variation == -0.0541 radians (3.1°W in radians, negative)

**Expected Result**: ❌ Test MUST FAIL

**Acceptance**:
- [ ] Test verifies all RMC fields extracted and converted
- [ ] Test checks variation sign (West is negative)
- [ ] Test validates COG in radians, SOG in knots

**Maps to Acceptance Scenario 4** (spec.md:67)

---

### T020 [P]: Integration Test - VTG Sentence to BoatData (COG/SOG + Calculated Variation)
**File**: `test/test_nmea0183_integration/test_vtg_to_boatdata.cpp`
**Description**: Test VHF VTG sentence updates COG/SOG and calculates variation
**Prerequisites**: T015 (integration test directory)
**Details**:
- Load MockSerialPort with VALID_VHVTG ("$VHVTG,054.7,T,057.9,M,5.5,N,10.2,K*48\r\n")
- Call NMEA0183Handler.processSentences()
- Assert BoatData.getGPSData().cog == 0.9548 radians (54.7° true COG)
- Assert BoatData.getGPSData().sog == 5.5 knots
- Assert BoatData.getCompassData().variation == -0.0558 radians (calculated: 54.7 - 57.9 = -3.2°)

**Expected Result**: ❌ Test MUST FAIL

**Acceptance**:
- [ ] Test verifies variation calculated from COG difference
- [ ] Test checks variation in valid range (±30°)
- [ ] Test validates true COG used (not magnetic)

**Maps to Acceptance Scenario 5** (spec.md:69)

---

### T021 [P]: Integration Test - Multi-Source Priority (NMEA 2000 Wins)
**File**: `test/test_nmea0183_integration/test_multi_source_priority.cpp`
**Description**: Test NMEA 2000 source (10 Hz) takes priority over NMEA 0183 (1 Hz)
**Prerequisites**: T015 (integration test directory)
**Details**:
- Register mock NMEA 2000 GPS source at 10 Hz frequency
- Load MockSerialPort with VALID_VHGGA (NMEA 0183 at 1 Hz)
- Call NMEA0183Handler.processSentences()
- Assert BoatData.updateGPS() returns false (rejected due to lower priority)
- Verify BoatData still contains NMEA 2000 data (not overwritten)

**Expected Result**: ❌ Test MUST FAIL

**Acceptance**:
- [ ] Test registers higher-priority source first
- [ ] Test verifies NMEA 0183 data rejected by BoatData
- [ ] Test confirms SourcePrioritizer honors frequency-based priority

**Maps to Acceptance Scenario 6** (spec.md:71)

---

### T022 [P]: Integration Test - Talker ID Filtering
**File**: `test/test_nmea0183_integration/test_talker_id_filter.cpp`
**Description**: Test sentences with invalid talker IDs are ignored
**Prerequisites**: T015 (integration test directory)
**Details**:
- Load MockSerialPort with "$GPGGA,123519,5230.5000,N,00507.0000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n" (talker ID "GP" not "VH")
- Call NMEA0183Handler.processSentences()
- Assert BoatData.getGPSData().available == false (no update)
- Repeat with talker ID "II" (Integrated Instruments)
- Assert sentence ignored (no BoatData update)

**Expected Result**: ❌ Test MUST FAIL

**Acceptance**:
- [ ] Test validates only "AP" and "VH" talker IDs accepted
- [ ] Test verifies "GP", "II", "WI" talker IDs ignored
- [ ] Test confirms BoatData not updated for ignored sentences

**Maps to Acceptance Scenario 7** (spec.md:73)

---

### T023 [P]: Integration Test - Unsupported Message Type Filtering
**File**: `test/test_nmea0183_integration/test_message_type_filter.cpp`
**Description**: Test unsupported sentence types are ignored
**Prerequisites**: T015 (integration test directory)
**Details**:
- Load MockSerialPort with "$APMWV,045.0,R,5.5,N,A*XX\r\n" (MWV wind data, not supported)
- Call NMEA0183Handler.processSentences()
- Assert no WindData update in BoatData
- Repeat with "$VHXDR,..." (XDR transducer data, explicitly out of scope)
- Assert sentence ignored

**Expected Result**: ❌ Test MUST FAIL

**Acceptance**:
- [ ] Test validates only RSA, HDM, GGA, RMC, VTG supported
- [ ] Test confirms MWV, XDR, DBT, etc. ignored
- [ ] Test verifies no error logs (silent ignore per FR-007)

**Maps to Acceptance Scenario 8** (spec.md:75)

---

### T024 [P]: Integration Test - Invalid Sentence Handling (Malformed/Out-of-Range)
**File**: `test/test_nmea0183_integration/test_invalid_sentences.cpp`
**Description**: Test malformed sentences and out-of-range values silently discarded
**Prerequisites**: T015 (integration test directory)
**Details**:
- **Test 1**: Bad checksum - "$APRSA,15.0,A*FF\r\n" (checksum should be *3C)
  - Assert BoatData not updated (silent discard per FR-024)
- **Test 2**: Out-of-range rudder angle - "$APRSA,120.0,A*XX\r\n" (exceeds ±90°)
  - Assert BoatData not updated (rejected per FR-026)
- **Test 3**: Out-of-range variation - "$VHVTG,054.7,T,090.0,M,5.5,N,10.2,K*XX\r\n" (variation = 35.3°, exceeds ±30°)
  - Assert BoatData.CompassData.variation not updated
- **Test 4**: Malformed sentence - "$APRSA,INVALID,A*XX\r\n" (non-numeric angle)
  - Assert sentence silently discarded

**Expected Result**: ❌ Test MUST FAIL

**Acceptance**:
- [ ] Test verifies no BoatData updates for invalid data
- [ ] Test confirms no error logs (silent discard)
- [ ] Test covers checksum, range, and parsing errors

**Maps to Edge Cases** (spec.md:79-85)

---

## Phase 3.3: Core Implementation (ONLY after tests are failing)

**CRITICAL**: Do NOT start this phase until all tests in Phase 3.2 are written and failing

### T025 [P]: Implement MockSerialPort
**File**: `src/mocks/MockSerialPort.cpp` (header already created in T002)
**Description**: Implement MockSerialPort methods to pass contract tests
**Prerequisites**: T008, T009, T010 (contract tests failing)
**Details**:
- Implement `available()` to return `strlen(mockData_) - position_`
- Implement `read()` to return `mockData_[position_++]` or -1 if empty
- Implement `begin()` as no-op
- Implement `setMockData()` to set `mockData_` pointer and reset `position_ = 0`

**Acceptance**:
- [x] Contract tests T008, T009, T010 pass
- [x] Can simulate NMEA sentence byte streams
- [x] Compiles for native platform

---

### T026 [P]: Implement ESP32SerialPort
**File**: `src/hal/implementations/ESP32SerialPort.cpp` (header already created in T003)
**Description**: Implement ESP32SerialPort methods to wrap HardwareSerial
**Prerequisites**: T008, T009, T010 (contract tests failing)
**Details**:
- Implement constructor to store HardwareSerial pointer
- Implement `available()`, `read()`, `begin()` as direct delegates
- No additional logic needed (thin wrapper)

**Acceptance**:
- [x] Contract tests pass with ESP32SerialPort (hardware test)
- [x] Compiles for ESP32 platform
- [x] No memory leaks (pointer not owned)

---

### T027 [P]: Implement UnitConverter Utility
**File**: `src/utils/UnitConverter.h` (implementation in header, created in T004)
**Description**: Implement unit conversion functions to pass unit tests
**Prerequisites**: T011, T012, T013 (unit tests failing)
**Details**:
- Implement `degreesToRadians(degrees)` as `degrees * DEG_TO_RAD`
- Implement `nmeaCoordinateToDecimal(degreesMinutes, hemisphere)`:
  - Extract degrees: `int deg = (int)(degreesMinutes / 100)`
  - Extract minutes: `double min = degreesMinutes - (deg * 100)`
  - Convert: `double decimal = deg + (min / 60.0)`
  - Apply sign: if (hemisphere == 'S' || hemisphere == 'W') decimal = -decimal
- Implement `calculateVariation(trueCOG, magCOG)`:
  - `double var = trueCOG - magCOG`
  - Normalize to [-180, 180]: while (var > 180) var -= 360; while (var < -180) var += 360
- Implement `normalizeAngle(radians)`:
  - while (radians >= 2*M_PI) radians -= 2*M_PI
  - while (radians < 0) radians += 2*M_PI

**Acceptance**:
- [x] Unit tests T011, T012, T013 pass
- [x] All functions static and header-only
- [x] Compiles for native and ESP32

---

### T028 [P]: Implement NMEA0183ParseRSA Custom Parser
**File**: `src/utils/NMEA0183Parsers.cpp` (header created in T005)
**Description**: Implement RSA parser to pass unit test
**Prerequisites**: T014 (unit test failing)
**Details**:
- Validate talker ID: `if (strcmp(msg.Sender(), "AP") != 0) return false;`
- Validate message code: `if (strcmp(msg.MessageCode(), "RSA") != 0) return false;`
- Validate status: `if (strcmp(msg.Field(1), "A") != 0) return false;`
- Extract angle: `rudderAngle = atof(msg.Field(0));`
- Return true

**Acceptance**:
- [x] Unit test T014 passes
- [x] Parser follows NMEA0183 library pattern
- [x] Returns false for invalid talker ID, message code, or status

---

### T029: Create NMEA0183Handler Component Structure
**File**: `src/components/NMEA0183Handler.h`, `src/components/NMEA0183Handler.cpp`
**Description**: Implement NMEA0183Handler class skeleton (constructor, dependencies, processSentences stub)
**Prerequisites**: T001 (ISerialPort), T004 (UnitConverter), T005 (Parsers)
**Details**:
- Define NMEA0183Handler class with constructor:
  - `NMEA0183Handler(tNMEA0183* nmea0183, ISerialPort* serialPort, BoatData* boatData, WebSocketLogger* logger)`
- Store dependencies as private members
- Implement `void init()` to call `serialPort_->begin(4800)`
- Implement `void processSentences()` stub (will be filled in T030-T035)
- Define handler dispatch table (struct with message code + function pointer)
- Declare private handler methods: `handleRSA()`, `handleHDM()`, `handleGGA()`, `handleRMC()`, `handleVTG()`

**Acceptance**:
- [ ] Class compiles with all dependencies injected
- [ ] Constructor stores pointers without taking ownership
- [ ] processSentences() is callable (even if stub)

---

### T030 [P]: Implement HandleRSA Sentence Handler
**File**: `src/components/NMEA0183Handler.cpp` (add to existing file)
**Description**: Implement RSA handler to pass integration test T016
**Prerequisites**: T029 (handler structure), T016 (integration test failing)
**Details**:
- In `handleRSA(const tNMEA0183Msg& msg)`:
  - Call `NMEA0183ParseRSA(msg, rudderAngle)`
  - If parse fails, return (silent discard)
  - Validate range: `if (fabs(rudderAngle) > 90.0) return;` (FR-026)
  - Convert to radians: `double angleRadians = UnitConverter::degreesToRadians(rudderAngle);`
  - Update BoatData: `bool accepted = boatData_->updateRudder(angleRadians, "NMEA0183-AP");`
  - Log DEBUG if accepted (optional, per Principle V)

**Acceptance**:
- [x] Integration test T016 passes
- [x] Out-of-range angles rejected (±90° limit)
- [x] Silent discard for invalid sentences

---

### T031 [P]: Implement HandleHDM Sentence Handler
**File**: `src/components/NMEA0183Handler.cpp` (add to existing file)
**Description**: Implement HDM handler to pass integration test T017
**Prerequisites**: T029 (handler structure), T017 (integration test failing)
**Details**:
- Parse using library: `double heading; if (!NMEA0183ParseHDM(msg, heading)) return;`
- Validate talker ID: `if (strcmp(msg.Sender(), "AP") != 0) return;`
- Validate range: `if (heading < 0.0 || heading > 360.0) return;`
- Convert: `double headingRadians = UnitConverter::degreesToRadians(heading);`
- Update: `boatData_->updateCompass(0.0, headingRadians, 0.0, "NMEA0183-AP");` (trueHdg=0 not updated by HDM)

**Acceptance**:
- [x] Integration test T017 passes
- [x] Only AP talker ID accepted
- [x] Heading in [0, 360] range

---

### T032 [P]: Implement HandleGGA Sentence Handler
**File**: `src/components/NMEA0183Handler.cpp` (add to existing file)
**Description**: Implement GGA handler to pass integration test T018
**Prerequisites**: T029 (handler structure), T018 (integration test failing)
**Details**:
- Parse using library: `double lat, lon; int fixQuality; if (!NMEA0183ParseGGA(msg, lat, lon, time, fixQuality, ...)) return;`
- Validate talker ID: `if (strcmp(msg.Sender(), "VH") != 0) return;`
- Validate fix quality > 0
- Validate range: `if (fabs(lat) > 90.0 || fabs(lon) > 180.0) return;`
- Update: `boatData_->updateGPS(lat, lon, 0.0, 0.0, "NMEA0183-VH");` (COG/SOG not in GGA)

**Acceptance**:
- [x] Integration test T018 passes
- [x] Only VH talker ID accepted
- [x] Coordinate range validation

---

### T033 [P]: Implement HandleRMC Sentence Handler
**File**: `src/components/NMEA0183Handler.cpp` (add to existing file)
**Description**: Implement RMC handler to pass integration test T019
**Prerequisites**: T029 (handler structure), T019 (integration test failing)
**Details**:
- Parse: `double lat, lon, sog, cog, variation; char status; if (!NMEA0183ParseRMC(msg, lat, lon, cog, sog, date, variation, &status)) return;`
- Validate talker ID "VH", status 'A', ranges (lat, lon, sog, variation ±30°)
- Convert: COG and variation to radians
- Update GPS: `boatData_->updateGPS(lat, lon, cogRadians, sog, "NMEA0183-VH");`
- Update variation: `boatData_->updateCompass(0.0, 0.0, variationRadians, "NMEA0183-VH");`

**Acceptance**:
- [x] Integration test T019 passes
- [x] Both GPS and variation updated
- [x] Variation range validated (±30°)

---

### T034 [P]: Implement HandleVTG Sentence Handler
**File**: `src/components/NMEA0183Handler.cpp` (add to existing file)
**Description**: Implement VTG handler to pass integration test T020
**Prerequisites**: T029 (handler structure), T020 (integration test failing)
**Details**:
- Parse: `double trueCOG, magCOG, sog; if (!NMEA0183ParseVTG(msg, trueCOG, magCOG, sog)) return;`
- Validate talker ID "VH"
- Calculate variation: `double variation = UnitConverter::calculateVariation(trueCOG, magCOG);`
- Validate variation range: `if (fabs(variation) > 30.0) return;`
- Convert: trueCOG and variation to radians
- Update GPS: `boatData_->updateGPS(0.0, 0.0, trueCOGRadians, sog, "NMEA0183-VH");`
- Update variation: `boatData_->updateCompass(0.0, 0.0, variationRadians, "NMEA0183-VH");`

**Acceptance**:
- [x] Integration test T020 passes
- [x] Variation calculated from COG difference
- [x] Variation range validated

---

### T035: Implement processSentences() Main Loop
**File**: `src/components/NMEA0183Handler.cpp` (complete method stub from T029)
**Description**: Implement sentence polling and handler dispatch
**Prerequisites**: T030-T034 (all handlers implemented)
**Details**:
- Read bytes from serial port in loop while `serialPort_->available() > 0`
- Feed bytes to NMEA parser: `nmea0183_->HandleByte(byte)`
- When parser signals complete sentence, look up handler in dispatch table
- Call appropriate handler method (handleRSA, handleHDM, etc.)
- If message code not in table, ignore (silent discard per FR-007)
- Add basic performance logging (optional): measure time per sentence

**Acceptance**:
- [x] All integration tests T016-T024 pass
- [x] Handler dispatch table works correctly
- [x] Unsupported message types silently ignored

---

## Phase 3.4: Integration (main.cpp and ReactESP)

### T036: Integrate NMEA0183Handler into main.cpp Setup
**File**: `src/main.cpp`
**Description**: Initialize Serial2, NMEA0183Handler, and register with BoatData
**Prerequisites**: T035 (handler implementation complete)
**Details**:
- After BoatData initialization, create Serial2 adapter:
  ```cpp
  ISerialPort* serial0183 = new ESP32SerialPort(&Serial2);
  ```
- Create NMEA0183 library instance: `tNMEA0183* nmea0183 = new tNMEA0183();`
- Create handler: `NMEA0183Handler* nmea0183Handler = new NMEA0183Handler(nmea0183, serial0183, boatData, &logger);`
- Call `nmea0183Handler->init();` to start Serial2 at 4800 baud
- Log initialization: `logger.broadcastLog(LogLevel::INFO, F("NMEA0183"), F("INIT"), F("{\"port\":\"Serial2\",\"baud\":4800}"));`

**Acceptance**:
- [ ] Serial2 initialized at 4800 baud
- [ ] Handler instance created with dependency injection
- [ ] Compiles for ESP32 platform
- [ ] Logs initialization success

---

### T037: Register NMEA0183Handler ReactESP Event Loop
**File**: `src/main.cpp`
**Description**: Add ReactESP periodic task to process NMEA sentences every 10ms
**Prerequisites**: T036 (handler integrated)
**Details**:
- After other ReactESP loop registrations, add:
  ```cpp
  app.onRepeat(10, []() {
      nmea0183Handler->processSentences();
  });
  ```
- Ensure loop registered BEFORE `app.tick()` in main loop
- Log loop registration: `logger.broadcastLog(LogLevel::DEBUG, F("NMEA0183"), F("LOOP_REGISTERED"), F("{\"interval\":10}"));`

**Acceptance**:
- [ ] ReactESP loop calls processSentences() every 10ms
- [ ] Non-blocking operation (other loops still run)
- [ ] Compiles and boots on ESP32

---

### T038: Add WebSocket Logging for NMEA Events
**File**: `src/components/NMEA0183Handler.cpp`
**Description**: Add DEBUG logging for successful sentence processing
**Prerequisites**: T035 (handler implementation)
**Details**:
- In each handler (handleRSA, handleHDM, etc.), after successful BoatData update:
  ```cpp
  if (accepted) {
      logger_->broadcastLog(LogLevel::DEBUG, F("NMEA0183"), F("SENTENCE_PROCESSED"),
          String(F("{\"type\":\"RSA\",\"source\":\"NMEA0183-AP\",\"value\":")) + String(angleRadians) + F("}"));
  }
  ```
- Log rejected sentences at WARN level (only for higher-priority source conflicts, not invalid data per FR-024)
- Use F() macro for all string literals (Constitutional Principle II)

**Acceptance**:
- [ ] DEBUG logs appear for valid sentences
- [ ] No logs for silently discarded sentences (checksum, out-of-range)
- [ ] Logs include sentence type and source ID

---

### T039: Register NMEA0183 Sources with BoatData Prioritizer
**File**: `src/main.cpp`
**Description**: Register "NMEA0183-AP" and "NMEA0183-VH" sources with SourcePrioritizer
**Prerequisites**: T036 (handler integrated)
**Details**:
- After NMEA0183Handler initialization, register sources:
  ```cpp
  // Assuming SourcePrioritizer is accessible via BoatData
  boatData->getSourcePrioritizer()->registerSource("NMEA0183-AP", SensorType::COMPASS, ProtocolType::NMEA0183);
  boatData->getSourcePrioritizer()->registerSource("NMEA0183-VH", SensorType::GPS, ProtocolType::NMEA0183);
  ```
- Sources will auto-prioritize based on update frequency (NMEA 2000 at 10 Hz > NMEA 0183 at 1 Hz)

**Acceptance**:
- [ ] Sources registered before first sentence processed
- [ ] Multi-source priority test T021 passes (NMEA 2000 wins)
- [ ] Source IDs visible in BoatData diagnostics

---

## Phase 3.5: Hardware Validation & Polish

### T040: Create Hardware Test for Serial2 Timing
**File**: `test/test_nmea0183_hardware/test_main.cpp`
**Description**: Validate Serial2 4800 baud reception and 50ms processing budget on ESP32
**Prerequisites**: T037 (ReactESP integration complete)
**Details**:
- Connect ESP32 Serial2 to NMEA simulator or loopback test
- Send continuous NMEA sentences at 1 Hz (typical rate)
- Measure processing time per sentence using millis()
- Assert processing time < 50ms (FR-027)
- Test buffer handling: send burst of 10 sentences, verify all processed
- Test overflow: send 100 sentences rapidly, verify FIFO drop oldest (FR-032)

**Acceptance**:
- [ ] All sentences processed within 50ms budget
- [ ] No buffer overruns at normal rates (1-7 sentences/sec)
- [ ] Graceful degradation at overflow (oldest dropped)

**Test Execution**: `pio test -e esp32dev_test -f test_nmea0183_hardware`

---

### T041: Validate Memory Footprint (Static Allocation)
**File**: N/A (build verification task)
**Description**: Verify NMEA 0183 feature meets memory budget (RAM < 200 bytes, Flash < 20KB)
**Prerequisites**: T039 (all implementation complete)
**Details**:
- Build for ESP32: `pio run -e esp32dev`
- Check RAM usage in build output: should increase ~140 bytes from baseline
- Check Flash usage: should increase ~15KB from baseline
- Verify no heap allocations in handler code (use static analysis or manual review)
- Document actual footprint in research.md "Memory Impact Estimate" section

**Acceptance**:
- [ ] RAM increase ≤ 200 bytes (target: ~140 bytes)
- [ ] Flash increase ≤ 20KB (target: ~15KB)
- [ ] All allocations are static (no `new` in handler except initialization)

**Constitutional Compliance**: Principle II (Resource Management)

---

### T042 [P]: Update CLAUDE.md Integration Guide
**File**: `CLAUDE.md`
**Description**: Document NMEA 0183 handler usage and integration
**Prerequisites**: T039 (implementation complete)
**Details**:
- Add section: "## NMEA 0183 Integration"
- Document supported sentence types (RSA, HDM, GGA, RMC, VTG)
- Document talker IDs (AP, VH)
- Document initialization sequence (Serial2 → Handler → ReactESP loop)
- Document testing commands:
  ```bash
  # All NMEA 0183 tests
  pio test -e native -f test_nmea0183_*

  # Hardware validation
  pio test -e esp32dev_test -f test_nmea0183_hardware
  ```
- Document WebSocket log monitoring for NMEA events

**Acceptance**:
- [ ] CLAUDE.md updated with NMEA 0183 section
- [ ] Test commands documented
- [ ] Integration pattern clear for future features

---

### T043 [P]: Update README.md Feature Status
**File**: `README.md`
**Description**: Mark NMEA 0183 handlers as implemented
**Prerequisites**: T039 (implementation complete)
**Details**:
- Update feature list to mark "R007 - NMEA 0183 Data Handlers" as ✅ Complete
- Add supported sentence types to feature description
- Update test coverage statistics (35-40 tests added)

**Acceptance**:
- [ ] README.md reflects NMEA 0183 implementation
- [ ] Feature status accurate

---

### T044: Constitutional Compliance Validation
**File**: N/A (review task)
**Description**: Verify all 7 constitutional principles met
**Prerequisites**: T041 (memory validated), T042 (docs updated)
**Details**:
- **Principle I (HAL)**: ✓ ISerialPort abstracts Serial2, MockSerialPort for testing
- **Principle II (Resources)**: ✓ Static allocation (~140 bytes RAM, ~15KB flash), F() macros used
- **Principle III (QA Review)**: ✓ Custom RSA parser flagged for human review, all code reviewed
- **Principle IV (Modular)**: ✓ Single-responsibility components (Handler, Parsers, UnitConverter)
- **Principle V (Network Debug)**: ✓ WebSocket logging for sentence events (DEBUG/WARN levels)
- **Principle VI (Always-On)**: ✓ ReactESP 10ms non-blocking loop, no sleep modes
- **Principle VII (Fail-Safe)**: ✓ Graceful degradation with silent discard, no crashes
- Create checklist in plan.md "Constitution Check" section, mark all [x]

**Acceptance**:
- [ ] All 7 principles validated
- [ ] No constitutional violations
- [ ] plan.md Constitution Check updated

---

## Dependencies

```
Setup (T001-T006)
    ↓
Tests Written (T007-T024) ← MUST FAIL BEFORE IMPLEMENTATION
    ↓
Implementation (T025-T035) ← Makes tests pass
    ↓
Integration (T036-T039) ← Wires into main.cpp
    ↓
Polish (T040-T044) ← Validation and docs
```

**Detailed Dependencies**:
- T001 (ISerialPort) blocks T002, T003, T007
- T002 (MockSerialPort) blocks T008-T010
- T004 (UnitConverter) blocks T011-T013
- T005 (Parsers) blocks T014
- T006 (Test fixtures) blocks T015-T024
- T007 (Contract test dir) blocks T008-T010
- T015 (Integration test dir) blocks T016-T024
- T029 (Handler structure) blocks T030-T035
- All tests (T007-T024) block corresponding implementation tasks
- T035 (processSentences) blocks T036
- T036 (main.cpp init) blocks T037, T039
- T039 (source registration) blocks T040

---

## Parallel Execution Examples

### Phase 3.1: Setup (Parallelizable)
```bash
# T002, T003, T004, T005, T006 can all run in parallel (different files, no dependencies after T001)
# Execute simultaneously:
# - T002: Create MockSerialPort
# - T003: Create ESP32SerialPort
# - T004: Create UnitConverter
# - T005: Create NMEA0183Parsers
# - T006: Create test fixtures
```

### Phase 3.2: Tests (Highly Parallelizable)
```bash
# After test directories created (T007, T015), all test files can be written in parallel:
# Contract tests: T008, T009, T010 (same file, but independent test cases)
# Unit tests: T011, T012, T013, T014 (different files)
# Integration tests: T016-T024 (different files)

# Execute integration tests in parallel (9 independent test files):
# - T016: test_rsa_to_boatdata.cpp
# - T017: test_hdm_to_boatdata.cpp
# - T018: test_gga_to_boatdata.cpp
# - T019: test_rmc_to_boatdata.cpp
# - T020: test_vtg_to_boatdata.cpp
# - T021: test_multi_source_priority.cpp
# - T022: test_talker_id_filter.cpp
# - T023: test_message_type_filter.cpp
# - T024: test_invalid_sentences.cpp
```

### Phase 3.3: Implementation (Moderate Parallelization)
```bash
# After handler structure (T029), all handler methods can be implemented in parallel:
# BUT all in same file (NMEA0183Handler.cpp), so coordinate to avoid conflicts
# Alternative: Implement T030-T034 sequentially, OR use git branches per handler

# T025, T026, T027, T028 are independent (different files):
# - T025: MockSerialPort.cpp
# - T026: ESP32SerialPort.cpp
# - T027: UnitConverter.h
# - T028: NMEA0183Parsers.cpp
```

### Phase 3.5: Documentation (Fully Parallelizable)
```bash
# T042, T043 can run in parallel (different files):
# - T042: Update CLAUDE.md
# - T043: Update README.md
```

---

## Test Execution Commands

### Run All NMEA 0183 Tests (Native Platform)
```bash
pio test -e native -f test_nmea0183_*
```

### Run Specific Test Groups
```bash
# Contract tests only
pio test -e native -f test_nmea0183_contracts

# Integration tests only
pio test -e native -f test_nmea0183_integration

# Unit tests only
pio test -e native -f test_nmea0183_units
```

### Run Hardware Validation Tests (ESP32 Required)
```bash
pio test -e esp32dev_test -f test_nmea0183_hardware
```

### Run Specific Integration Scenario
```bash
# Example: Test only RSA sentence handling
pio test -e native -f test_nmea0183_integration --gtest_filter=*rsa_to_boatdata*
```

---

## Notes

- **[P] Marking**: 21 tasks marked [P] can run in parallel with their peer tasks (different files, no shared dependencies)
- **TDD Discipline**: Phase 3.2 tests MUST be written and MUST FAIL before Phase 3.3 implementation starts
- **Commit Frequency**: Commit after each task completion (40 commits for feature)
- **Test-First**: Integration tests T016-T024 map directly to acceptance scenarios in spec.md:61-75
- **Silent Discard**: FR-024, FR-025, FR-026 require NO logging for invalid sentences (malformed, bad checksum, out-of-range)
- **Constitutional Flags**: Custom RSA parser (T028) requires human review per Principle III

---

## Validation Checklist
*GATE: Verify before marking feature complete*

- [x] All contracts (ISerialPort) have corresponding tests (T008-T010)
- [x] All entities (UnitConverter, Parsers, Handler) have implementation tasks
- [x] All tests come before implementation (Phase 3.2 before Phase 3.3)
- [x] Parallel tasks truly independent (verified file paths)
- [x] Each task specifies exact file path
- [x] No task modifies same file as another [P] task (except intentional sequential tasks in Phase 3.3)
- [x] All acceptance scenarios (spec.md:61-75) have integration tests (T016-T024)
- [x] Memory budget validated (T041)
- [x] Constitutional compliance validated (T044)

---

**Tasks Generated**: 44 tasks ready for execution
**Next Step**: Start with Phase 3.1 Setup tasks (T001-T006)
**Estimated Completion**: 12-16 hours (parallel execution can reduce to 8-10 hours)
