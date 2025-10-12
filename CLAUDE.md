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

### QA-First Review Process - NON-NEGOTIABLE
- **All code changes** reviewed by QA subagent before merge
- QA validates: memory safety, resource usage, error handling, Arduino best practices
- Hardware-dependent tests kept minimal
- Critical paths require manual human + QA approval

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

## Enhanced BoatData Integration Guide (R005)

### Overview
Enhanced BoatData v2.0.0 extends marine sensor data structures to include:
- **GPS variation** (moved from CompassData to GPSData)
- **Compass motion sensors** (rate of turn, heel, pitch, heave)
- **DST sensor data** (Depth/Speed/Temperature - renamed from SpeedData)
- **Engine telemetry** (RPM, oil temperature, alternator voltage)
- **Saildrive status** (engagement monitoring via 1-wire)
- **Battery monitoring** (dual banks A/B via 1-wire)
- **Shore power** (connection status and power draw via 1-wire)

### Data Structures (src/types/BoatDataTypes.h)

#### GPSData (Enhanced)
```cpp
struct GPSData {
    double latitude;           // Decimal degrees, ±90°
    double longitude;          // Decimal degrees, ±180°
    double cog;                // Course over ground, radians [0, 2π]
    double sog;                // Speed over ground, knots
    double variation;          // Magnetic variation, radians (MOVED from CompassData)
    bool available;
    unsigned long lastUpdate;
};
```

#### CompassData (Enhanced)
```cpp
struct CompassData {
    double trueHeading;        // Radians [0, 2π]
    double magneticHeading;    // Radians [0, 2π]
    double rateOfTurn;         // Radians/second, positive = starboard turn (NEW)
    double heelAngle;          // Radians [-π/2, π/2], positive = starboard (MOVED from SpeedData)
    double pitchAngle;         // Radians [-π/6, π/6], positive = bow up (NEW)
    double heave;              // Meters [-5.0, 5.0], positive = upward (NEW)
    bool available;
    unsigned long lastUpdate;
};
```

#### DSTData (NEW - replaces SpeedData)
```cpp
struct DSTData {
    double depth;              // Depth below waterline, meters [0, 100]
    double measuredBoatSpeed;  // Speed through water, m/s [0, 25]
    double seaTemperature;     // Water temperature, Celsius [-10, 50]
    bool available;
    unsigned long lastUpdate;
};
// Backward compatibility: typedef DSTData SpeedData;
```

#### EngineData (NEW)
```cpp
struct EngineData {
    double engineRev;          // Engine RPM [0, 6000]
    double oilTemperature;     // Oil temp, Celsius [-10, 150]
    double alternatorVoltage;  // Alternator output, volts [0, 30]
    bool available;
    unsigned long lastUpdate;
};
```

#### SaildriveData (NEW)
```cpp
struct SaildriveData {
    bool saildriveEngaged;     // true = deployed/engaged, false = retracted
    bool available;
    unsigned long lastUpdate;
};
```

#### BatteryData (NEW - dual banks)
```cpp
struct BatteryData {
    // Battery A (House Bank)
    double voltageA;           // Volts [0, 30]
    double amperageA;          // Amperes [-200, 200], +charge/-discharge
    double stateOfChargeA;     // Percent [0.0, 100.0]
    bool shoreChargerOnA;
    bool engineChargerOnA;

    // Battery B (Starter Bank)
    double voltageB;
    double amperageB;
    double stateOfChargeB;
    bool shoreChargerOnB;
    bool engineChargerOnB;

    bool available;
    unsigned long lastUpdate;
};
```

#### ShorePowerData (NEW)
```cpp
struct ShorePowerData {
    bool shorePowerOn;         // true = shore power connected
    double power;              // Shore power draw, watts [0, 5000]
    bool available;
    unsigned long lastUpdate;
};
```

### NMEA2000 PGN Handlers (src/components/NMEA2000Handlers.cpp)

