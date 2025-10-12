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

### WebSocket Log Filtering

To prevent queue overflow and reduce message volume, configure runtime log filters via HTTP endpoint:

**Configure filter (applies to all WebSocket clients)**:
```bash
# Set DEBUG level for NMEA2000 component only
curl -X POST "http://<ESP32_IP>/log-filter?level=DEBUG&components=NMEA2000"

# Filter by event prefix (all PGN130306 events)
curl -X POST "http://<ESP32_IP>/log-filter?level=DEBUG&events=PGN130306_"

# Multiple components (comma-separated)
curl -X POST "http://<ESP32_IP>/log-filter?level=INFO&components=NMEA2000,GPS,OneWire"

# Multiple event prefixes
curl -X POST "http://<ESP32_IP>/log-filter?level=WARN&events=ERROR,FAILED,OUT_OF_RANGE"

# Combine filters (AND logic)
curl -X POST "http://<ESP32_IP>/log-filter?level=DEBUG&components=NMEA2000&events=PGN130306_"

# Reset to defaults (INFO level, all components/events)
curl -X POST "http://<ESP32_IP>/log-filter?level=INFO&components=&events="
```

**Query current filter**:
```bash
# GET request returns current filter configuration
curl -X GET "http://<ESP32_IP>/log-filter"
# Response: {"level":"DEBUG","components":"NMEA2000","events":"PGN130306_"}
```

**Filter behavior**:
- **Single shared filter**: Applies to all connected WebSocket clients
- **Default filter**: INFO level, all components, all events
- **Empty parameter**: Matches all (e.g., `components=` matches all components)
- **Level filtering**: Messages below minimum level are dropped
- **Component filtering**: Exact substring match in comma-separated list
- **Event filtering**: Prefix match (e.g., `PGN130306_` matches `PGN130306_UPDATE`, `PGN130306_OUT_OF_RANGE`)
- **AND logic**: Message must match level AND component AND event filters
- **Early exit**: Filtered messages never built or queued (reduces CPU/memory usage)
- **Automatic persistence**: Filter settings automatically saved to `/log-filter.json` on every change
- **Persists across reboots**: Filter configuration loaded from LittleFS on startup

**Common filter examples**:
```bash
# Monitor high-frequency NMEA2000 updates only
curl -X POST "http://192.168.1.100/log-filter?level=DEBUG&components=NMEA2000"

# Show only errors and warnings from all components
curl -X POST "http://192.168.1.100/log-filter?level=WARN"

# Debug specific PGN (Wind Data)
curl -X POST "http://192.168.1.100/log-filter?level=DEBUG&events=PGN130306_"

# Monitor WiFi and WebServer events
curl -X POST "http://192.168.1.100/log-filter?level=INFO&components=WiFi,WebServer"

# Production mode (errors only)
curl -X POST "http://192.168.1.100/log-filter?level=ERROR"
```

**Memory footprint**:
- **RAM**: 256 bytes (single LogFilter struct, static allocation)
- **Flash**: ~2KB code, `/log-filter.json` file (~100 bytes)

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

## NMEA 2000 Integration

### Overview
The NMEA 2000 handler processes 13 Parameter Group Numbers (PGNs) from the CAN bus, converting data to BoatData format with automatic multi-source prioritization. Supports GPS navigation, compass/attitude data, DST (Depth/Speed/Temperature) sensors, engine monitoring, and wind data.

### Supported PGNs

**GPS Data (4 PGNs)**:
- **PGN 129025**: Position Rapid Update (10 Hz) → GPSData lat/lon
- **PGN 129026**: COG & SOG Rapid Update (10 Hz) → GPSData cog/sog
- **PGN 129029**: GNSS Position Data (1 Hz) → GPSData (with variation, altitude, quality)
- **PGN 127258**: Magnetic Variation (1 Hz) → GPSData.variation

**Compass/Attitude Data (4 PGNs)**:
- **PGN 127250**: Vessel Heading (10 Hz) → CompassData true/magnetic heading
- **PGN 127251**: Rate of Turn (10 Hz) → CompassData.rateOfTurn
- **PGN 127252**: Heave (10 Hz) → CompassData.heave
- **PGN 127257**: Attitude (10 Hz) → CompassData heel/pitch

