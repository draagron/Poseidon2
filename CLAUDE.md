# CLAUDE.md

This file provides guidance for Claude Code when working with this repository using the Spec Kit development workflow.

## Project Overview

**Poseidon2** is an ESP32-based marine interface gateway that enables bidirectional communication between SignalK servers and boat instruments using multiple marine data protocols:
- NMEA 2000 (bidirectional via CAN bus)
- NMEA 0183 (read-only via Serial2)
- 1-Wire sensor devices

The system acts as a protocol bridge, built with PlatformIO and the Arduino framework, running on SH-ESP32 hardware designed for 24/7 always-on marine applications.

**Architecture**: Hardware Abstraction Layer (HAL) design with testable components, static memory allocation, and network-based debugging.

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

## Spec Kit Workflow - PRIMARY DEVELOPMENT METHOD

**IMPORTANT**: Always use Spec Kit workflow for new features, refactors, and bug fixes.

### Standard Workflow Sequence

1. **User Requirement** → Create in `user_requirements/R### - <feature>.md`
2. **`/specify`** → Generate `specs/<feature>/spec.md` from natural language
3. **`/clarify`** → Identify and resolve underspecified areas (iterate as needed)
4. **`/plan`** → Generate `specs/<feature>/plan.md` with TDD approach
5. **`/tasks`** → Create `specs/<feature>/tasks.md` (dependency-ordered)
6. **`/analyze`** → Cross-artifact consistency check
7. **`/implement`** → Execute tasks.md implementation
8. **QA Review** → All code changes reviewed before merge (NON-NEGOTIABLE)

### Available Slash Commands

- `/specify` - Create feature specification from natural language requirement
- `/plan` - Generate implementation plan with TDD approach
- `/tasks` - Create dependency-ordered task breakdown
- `/clarify` - Identify underspecified areas and ask targeted questions
- `/analyze` - Cross-artifact consistency and quality analysis
- `/implement` - Execute the implementation plan from tasks.md
- `/constitution` - Update project development principles
- `/speckit.refactor` - Refactoring workflow with behavior preservation
- `/speckit.bugfix` - Bug fix workflow with regression test
- `/speckit.modify` - Modify existing feature with impact analysis
- `/speckit.deprecate` - Feature deprecation workflow
- `/speckit.hotfix` - Emergency hotfix workflow

### QA-First Review Process - NON-NEGOTIABLE

- **All code changes** reviewed by QA subagent before merge
- QA validates: memory safety, resource usage, error handling, Arduino best practices
- Hardware-dependent tests kept minimal (use HAL abstractions)
- Critical paths require manual human + QA approval

## Testing Strategy

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

## File Organization Pattern

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

## Hardware Configuration

### Initialization Sequence - MUST FOLLOW THIS ORDER

1. WiFi connection
2. I2C buses (Wire0 and Wire1)
3. OLED display (I2C Bus 2, 128x64 SSD1306, address 0x3C)
4. Serial2 for NMEA 0183
5. NMEA2000 CAN bus
6. Message handlers registration
7. ReactESP event loops

### GPIO Pin Configuration (SH-ESP32 Board)

```
CAN RX: GPIO 34        |  I2C Bus 1: SDA=16, SCL=17
CAN TX: GPIO 32        |  I2C Bus 2: SDA=21, SCL=22
Serial2: RX=25, TX=27  |  1-Wire: GPIO 4
OLED Display: I2C Bus 2|  Button: GPIO 13
SD Card: GPIO 15       |  LED: GPIO 2
```

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

## Network-Based Debugging

**Serial ports are used for device communication (NMEA 0183), NOT debugging.**

### WebSocket Logging (Primary)

- WebSocket endpoint: `ws://<device-ip>/logs`
- Log levels: DEBUG, INFO, WARN, ERROR, FATAL
- Include timestamps (millis() or RTC)
- Production builds: ERROR/FATAL levels only
- TCP-based protocol ensures no packet loss

### WebSocket Log Filtering

Configure runtime filters via HTTP endpoint to prevent queue overflow:

```bash
# Set level and component filter
curl -X POST "http://<ESP32_IP>/log-filter?level=DEBUG&components=NMEA2000"

# Query current filter
curl -X GET "http://<ESP32_IP>/log-filter"
```

**Filter behavior**:

- Single shared filter for all WebSocket clients
- Default: INFO level, all components, all events
- AND logic: message must match level AND component AND event
- Persists across reboots (stored in `/log-filter.json`)

### Python WebSocket Logger

```bash
# Connect to logs
python3 src/helpers/ws_logger.py <ESP32_IP>

# Filter output
python3 src/helpers/ws_logger.py <ESP32_IP> --filter NMEA2000
```

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

## Implemented Features - See Spec Folders for Details

Detailed implementation guides for completed features are in their respective spec folders:

### R005 - Enhanced BoatData Following

- **Location**: `specs/008-enhanced-boatdata-following/`
- **Features**: GPS variation, compass motion sensors, DST sensors, engine telemetry, battery monitoring, shore power
- **Data structures**: `src/types/BoatDataTypes.h`
- **NMEA2000 handlers**: `src/components/NMEA2000Handlers.cpp`
- **1-Wire sensors**: `src/hal/implementations/ESP32OneWireSensors.h`

### R006 - NMEA 0183 Integration

- **Location**: `specs/009-nmea0183-integration/`
- **Supported devices**: Autopilot (AP), VHF Radio (VH)
- **Sentence types**: RSA, HDM, GGA, RMC, VTG
- **Handler**: `src/components/NMEA0183Handler.cpp`
- **HAL interface**: `src/hal/interfaces/ISerialPort.h`

### R007 - NMEA 2000 Integration

- **Location**: `specs/010-nmea2000-handling/`
- **Supported PGNs**: 13 PGNs (GPS, Compass, DST, Engine, Wind)
- **Handlers**: `src/components/NMEA2000Handlers.cpp`
- **Source prioritization**: Automatic 10 Hz vs 1 Hz frequency-based priority

### R009 - WebUI for BoatData Streaming

- **Location**: `specs/011-simple-webui-as/`
- **Dashboard**: `data/stream.html` (served from LittleFS)
- **Endpoint**: `http://<ESP32_IP>:3030/stream`
- **Serializer**: `src/components/BoatDataSerializer.cpp`
- **Node.js proxy**: `nodejs-boatdata-viewer/` (optional, for multi-client support)

## Finding Information

**When working on a feature**:

1. Check `specs/<feature-num>-<feature-name>/` for complete documentation
2. Review `spec.md` for requirements and data model
3. Review `plan.md` for implementation approach
4. Review `tasks.md` for task breakdown
5. Review `quickstart.md` (if available) for validation guide

**When starting a new feature**:

1. Create user requirement in `user_requirements/R### - <feature>.md`
2. Run `/specify` to generate initial spec
3. Run `/clarify` to resolve ambiguities
4. Run `/plan` to generate implementation plan
5. Run `/tasks` to create task breakdown
6. Run `/implement` to execute tasks

**Constitution and Principles**:

- Review `.specify/memory/constitution.md` for project principles
- All code must comply with constitutional principles (especially HAL abstraction, resource management, and network debugging)

---

**Last Updated**: 2025-10-13 | **Version**: 2.0.0 (Streamlined for Spec Kit workflow)