#### Implemented Handlers
- **PGN 127251**: Rate of Turn → CompassData.rateOfTurn
- **PGN 127252**: Heave → CompassData.heave (vertical displacement, ±5.0m range, positive = upward)
- **PGN 127257**: Attitude (heel/pitch) → CompassData (Note: heave comes from PGN 127252, not 127257)
- **PGN 129029**: GNSS Position (enhanced) → GPSData.variation
- **PGN 128267**: Water Depth → DSTData.depth
- **PGN 128259**: Speed (Water Referenced) → DSTData.measuredBoatSpeed
- **PGN 130316**: Temperature Extended Range → DSTData.seaTemperature (Kelvin→Celsius)
- **PGN 127488**: Engine Parameters, Rapid → EngineData.engineRev
- **PGN 127489**: Engine Parameters, Dynamic → EngineData oil temp/voltage

#### Registration (when NMEA2000 is initialized)
```cpp
#include "components/NMEA2000Handlers.h"

// In setup() after NMEA2000 initialization:
RegisterN2kHandlers(&NMEA2000, boatData, &logger);
```

### 1-Wire Sensor Setup (src/hal/implementations/ESP32OneWireSensors.h)

#### Initialization
```cpp
void setup() {
    // ... initialization ...

    // Display refresh loops - 1s animation, 5s status
    app.onRepeat(DISPLAY_ANIMATION_INTERVAL_MS, []() {
        if (displayManager != nullptr) {
            displayManager->updateAnimationIcon();
        }
    }
});

// Shore power polling (0.5 Hz)
app.onRepeat(2000, []() {
    if (oneWireSensors != nullptr && boatData != nullptr) {
        ShorePowerData shorePower;
        if (oneWireSensors->readShorePower(shorePower)) {
            boatData->setShorePowerData(shorePower);
            logger.broadcastLog(LogLevel::DEBUG, "OneWire", "SHORE_POWER_UPDATE", ...);
        }
    }
});
```

### Validation Rules (src/utils/DataValidation.h)

All sensor data is validated at the HAL boundary:

#### Angle Validation
- **Pitch**: Clamp to [-π/6, π/6] (±30°), warn if exceeded
- **Heel**: Clamp to [-π/2, π/2] (±90°), warn if exceeds ±45°
- **Rate of turn**: Clamp to [-π, π] rad/s

#### Range Validation
- **Heave**: Clamp to [-5.0, 5.0] meters
- **Depth**: Clamp to [0, 100] meters, reject negative
- **Engine RPM**: Clamp to [0, 6000] RPM
- **Battery voltage**: Clamp to [0, 30]V, warn if outside [10, 15]V (12V system)
- **Battery amperage**: Clamp to [-200, 200]A (signed: +charge, -discharge)
- **Shore power**: Clamp to [0, 5000]W, warn if >3000W

#### Unit Conversions
- **Temperature (PGN 130316)**: Kelvin → Celsius (`DataValidation::kelvinToCelsius()`)
- **All other units**: Direct NMEA2000 native units (no conversion)

### Memory Footprint (v2.0.0)
- **Total BoatDataStructure**: ~560 bytes (~0.17% of ESP32 RAM)
- **Delta from v1.0.0**: +256 bytes (acceptable per Constitution Principle II)
- **1-Wire polling loops**: ~150 bytes stack
- **Total feature impact**: ~710 bytes RAM (~0.22% of ESP32 RAM)

### Testing Strategy

#### Test Organization
```bash
# Contract tests (HAL interface validation)
pio test -e native -f test_boatdata_contracts

# Integration tests (end-to-end scenarios)
pio test -e native -f test_boatdata_integration

# Unit tests (validation/conversion logic)
pio test -e native -f test_boatdata_units

# Hardware tests (ESP32 required)
pio test -e esp32dev_test -f test_boatdata_hardware
```

#### Test Coverage
- **Contract tests**: IOneWireSensors interface, data structure memory layout
- **Integration tests**: GPS variation, compass attitude, DST sensors, engine, battery, saildrive, shore power
- **Unit tests**: Validation/clamping, unit conversions, sign conventions
- **Hardware tests**: 1-wire bus communication, NMEA2000 PGN timing