**DST Data (3 PGNs)**:
- **PGN 128267**: Water Depth (10 Hz) → DSTData.depth
- **PGN 128259**: Boat Speed (10 Hz) → DSTData.measuredBoatSpeed
- **PGN 130316**: Temperature Extended Range (10 Hz) → DSTData.seaTemperature

**Engine Data (2 PGNs)**:
- **PGN 127488**: Engine Parameters Rapid (10 Hz) → EngineData.engineRev
- **PGN 127489**: Engine Parameters Dynamic (1 Hz) → EngineData oil temp/voltage

**Wind Data (1 PGN)**:
- **PGN 130306**: Wind Data (10 Hz) → WindData apparent angle/speed

### Integration Pattern

**Initialization Sequence** (in `main.cpp`):
```cpp
// 1. Add includes
#include <NMEA2000.h>
#include <NMEA2000_esp32.h>
#include <N2kMessages.h>
#include "components/NMEA2000Handlers.h"

// 2. Create NMEA2000 global variable (after BoatData, before setup())
tNMEA2000_esp32 NMEA2000(CAN_TX_PIN, CAN_RX_PIN);

// 3. In setup(), after Serial2 initialization (step 5):
void setup() {
    // ... earlier initialization ...

    // STEP 5: NMEA2000 CAN bus initialization
    NMEA2000.SetProductInformation(
        "00000001",                // Manufacturer's Model serial code
        100,                       // Manufacturer's product code
        "Poseidon2 Gateway",       // Manufacturer's Model ID
        "1.0.0",                   // Manufacturer's Software version code
        "1.0.0"                    // Manufacturer's Model version
    );

    NMEA2000.SetDeviceInformation(
        1,                         // Unique number (1-254)
        130,                       // Device function: PC Gateway
        25,                        // Device class: Network Device
        2046                       // Manufacturer code (use 2046 for experimenting)
    );

    NMEA2000.SetMode(tNMEA2000::N2km_ListenAndNode);
    NMEA2000.EnableForward(false);  // No forwarding to USB/Serial

    if (!NMEA2000.Open()) {
        logger.broadcastLog(LogLevel::ERROR, "NMEA2000", "CAN_INIT_FAILED", F("{}"));
    } else {
        logger.broadcastLog(LogLevel::INFO, "NMEA2000", "CAN_INIT_SUCCESS", F("{}"));
    }

    // STEP 6: Register message handlers
    RegisterN2kHandlers(&NMEA2000, boatData, &logger);

    // Register sources with BoatData prioritizer
    boatData->registerSource("NMEA2000-GPS", SensorType::GPS, 10.0);
    boatData->registerSource("NMEA2000-COMPASS", SensorType::COMPASS, 10.0);
    boatData->registerSource("NMEA2000-DST", SensorType::DST, 10.0);
    boatData->registerSource("NMEA2000-ENGINE", SensorType::ENGINE, 10.0);
    boatData->registerSource("NMEA2000-WIND", SensorType::WIND, 10.0);

    // ... continue with ReactESP event loops ...
}

// 4. Add NMEA2000.ParseMessages() to ReactESP event loop (10ms interval)
reactESP.onRepeat(10, []() {
    NMEA2000.ParseMessages();
});
```

### Source Prioritization

NMEA 2000 sources integrate with BoatData's multi-source prioritization:
- **Source IDs**: "NMEA2000-GPS", "NMEA2000-COMPASS", "NMEA2000-DST", "NMEA2000-ENGINE", "NMEA2000-WIND"
- **Update frequency**: ~10 Hz typical for most NMEA 2000 sources (1 Hz for some)
- **Automatic priority**: NMEA 2000 sources (10 Hz) naturally take precedence over NMEA 0183 (1 Hz)
- **Failover**: If NMEA 2000 source becomes stale (>5 seconds), system automatically falls back to NMEA 0183

**Example**: If both NMEA 2000 GPS (10 Hz) and NMEA 0183 VHF GPS (1 Hz) are available, BoatData uses NMEA 2000 data. If NMEA 2000 GPS fails, system automatically switches to NMEA 0183 GPS data.

### Unit Conversion

All NMEA 2000 data is automatically converted to BoatData target units:

