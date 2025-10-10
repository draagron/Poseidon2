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
#include "hal/implementations/ESP32OneWireSensors.h"

// In setup() after I2C initialization:
ESP32OneWireSensors* oneWireSensors = new ESP32OneWireSensors(4);  // GPIO 4

if (oneWireSensors->initialize()) {
    Serial.println("1-Wire bus initialized successfully");
    logger.broadcastLog(LogLevel::INFO, "Main", "ONEWIRE_INIT_SUCCESS",
                        F("{\"bus\":\"GPIO4\"}"));
} else {
    Serial.println("WARNING: 1-Wire initialization failed");
    // Graceful degradation - continue without 1-wire sensors
}
```

#### ReactESP Event Loops (polling rates)
```cpp
// Saildrive polling (1 Hz)
app.onRepeat(1000, []() {
    if (oneWireSensors != nullptr && boatData != nullptr) {
        SaildriveData saildriveData;
        if (oneWireSensors->readSaildriveStatus(saildriveData)) {
            boatData->setSaildriveData(saildriveData);
            logger.broadcastLog(LogLevel::DEBUG, "OneWire", "SAILDRIVE_UPDATE",
                String(F("{\"engaged\":")) + (saildriveData.saildriveEngaged ? F("true") : F("false")) + F("}"));
        } else {
            logger.broadcastLog(LogLevel::WARN, "OneWire", "SAILDRIVE_READ_FAILED",
                F("{\"reason\":\"Sensor read error or CRC failure\"}"));
        }
    }
});

// Battery polling (0.5 Hz)
app.onRepeat(2000, []() {
    if (oneWireSensors != nullptr && boatData != nullptr) {
        BatteryMonitorData batteryA, batteryB;
        bool successA = oneWireSensors->readBatteryA(batteryA);
        bool successB = oneWireSensors->readBatteryB(batteryB);

        if (successA && successB) {
            BatteryData batteryData;
            // ... populate batteryData from batteryA and batteryB
            boatData->setBatteryData(batteryData);
            logger.broadcastLog(LogLevel::DEBUG, "OneWire", "BATTERY_UPDATE", ...);
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