### Migration Path (SpeedData → DSTData)

#### Phase 1: Introduce DSTData (current release)
```cpp
typedef DSTData SpeedData;  // Backward compatibility typedef
```

#### Phase 2: Update consuming code
```cpp
// Old (deprecated):
double speed = boatData.speed.measuredBoatSpeed;

// New (recommended):
double speed = boatData.dst.measuredBoatSpeed;
```

#### Phase 3: Remove typedef (future release 2.1.0)
- Remove `typedef DSTData SpeedData;` from BoatDataTypes.h
- All code must use `DSTData` directly

### Troubleshooting

#### 1-Wire Sensor Issues
- **No devices found**: Check GPIO 4 wiring, verify DS2438 family code 0x26
- **CRC failures**: Check bus termination, reduce bus length, check 4.7kΩ pull-up resistor
- **Intermittent reads**: Add 50ms delay between reads, check power supply stability

#### NMEA2000 Issues
- **PGN not received**: Verify device is sending PGN, check CAN bus termination (120Ω)
- **Parsing failures**: Enable WebSocket logging (DEBUG level), check NMEA2000 library version
- **Timing issues**: Reduce ReactESP event loop frequency, check for blocking code in handlers

#### Validation Warnings
- **Out-of-range values**: Check sensor calibration, verify NMEA2000 PGN field scaling
- **Availability flags false**: Check sensor wiring, verify device enumeration in logs

### Constitutional Compliance Checklist
- ✅ **Principle I** (HAL Abstraction): IOneWireSensors interface for all 1-wire access
- ✅ **Principle II** (Resource Management): Static allocation only, +256 bytes RAM
- ✅ **Principle V** (Network Debugging): WebSocket logging for all sensor updates
- ✅ **Principle VI** (Always-On): Non-blocking ReactESP event loops
- ✅ **Principle VII** (Fail-Safe): Graceful degradation on sensor failures

### References
- **Specification**: `specs/008-enhanced-boatdata-following/spec.md`
- **Implementation Plan**: `specs/008-enhanced-boatdata-following/plan.md`
- **Task List**: `specs/008-enhanced-boatdata-following/tasks.md`
- **Data Model**: `specs/008-enhanced-boatdata-following/data-model.md`
- **Research**: `specs/008-enhanced-boatdata-following/research.md`
- **Quickstart**: `specs/008-enhanced-boatdata-following/quickstart.md`

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

## NMEA 0183 Integration

### Overview
The NMEA 0183 handler parses sentences from autopilot (AP) and VHF radio (VH) devices, converting data to BoatData format and updating the centralized repository. Supports 5 sentence types: RSA (rudder angle), HDM (magnetic heading), GGA (GPS position), RMC (GPS with COG/SOG/variation), and VTG (true/magnetic COG with calculated variation).

### Supported Devices and Sentences

**Autopilot (AP talker ID)**:
- **APRSA**: Rudder Sensor Angle → Updates `BoatData.RudderData.steeringAngle`
- **APHDM**: Heading Magnetic → Updates `BoatData.CompassData.magneticHeading`

**VHF Radio (VH talker ID)**:
- **VHGGA**: GPS Fix Data → Updates `BoatData.GPSData` (latitude, longitude)
- **VHRMC**: Recommended Minimum Navigation → Updates GPS position, COG, SOG, variation, true heading
- **VHVTG**: Track Made Good → Updates COG, SOG; calculates variation from true/magnetic COG difference

**All other talker IDs and sentence types are silently ignored** (no logging, no error counting).

### Integration Pattern