| Data Type | NMEA 2000 Unit | BoatData Unit | Conversion |
|-----------|----------------|---------------|------------|
| Heading angles | Radians (0-2π) | Radians (0-2π) | No conversion |
| Wind angles | Radians (-π to π) | Radians (-π to π) | No conversion |
| GPS coordinates | Decimal degrees | Decimal degrees | No conversion |
| Speed (SOG/Wind) | Meters/second | Knots | `m_s * 1.94384` |
| Temperature | Kelvin | Celsius | `kelvin - 273.15` |
| Angles (pitch/heel) | Radians | Radians | No conversion |

### Error Handling

**Graceful Degradation**:
- **Parse failure**: ERROR log, BoatData availability=false, existing data preserved
- **Unavailable data (N2kDoubleNA)**: DEBUG log, no update, existing data preserved
- **Out-of-range values**: WARN log with original and clamped values, BoatData updated with clamped value
- **CAN bus failure**: System continues operation, failover to NMEA 0183 sources

**No handler crashes**: All error conditions handled gracefully with WebSocket logging.

### Performance Constraints

- **Handler execution**: <1ms per message (typical 0.5ms)
- **Message rate**: 100-1000 messages/second sustained
- **Update rate**: 10 Hz typical (1 Hz for some PGNs)
- **Non-blocking**: ReactESP event loop with 10ms polling interval
- **Latency**: <2ms from CAN interrupt to BoatData update

### Memory Footprint

**Static Allocation**:
- N2kBoatDataHandler class: 16 bytes (2 pointers + vtable)
- tNMEA2000_esp32 instance: ~200 bytes
- CAN message buffers: ~2KB (NMEA2000 library internal)
- Handler functions: ~10KB flash (5 new handlers)
- **Total RAM**: ~2.2KB (0.7% of ESP32 320KB RAM)
- **Total Flash**: ~12KB new code (0.6% of 1.9MB partition)

**Constitutional Compliance**: ✓ Static allocation only, well under resource limits (Principle II)

### Testing

**Run all NMEA 2000 tests**:
```bash
# All native tests (no hardware required)
pio test -e native -f test_nmea2000

# Specific test groups
pio test -e native -f test_nmea2000_contracts    # Handler function compliance
pio test -e native -f test_nmea2000_integration  # End-to-end scenarios
pio test -e native -f test_nmea2000_units        # Validation/conversion logic

# Hardware validation (ESP32 + CAN bus required)
pio test -e esp32dev_test -f test_nmea2000_hardware
```

**Hardware Test Requirements**:
- ESP32 with CAN transceivers on GPIO 34 (RX) and GPIO 32 (TX)
- NMEA 2000 bus with 12V power and 120Ω terminators
- At least one NMEA 2000 device broadcasting known PGNs
- Optional: NMEA 2000 simulator for testing without real devices

### WebSocket Log Monitoring

Monitor NMEA 2000 processing events:
```bash
# Connect to WebSocket logs
python3 src/helpers/ws_logger.py <ESP32_IP> --filter NMEA2000
```

**Log Levels**:
- **DEBUG**: Valid PGN processed (PGN number, parsed fields, updated values)
- **INFO**: Handler registration success (13 PGNs registered)
- **WARN**: Out-of-range value clamped (original value, clamped value, reason)
- **ERROR**: Parse failure or CAN bus error (PGN number, failure reason)

**Example Log Output**:
```json
{"level":"INFO","component":"NMEA2000","event":"HANDLERS_REGISTERED","data":{"count":13,"pgns":[129025,129026,129029,127258,127250,127251,127252,127257,128267,128259,130316,127488,127489,130306]}}
{"level":"DEBUG","component":"NMEA2000","event":"PGN129025_UPDATE","data":{"latitude":37.7749,"longitude":-122.4194}}
{"level":"DEBUG","component":"NMEA2000","event":"PGN130306_UPDATE","data":{"wind_angle":0.785,"wind_speed":12.5}}
{"level":"WARN","component":"NMEA2000","event":"PGN127489_OIL_TEMP_HIGH","data":{"oil_temp_c":125,"threshold":120}}
```

### Troubleshooting

