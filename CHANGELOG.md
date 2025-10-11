# Changelog

All notable changes to the Poseidon2 project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added - NMEA 0183 Data Handlers (Phase 3.1: Setup Complete)

#### HAL Interfaces
- **ISerialPort** (`src/hal/interfaces/ISerialPort.h`): Hardware abstraction layer interface for serial port communication
  - Non-blocking `available()` method to check bytes in receive buffer
  - FIFO-ordered `read()` method for byte-by-byte consumption
  - Idempotent `begin()` method for serial port initialization
  - Complete behavioral contract documentation (non-blocking, -1 on empty, FIFO order)

#### HAL Implementations
- **ESP32SerialPort** (`src/hal/implementations/ESP32SerialPort.h/cpp`): ESP32 hardware adapter for Serial2
  - Thin wrapper around Arduino HardwareSerial class
  - Delegates to Serial2 for NMEA 0183 reception at 4800 baud
  - Zero-copy pointer storage (no ownership)
  - Designed for GPIO25 (RX) / GPIO27 (TX) on ESP32

#### Mock Implementations
- **MockSerialPort** (`src/mocks/MockSerialPort.h/cpp`): Mock serial port for testing
  - Simulates byte-by-byte NMEA sentence reading
  - `setMockData()` method for loading predefined test sentences
  - Non-blocking behavior matching hardware serial
  - Enables native platform testing without ESP32 hardware

#### Utilities
- **UnitConverter** (`src/utils/UnitConverter.h`): Static utility functions for marine data unit conversions
  - Degrees ” Radians conversion (`degreesToRadians`, `radiansToDegrees`)
  - NMEA coordinate format (DDMM.MMMM) ’ Decimal degrees conversion
  - Magnetic variation calculation from true/magnetic course difference
  - Angle normalization to [0, 2À] and [-À, À] ranges
  - Header-only implementation for performance (inline expansion)

- **NMEA0183Parsers** (`src/utils/NMEA0183Parsers.h/cpp`): Custom NMEA 0183 sentence parsers
  - `NMEA0183ParseRSA()`: Rudder Sensor Angle parser for autopilot data
  - Validates talker ID ("AP" for autopilot), message code, and status field
  - Returns bool for success/failure (library parser pattern)
  - Complements ttlappalainen/NMEA0183 library (RSA not included in library)

#### Test Infrastructure
- **NMEA 0183 Test Fixtures** (`test/helpers/nmea0183_test_fixtures.h`): Shared test utilities
  - Valid NMEA sentence constants with correct checksums (RSA, HDM, GGA, RMC, VTG)
  - Invalid sentence fixtures for error testing (bad checksum, out-of-range, wrong talker ID)
  - Checksum calculation and verification helper functions
  - Header-only fixture library for all test files

### Technical Details
- **Memory Footprint**: ~140 bytes static allocation (0.04% of ESP32 RAM)
- **Flash Impact**: ~15KB production code (~0.8% of partition)
- **Constitutional Compliance**:
  -  Principle I: Hardware Abstraction Layer (ISerialPort interface)
  -  Principle II: Resource Management (static allocation, F() macros, header-only utilities)
  -  Principle IV: Modular Design (single-responsibility components, dependency injection)

### Changed
- Updated `platformio.ini`: Added NMEA 0183 source files to build
- Updated `tasks.md`: Marked Phase 3.1 tasks (T001-T006) as completed

### Validated
-  Build successful: `pio run -e esp32dev` (5.08 seconds)
-  RAM usage: 13.7% (44,756 / 327,680 bytes)
-  Flash usage: 47.8% (939,289 / 1,966,080 bytes) - well under 80% limit
-  All files compile without errors for ESP32 platform
-  Mock implementations compile for native platform (tests upcoming in Phase 3.2)

### Next Steps (Phase 3.2: Tests First - TDD)
- T007-T010: Contract tests for ISerialPort interface (HAL validation)
- T011-T014: Unit tests for UnitConverter and NMEA0183ParseRSA
- T015-T024: Integration tests for end-to-end sentence processing scenarios
- **CRITICAL**: All tests must be written and must FAIL before Phase 3.3 implementation

---

**Phase 3.1 Status**:  **COMPLETE** (6/6 tasks, 100%)
**Date**: 2025-10-11
**Branch**: `006-nmea-0183-handlers`
**Estimated Time**: 45 minutes actual (vs 2-3 hours estimated)
