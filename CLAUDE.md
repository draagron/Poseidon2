# CLAUDE.md

This file provides guidance to Claude Code when working with code in this repository.

## Project Overview

**Poseidon2** is an ESP32-based marine interface gateway that enables bidirectional communication between SignalK servers and boat instruments using multiple marine data protocols:
- NMEA 2000 (bidirectional via CAN bus)
- NMEA 0183 (read-only via Serial2)
- 1-Wire sensor devices

The system acts as a protocol bridge, built with PlatformIO and the Arduino framework, running on SH-ESP32 hardware designed for 24/7 always-on marine applications.

## Development Environment

**Platform**: PlatformIO with Arduino framework
**Target Hardware**: ESP32 (ESP32, ESP32-S2, ESP32-C3, ESP32-S3)
**Language**: C++ (C++11 minimum, C++14 preferred)

### Essential Commands

```bash
# Build the project
pio run

# Upload to ESP32
pio run --target upload

# Run tests
pio test -e native

# Run all tests for a feature
pio test -e native -f test_<feature>_*
```

## Spec Kit Structure

```
.specify/
├── memory/
│   └── constitution.md          # Development principles and standards (v1.2.0)
└── templates/
    ├── plan-template.md         # Implementation planning template
    ├── spec-template.md         # Feature specification template
    └── tasks-template.md        # Task breakdown template

user_requirements/
└── R*** - <feature_name>.md     # User requirements

specs/
└── <feature_num>-<feature_name>/
    ├── spec.md                  # Feature specification
    ├── plan.md                  # Implementation plan
    ├── tasks.md                 # Task breakdown
    └── quickstart.md            # (optional) Validation guide
```

## Spec Kit Workflow

### Slash Commands Available
Use these commands for structured development workflows:
- `/specify` - Create feature specification from natural language
- `/plan` - Generate implementation plan with TDD approach
- `/tasks` - Create dependency-ordered task list
- `/clarify` - Identify underspecified areas in spec
- `/analyze` - Cross-artifact consistency analysis
- `/implement` - Execute implementation plan
- `/constitution` - Update project constitution

### Extension Workflows
- `/speckit.bugfix` - Bug fix with regression test requirement
- `/speckit.modify` - Modify existing feature with impact analysis
- `/speckit.refactor` - Refactoring with metrics tracking
- `/speckit.hotfix` - Emergency hotfix with expedited process
- `/speckit.deprecate` - Feature deprecation with phased sunset

## Constitutional Compliance

**BEFORE implementing any feature**, verify against constitution principles (`.specify/memory/constitution.md`):
1. **Hardware Abstraction** - HAL interfaces used?
2. **Resource Management** - Static allocation, stack limits, F() macros?
3. **QA Review Process** - Review planned?
4. **Modular Design** - Single responsibility, dependency injection?
5. **Network Debugging** - WebSocket logging implemented?
6. **Always-On Operation** - No sleep modes?
7. **Fail-Safe Operation** - Watchdog, safe mode, graceful degradation?

See `.specify/templates/plan-template.md` for detailed constitution checklist.

## Architecture & Key Constraints

### Hardware Abstraction Layer (HAL) - CRITICAL
All hardware interactions MUST be abstracted through interfaces:
- Hardware dependencies isolated to HAL modules (`src/hal/`)
- Mock implementations required for all HAL interfaces
- Business logic must be separable from hardware I/O for testing
- No direct GPIO/peripheral access outside HAL layer

### Resource Management - NON-NEGOTIABLE
ESP32 memory constraints require strict discipline:
- **Static allocation preferred** - minimize heap usage to avoid fragmentation
- **Stack monitoring** - ESP32 default is 8KB per task, stay within limits
- **Flash tracking** - compile warnings at >80% capacity
- **String handling** - Use `F()` macro or PROGMEM for literals; prefer char arrays over String class
- **RTOS tasks** - Explicitly specify stack sizes

### Testing Strategy

**Test Organization** (PlatformIO Grouped Tests):
```
test/
├── helpers/                     # Shared test utilities (not a test)
├── test_<feature>_contracts/    # HAL interface tests
├── test_<feature>_integration/  # End-to-end scenarios
├── test_<feature>_units/        # Formula/utility tests
└── test_<feature>_hardware/     # Hardware timing tests (ESP32)
```

**Test Discovery**: PlatformIO discovers only directories with `test_` prefix.

