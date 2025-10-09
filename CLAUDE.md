# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**Poseidon2** is an ESP32-based marine interface gateway that enables bidirectional communication between SignalK servers and boat instruments using multiple marine data protocols:
- NMEA 2000 (bidirectional via CAN bus)
- NMEA 0183 (read-only via Serial2)
- 1-Wire sensor devices

The system acts as a protocol bridge, translating between older NMEA 0183 equipment, modern NMEA 2000 networks, and SignalK systems. Built with PlatformIO and the Arduino framework, it runs on SH-ESP32 hardware designed for 24/7 always-on marine applications.

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

# Monitor serial output
pio run --target monitor

# Clean build artifacts
pio run --target clean

# Build and upload in one command
pio run --target upload && pio run --target monitor
```

## Architecture & Key Constraints

### Hardware Abstraction Layer (HAL) - CRITICAL
All hardware interactions MUST be abstracted through interfaces or wrapper classes:
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

### GPIO Pin Configuration (SH-ESP32 Board)
```
CAN RX: GPIO 34        |  I2C Bus 1: SDA=16, SCL=17
CAN TX: GPIO 32        |  I2C Bus 2: SDA=21, SCL=22
Serial2: RX=25, TX=27  |  1-Wire: GPIO 4
OLED Display: I2C Bus 2|  Button: GPIO 13
SD Card: GPIO 15       |  LED: GPIO 2
```

### Initialization Sequence - MUST FOLLOW THIS ORDER
1. WiFi connection
2. I2C buses (Wire0 and Wire1)
3. OLED display (I2C Bus 2, 128x64 SSD1306, address 0x3C)
4. Serial2 for NMEA 0183
5. NMEA2000 CAN bus
6. Message handlers registration
7. ReactESP event loops

### Network-Based Debugging - Serial Ports Reserved
Serial ports are used for device communication (NMEA 0183), NOT debugging:
- Use WebSocket logging for reliable debug output (ws://<device-ip>/logs)
- Log levels: DEBUG, INFO, WARN, ERROR, FATAL
- Include timestamps (millis() or RTC)
- Production builds: only ERROR/FATAL levels
- TCP-based protocol ensures no packet loss
- Fallback: Store critical errors to flash (SPIFFS/LittleFS) if WebSocket unavailable

### Always-On Operation
- WiFi MUST remain always-on (no power management)
- Deep sleep/light sleep modes NOT permitted
- Designed for 24/7 operation with continuous network availability
- Watchdog timer enabled in production builds

## Project Structure

```
.specify/
├── memory/
│   └── constitution.md          # Development principles and standards (v1.1.0)
└── templates/
    ├── plan-template.md         # Implementation planning template
    ├── spec-template.md         # Feature specification template
    └── tasks-template.md        # Task breakdown template

examples/poseidongw/             # Reference implementation - USE AS GUIDE
└── src/
    ├── main.cpp                 # Working example of initialization & event loops
    ├── BoatData.h               # Data structures for marine data
    ├── NMEA0183Handlers.cpp/h   # NMEA 0183 message parsing
    └── Seasmart.h               # Protocol conversion utilities