**No data from NMEA 2000 devices**:
1. Verify CAN bus initialization: Check WebSocket logs for "CAN_INIT_SUCCESS"
2. Check physical wiring: CAN RX=GPIO 34, CAN TX=GPIO 32, 120Ω terminators
3. Verify bus power: NMEA 2000 bus requires 12V power
4. Check device addressing: Monitor WebSocket logs for address claiming messages
5. Verify PGN registration: Check for "HANDLERS_REGISTERED" INFO message

**Data not updating BoatData**:
1. Verify handler registration: Check for "HANDLERS_REGISTERED" in WebSocket logs
2. Check PGN support: Only 13 PGNs listed above are processed
3. Verify data ranges: Out-of-range values are clamped but still updated
4. Check source registration: Verify "NMEA2000-*" sources registered with BoatData

**NMEA 2000 data ignored (lower priority)**:
- **Unexpected behavior**: NMEA 2000 should have highest priority (10 Hz)
- Check update frequency: Verify NMEA 2000 messages arriving at 10 Hz
- Monitor active source: Use BoatData diagnostics to check `activeSource` for each sensor type
- If NMEA 0183 has higher measured frequency, NMEA 2000 may be filtered or failing

**CAN bus errors**:
1. Check termination resistors: Both ends of backbone need 120Ω terminators
2. Verify cable quality: Use proper NMEA 2000 certified cables (DeviceNet compatible)
3. Check cable length: Maximum backbone length 200m, maximum drop length 6m
4. Monitor error counters: WebSocket logs show CAN bus error counts

### Handler Implementation Pattern

Each handler follows this 8-step pattern:

```cpp
void HandleN2kPGN<NUMBER>(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger) {
    if (boatData == nullptr || logger == nullptr) return;

    // 1. Parse PGN using NMEA2000 library function
    double field1, field2;
    if (ParseN2kPGN<NUMBER>(N2kMsg, field1, field2)) {

        // 2. Check for N2kDoubleNA unavailable values
        if (N2kIsNA(field1)) {
            logger->broadcastLog(LogLevel::DEBUG, "NMEA2000", "PGN<NUMBER>_NA", F("{}"));
            return;
        }

        // 3. Validate data using DataValidation helpers
        bool valid = DataValidation::isValid<Type>(field1);
        if (!valid) {
            logger->broadcastLog(LogLevel::WARN, "NMEA2000", "PGN<NUMBER>_OUT_OF_RANGE", ...);
            field1 = DataValidation::clamp<Type>(field1);
        }

        // 4. Get current data structure from BoatData
        DataType data = boatData->get<DataType>();

        // 5. Update fields
        data.field = field1;
        data.available = true;
        data.lastUpdate = millis();

        // 6. Store updated data
        boatData->set<DataType>(data);

        // 7. Log update (DEBUG level)
        logger->broadcastLog(LogLevel::DEBUG, "NMEA2000", "PGN<NUMBER>_UPDATE", ...);

        // 8. Increment message counter
        boatData->incrementNMEA2000Count();

    } else {
        // Parse failed - log ERROR and set availability to false
        logger->broadcastLog(LogLevel::ERROR, "NMEA2000", "PGN<NUMBER>_PARSE_FAILED", F("{}"));
        DataType data = boatData->get<DataType>();
        data.available = false;
        boatData->set<DataType>(data);
    }
}
```

**Key Points**:
- All handlers share same pattern for consistency
- Handlers are non-blocking (<1ms execution time)
- Null checks prevent crashes if BoatData or logger not initialized
- Graceful degradation on all error conditions
- WebSocket logging for all state changes

### HAL Abstraction

**NMEA2000 Library Provides HAL**:
- `tNMEA2000_esp32` abstracts CAN bus hardware for ESP32
- `tN2kMsg` provides hardware-independent message structure
- Handler functions operate on abstract message types (no direct hardware access)
- Mock implementations possible via custom `tNMEA2000` subclass for testing

**Benefits**:
- All handler logic testable on native platform (no ESP32 required)
- Hardware tests minimal (only CAN bus timing and addressing)
- Follows constitutional HAL principle (Principle I)

---
**NMEA 2000 Handler Version**: 1.0.0 | **Last Updated**: 2025-10-12
