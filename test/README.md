# Poseidon2 Test Suite

This directory contains all tests for the Poseidon2 marine gateway project, organized using PlatformIO's grouped test structure.

## Test Organization

Tests are grouped by feature and type into directories with `test_` prefix. Each group contains:
- `test_main.cpp` - Main entry point that runs all tests in the group using Unity framework
- Individual test files - Test implementations (no main() function)

### Test Groups

#### BoatData Tests

**test_boatdata_contracts/** - HAL Interface Contract Tests
- Validates: IBoatDataStore, ISensorUpdate, ICalibration, ISourcePrioritizer
- Files: 4 contract test files + test_main.cpp
- Run: `pio test -e native -f test_boatdata_contracts`

**test_boatdata_integration/** - Integration Test Scenarios
- Validates: 7 complete scenarios from quickstart.md
  1. Single GPS source
  2. Multi-source priority
  3. Source failover
  4. Manual override
  5. Derived calculations (to be created)
  6. Calibration update (to be created)
  7. Outlier rejection
- Files: 5 integration test files + test_main.cpp (2 missing)
- Run: `pio test -e native -f test_boatdata_integration`

**test_boatdata_units/** - Unit Tests for Formulas & Utilities
- Validates: AngleUtils, DataValidator, calculation formulas
- Files: test_main.cpp (test files to be created)
- Run: `pio test -e native -f test_boatdata_units`

**test_boatdata_timing/** - Hardware Performance Test
- Validates: 200ms calculation cycle timing on ESP32
- Files: test_main.cpp
- Run: `pio test -e esp32dev_test -f test_boatdata_timing`

#### WiFi Tests

**test_wifi_integration/** - WiFi Connection Scenarios
- Validates: Network connection, failover, recovery
- Files: 5 integration test files + test_main.cpp
- Run: `pio test -e native -f test_wifi_integration`

**test_wifi_units/** - WiFi Component Unit Tests
- Validates: Config parser, state machine, credentials, manager
- Files: 6 unit test files + test_main.cpp
- Run: `pio test -e native -f test_wifi_units`

**test_wifi_endpoints/** - WiFi HTTP API Tests
- Validates: /upload-wifi-config, /wifi-config, /wifi-status endpoints
- Files: 4 endpoint test files + test_main.cpp
- Run: `pio test -e native -f test_wifi_endpoints`

**test_wifi_connection/** - Hardware WiFi Test
- Validates: WiFi connection on ESP32 hardware
- Files: test_main.cpp
- Run: `pio test -e esp32dev_test -f test_wifi_connection`

#### Shared Utilities

**helpers/** - Shared Test Utilities (not a test)
- Contains: Mocks, fixtures, helper functions
- Usage: `#include "../helpers/test_mocks.h"`

## Running Tests

### Run All Tests
```bash
pio test -e native
```

### Run Tests by Feature
```bash
# All BoatData tests
pio test -e native -f test_boatdata_*

# All WiFi tests
pio test -e native -f test_wifi_*
```

### Run Tests by Type
```bash
# All contract tests (BoatData + WiFi)
pio test -e native -f test_*_contracts

# All integration tests
pio test -e native -f test_*_integration

# All unit tests
pio test -e native -f test_*_units
```

### Run Specific Test Group
```bash
pio test -e native -f test_boatdata_contracts
pio test -e native -f test_wifi_integration
```

### Hardware Tests (ESP32 Required)
```bash
pio test -e esp32dev_test -f test_boatdata_timing
pio test -e esp32dev_test -f test_wifi_connection
```

## Test Status

### Completed
- ✅ Test structure migration (2025-10-07)
- ✅ BoatData contract tests (4/4 tests)
- ✅ BoatData integration tests (5/7 tests)
- ✅ WiFi integration tests (5/5 tests)
- ✅ WiFi unit tests (6/6 tests)
- ✅ WiFi endpoint tests (4/4 tests)
- ✅ Hardware tests (2/2 tests)

### To Be Created
- ⚠️ BoatData integration: test_derived_calculation.cpp (T012)
- ⚠️ BoatData integration: test_calibration_update.cpp (T013)
- ⚠️ BoatData units: test_angle_utils.cpp (T015-T016)
- ⚠️ BoatData units: test_data_validator.cpp (T017-T018)
- ⚠️ BoatData units: test_calculation_formulas.cpp (T019-T024)

## Migration History

**2025-10-07**: Restructured from flat structure to grouped PlatformIO-compliant structure

**Old Structure** (not working with PlatformIO):
```
test/
├── contract/*.cpp          # Individual files
├── integration/*.cpp       # Individual files
└── unit/*.cpp             # Individual files
```

**New Structure** (PlatformIO-compliant):
```
test/
├── test_boatdata_contracts/    # Group with test_main.cpp
├── test_boatdata_integration/  # Group with test_main.cpp
├── test_boatdata_units/        # Group with test_main.cpp
├── test_wifi_integration/      # Group with test_main.cpp
├── test_wifi_units/            # Group with test_main.cpp
├── test_wifi_endpoints/        # Group with test_main.cpp
├── test_wifi_connection/       # Hardware test
└── test_boatdata_timing/       # Hardware test
```

**Benefits**:
- ✅ PlatformIO test discovery now works
- ✅ Easy filtering by feature or type
- ✅ Manageable number of directories (9 vs 70+)
- ✅ Better CI/CD integration
- ✅ Follows PlatformIO best practices

## Documentation

See also:
- `/CLAUDE.md` - Complete test organization and usage guide
- `/specs/003-boatdata-feature-as/tasks.md` - Test task definitions
- `/specs/003-boatdata-feature-as/quickstart.md` - Integration test scenarios
- `/platformio.ini` - Test configuration and examples

---

**Last Updated**: 2025-10-07
**Structure Version**: 2.0 (PlatformIO Grouped Tests)