user_requirements/
└── R001 - foundation.md         # Core requirements and principles
```

## Core Libraries & Documentation

**Pre-approved Libraries** (already in use):
- **NMEA2000**: https://github.com/ttlappalainen/NMEA2000 - API docs: https://ttlappalainen.github.io/NMEA2000/pg_lib_ref.html
- **NMEA0183**: https://github.com/ttlappalainen/NMEA0183
- **NMEA2000_esp32**: https://github.com/ttlappalainen/NMEA2000_esp32 - ESP32-specific CAN implementation
- **ReactESP**: https://github.com/mairas/ReactESP - Asynchronous event loops (required pattern)
- **ESPAsyncWebServer**: https://github.com/ESP32Async/ESPAsyncWebServer - Async HTTP/WebSocket
- **Adafruit_SSD1306**: OLED display driver

**Reference First**: Before implementing NMEA2000/0183 features, check:
1. `examples/poseidongw/src/` - working implementation patterns
2. Official library documentation (links above)
3. Library example code in repositories

## Development Workflow

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
**Mock-First**:
- Business logic tested with mocked hardware interfaces
- Unit tests run on native environment (development machine)
- Use PlatformIO native environment for non-hardware tests

**Minimal Hardware Testing**:
- Only for: HAL validation, sensor calibration, communication protocols, timing-critical operations
- Document hardware test setup requirements
- Prefer QA code review over re-running hardware tests

### Test Organization Pattern

**Structure** (PlatformIO Grouped Tests):
```
test/
├── helpers/                      # Shared test utilities (not a test)
│   ├── test_mocks.h             # Mock implementations
│   ├── test_fixtures.h          # Test data fixtures
│   └── test_utilities.h         # Common helpers
│
├── test_boatdata_contracts/     # BoatData HAL interface tests
│   ├── test_main.cpp            # Runs all contract tests
│   ├── test_iboatdatastore.cpp
│   ├── test_isensorupdate.cpp
│   ├── test_icalibration.cpp
│   └── test_isourceprioritizer.cpp
│
├── test_boatdata_integration/   # BoatData integration scenarios
│   ├── test_main.cpp            # Runs all 7 scenarios
│   ├── test_single_gps_source.cpp
│   ├── test_multi_source_priority.cpp
│   ├── test_source_failover.cpp
│   ├── test_manual_override.cpp
│   ├── test_derived_calculation.cpp (to be created)
│   ├── test_calibration_update.cpp (to be created)
│   └── test_outlier_rejection.cpp
│
├── test_boatdata_units/         # BoatData formula/utility tests
│   ├── test_main.cpp
│   ├── test_angle_utils.cpp (to be created)
│   ├── test_data_validator.cpp (to be created)
│   └── test_calculation_formulas.cpp (to be created)
│
├── test_boatdata_timing/        # Hardware test (ESP32)
│   └── test_main.cpp
│
├── test_wifi_integration/       # WiFi connection scenarios
│   ├── test_main.cpp
│   ├── test_first_time_config.cpp
│   ├── test_network_failover.cpp
│   ├── test_connection_loss_recovery.cpp
│   ├── test_all_networks_unavailable.cpp
│   └── test_services_run_independently.cpp
│
├── test_wifi_units/             # WiFi component tests
│   ├── test_main.cpp
│   ├── test_config_parser.cpp
│   ├── test_connection_state.cpp
│   ├── test_state_machine.cpp
│   ├── test_wifi_credentials.cpp
│   ├── test_wifi_config_file.cpp
│   └── test_wifi_manager_logic.cpp
│
├── test_wifi_endpoints/         # WiFi HTTP API tests
│   ├── test_main.cpp
│   ├── test_upload_config_endpoint.cpp
│   ├── test_get_config_endpoint.cpp
│   ├── test_status_endpoint.cpp
│   └── test_invalid_config_handling.cpp
│
└── test_wifi_connection/        # Hardware test (ESP32)
    └── test_main.cpp
```

**Test Discovery**: PlatformIO discovers only directories with `test_` prefix as test applications.

**Test Execution**: Each `test_main.cpp` uses Unity framework to run all tests in that group.

**Test Filtering Examples**:
```bash
# Run all tests
pio test -e native

# Run all BoatData tests
pio test -e native -f test_boatdata_*

# Run all WiFi tests
pio test -e native -f test_wifi_*

# Run only contract tests
pio test -e native -f test_*_contracts

# Run only integration tests
pio test -e native -f test_*_integration

# Run only unit tests
pio test -e native -f test_*_units

# Run specific test group
pio test -e native -f test_boatdata_contracts
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

### Code Standards - MUST FOLLOW

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

## Constitutional Compliance

**BEFORE implementing any feature**, verify against constitution principles (`.specify/memory/constitution.md`):
1. Hardware Abstraction (HAL interfaces used?)
2. Resource Management (static allocation, stack limits, F() macros?)
3. QA Review Process (review planned?)
4. Modular Design (single responsibility, dependency injection?)
5. Network Debugging (WebSocket logging implemented?)
6. Always-On Operation (no sleep modes?)
7. Fail-Safe Operation (watchdog, safe mode, graceful degradation?)

See `.specify/templates/plan-template.md` for detailed constitution checklist.

## Key Implementation Patterns

### Asynchronous Programming with ReactESP
The reference implementation (`examples/poseidongw/`) demonstrates the required event-driven pattern:
- Use ReactESP event loops for all periodic tasks
- No blocking delays in main loop
- Register callbacks for NMEA message handlers
- WebSocket logging via `WebSocketLogger` class (see src/utils/WebSocketLogger.h)

### NMEA 2000 Message Handling
Follow the pattern in reference implementation:
- Define message PGN lists in PROGMEM (see main.cpp:33-35)
- Register message handlers during initialization
- Use tBoatData structure for shared state
- Forward to SignalK via appropriate protocol

### NMEA 0183 Parsing
Reference: `examples/poseidongw/src/NMEA0183Handlers.cpp`
- Serial2 dedicated to NMEA 0183 input (read-only)
- Parse to internal data structures
- Convert to NMEA 2000 for network distribution

