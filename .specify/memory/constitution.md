<!--
SYNC IMPACT REPORT - Constitution Amendment
Version Change: 1.1.0 → 1.2.0 (MINOR)
Date: 2025-10-07

RATIONALE FOR MINOR VERSION BUMP:
- Updated Principle V: Network-Based Debugging to prescribe WebSocket logging instead of UDP
- Material change to prescribed logging mechanism reflects current implementation
- Aligns constitutional requirements with production codebase

MODIFIED PRINCIPLES:
- Principle V: Network-Based Debugging (lines 92-101)
  - Changed from UDP broadcast logging to WebSocket logging
  - Updated endpoint specification and reliability guarantees
  - Removed UDP-specific implementation details

ADDED SECTIONS:
- None

REMOVED SECTIONS:
- Principle V: UDP-specific implementation details removed

CONTENT CHANGES:
- "UDP broadcast logging" → "WebSocket logging"
- Added: "WebSocket endpoint: ws://<device-ip>/logs"
- Added: "TCP-based protocol ensures reliable delivery (no packet loss)"
- Updated fallback: "flash if UDP unavailable" → "flash if WebSocket unavailable"
- Removed: "Lightweight UDP logger to minimize network overhead"

TEMPLATES REQUIRING UPDATES:
✅ plan-template.md - Already aligned (references "Network Debugging" generically)
✅ spec-template.md - No constitution-specific dependencies
✅ tasks-template.md - No constitution-specific dependencies
✅ .claude/commands/*.md - No updates required

ALIGNMENT VALIDATION:
✅ Principle V updated to reflect WebSocket implementation
✅ All 7 core principles still have corresponding checklist items in plan-template.md
✅ Technology stack requirements remain consistent (ESPAsyncWebServer already approved)
✅ Documentation now aligns with src/utils/WebSocketLogger implementation

FOLLOW-UP TODOs:
- None - implementation already migrated to WebSocket (src/utils/WebSocketLogger.{h,cpp})

NOTES:
- Constitution v1.2.0 prescribes current implementation (not obsolete UDP approach)
- WebSocket provides superior reliability: TCP vs UDP, ordered delivery, connection state
- Previous amendment (v1.1.0) added reference documentation; this amendment updates core principle

PREVIOUS AMENDMENT (v1.0.0 → v1.1.0, 2025-10-06):
- Added "Reference Documentation and Examples" under Technology Stack
- Material expansion with library documentation links and reference implementation
-->

# Poseidon2 ESP32 Development Constitution
The Poseidon Gateway is an ESP32-based interface gateway that enables bidirectional communication between a SignalK server and boat instruments using multiple marine data protocols:
- NMEA 2000 (bidirectional)
- NMEA 0183 (readonly)
- 1-wire sensor devices

This system essentially enables integration of various marine instruments using different protocols by acting as a central hub that can translate and forward data between them, with a focus on bridging older NMEA 0183 equipment with modern NMEA 2000 networks and SignalK systems. 

The project is built using PlatformIO with the Arduino framework and runs on ESP32 hardware, specifically designed for marine applications. It serves as a bridge to integrate various marine instruments and sensors with modern SignalK-based systems.

## Core Principles

### I. Hardware Abstraction Layer (HAL)
Every hardware interaction must be abstracted through interfaces or wrapper classes:
- Hardware dependencies isolated to specific HAL modules
- Mock implementations required for all HAL interfaces
- Enable compilation and basic logic testing without physical hardware
- Clear separation between business logic and hardware I/O

### II. Resource-Aware Development
Memory and processing constraints are non-negotiable:
- Static memory allocation preferred over dynamic (minimize heap fragmentation)
- Stack usage monitored (ESP32 default: 8KB per task)
- Flash usage tracked (compile-time warnings at >80% capacity)
- Global variables minimized and justified
- String literals use `F()` macro or PROGMEM where applicable

### III. QA-First Review Process (NON-NEGOTIABLE)
Code quality enforced through AI-assisted peer review:
- All code changes reviewed by QA subagent before merge
- QA subagent validates: memory safety, resource usage, error handling, Arduino best practices
- Hardware-dependent tests kept minimal - logic validated through code review
- Critical paths require manual human review + QA subagent approval

### IV. Modular Component Design
Functionality organized into focused, testable components:
- Each feature implemented as a modular component or class
- Components must have single, well-defined responsibility
- Dependency injection used for hardware dependencies
- Public interfaces documented with usage examples

### V. Network-Based Debugging
Observability through WebSocket logging (serial ports reserved for device communication):
- WebSocket logging mandatory for all major operations
- WebSocket endpoint: ws://<device-ip>/logs
- TCP-based protocol ensures reliable delivery (no packet loss)
- Log levels: DEBUG, INFO, WARN, ERROR, FATAL
- Timestamps included in production builds (millis() or RTC)
- JSON output option for machine-readable diagnostics
- Debug builds broadcast verbose output; production builds only ERROR/FATAL
- Fallback: store critical errors to flash if WebSocket unavailable

### VI. Always-On Operation
Permanently powered system requirements:
- WiFi must remain always-on for continuous connectivity
- Deep sleep/light sleep modes NOT permitted
- Peripheral power management NOT required
- System designed for 24/7 operation with continuous network availability

### VII. Fail-Safe Operation
Robust error handling and recovery:
- Watchdog timer enabled in production builds
- Safe mode/recovery mode available on boot
- Critical errors logged to persistent storage (SPIFFS/LittleFS)
- Graceful degradation preferred over hard crashes

## Technology Stack

### Platform Requirements
- **MCU**: ESP32 family (ESP32, ESP32-S2, ESP32-C3, ESP32-S3)
- **Framework**: Arduino framework
- **Build System**: PlatformIO
- **Language**: C++ (C++11 minimum, C++14 preferred)

### GPIO Pin Configuration

The system uses specific GPIO pins on the SH-ESP32 board:
- CAN RX: GPIO 34
- CAN TX: GPIO 32
- I2C Bus 1: SDA=16, SCL=17
- I2C Bus 2: SDA=21, SCL=22
- Serial2: RX=25, TX=27
- 1-Wire: GPIO 4
- OLED Display: Connected to I2C Bus 2
- Button: GPIO 13
- SD Card: GPIO 15
- Built-in LED: GPIO 2

**Hardware Initialization Sequence**:
1. WiFi connection
2. I2C buses (Wire0 and Wire1)
3. OLED display
4. Serial2 for NMEA 0183
5. NMEA2000 CAN bus
6. Message handlers registration
7. ReactESP event loops

### Mandatory Libraries
- **Arduino Core for ESP32**: Latest stable version
- Standard Arduino libraries for cross-platform compatibility where possible

### Approved Third-Party Libraries

**Core Libraries** (pre-approved):
- **NMEA0183**: https://github.com/ttlappalainen/NMEA0183 - NMEA0183 communication
- **NMEA2000**: https://github.com/ttlappalainen/NMEA2000 - NMEA2000 communication
- **NMEA2000_esp32**: https://github.com/ttlappalainen/NMEA2000_esp32 - ESP32-specific NMEA2000 implementation
- **ReactESP**: https://github.com/mairas/ReactESP - Asynchronous programming and event loops
- **ESPAsyncWebServer**: https://github.com/ESP32Async/ESPAsyncWebServer - Asynchronous HTTP and WebSocket server
- **Adafruit_SSD1306**: https://github.com/adafruit/Adafruit_SSD1306 - For Monochrome OLED display based on SSD1306 driver

**Additional Library Requirements**:
- Must be actively maintained (updated within last 12 months)
- Compatible with PlatformIO and Arduino framework
- License compatible with project requirements
- Memory footprint documented and acceptable

### Reference Documentation and Examples

**Official Library Documentation**:
- **NMEA2000 Library Reference**: https://ttlappalainen.github.io/NMEA2000/pg_lib_ref.html - Comprehensive API documentation for NMEA2000 library
- **NMEA2000 Repository**: https://github.com/ttlappalainen/NMEA2000 - Source code and examples for reference and inspiration
- **NMEA0183 Repository**: https://github.com/ttlappalainen/NMEA0183 - Source code and examples for reference and inspiration

**Project Reference Implementation**:
- **Example Gateway**: `examples/poseidongw/` - Fully functioning early version of the Poseidon gateway
- **Reference Source**: `examples/poseidongw/src/` - Source files to be used as reference and inspiration for new development
- These files demonstrate working implementations of the core principles and serve as architectural guidance

## Development Workflow

### Code Review Gates
1. **QA Subagent Review** (automated):
   - Memory safety analysis
   - Resource usage validation
   - Code style compliance
   - Error handling completeness
   - Documentation completeness

2. **Human Review** (manual, for critical features):
   - Architecture alignment
   - Security implications
   - Performance characteristics
   - User experience considerations

### Testing Strategy
**Mock-First Testing**:
- Business logic tested with mocked hardware interfaces
- Unit tests run on development machine (native environment)
- PlatformIO native environment used for non-hardware-dependent tests

**Minimal Hardware Testing**:
- Hardware tests limited to: HAL validation, sensor calibration, communication protocols, timing-critical operations
- Hardware tests documented with setup requirements
- Regression testing via QA subagent code review preferred over re-running hardware tests

**Integration Testing**:
- Multi-component interactions tested with mocked hardware
- Communication protocol contracts validated
- State machine transitions verified

### Version Control
- **Firmware Versioning**: MAJOR.MINOR.PATCH semantic versioning
- **Git Workflow**: Feature branches merged to `main` after QA approval
- **Commit Messages**: Conventional commits format (feat:, fix:, refactor:, docs:)
- **Tags**: Each release tagged with firmware version

### Build Configurations
- **Development**: Serial debugging, verbose logging, assertions enabled
- **Staging**: Production optimizations, moderate logging, OTA update testing
- **Production**: Full optimizations, error-level logging only, watchdog enabled, OTA updates enabled

## Code Standards

### File Organization
```
src/
├── main.cpp              # Entry point
├── config.h              # Compile-time configuration
├── hal/                  # Hardware Abstraction Layer
│   ├── interfaces/       # HAL interfaces
│   └── implementations/  # Hardware-specific implementations
├── components/           # Feature components
├── utils/                # Utility functions
└── mocks/                # Mock implementations for testing
```

### Memory Management
- Heap allocations minimized and tracked
- String operations use `String` class sparingly (prefer char arrays for embedded)
- Buffer sizes validated against available memory
- RTOS task stack sizes explicitly specified

### Error Handling
- Return codes or result types for all operations that can fail
- No silent failures - all errors logged
- Critical errors trigger safe mode or controlled restart
- User-facing error messages actionable

### Documentation Requirements
- All public functions/classes documented with doxygen-style comments
- HAL interfaces include usage examples
- Complex algorithms explained with inline comments
- README.md per component describing purpose and dependencies

## Governance

This constitution supersedes all other development practices. All code changes must demonstrate compliance with these principles.

**Amendment Process**:
- Proposed changes documented with rationale
- Team review and approval required
- Migration plan for existing code if applicable
- Constitution version incremented

**Enforcement**:
- QA subagent configured to validate constitutional compliance
- Pull requests must pass QA review before merge
- Exceptions require documented justification and approval
- Regular audits to ensure ongoing compliance

**Version**: 1.2.0 | **Ratified**: 2025-10-06 | **Last Amended**: 2025-10-07