**Initialization Sequence** (in `main.cpp`):
```cpp
// 1. Initialize Serial2 for NMEA 0183 (4800 baud, 8N1)
Serial2.begin(4800, SERIAL_8N1, 25, 27); // RX=GPIO25, TX=GPIO27 (SH-ESP32)

// 2. Create ESP32 Serial Port adapter (HAL)
ISerialPort* serialPort = new ESP32SerialPort(&Serial2);

// 3. Create NMEA0183 handler with BoatData reference
NMEA0183Handler* nmea0183Handler = new NMEA0183Handler(serialPort, boatData);

// 4. Register source identifiers with BoatData prioritizer
boatData->registerSource("NMEA0183-AP", SensorType::COMPASS, 1.0); // 1 Hz typical
boatData->registerSource("NMEA0183-VH", SensorType::GPS, 1.0);     // 1 Hz typical

// 5. Add ReactESP event loop for sentence processing (10ms polling)
reactESP.onRepeat(10, [nmea0183Handler]() {
    nmea0183Handler->processSentences();
});
```

### Source Prioritization

NMEA 0183 sources integrate with BoatData's multi-source prioritization:
- **Source IDs**: "NMEA0183-AP" (autopilot), "NMEA0183-VH" (VHF radio)
- **Update frequency**: ~1 Hz typical for NMEA 0183 sources
- **Automatic priority**: NMEA 2000 sources (10 Hz) naturally take precedence over NMEA 0183 (1 Hz)
- **Failover**: If NMEA 2000 source becomes stale (>5 seconds), system automatically falls back to NMEA 0183

Example: If both NMEA 2000 GPS (10 Hz) and VHGGA (1 Hz) are available, BoatData uses NMEA 2000 data. If NMEA 2000 GPS fails, system automatically switches to VHGGA data.

### Unit Conversion

All NMEA 0183 data is automatically converted to BoatData target units:

| Data Type | NMEA 0183 Unit | BoatData Unit | Conversion |
|-----------|----------------|---------------|------------|
| Heading angles | Degrees (0-360) | Radians (0-2π) | `angle * DEG_TO_RAD` |
| Wind angles | Degrees (-180-180) | Radians (-π to π) | `angle * DEG_TO_RAD` |
| GPS coordinates | Degrees-minutes (DDMM.MMMM) | Decimal degrees | Extract degrees/minutes, convert |
| Speed (SOG) | Meters/second | Knots | `m_s * 1.94384` |
| Course (COG) | Degrees (0-360) | Radians (0-2π) | `angle * DEG_TO_RAD` |
| Variation | Degrees | Radians | `angle * DEG_TO_RAD` |

### Error Handling

**Graceful Degradation** (FR-024, FR-025, FR-026, FR-032):
- **Invalid checksum**: Sentence silently discarded, no logging
- **Malformed sentence**: Silently discarded, no logging
- **Out-of-range values**: Sentence rejected, BoatData not updated
  - Rudder angle: -90° to +90°
  - Latitude: -90° to +90°
  - Longitude: -180° to +180°
  - Variation: -30° to +30°
  - Speed: 0 to 100 knots
  - Heading: 0° to 360°
- **Buffer overflow**: FIFO drop oldest data, continue processing

**No Error Counters**: Per user requirement, invalid sentences do not increment error counters or generate logs. System continues normal operation without indication of discarded data.

### Performance Constraints

- **Processing budget**: ≤50ms per sentence (FR-027)
- **Baud rate**: 4800 bps (NMEA 0183 standard)
- **Update rate**: ~1 Hz typical (7 Hz max burst in practice)
- **Non-blocking**: ReactESP 10ms polling loop, no blocking operations

### Memory Footprint

**Static Allocation**:
- Sentence buffer: 82 bytes (NMEA max length)
- Handler state: ~50 bytes (message handlers, flags)
- ISerialPort adapter: 8 bytes (pointer wrapper)
- **Total RAM**: ~140 bytes (0.04% of ESP32 320KB RAM)

**Flash Impact**:
- Parser functions: ~8KB (5 handlers + custom RSA)
- Unit conversion utilities: ~2KB
- HAL interfaces: ~1KB
- **Total Flash**: ~15KB production (~0.8% of 1.9MB partition)

**Constitutional Compliance**: ✓ Well under resource limits (Principle II)

### Testing

