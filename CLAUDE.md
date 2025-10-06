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
3. OLED display
4. Serial2 for NMEA 0183
5. NMEA2000 CAN bus
6. Message handlers registration
7. ReactESP event loops

### Network-Based Debugging - Serial Ports Reserved
Serial ports are used for device communication (NMEA 0183), NOT debugging:
- Use UDP broadcast logging for debug output (port 4444)
- Log levels: DEBUG, INFO, WARN, ERROR, FATAL
- Include timestamps (millis() or RTC)
- Production builds: only ERROR/FATAL levels
- Fallback: Store critical errors to flash (SPIFFS/LittleFS) if network unavailable

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
- No silent failures - all errors logged via UDP
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
5. Network Debugging (UDP logging implemented?)
6. Always-On Operation (no sleep modes?)
7. Fail-Safe Operation (watchdog, safe mode, graceful degradation?)

See `.specify/templates/plan-template.md` for detailed constitution checklist.

## Key Implementation Patterns

### Asynchronous Programming with ReactESP
The reference implementation (`examples/poseidongw/`) demonstrates the required event-driven pattern:
- Use ReactESP event loops for all periodic tasks
- No blocking delays in main loop
- Register callbacks for NMEA message handlers
- UDP logging via `remotelog()` function (see main.cpp:86-95)

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

## Build Configurations

**Development**:
- UDP debug logging enabled (verbose)
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
5. Monitor UDP logs (port 4444) for detailed error messages

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
1. Check UDP logs for `CONNECTION_LOST` event
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
4. UDP logger, OLED display, NMEA handlers init before WiFi

**Debug**: Monitor serial output for service startup timestamps

#### UDP Logs Not Received
**Cause**: Network configuration or firewall

**Solutions**:
1. Verify device and listener on same network segment
2. Use broadcast address (255.255.255.255)
3. Check firewall allows UDP port 4444 inbound
4. Listener command: `nc -ul 4444` (macOS/Linux)
5. Logs buffer until WiFi connected - wait for connection

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

**UDP Logging**:
```cpp
// All logging uses F() macro for flash storage
logger->broadcastLog(LogLevel::INFO, F("WiFiManager"), F("CONNECTION_ATTEMPT"),
    String("{\"ssid\":\"") + ssid + "\",\"timeout\":30}");
```

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