### OLED Display Management

The OLED display feature provides real-time system status on a 128x64 SSD1306 OLED connected via I2C Bus 2.

#### Display Initialization

Initialize display after WiFi connection in `main.cpp`:

```cpp
#include "hal/implementations/ESP32DisplayAdapter.h"
#include "hal/implementations/ESP32SystemMetrics.h"
#include "components/DisplayManager.h"

// Global instances
ESP32DisplayAdapter* displayAdapter = nullptr;
ESP32SystemMetrics* systemMetrics = nullptr;
DisplayManager* displayManager = nullptr;

void setup() {
    // ... WiFi initialization ...

    // Initialize OLED display (after WiFi, before NMEA)
    Serial.println(F("Initializing OLED display..."));
    displayAdapter = new ESP32DisplayAdapter();
    systemMetrics = new ESP32SystemMetrics();
    displayManager = new DisplayManager(displayAdapter, systemMetrics, &logger);

    if (displayManager->init()) {
        Serial.println(F("OLED display initialized successfully"));
    } else {
        Serial.println(F("WARNING: OLED display initialization failed"));
        // Graceful degradation: Continue operation without display
    }

    // ... continue with NMEA initialization ...
}
```

#### Hardware Configuration

- **Display**: SSD1306 OLED, 128x64 pixels, monochrome
- **I2C Bus**: Bus 2 (SDA=GPIO21, SCL=GPIO22)
- **I2C Address**: 0x3C (common for 128x64 displays)
- **Clock Speed**: 400kHz (fast mode)
- **Library**: Adafruit_SSD1306 + Adafruit_GFX

#### ReactESP Display Loops

Add periodic display updates after other event loops:

```cpp
void setup() {
    // ... initialization ...

    // Display refresh loops - 1s animation, 5s status
    app.onRepeat(DISPLAY_ANIMATION_INTERVAL_MS, []() {
        if (displayManager != nullptr) {
            displayManager->updateAnimationIcon();
        }
    });

    app.onRepeat(DISPLAY_STATUS_INTERVAL_MS, []() {
        if (displayManager != nullptr) {
            displayManager->renderStatusPage();
        }
    });
}
```

**Timing Requirements**:
- Animation icon update: 1 second (rotating spinner: / - \ |)
- Status page refresh: 5 seconds (WiFi, RAM, flash, CPU metrics)
- Display update duration: < 50ms typical, < 200ms max

#### Display Layout

6-line text display (font size 1, 5x7 pixels per character):

```
Line 0 (y=0):  WiFi: <SSID>         (21 chars max)
Line 1 (y=10): IP: <IP Address>     (e.g., "192.168.1.100")
Line 2 (y=20): RAM: <Free KB>       (e.g., "RAM: 244KB")
Line 3 (y=30): Flash: <Used/Total>  (e.g., "Flash: 830/1920KB")
Line 4 (y=40): CPU Idle: <%>        (e.g., "CPU Idle: 85%")
Line 5 (y=50): [ <icon> ]           (right corner, rotating icon)
```

#### HAL Interfaces

Display system uses Hardware Abstraction Layer for testability:

- **IDisplayAdapter**: Abstracts SSD1306 OLED hardware
  - Production: `ESP32DisplayAdapter` (uses Adafruit_SSD1306)
  - Testing: `MockDisplayAdapter` (for unit/integration tests)

- **ISystemMetrics**: Abstracts ESP32 system metrics
  - Production: `ESP32SystemMetrics` (uses ESP.h and FreeRTOS APIs)
  - Testing: `MockSystemMetrics` (returns fixed values)

#### Graceful Degradation

Display initialization failures DO NOT halt system operation:

```cpp
if (displayManager->init()) {
    // Display ready - render startup progress
    displayManager->renderStartupProgress(status);
} else {
    // Display unavailable - log error and continue
    logger.broadcastLog(LogLevel::ERROR, "Main", "DISPLAY_INIT_FAILED",
                        F("{\"reason\":\"I2C communication error\"}"));
    // System continues without display (FR-027)
}
```

**Common Failure Modes**:
- I2C communication error (display not connected or wrong address)
- I2C bus conflict (another device using same address)
- Display initialization timeout

**Recovery**: Reboot device or check I2C connections. System continues operating normally.

#### Memory Footprint

Display system uses efficient static allocation (constitutional compliance):

