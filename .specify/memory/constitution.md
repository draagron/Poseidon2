# [PROJECT_NAME] Constitution
<!-- Example: Spec Constitution, TaskFlow Constitution, etc. -->

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

### VIII. Workflow Selection
Development activities SHALL use the appropriate workflow type based on the nature of the work. Each workflow enforces specific quality gates and documentation requirements tailored to its purpose:

- **Feature Development** (`/specify`): New functionality - requires full specification, planning, and TDD approach
- **Bug Fixes** (`/bugfix`): Defect remediation - requires regression test BEFORE applying fix
- **Modifications** (`/modify`): Changes to existing features - requires impact analysis and backward compatibility assessment
- **Refactoring** (`/refactor`): Code quality improvements - requires baseline metrics, behavior preservation guarantee, and incremental validation
- **Hotfixes** (`/hotfix`): Emergency production issues - expedited process with deferred testing and mandatory post-mortem
- **Deprecation** (`/deprecate`): Feature sunset - requires phased rollout (warnings → disabled → removed), migration guide, and stakeholder approvals

The wrong workflow SHALL NOT be used - features must not bypass specification, bugs must not skip regression tests, and refactorings must not alter behavior.

### No Backward Compatibility Required
When implementing new features, there should not be any bloating of specs or implementation for the sake of backward compatibility. The codebase should be kept as lean and clear as possible to fit the latest requirements only. 

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

### Core Workflow (Feature Development)
1. Feature request initiates with `/specify <description>`
2. Clarification via `/clarify` to resolve ambiguities
3. Technical planning with `/plan` to create implementation design
4. Task breakdown using `/tasks` for execution roadmap
5. Implementation via `/implement` following task order

### Extension Workflows
- **Bugfix**: `/bugfix "<description>"` → bug-report.md + tasks.md with regression test requirement
- **Modification**: `/modify <feature_num> "<description>"` → modification.md + impact analysis + tasks.md
- **Refactor**: `/refactor "<description>"` → refactor.md + baseline metrics + incremental tasks.md
- **Hotfix**: `/hotfix "<incident>"` → hotfix.md + expedited tasks.md + post-mortem.md (within 48 hours)
- **Deprecation**: `/deprecate <feature_num> "<reason>"` → deprecation.md + dependency scan + phased tasks.md


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


3. **Bugfix**:
- Bug reproduction MUST be documented with exact steps
- Regression test MUST be written before fix is applied
- Root cause MUST be identified and documented
- Prevention strategy MUST be defined

4. **Modification**:
- Impact analysis MUST identify all affected files and contracts
- Original feature spec MUST be linked
- Backward compatibility MUST be assessed
- Migration path MUST be documented if breaking changes

5. **Refactor**:
- Baseline metrics MUST be captured before any changes
- Tests MUST pass after EVERY incremental change
- Behavior preservation MUST be guaranteed (tests unchanged)
- Target metrics MUST show measurable improvement

6. **Hotfix**:
- Severity MUST be assessed (P0/P1/P2)
- Rollback plan MUST be prepared before deployment
- Fix MUST be deployed and verified before writing tests (exception to TDD)
- Post-mortem MUST be completed within 48 hours of resolution



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
<!-- Example: Constitution supersedes all other practices; Amendments require documentation, approval, migration plan -->

[GOVERNANCE_RULES]
<!-- Example: All PRs/reviews must verify compliance; Complexity must be justified; Use [GUIDANCE_FILE] for runtime development guidance -->

**Version**: [CONSTITUTION_VERSION] | **Ratified**: [RATIFICATION_DATE] | **Last Amended**: [LAST_AMENDED_DATE]
<!-- Example: Version: 2.1.1 | Ratified: 2025-06-13 | Last Amended: 2025-07-16 -->