**Run all NMEA 0183 tests** (contracts, integration, units):
```bash
# All native tests (no hardware required)
pio test -e native -f test_nmea0183

# Specific test groups
pio test -e native -f test_nmea0183_contracts    # HAL interface validation
pio test -e native -f test_nmea0183_integration  # End-to-end scenarios
pio test -e native -f test_nmea0183_units        # Unit conversions, parsers

# Hardware validation (ESP32 required)
pio test -e esp32dev_test -f test_nmea0183_hardware
```

**Hardware Test Requirements**:
- ESP32 with Serial2 connected (GPIO 25/27 on SH-ESP32)
- NMEA 0183 sentence source (GPS simulator, autopilot, or loopback)
- Optional: TX-RX loopback for self-test mode

### WebSocket Log Monitoring

Monitor NMEA 0183 processing events:
```bash
# Activate Python virtual environment
source src/helpers/websocket_env/bin/activate

# Connect to WebSocket logs
python3 src/helpers/ws_logger.py <ESP32_IP>

# Filter for NMEA events
python3 src/helpers/ws_logger.py <ESP32_IP> --filter NMEA
```

**Log Levels**:
- **DEBUG**: Valid sentence processed (talker ID, sentence type, data extracted)
- **WARN**: Out-of-range value rejected (sentence details, reason)
- **ERROR**: Unexpected handler failure (should not occur in normal operation)

**Note**: Invalid checksums and malformed sentences are silently discarded per FR-024/FR-025 (no logs generated).

### Troubleshooting

**No data from NMEA 0183 devices**:
1. Verify Serial2 initialization: 4800 baud, GPIO 25 (RX), GPIO 27 (TX)
2. Check physical wiring: NMEA TX → ESP32 GPIO25, common ground
3. Monitor WebSocket logs for sentence processing (DEBUG level)
4. Test with loopback: Connect GPIO 25 ↔ GPIO 27, send test sentence

**Data not updating BoatData**:
1. Verify talker ID: Only AP and VH are processed
2. Check sentence type: Only RSA, HDM, GGA, RMC, VTG supported
3. Verify data ranges: Out-of-range values silently rejected
4. Check source registration: "NMEA0183-AP" and "NMEA0183-VH" must be registered

**NMEA 0183 data ignored (NMEA 2000 preferred)**:
- **Expected behavior**: NMEA 2000 sources (10 Hz) automatically prioritized over NMEA 0183 (1 Hz)
- Verify with BoatData diagnostics: Check `activeSource` for each sensor type
- NMEA 0183 will be used only if NMEA 2000 source is unavailable or stale (>5 seconds)

**Processing time exceeds 50ms budget**:
1. Monitor WebSocket logs for "PROCESSING_TIME" warnings
2. Check for sentence bursts (>7 sentences/second)
3. Verify no blocking operations in handler code
4. Review ReactESP loop timing (10ms interval)

### Custom RSA Parser

The RSA (Rudder Sensor Angle) sentence is proprietary and not included in the standard NMEA0183 library. Custom parser implemented following library patterns:

**RSA Sentence Format**: `$APRSA,<starboard_angle>,A,<port_angle>,V*<checksum>`
- Field 1: Starboard rudder angle (degrees, positive = starboard)
- Field 2: Starboard status (A=valid, V=invalid)
- Field 3: Port rudder angle (degrees, positive = starboard)
- Field 4: Port status (A=valid, V=invalid)

**Implementation**: See `src/utils/NMEA0183Parsers.cpp` for custom `NMEA0183ParseRSA()` function.

### HAL Abstraction

**ISerialPort Interface** (`src/hal/interfaces/ISerialPort.h`):
- Abstracts Serial2 hardware for testability
- Methods: `int available()`, `int read()`, `void begin(unsigned long baud)`
- Mock implementation: `MockSerialPort` for native tests
- ESP32 implementation: `ESP32SerialPort` wraps `HardwareSerial`

**Benefits**:
- All NMEA parsing logic testable on native platform (no ESP32 required)
- Hardware tests minimal (only Serial2 timing validation)
- Follows constitutional HAL principle (Principle I)

---
**NMEA 0183 Handler Version**: 1.0.0 | **Last Updated**: 2025-10-11