**Test Filtering Examples**:
```bash
# Run all tests for a feature
pio test -e native -f test_<feature>_*

# Run only contract tests
pio test -e native -f test_*_contracts

# Run only integration tests
pio test -e native -f test_*_integration

# Run only unit tests
pio test -e native -f test_*_units
```

### File Organization Pattern
```
src/
├── main.cpp              # Entry point
├── config.h              # Compile-time configuration
├── hal/                  # Hardware Abstraction Layer
│   ├── interfaces/       # HAL interfaces (abstract)
│   └── implementations/  # Hardware-specific implementations
├── components/           # Feature components (single responsibility)
├── utils/                # Utility functions
└── mocks/                # Mock implementations for testing
```

## Core Libraries & Documentation

**Pre-approved Libraries** (already in use):
- **NMEA2000**: https://github.com/ttlappalainen/NMEA2000 - API docs: https://ttlappalainen.github.io/NMEA2000/pg_lib_ref.html
- **NMEA0183**: https://github.com/ttlappalainen/NMEA0183
- **NMEA2000_esp32**: https://github.com/ttlappalainen/NMEA2000_esp32 - ESP32-specific CAN implementation
- **ReactESP**: https://github.com/mairas/ReactESP - Asynchronous event loops (required pattern)
- **ESPAsyncWebServer**: https://github.com/ESP32Async/ESPAsyncWebServer - Async HTTP/WebSocket
- **Adafruit_SSD1306**: OLED display driver

**Reference First**: Before implementing features, check:
1. `examples/poseidongw/src/` - working implementation patterns
2. Official library documentation (links above)
3. Library example code in repositories

## Code Standards - MUST FOLLOW

**Memory Management**:
- Minimize and track heap allocations
- Validate buffer sizes against available memory
- Specify RTOS task stack sizes explicitly

**Error Handling**:
- Return codes/result types for operations that can fail
- No silent failures - all errors logged via WebSocket
- Critical errors trigger safe mode or controlled restart
- User-facing error messages must be actionable

**Documentation**:
- Doxygen-style comments for all public functions/classes
- HAL interfaces include usage examples
- Complex algorithms need inline explanations
- README.md per component (purpose + dependencies)

**Version Control**:
- Conventional commits format (feat:, fix:, refactor:, docs:)
- Firmware versioning: MAJOR.MINOR.PATCH (semantic)
- Feature branches merged to `main` after QA approval
- Tag releases with firmware version

## Initialization Sequence - MUST FOLLOW THIS ORDER
1. WiFi connection
2. I2C buses (Wire0 and Wire1)
3. OLED display (I2C Bus 2, 128x64 SSD1306, address 0x3C)
4. Serial2 for NMEA 0183
5. NMEA2000 CAN bus
6. Message handlers registration
7. ReactESP event loops

## GPIO Pin Configuration (SH-ESP32 Board)
```
CAN RX: GPIO 34        |  I2C Bus 1: SDA=16, SCL=17
CAN TX: GPIO 32        |  I2C Bus 2: SDA=21, SCL=22
Serial2: RX=25, TX=27  |  1-Wire: GPIO 4
OLED Display: I2C Bus 2|  Button: GPIO 13
SD Card: GPIO 15       |  LED: GPIO 2
```

## Network-Based Debugging
Serial ports are used for device communication (NMEA 0183), NOT debugging:
- Use WebSocket logging for reliable debug output (ws://<device-ip>/logs)
- Log levels: DEBUG, INFO, WARN, ERROR, FATAL
- Include timestamps (millis() or RTC)
- Production builds: only ERROR/FATAL levels
- TCP-based protocol ensures no packet loss
- Fallback: Store critical errors to flash (SPIFFS/LittleFS) if WebSocket unavailable

## Key Implementation Patterns

### Asynchronous Programming with ReactESP
The reference implementation (`examples/poseidongw/`) demonstrates the required event-driven pattern:
- Use ReactESP event loops for all periodic tasks
- No blocking delays in main loop
- Register callbacks for NMEA message handlers
- WebSocket logging via `WebSocketLogger` class (see src/utils/WebSocketLogger.h)

### Always-On Operation
- WiFi MUST remain always-on (no power management)
- Deep sleep/light sleep modes NOT permitted
- Designed for 24/7 operation with continuous network availability
- Watchdog timer enabled in production builds

## Build Configurations

**Development**:
- WebSocket debug logging enabled (verbose)
- Assertions enabled
- All log levels active

**Production**:
- ERROR/FATAL logging only
- Watchdog timer enabled
- OTA updates enabled
- Optimizations on