- **DisplayMetrics**: 21 bytes (system resource metrics)
- **SubsystemStatus**: 66 bytes (WiFi, filesystem, web server status)
- **DisplayPage**: 10 bytes (page definition)
- **SSD1306 Framebuffer**: 1024 bytes (128x64 / 8 bits)
- **Total RAM**: ~1.1KB (0.3% of ESP32's 320KB RAM)

All string literals use `F()` macro to store in flash (PROGMEM).

#### Troubleshooting

**Display not initializing**:
1. Verify I2C connections (SDA=GPIO21, SCL=GPIO22)
2. Check I2C address (0x3C for 128x64, 0x3D alternative)
3. Test with I2C scanner: `Wire.begin(21, 22); Wire.beginTransmission(0x3C);`
4. Monitor WebSocket logs for "DISPLAY_INIT_FAILED" event

**Display showing garbage**:
1. Verify display resolution matches code (128x64)
2. Check I2C clock speed (400kHz max for most SSD1306)
3. Ensure proper power supply (3.3V stable)

**Display frozen**:
1. Check ReactESP event loops running (`app.tick()` in main loop)
2. Verify display manager not null before calling methods
3. Check for watchdog timer resets (excessive blocking in render loop)

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

## WiFi Network Management

### Configuration File Location
- **Path**: `/wifi.conf` (LittleFS flash filesystem)
- **Format**: Plain text, comma-separated: `SSID,password`
- **Max Networks**: 3
- **Priority**: Line order (first = highest priority)

**Example**:
```
HomeNetwork,mypassword123
Marina_Guest,guestpass
BackupNetwork,backup123
```

### HTTP API Endpoints

#### POST /upload-wifi-config
Upload new WiFi configuration file.
- **Input**: Multipart form-data with `config` field
- **Validation**: SSID (1-32 chars), Password (0 or 8-63 chars)
- **Response**: 200 (success + reboot scheduled) or 400 (validation errors)
- **Side Effect**: Device reboots after 5 seconds to apply new config

#### GET /wifi-config
Retrieve current configuration (passwords redacted).
- **Response**: JSON with SSIDs and priority order
- **Security**: Passwords never exposed in API responses

#### GET /wifi-status
Get current connection status.
- **States**: CONNECTED, CONNECTING, DISCONNECTED, FAILED
- **Data**: SSID, IP address, signal strength (RSSI), uptime

### Connection State Machine

```
DISCONNECTED → CONNECTING → CONNECTED
             ↓ (timeout)
           FAILED → [next network] → CONNECTING
                  ↓ (all failed)
              DISCONNECTED → [reboot after 5s]
```

**State Transitions**:
- `DISCONNECTED → CONNECTING`: Connection attempt started
- `CONNECTING → CONNECTED`: WiFi.status() == WL_CONNECTED
- `CONNECTING → FAILED`: Timeout after 30 seconds
- `FAILED → CONNECTING`: Try next network in priority list
- `CONNECTED → DISCONNECTED`: Connection lost (triggers retry)

**Key Behaviors**:
- **Initial Boot**: Attempts networks in priority order (0 → 1 → 2)
- **Connection Loss**: Retries SAME network (no failover to other networks)
- **All Networks Fail**: Reboot after 5-second delay, repeat cycle
- **Timeout**: 30 seconds per network attempt

### Troubleshooting Common Issues

#### Device Stuck in Reboot Loop
**Cause**: No valid WiFi configuration or all networks unavailable

**Solutions**:
1. Check `/wifi.conf` exists in LittleFS (upload via `pio run -t uploadfs`)
2. Verify SSID and password are correct
3. Ensure at least one network is available and in range
4. Check network is 2.4 GHz (ESP32 classic doesn't support 5 GHz)
5. Monitor WebSocket logs for detailed error messages:
   ```bash
   source src/helpers/websocket_env/bin/activate
   python3 src/helpers/ws_logger.py <ip>
   ```

#### Configuration Upload Returns 400 Error
**Cause**: Validation failure

**Common Errors**:
- SSID exceeds 32 characters (WiFi spec limit)
- Password is 1-7 characters (must be 0 or 8+ for WPA2)
- More than 3 networks in file
- Malformed lines (missing comma separator)
- Newline characters in SSID/password

**Fix**: Validate config file format before uploading

#### Connection Lost - Device Not Reconnecting
**Cause**: WiFi disconnect event not triggering retry

**Debug Steps**:
1. Check WebSocket logs for `CONNECTION_LOST` event
2. Verify `handleDisconnect()` is registered as WiFi event callback
3. Confirm ReactESP event loop is running (`app.tick()` in main loop)
4. Check `currentNetworkIndex` hasn't changed (should stay same for retry)

**Expected Behavior**: Device retries same network indefinitely until reconnection

#### Services Not Starting During WiFi Connection
**Cause**: Blocking code in WiFi initialization

**Constitutional Requirement**: Services MUST start within 2 seconds, independent of WiFi

**Verify**:
1. Check `setup()` function doesn't block on WiFi connection
2. `WiFiManager::connect()` returns immediately (async operation)
3. ReactESP event loops registered before WiFi attempts
4. WebSocket logger, OLED display, NMEA handlers init before WiFi

**Debug**: Monitor serial output for service startup timestamps

#### WebSocket Logs Not Received
**Cause**: Network configuration or WebSocket connection failure

**Solutions**:
1. Verify device and computer on same network
2. Check firewall allows HTTP/WebSocket port 80
3. Activate Python virtual environment: `source src/helpers/websocket_env/bin/activate`
4. Verify device IP address
5. Use `python3 src/helpers/ws_logger.py <ip> --reconnect` for auto-reconnect
6. Logs buffer until WiFi connected - wait for connection

#### Flash Filesystem Errors
**Cause**: LittleFS corruption or not formatted

**Solutions**:
```bash
# Erase and reformat flash
pio run -e esp32dev -t erase

# Upload filesystem image
pio run -e esp32dev -t uploadfs

# Reflash firmware
pio run -e esp32dev -t upload
```

### Performance Benchmarks

**Connection Timing** (from boot):
- First network available: **< 30 seconds**
- First fails, second succeeds: **30-60 seconds**
- All networks fail: **~95 seconds** (90s attempts + 5s reboot delay)

**Service Startup**:
- LittleFS mount: **< 100 ms**
- Config file parse: **< 50 ms**
- WiFi attempt start: **< 50 ms** (non-blocking)
- All services operational: **< 2 seconds** ⚠️ **Constitutional requirement**

**Memory Usage**:
- WiFiConfigFile struct: **~334 bytes RAM** (3 networks)
- Flash storage: **~200 bytes** (config file)
- Code footprint: **~850 KB flash** (43% of partition)

**Validation**: Run `test/test_services_run_independently.cpp` to verify timing requirements

### Code Patterns

**WiFi Initialization** (src/main.cpp):
```cpp
// 1. Mount filesystem
fileSystem->mount();

// 2. Create WiFiManager with HAL dependencies
wifiManager = new WiFiManager(&wifiAdapter, &fileSystem, &logger, &timeoutMgr);

// 3. Load config
wifiManager->loadConfig(config);

// 4. Register ReactESP event loops BEFORE WiFi attempt
app.onRepeat(1000, checkConnectionTimeout);

// 5. Start connection (non-blocking)
wifiManager->connect(state, config);

// 6. Register WiFi event handlers
WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
        onWiFiConnected();
    }
});
```

**WebSocket Logging**:
```cpp
// All logging uses WebSocketLogger for reliable delivery
logger.broadcastLog(LogLevel::INFO, F("WiFiManager"), F("CONNECTION_ATTEMPT"),
    String("{\"ssid\":\"") + ssid + "\",\"timeout\":30}");
```
- Endpoint: ws://<device-ip>/logs
- Client: `source src/helpers/websocket_env/bin/activate && python3 src/helpers/ws_logger.py <ip>`
- Protocol: TCP-based WebSocket (no packet loss)

**HAL Abstraction** (for testability):
```cpp
// Production: Real hardware
IWiFiAdapter* wifi = new ESP32WiFiAdapter();
IFileSystem* fs = new LittleFSAdapter();

// Testing: Mocks
IWiFiAdapter* wifi = new MockWiFiAdapter();
IFileSystem* fs = new MockFileSystem();

// Both use same WiFiManager interface
WiFiManager manager(wifi, fs);
```

### Testing WiFi Management

**Unit Tests** (no hardware):
```bash
pio test -e native -f test_wifi_credentials
pio test -e native -f test_config_parser
pio test -e native -f test_wifi_manager_logic
```

**Integration Tests** (mocked hardware):
```bash
pio test -e native -f test_first_time_config
pio test -e native -f test_network_failover
pio test -e native -f test_connection_loss_recovery
pio test -e native -f test_all_networks_unavailable
```

**Hardware Tests** (ESP32 required):
```bash
# Update WiFi credentials in test/test_wifi_connection/test_main.cpp first!
pio test -e esp32dev_test -f test_wifi_connection
```

See `test/test_wifi_connection/README.md` for detailed hardware test setup.

---
**WiFi Management Version**: 1.0.0 | **Last Updated**: 2025-10-06

## BoatData Integration

### Overview
The BoatData feature provides a centralized data model for marine sensor data with:
- Automatic multi-source prioritization (GPS, compass)
- Real-time derived parameter calculations (200ms cycle)
- Web-based calibration interface
- Data validation and outlier rejection

### Architecture

**Core Components**:
- `BoatData`: Central data repository implementing `IBoatDataStore` and `ISensorUpdate`
- `SourcePrioritizer`: Multi-source management with frequency-based automatic priority
- `CalculationEngine`: Derived sailing parameter calculations (TWS, TWA, VMG, leeway, current)
- `CalibrationManager`: LittleFS persistence for calibration parameters
- `CalibrationWebServer`: HTTP API for calibration updates

**Data Flow**:
```
NMEA0183/NMEA2000/1-Wire Handlers
    ↓ (updateGPS/updateCompass/etc.)
BoatData (validates, stores raw sensor data)
    ↓ (every 200ms)
CalculationEngine (calculates derived parameters)
    ↓
BoatData.derived (TWS, TWA, STW, VMG, SOC, DOC, leeway, etc.)
    ↓
Display / Logger / Web API
```

### NMEA Handler Integration

**ISensorUpdate Interface**:
NMEA message handlers update sensor data using the `ISensorUpdate` interface.

**Example: NMEA0183 RMC Message Handler**:
```cpp
#include "components/BoatData.h"

extern BoatData* boatData;  // Global instance from main.cpp

void handleNMEA0183_RMC(tNMEA0183Msg& msg) {
    // Parse RMC message fields
    double lat = parseLatitude(msg);      // Decimal degrees, North positive
    double lon = parseLongitude(msg);     // Decimal degrees, East positive
    double cog = parseCOG(msg) * DEG_TO_RAD;  // Convert to radians
    double sog = parseSOG(msg);           // Knots

    // Update BoatData via ISensorUpdate interface
    bool accepted = boatData->updateGPS(lat, lon, cog, sog, "GPS-NMEA0183");

    if (!accepted) {
        // Data rejected (outlier or validation failure)
        Serial.println(F("GPS data rejected - outlier detected"));
    }
}
```

**Example: NMEA2000 PGN 129029 (GNSS Position)**:
```cpp
#include <NMEA2000.h>
#include "components/BoatData.h"

extern BoatData* boatData;

void handleNMEA2000_129029(const tN2kMsg& msg) {
    unsigned char SID;
    uint16_t DaysSince1970;
    double SecondsSinceMidnight;
    double Latitude;
    double Longitude;
    double Altitude;
    tN2kGNSStype GNSStype;
    tN2kGNSSmethod GNSSmethod;
    unsigned char nSatellites;
    double HDOP;
    double PDOP;
    double GeoidalSeparation;
    unsigned char nReferenceStations;
    tN2kGNSStype ReferenceStationType;
    uint16_t ReferenceSationID;
    double AgeOfCorrection;

    if (ParseN2kPGN129029(msg, SID, DaysSince1970, SecondsSinceMidnight,
                          Latitude, Longitude, Altitude,
                          GNSStype, GNSSmethod, nSatellites, HDOP, PDOP,
                          GeoidalSeparation, nReferenceStations,
                          ReferenceStationType, ReferenceSationID,
                          AgeOfCorrection)) {

        // Note: PGN 129029 doesn't include COG/SOG - use PGN 129026 for COG/SOG
        // For now, update position only (COG/SOG set to 0)
        bool accepted = boatData->updateGPS(Latitude, Longitude, 0.0, 0.0, "GPS-NMEA2000");

        if (!accepted) {
            Serial.println(F("GPS data rejected"));
        }
    }
}
```

**Example: NMEA2000 PGN 130306 (Wind Data)**:
```cpp
void handleNMEA2000_130306(const tN2kMsg& msg) {
    unsigned char SID;
    double WindSpeed;
    double WindAngle;
    tN2kWindReference WindReference;

    if (ParseN2kPGN130306(msg, SID, WindSpeed, WindAngle, WindReference)) {
        // Convert wind angle to radians, range [-π, π]
        double awaRadians = WindAngle;
        if (awaRadians > M_PI) {
            awaRadians -= 2.0 * M_PI;
        }

        // Update BoatData
        bool accepted = boatData->updateWind(awaRadians, WindSpeed, "WIND-NMEA2000");

        if (!accepted) {
            Serial.println(F("Wind data rejected"));
        }
    }
}
```

**Available Update Methods**:
```cpp
// GPS data (latitude, longitude, COG, SOG)
bool updateGPS(double lat, double lon, double cog, double sog, const char* sourceId);

// Compass data (true heading, magnetic heading, variation)
bool updateCompass(double trueHdg, double magHdg, double variation, const char* sourceId);

// Wind data (apparent wind angle, apparent wind speed)
bool updateWind(double awa, double aws, const char* sourceId);

// Speed and heel data (heel angle, boat speed from paddle wheel)
bool updateSpeed(double heelAngle, double boatSpeed, const char* sourceId);

// Rudder data (steering angle)
bool updateRudder(double angle, const char* sourceId);
```

**Return Value**:
- `true`: Data accepted and stored
- `false`: Data rejected (outlier, out of range, or rate-of-change exceeded)

**Data Validation**:
All incoming sensor data is validated:
- **Range checks**: Latitude [-90, 90], Longitude [-180, 180], SOG >= 0, etc.
- **Rate-of-change checks**: Rejects physically impossible changes (e.g., position change >0.1°/sec)
- **Outlier detection**: Hybrid range + rate-of-change validation

See `src/utils/DataValidator.h` for validation thresholds.

### Multi-Source Prioritization

**Automatic Priority**:
Sources are automatically prioritized by update frequency (higher Hz = higher priority).

**Example**:
- GPS-A (NMEA0183): 1 Hz → Priority 2
- GPS-B (NMEA2000): 10 Hz → Priority 1 (active source)

**Manual Override** (via web API):
```bash
# Force GPS-A as active source regardless of frequency
curl -X POST http://<device-ip>/api/source-priority \
  -H "Content-Type: application/json" \
  -d '{"sensorType":"GPS","sourceId":"GPS-NMEA0183","manual":true}'
```

**Failover**:
If active source stops updating for >5 seconds, system automatically fails over to next-priority source.

### Calculation Cycle

**Frequency**: Every 200ms (5 Hz) via ReactESP `app.onRepeat(200, calculateDerivedParameters)`

**Calculated Parameters** (11 total):
1. **awaOffset**: AWA corrected for masthead offset (radians)
2. **awaHeel**: AWA corrected for heel (radians)
3. **leeway**: Leeway angle (radians)
4. **stw**: Speed through water, corrected for leeway (knots)
5. **tws**: True wind speed (knots)
6. **twa**: True wind angle, relative to boat (radians)
7. **wdir**: Wind direction, magnetic (radians, 0-2π)
8. **vmg**: Velocity made good (knots, signed)
9. **soc**: Speed of current (knots)
10. **doc**: Direction of current, magnetic (radians, 0-2π)

**Required Sensor Data**:
- GPS (lat, lon, COG, SOG)
- Compass (true heading, magnetic heading, variation)
- Wind (AWA, AWS)
- Speed (heel angle, boat speed)
- Calibration (K factor, wind offset)

**Timing Monitoring**:
- Calculation duration measured every cycle
- Warning logged if duration exceeds 200ms
- Skip-and-continue strategy on overrun (constitutional requirement)

**Diagnostics**:
```cpp
DiagnosticData diag = boatData->getDiagnostics();
Serial.printf("Calculation count: %lu\n", diag.calculationCount);
Serial.printf("Overruns: %lu\n", diag.calculationOverruns);
Serial.printf("Last duration: %lu ms\n", diag.lastCalculationDuration);
```

### Calibration Web API

**GET /api/calibration** - Retrieve current calibration:
```bash
curl http://<device-ip>/api/calibration
```

**Response**:
```json
{
  "leewayKFactor": 1.0,
  "windAngleOffset": 0.0,
  "loaded": true,
  "lastModified": 1234567890
}
```

**POST /api/calibration** - Update calibration:
```bash
curl -X POST http://<device-ip>/api/calibration \
  -H "Content-Type: application/json" \
  -d '{
    "leewayKFactor": 0.65,
    "windAngleOffset": 0.087
  }'
```

**Validation Rules**:
- `leewayKFactor` must be > 0
- `windAngleOffset` must be in range [-2π, 2π]

**Persistence**:
- Calibration saved to `/calibration.json` on LittleFS
- Loaded automatically on boot
- Default values used if file missing: K=1.0, offset=0.0

**Security**: No authentication required (open access on private boat network, per FR-036)

### Data Units

All angles stored in **radians**, speeds in **knots**, coordinates in **decimal degrees**:

| Field | Unit | Range |
|-------|------|-------|
| Latitude | Decimal degrees | [-90, 90] |
| Longitude | Decimal degrees | [-180, 180] |
| COG | Radians | [0, 2π] |
| SOG | Knots | [0, 100] |
| True/Magnetic Heading | Radians | [0, 2π] |
| Variation | Radians | [-π, π] |
| AWA | Radians | [-π, π] |
| AWS | Knots | [0, 100] |
| Heel Angle | Radians | [-π/2, π/2] |
| Boat Speed | Knots | [0, 50] |

**Conversion Constants**:
```cpp
#define DEG_TO_RAD (M_PI / 180.0)
#define RAD_TO_DEG (180.0 / M_PI)
```

### Testing BoatData

Tests are organized into grouped directories following PlatformIO best practices. Each test group runs multiple related tests via a single `test_main.cpp` entry point.

**Run All BoatData Tests**:
```bash
pio test -e native -f test_boatdata_*
```

**Contract Tests** (HAL interface validation):
```bash
pio test -e native -f test_boatdata_contracts
```
Tests: IBoatDataStore, ISensorUpdate, ICalibration, ISourcePrioritizer contracts

**Integration Tests** (complete scenarios):
```bash
pio test -e native -f test_boatdata_integration
```
Tests: 7 integration scenarios (single GPS, multi-source, failover, manual override, derived calculations, calibration, outlier rejection)

**Unit Tests** (formula and utility validation):
```bash
pio test -e native -f test_boatdata_units
```
Tests: AngleUtils, DataValidator, calculation formulas (AWA, leeway, TWS, TWA, VMG, current)

**Hardware Test** (ESP32 required, timing validation):
```bash
pio test -e esp32dev_test -f test_boatdata_timing
```
Tests: 200ms calculation cycle performance on actual hardware

**Run Specific Test Types Across All Features**:
```bash
# All contract tests (BoatData + WiFi)
pio test -e native -f test_*_contracts

# All integration tests (BoatData + WiFi)
pio test -e native -f test_*_integration

# All unit tests (BoatData + WiFi)
pio test -e native -f test_*_units
```

### Memory Footprint

**Static Allocation** (constitutional requirement - validated 2025-10-07):

*Sensor Data Structures*:
- GPSData: 48 bytes (5 doubles + bool + unsigned long)
- CompassData: 40 bytes (3 doubles + bool + unsigned long)
- WindData: 32 bytes (2 doubles + bool + unsigned long)
- SpeedData: 32 bytes (2 doubles + bool + unsigned long)
- RudderData: 24 bytes (1 double + bool + unsigned long)
- CalibrationData: 24 bytes (2 doubles + bool)
- DerivedData: 96 bytes (10 doubles + bool + unsigned long)
- DiagnosticData: 48 bytes (6 unsigned longs)

*Composite Structures*:
- BoatDataStructure: 344 bytes
- SensorSource: 72 bytes (per source)
- SourceManager: 744 bytes (10 sources: 5 GPS + 5 Compass)
- CalibrationParameters: 40 bytes (2 doubles + 2 ulongs + bool)

**TOTAL STATIC ALLOCATION: 1,128 bytes**
- ✅ Well under 2,000-byte target (56% of limit)
- ✅ 0.34% of ESP32's 320KB RAM
- ✅ Constitutional compliance: Principle II (Resource Management)

**Flash Storage**:
- Code: ~30KB (BoatData + CalculationEngine + APIs)
- Calibration file: ~200 bytes
- **Total Flash Impact**: ~30.2KB (1.5% of 1.9MB partition)

### Troubleshooting

**GPS data not updating**:
1. Check source registration: `sourcePrioritizer->registerSource("GPS-NMEA0183", SensorType::GPS, ...)`
2. Verify update calls return `true` (not rejected)
3. Check diagnostic counters: `boatData->getDiagnostics().rejectionCount`

**Calculation overruns**:
1. Monitor WebSocket logs for "OVERRUN" warnings:
   ```bash
   source src/helpers/websocket_env/bin/activate
   python3 src/helpers/ws_logger.py <ip> --filter WARN
   ```
2. Check `diagnostics.calculationOverruns` counter
3. Expected duration: <50ms typical, <200ms max

**Calibration not persisting**:
1. Verify LittleFS mounted: Serial output "LittleFS mounted successfully"
2. Check `/calibration.json` exists: `fileSystem->exists("/calibration.json")`
3. Verify web API returns 200 status

**Source failover not working**:
1. Check stale threshold: Source must have no updates for >5 seconds
2. Verify `prioritizer->updatePriorities()` called periodically
3. Check diagnostic logs for failover events

---
**BoatData Version**: 1.0.0 | **Last Updated**: 2025-10-07
