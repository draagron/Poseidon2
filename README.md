# Poseidon2 Marine Gateway

**ESP32-based marine interface gateway** enabling bidirectional communication between SignalK servers and boat instruments using multiple marine data protocols.

[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32-orange)](https://platformio.org/)
[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Hardware](https://img.shields.io/badge/Hardware-ESP32-green)](https://www.espressif.com/en/products/socs/esp32)

## Overview

Poseidon2 acts as a protocol bridge translating between:
- **NMEA 2000** (bidirectional via CAN bus)
- **NMEA 0183** (read-only via Serial2)
- **SignalK** (WebSocket/HTTP)
- **1-Wire** sensor devices

Built with PlatformIO and Arduino framework, it runs on SH-ESP32 hardware designed for 24/7 always-on marine applications.

## Features

### WiFi Network Management
- ✅ **Automatic Connection**: Priority-ordered WiFi with 30-second timeout failover
- ✅ **Persistent Storage**: LittleFS flash-based configuration (`/wifi.conf`)
- ✅ **HTTP Configuration API**: Upload/retrieve WiFi settings via web interface
- ✅ **Connection Recovery**: Auto-retry on disconnection (no failover to other networks)
- ✅ **Fail-Safe Mode**: Reboot loop when all networks exhausted
- ✅ **Independent Services**: Non-blocking initialization (services run without WiFi)

### Marine Protocols
- ✅ **Enhanced BoatData (R005)**: Comprehensive marine sensor data structures
  - GPS variation, compass motion sensors (rate of turn, heel, pitch, heave)
  - DST sensors (Depth/Speed/Temperature)
  - Engine telemetry (RPM, oil temp, alternator voltage)
  - Saildrive engagement status (1-wire)
  - Dual battery bank monitoring (1-wire)
  - Shore power monitoring (1-wire)
  - 8 NMEA2000 PGN handlers with validation
  - Memory footprint: ~560 bytes BoatData structure
- 🚧 **NMEA 2000** (CAN bus): PGN handlers ready, awaiting bus initialization
- ✅ **NMEA 0183** (Serial2): Autopilot (RSA, HDM) and VHF (GGA, RMC, VTG) sentence parsing
- 🚧 **SignalK Integration**: Real-time data streaming
- ✅ **1-Wire Sensors**: Marine sensor interface (saildrive, battery, shore power)

### System Features
- ✅ **Always-On Operation**: No sleep modes, 24/7 uptime
- ✅ **WebSocket Logging**: Reliable network-based debugging
- ✅ **Hardware Abstraction**: Testable via mocks (HAL pattern)
- ✅ **ReactESP**: Event-driven architecture for responsive operation
- ✅ **OLED Display**: Real-time system status and diagnostics on 128x64 SSD1306
- ✅ **Loop Frequency Monitoring**: Real-time main loop frequency measurement (5-second averaging)
- ✅ **WebUI Dashboard** (R009): Real-time marine sensor data visualization
  - WebSocket streaming at 1 Hz with 10 sensor cards
  - GPS, compass, wind, DST sensors, engine, battery, shore power, calculated performance
  - Responsive HTML dashboard with auto-reconnect
  - Optional Node.js proxy for multi-client support
- 🚧 **SD Card Logging**: Optional data recording

## Hardware Requirements

### Supported Boards
- **ESP32** (ESP32, ESP32-S2, ESP32-C3, ESP32-S3)
- **SH-ESP32** (recommended - optimized GPIO pinout)

### Pin Configuration (SH-ESP32)
```
CAN RX/TX:   GPIO 34/32    |  Serial2: RX=25, TX=27
I2C Bus 1:   SDA=16, SCL=17 |  I2C Bus 2: SDA=21, SCL=22
1-Wire:      GPIO 4         |  Button: GPIO 13
OLED:        I2C Bus 2      |  LED: GPIO 2
SD Card:     GPIO 15        |
```

### External Components
- **CAN Transceiver**: MCP2562 or SN65HVD230 (for NMEA 2000)
- **OLED Display**: SSD1306 128x64 (I2C)
- **SD Card Module**: SPI interface (optional)

## Quick Start

### 1. Installation

#### Prerequisites
- [PlatformIO Core](https://platformio.org/install) or [PlatformIO IDE](https://platformio.org/platformio-ide)
- USB cable for ESP32
- ESP32 board drivers (CH340/CP2102)

#### Clone and Build
```bash
git clone https://github.com/yourusername/Poseidon2.git
cd Poseidon2
pio run
```

### 2. WiFi Configuration

#### Create Configuration File

Create `wifi.conf` with your networks (plain text, max 3):

```
HomeNetwork,mypassword123
Marina_Guest,guestpass
Alternate,backuppass
```

**Format**:
- One network per line: `SSID,password`
- SSID: 1-32 characters
- Password: 0 (open) or 8-63 characters (WPA2)
- Priority order: First line = highest priority

#### Upload Configuration

**Method 1: Pre-Upload** (before first boot)
```bash
# Place wifi.conf in data/ directory
mkdir -p data
cp wifi.conf data/
pio run --target uploadfs
```

**Method 2: HTTP API** (after connected)
```bash
curl -X POST -F "config=@wifi.conf" http://<device-ip>/upload-wifi-config
```

### 3. Upload Firmware

```bash
# Build and upload
pio run --target upload

# Monitor serial output
pio run --target monitor
```

### 4. Verify Connection

#### Check Status
```bash
curl http://<device-ip>/wifi-status
```

**Response**:
```json
{
  "status": "CONNECTED",
  "ssid": "HomeNetwork",
  "ip_address": "192.168.1.100",
  "signal_strength": -45
}
```

#### View Configuration
```bash
curl http://<device-ip>/wifi-config
```

**Response** (passwords redacted):
```json
{
  "networks": [
    {"ssid": "HomeNetwork", "priority": 1},
    {"ssid": "Marina_Guest", "priority": 2}
  ],
  "max_networks": 3,
  "current_connection": "HomeNetwork"
}
```

## WiFi Configuration API

### POST /upload-wifi-config
Upload new WiFi configuration (triggers reboot after 5s).

**Request**:
```bash
curl -X POST -F "config=@wifi.conf" http://<device-ip>/upload-wifi-config
```

**Success Response** (200):
```json
{
  "status": "success",
  "message": "Configuration uploaded successfully. Device will reboot in 5 seconds.",
  "networks_count": 3
}
```

**Error Response** (400):
```json
{
  "status": "error",
  "message": "Invalid configuration file",
  "errors": [
    "Line 1: SSID exceeds 32 characters",
    "Line 3: Password must be 8-63 characters for WPA2"
  ]
}
```

### GET /wifi-config
Retrieve current configuration (passwords redacted).

**Response** (200):
```json
{
  "networks": [
    {"ssid": "Network1", "priority": 1},
    {"ssid": "Network2", "priority": 2}
  ],
  "max_networks": 3,
  "current_connection": "Network1"
}
```

### GET /wifi-status
Get current connection status.

**Response - Connected** (200):
```json
{
  "status": "CONNECTED",
  "ssid": "HomeNetwork",
  "ip_address": "192.168.1.100",
  "signal_strength": -45,
  "uptime_seconds": 3600
}
```

**Response - Connecting** (200):
```json
{
  "status": "CONNECTING",
  "current_attempt": "Marina_Guest",
  "attempt_number": 2,
  "time_remaining_seconds": 15
}
```

**Response - Disconnected** (200):
```json
{
  "status": "DISCONNECTED",
  "retry_count": 5,
  "next_reboot_in_seconds": 3
}
```

## Connection Behavior

### Network Priority
1. Attempts networks in order (line 1 → line 2 → line 3)
2. Each network gets 30-second timeout
3. Connects to first available network
4. If all fail: reboot after 5-second delay, retry cycle

### Connection Loss
- **Disconnect event detected** → Retry same network
- **NO failover** to other configured networks
- Continuous retry until reconnection succeeds

### Failover (Initial Boot Only)
- Network 1 unavailable (30s timeout) → Try Network 2
- Network 2 unavailable (30s timeout) → Try Network 3
- All unavailable → Reboot after 5s, repeat cycle

### Performance
- **First network available**: < 30 seconds
- **First fails, second succeeds**: 30-60 seconds
- **All networks fail**: ~95 seconds (90s attempts + 5s delay)

## OLED Display

### Overview
Real-time system status display on 128x64 SSD1306 OLED connected via I2C Bus 2.

### Display Content

#### Startup Screen
Shows subsystem initialization progress:
- "Poseidon2 Gateway" header
- WiFi connection status (Connecting/Connected/Failed)
- Filesystem mount status (Mounting/Mounted/Failed)
- Web server startup status (Starting/Running/Failed)

#### Runtime Status Screen
6-line display showing:
- **Line 0**: WiFi SSID (or "Disconnected")
- **Line 1**: IP address (e.g., "192.168.1.100")
- **Line 2**: Free RAM (e.g., "RAM: 244KB")
- **Line 3**: Flash usage (e.g., "Flash: 830/1920KB")
- **Line 4**: Loop frequency (e.g., "Loop: 212 Hz") - Real-time main loop measurement
- **Line 5**: Rotating animation icon (/, -, \, |)

### Display Updates
- **Animation**: 1-second refresh (rotating spinner)
- **Status**: 5-second refresh (metrics update)
- **WiFi events**: Real-time updates on state changes

### Hardware Configuration
- **Display**: SSD1306 OLED, 128x64 pixels, monochrome
- **Connection**: I2C Bus 2 (SDA=GPIO21, SCL=GPIO22)
- **I2C Address**: 0x3C (standard for 128x64 displays)
- **Clock Speed**: 400kHz (fast mode)
- **Library**: Adafruit_SSD1306 + Adafruit_GFX

### Graceful Degradation
If display initialization fails:
- Error logged via WebSocket
- System continues operation without display
- All other subsystems function normally

### Memory Footprint
- **Static allocation**: ~97 bytes (DisplayMetrics, SubsystemStatus)
- **Framebuffer**: 1024 bytes (SSD1306 requirement)
- **Total RAM**: ~1.1KB (0.34% of ESP32 RAM)
- **Flash usage**: +30KB (~1.5% of partition)

### Testing
```bash
# Contract tests (HAL interface validation)
pio test -e native -f test_oled_contracts

# Integration tests (5 end-to-end scenarios)
pio test -e native -f test_oled_integration

# Unit tests (component logic, formatters)
pio test -e native -f test_oled_units

# Hardware tests (ESP32 with OLED required)
pio test -e esp32dev_test -f test_oled_hardware
```

## WebUI Dashboard

### Overview
Real-time marine sensor data visualization via WebSocket streaming at 1 Hz. The dashboard displays all 10 sensor groups with automatic updates and unit conversions.

### Access Dashboard

**Direct from ESP32**:
```
http://<ESP32_IP>:3030/stream
```

Example: `http://192.168.10.3:3030/stream`

**Via Node.js Proxy** (recommended for multiple clients):
```bash
cd nodejs-boatdata-viewer
npm install      # First time only
npm start        # Starts proxy on localhost:3030

# Access dashboard
http://localhost:3030/stream
```

### Dashboard Features

#### 10 Sensor Cards

1. **GPS Navigation**: Latitude, Longitude, COG, SOG, Variation
2. **Compass & Attitude**: True/Magnetic Heading, Rate of Turn, Heel, Pitch, Heave
3. **Wind Data**: Apparent Wind Angle, Apparent Wind Speed
4. **DST**: Depth, Boat Speed, Sea Temperature
5. **Rudder**: Steering Angle
6. **Engine**: RPM, Oil Temperature, Alternator Voltage
7. **Saildrive**: Engagement Status
8. **Battery**: Dual banks (House/Starter) with voltage, amperage, SoC, charger status
9. **Shore Power**: Connection status, power draw
10. **Calculated Performance**:
    - Apparent Wind (Corrected): AWA Offset, AWA Heel
    - True Wind: TWS, TWA, Wind Direction
    - Speed & Leeway: STW, Leeway, VMG
    - Current: SOC, DOC

#### Real-Time Updates
- **Update rate**: 1 Hz (1-second refresh)
- **Unit conversions**: Radians→Degrees, meters/second→Knots
- **Availability indicators**: Color-coded cards (green=available, gray=unavailable)
- **Connection status**: Real-time WebSocket connection indicator

#### Auto-Reconnect
- Exponential backoff: 1s, 2s, 4s (max 10s)
- Handles firmware updates and ESP32 reboots
- Visual connection status indicator

### Node.js Proxy Server

**Purpose**: Relay ESP32 WebSocket data to multiple browser clients

**Why Use Proxy**:
- ESP32 WebSocket limited to ~8 concurrent connections
- Monitor data while testing firmware
- Access from different network segments
- No ESP32 resource impact (single connection to ESP32)

**Setup**:
1. Configure ESP32 IP in `nodejs-boatdata-viewer/config.json`
2. Install dependencies: `npm install`
3. Start proxy: `npm start`
4. Access dashboard: `http://localhost:3030/stream.html`

**Files**:
- `server.js`: WebSocket relay logic (231 lines)
- `config.json`: ESP32 IP/port configuration
- `public/stream.html`: Dashboard HTML (identical to ESP32 version)
- `package.json`: Node.js dependencies

### WebSocket API

**Endpoint**: `ws://<ESP32_IP>:3030/boatdata`

**Message Format**: JSON (1 Hz broadcast)
```json
{
  "timestamp": 1234567890,
  "gps": { "latitude": 37.7749, "longitude": -122.4194, ... },
  "compass": { "trueHeading": 1.571, "magneticHeading": 1.833, ... },
  "wind": { ... },
  "dst": { ... },
  "rudder": { ... },
  "engine": { ... },
  "saildrive": { ... },
  "battery": { ... },
  "shorePower": { ... },
  "derived": { ... }
}
```

**Connection Limits**:
- **Direct ESP32**: 8 concurrent WebSocket clients (ESPAsyncWebServer limit)
- **Via Proxy**: Unlimited clients (Node.js handles relay)

### Troubleshooting

**Dashboard not loading**:
1. Verify ESP32 connected to WiFi: `curl http://<ESP32_IP>/wifi-status`
2. Upload dashboard file: `pio run --target uploadfs`
3. Check LittleFS mounted: Monitor WebSocket logs for "FILESYSTEM_MOUNTED"
4. Test HTTP endpoint: `curl http://<ESP32_IP>:3030/stream.html`

**WebSocket not connecting**:
1. Check firewall allows port 3030
2. Verify WebSocket endpoint: `ws://<ESP32_IP>:3030/boatdata`
3. Browser console: Check for WebSocket connection errors
4. Monitor ESP32 logs: `python3 src/helpers/ws_logger.py <ESP32_IP> --filter WebSocket`

**No data updates**:
1. Check if sensor data available (cards show "Not Available")
2. Monitor serialization logs: `python3 src/helpers/ws_logger.py <ESP32_IP> --filter BoatDataSerializer`
3. Verify 1 Hz broadcast loop running (check main.cpp line 694)
4. Browser console: Check if WebSocket receiving messages

**Derived data not calculating**:
1. Verify sensor availability: Requires GPS, Compass, Wind, DST all available
2. Check CalculationEngine initialized: Monitor for "CALCULATION_ENGINE" logs
3. Verify 200ms calculation loop running (main.cpp line 650)
4. Monitor for calculation overruns: Watch for "OVERRUN" warnings

### Performance
- **Serialization**: <50ms per JSON conversion (@ 240 MHz ESP32)
- **Payload size**: ~1500-1800 bytes JSON per message
- **Memory footprint**: ~3KB RAM (0.9% of ESP32), +15KB flash
- **Latency**: <100ms from sensor update to browser display

### Testing
```bash
# BoatDataSerializer unit tests
pio test -e native -f test_serializer_units

# WebSocket integration tests
pio test -e native -f test_webui_integration

# Hardware tests (ESP32 + browser required)
pio test -e esp32dev_test -f test_webui_hardware
```

**Manual Testing**:
1. Upload firmware: `pio run --target upload`
2. Upload dashboard: `pio run --target uploadfs`
3. Access: `http://<ESP32_IP>:3030/stream`
4. Verify all 10 cards display
5. Check 1 Hz updates (watch timestamps)
6. Test disconnect/reconnect (watch auto-reconnect)

## Source Statistics Tracking

### Overview
Real-time tracking and visualization of NMEA 2000/0183 message sources for system diagnostics and topology understanding. The feature provides:

- **Source discovery**: Automatic detection by SID (NMEA2000) or talker ID (NMEA0183)
- **Frequency tracking**: Rolling 10-sample average for update rate calculation (Hz)
- **Staleness monitoring**: 5-second threshold for detecting inactive sources
- **Garbage collection**: Automatic removal of sources stale >5 minutes
- **WebSocket streaming**: Full snapshot + incremental delta updates (500ms batching)
- **WebUI dashboard**: Real-time visualization organized by BoatData categories
- **Node.js proxy**: Multi-client support via WebSocket relay

**Memory**: ~5.3KB static allocation for 50 sources maximum

### Access Source Statistics Dashboard

**Direct from ESP32**:
```
http://<ESP32_IP>:3030/sources
```

Example: `http://192.168.10.3:3030/sources`

**Via Node.js Proxy** (recommended for multiple clients):
```bash
cd nodejs-boatdata-viewer
npm install      # First time only
npm start        # Starts proxy on localhost:3030

# Access dashboard
http://localhost:3030/sources.html
```

### Dashboard Features

#### Organized by BoatData Category
- **GPS**: PGN129025, PGN129026, PGN129029, PGN127258, GGA, RMC, VTG
- **Compass**: PGN127250, PGN127251, PGN127252, PGN127257, HDM
- **Wind**: PGN130306
- **DST**: PGN128267, PGN128259, PGN130316
- **Rudder**: RSA
- **Engine**: PGN127488, PGN127489

#### Per-Source Information
- **Source ID**: Format "NMEA2000-<SID>" or "NMEA0183-<TalkerID>"
- **Update Frequency**: Hz (calculated from 10-sample rolling average)
- **Time Since Last Update**: Milliseconds
- **Staleness Indicator**: 🟢 Green (active) or 🔴 Red (>5 seconds stale)

#### Real-Time Updates
- **Update rate**: 500ms (2 Hz delta updates)
- **Frequency accuracy**: ±10% tolerance
- **Staleness detection**: <5.5 seconds
- **Connection status**: Visual WebSocket connection indicator

### WebSocket API

**Endpoint**: `ws://<ESP32_IP>:3030/source-stats`

**Message Types**:

1. **Full Snapshot** (on connect):
```json
{
  "event": "fullSnapshot",
  "version": 1,
  "timestamp": 123456789,
  "sources": {
    "GPS": {
      "PGN129025": [
        {
          "sourceId": "NMEA2000-42",
          "protocol": "NMEA2000",
          "frequency": 10.1,
          "timeSinceLast": 98,
          "isStale": false
        }
      ]
    }
  }
}
```

2. **Delta Update** (every 500ms):
```json
{
  "event": "deltaUpdate",
  "timestamp": 123457289,
  "changes": [
    {
      "sourceId": "NMEA2000-42",
      "frequency": 10.2,
      "timeSinceLast": 102,
      "isStale": false
    }
  ]
}
```

3. **Source Removed** (on GC):
```json
{
  "event": "sourceRemoved",
  "sourceId": "NMEA2000-42",
  "timestamp": 123456789,
  "reason": "stale"
}
```

### Memory Diagnostics Endpoint

Query current memory usage and source count:

```bash
curl http://<ESP32_IP>/diagnostics
```

**Response**:
```json
{
  "memory": {
    "freeHeap": 240000,
    "usedHeap": 80000,
    "totalHeap": 320000
  },
  "sources": {
    "count": 12,
    "max": 50
  }
}
```

### Node.js Proxy Support

The Node.js proxy relays source statistics to multiple clients:

**Setup**:
1. Configure ESP32 IP in `nodejs-boatdata-viewer/config.json`:
```json
{
  "esp32": {
    "host": "192.168.1.100",
    "port": 3030
  },
  "server": {
    "port": 3000
  },
  "boatDataPath": "/boatdata",
  "sourceStatsPath": "/source-stats"
}
```

2. Access proxy dashboard: `http://localhost:3000/sources.html`

**Benefits**:
- Supports 10+ concurrent browser clients
- Auto-reconnects to ESP32 on disconnect
- No ESP32 resource impact (single connection from proxy)

### Performance

- **Memory footprint**: <10KB RAM for 50 sources
- **JSON payload**: <5KB (full snapshot, 30 sources)
- **WebSocket batching**: 500ms ±50ms
- **Frequency accuracy**: ±10%
- **Staleness detection**: <5.5 seconds
- **Visual update latency**: <200ms

### Troubleshooting

**Dashboard not loading**:
1. Verify ESP32 connected: `curl http://<ESP32_IP>/wifi-status`
2. Upload dashboard: `pio run --target uploadfs`
3. Check LittleFS mounted: Monitor for "FILESYSTEM_MOUNTED" logs
4. Test HTTP endpoint: `curl http://<ESP32_IP>:3030/sources`

**No sources appearing**:
1. Verify NMEA devices connected (check physical connections)
2. Monitor source registry logs: `python3 src/helpers/ws_logger.py <ESP32_IP> --filter SourceRegistry`
3. Check NMEA handler initialization: Look for "NMEA2000" and "NMEA0183" init logs
4. Verify recordUpdate() calls in handlers (should see SOURCE_DISCOVERED events)

**Frequency shows 0.0 Hz**:
1. Wait for 10 samples to collect (1 second at 10 Hz)
2. Check buffer full flag: bufferFull should be true
3. Verify messages arriving regularly (not intermittent)

**Staleness not updating**:
1. Verify updateStaleFlags() timer running (500ms ReactESP loop)
2. Check WebSocket delta updates being sent
3. Monitor for hasChanges() flag (should trigger updates)

### Testing

```bash
# All source statistics tests
pio test -e native -f test_source_stats_*

# Specific test groups
pio test -e native -f test_source_stats_units        # Unit tests (FrequencyCalculator, etc.)
pio test -e native -f test_source_stats_contracts    # Contract tests (SourceRegistry invariants)
pio test -e native -f test_source_stats_integration  # Integration tests (end-to-end)

# Build and upload
pio run
pio run --target upload
pio run --target uploadfs
```

**Validation Guide**: See `specs/012-sources-stats-and/quickstart.md` for detailed validation scenarios with real NMEA devices.

## Development

### Project Structure
```
Poseidon2/
├── src/
│   ├── main.cpp                         # Entry point and ReactESP event loops
│   ├── config.h                         # Compile-time configuration
│   ├── hal/                             # Hardware Abstraction Layer
│   │   ├── interfaces/                  # HAL interfaces
│   │   │   ├── IWiFiAdapter.h           # WiFi abstraction interface
│   │   │   ├── IFileSystem.h            # Filesystem abstraction interface
│   │   │   ├── IBoatDataStore.h         # BoatData query interface
│   │   │   ├── ISensorUpdate.h          # Sensor data input interface
│   │   │   ├── ISourcePrioritizer.h     # Multi-source management interface
│   │   │   ├── ICalibration.h           # Calibration interface
│   │   │   ├── IDisplayAdapter.h        # OLED display abstraction interface
│   │   │   └── ISystemMetrics.h         # System metrics abstraction interface
│   │   └── implementations/             # ESP32-specific implementations
│   │       ├── ESP32WiFiAdapter.cpp/h   # ESP32 WiFi adapter
│   │       ├── LittleFSAdapter.cpp/h    # LittleFS filesystem adapter
│   │       ├── ESP32DisplayAdapter.cpp/h # ESP32 OLED display adapter
│   │       └── ESP32SystemMetrics.cpp/h  # ESP32 system metrics adapter
│   ├── components/                      # Feature components
│   │   ├── WiFiManager.cpp/h            # WiFi connection orchestrator
│   │   ├── ConfigParser.cpp/h           # WiFi config file parser
│   │   ├── ConfigWebServer.cpp/h        # WiFi HTTP API endpoints
│   │   ├── ConnectionStateMachine.cpp/h # WiFi state machine
│   │   ├── WiFiConfigFile.h             # WiFi config data structure
│   │   ├── WiFiConnectionState.h        # WiFi connection state structure
│   │   ├── WiFiCredentials.h            # WiFi credentials structure
│   │   ├── BoatData.cpp/h               # Central marine data repository
│   │   ├── BoatDataSerializer.cpp/h     # JSON serialization for WebSocket streaming
│   │   ├── SourcePrioritizer.cpp/h      # Multi-source GPS/compass prioritization
│   │   ├── CalculationEngine.cpp/h      # Derived sailing parameters
│   │   ├── CalibrationManager.cpp/h     # Persistent calibration storage
│   │   ├── CalibrationWebServer.cpp/h   # Calibration HTTP API
│   │   ├── DisplayManager.cpp/h         # OLED display orchestration
│   │   ├── MetricsCollector.cpp/h       # System metrics collection
│   │   ├── DisplayFormatter.h           # Display string formatting
│   │   ├── StartupProgressTracker.cpp/h # Subsystem initialization tracking
│   │   ├── SourceRegistry.cpp/h         # NMEA source statistics registry
│   │   ├── SourceStatistics.h           # Source statistics data structures
│   │   ├── SourceStatsSerializer.cpp/h  # JSON serialization for source stats
│   │   └── SourceStatsHandler.cpp/h     # WebSocket handler for source stats
│   ├── utils/                           # Utility functions
│   │   ├── WebSocketLogger.cpp/h        # WebSocket-based logging
│   │   ├── TimeoutManager.cpp/h         # ReactESP timeout tracking
│   │   ├── DataValidator.h              # Marine data validation
│   │   ├── AngleUtils.h                 # Angle normalization utilities
│   │   ├── DisplayLayout.h              # OLED display layout utilities
│   │   ├── FrequencyCalculator.cpp/h    # Frequency calculation utility
│   │   └── LogEnums.h                   # Logging enumerations
│   ├── types/                           # Type definitions
│   │   ├── BoatDataTypes.h              # Marine data structures
│   │   └── DisplayTypes.h               # OLED display data structures
│   ├── mocks/                           # Mock implementations for testing
│   │   ├── MockWiFiAdapter.cpp/h        # Mock WiFi for unit tests
│   │   ├── MockFileSystem.cpp/h         # Mock filesystem for unit tests
│   │   ├── MockBoatDataStore.h          # Mock BoatData for unit tests
│   │   ├── MockSourcePrioritizer.h      # Mock prioritizer for unit tests
│   │   ├── MockCalibration.h            # Mock calibration for unit tests
│   │   ├── MockDisplayAdapter.h         # Mock OLED display for unit tests
│   │   └── MockSystemMetrics.h          # Mock system metrics for unit tests
│   └── helpers/                         # Development tools
│       ├── ws_logger.py                 # WebSocket log client (Python)
│       └── websocket_env/               # Python virtual environment
├── test/                                # PlatformIO grouped tests
│   ├── helpers/                         # Shared test utilities
│   │   ├── test_mocks.h                 # Test mock implementations
│   │   ├── test_fixtures.h              # Test data fixtures
│   │   └── test_utilities.h             # Common test helpers
│   ├── test_wifi_units/                 # WiFi unit tests (native)
│   ├── test_wifi_integration/           # WiFi integration tests (native)
│   ├── test_wifi_endpoints/             # WiFi HTTP API tests (native)
│   ├── test_wifi_contracts/             # WiFi HAL contract tests (native)
│   ├── test_wifi_connection/            # WiFi hardware tests (ESP32)
│   ├── test_boatdata_contracts/         # BoatData HAL contract tests (native)
│   ├── test_boatdata_integration/       # BoatData integration tests (native)
│   ├── test_boatdata_units/             # BoatData unit tests (native)
│   ├── test_boatdata_hardware/          # BoatData hardware tests (ESP32)
│   ├── test_oled_contracts/             # OLED HAL contract tests (native)
│   ├── test_oled_integration/           # OLED integration tests (native)
│   ├── test_oled_units/                 # OLED unit tests (native)
│   ├── test_oled_hardware/              # OLED hardware tests (ESP32)
│   ├── test_source_stats_units/         # Source statistics unit tests (native)
│   ├── test_source_stats_contracts/     # Source statistics contract tests (native)
│   └── test_source_stats_integration/   # Source statistics integration tests (native)
├── data/                                # LittleFS filesystem files
│   ├── stream.html                      # WebUI dashboard (served from ESP32)
│   ├── sources.html                     # Source statistics dashboard (served from ESP32)
│   ├── calibration.json                 # Default calibration parameters
│   └── log-filter.json                  # WebSocket logging filter config
├── nodejs-boatdata-viewer/              # Node.js WebSocket proxy server
│   ├── server.js                        # WebSocket relay and HTTP server
│   ├── config.json                      # ESP32 IP/port configuration
│   ├── package.json                     # Node.js dependencies
│   ├── public/
│   │   ├── stream.html                  # BoatData dashboard (identical to ESP32 version)
│   │   └── sources.html                 # Source statistics dashboard (identical to ESP32 version)
│   └── README.md                        # Proxy server documentation
├── examples/poseidongw/                 # Reference implementation
├── specs/                               # Feature specifications
│   ├── 001-create-feature-spec/         # WiFi management spec
│   ├── 002-create-feature-spec/         # (deprecated)
│   ├── 003-boatdata-feature-as/         # BoatData feature spec
│   ├── 004-removal-of-udp/              # UDP removal documentation
│   ├── 005-oled-basic-info/             # OLED display feature spec
│   ├── 008-enhanced-boatdata-following/ # Enhanced BoatData spec (R005)
│   ├── 011-simple-webui-as/             # WebUI dashboard spec (R009)
│   └── 012-sources-stats-and/           # Source statistics tracking spec (R010)
├── user_requirements/                   # User requirements
│   ├── R001 - foundation.md             # Core requirements
│   ├── R002 - boatdata.md               # BoatData requirements
│   ├── R003 - cleanup udp leftovers.md  # UDP cleanup requirements
│   ├── R004 - OLED basic info.md        # OLED display requirements
│   ├── R005 - enhanced boatdata.md      # Enhanced BoatData requirements
│   ├── R007 - NMEA 0183 data.md         # NMEA 0183 handlers
│   ├── R009 - webui.md                  # WebUI dashboard requirements
│   └── R010 - source statistics.md      # Source statistics tracking requirements
├── .specify/                            # Development framework
│   ├── memory/
│   │   └── constitution.md              # Development principles (v1.2.0)
│   └── templates/
│       ├── spec-template.md             # Feature specification template
│       ├── plan-template.md             # Implementation planning template
│       └── tasks-template.md            # Task breakdown template
└── platformio.ini                       # PlatformIO configuration
```

### Building

```bash
# Build for ESP32
pio run -e esp32dev

# Build and upload
pio run -e esp32dev -t upload

# Monitor serial output
pio run -e esp32dev -t monitor

# Clean build
pio run -t clean
```

### Testing

#### Unit Tests (No Hardware Required)
```bash
# Run all unit tests on native platform
pio test -e native

# Run specific test
pio test -e native -f test_config_parser
```

#### Integration Tests (No Hardware Required)
```bash
# All integration tests (mocked WiFi/filesystem)
pio test -e native -f test_first_time_config
pio test -e native -f test_network_failover
pio test -e native -f test_connection_loss_recovery
```

#### Hardware Tests (ESP32 Required)
```bash
# IMPORTANT: Update WiFi credentials in test/test_wifi_connection/test_main.cpp first!
pio test -e esp32dev_test -f test_wifi_connection

# NMEA 0183 hardware validation (requires Serial2 connection)
pio test -e esp32dev_test -f test_nmea0183_hardware
```

See [`test/test_wifi_connection/README.md`](test/test_wifi_connection/README.md) for hardware test setup.

#### NMEA 0183 Tests
```bash
# All NMEA 0183 tests (contracts, integration, units - no hardware)
pio test -e native -f test_nmea0183

# Specific test groups
pio test -e native -f test_nmea0183_contracts    # HAL interface validation
pio test -e native -f test_nmea0183_integration  # End-to-end scenarios
pio test -e native -f test_nmea0183_units        # Unit conversions, parsers
```

### WebSocket Debug Logging

All WiFi and system events are logged via WebSocket for reliable debugging:

```bash
# Activate Python virtual environment
source src/helpers/websocket_env/bin/activate

# Connect to WebSocket logs
python3 src/helpers/ws_logger.py <ESP32_IP>

# With log filtering
python3 src/helpers/ws_logger.py <ESP32_IP> --filter WARN

# With auto-reconnect
python3 src/helpers/ws_logger.py <ESP32_IP> --reconnect
```

**WebSocket Endpoint**: `ws://<device-ip>/logs`
**Protocol**: TCP-based (reliable delivery, no packet loss)
**Client**: Python script at `src/helpers/ws_logger.py`

**Example Log Output**:
```json
{"timestamp":1234567890,"level":"INFO","component":"WiFiManager","event":"CONNECTION_ATTEMPT","data":{"ssid":"HomeNetwork","attempt":1,"timeout_seconds":30}}
{"timestamp":1234567895,"level":"INFO","component":"WiFiManager","event":"CONNECTION_SUCCESS","data":{"ssid":"HomeNetwork","ip":"192.168.1.100"}}
```

## More logging 

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

## Troubleshooting

### Device Not Connecting to WiFi

**Symptoms**: Reboot loop, no connection

**Solutions**:
1. Verify `wifi.conf` format (SSID,password per line)
2. Check SSID and password are correct
3. Ensure network is 2.4 GHz (ESP32 classic doesn't support 5 GHz)
4. Verify device is within WiFi range
5. Monitor WebSocket logs for detailed error messages:
   ```bash
   source src/helpers/websocket_env/bin/activate
   python3 src/helpers/ws_logger.py <ip>
   ```

### Configuration Upload Fails (400 Error)

**Symptoms**: HTTP 400 with validation errors

**Solutions**:
1. SSID must be 1-32 characters
2. Password must be 0 (open) or 8-63 characters (WPA2)
3. Maximum 3 networks allowed
4. No newline characters in SSID/password
5. Use comma separator (not space or tab)

### Device Reboots After Config Upload

**Expected Behavior**: Device reboots 5 seconds after successful config upload to apply new settings.

### WebSocket Logs Not Received

**Cause**: Network configuration, firewall, or WebSocket connection issues

**Solutions**:
1. Verify device and computer on same network segment
2. Check firewall allows HTTP port 80 (WebSocket upgrade)
3. Ensure device has WiFi connection (logs buffer until connected)
4. Activate Python virtual environment: `source src/helpers/websocket_env/bin/activate`
5. Check device IP address is correct

### LittleFS Mount Failed

**Symptoms**: Serial output shows "Failed to mount LittleFS"

**Solutions**:
```bash
# Erase flash and reflash
pio run -t erase
pio run -t upload
pio run -t uploadfs
```

## Performance & Resource Usage

### Memory Footprint
- **WiFi Config**: ~334 bytes RAM (3 networks)
- **BoatData v2.0**: ~560 bytes RAM (enhanced structures, static allocation)
- **OLED Display**: ~1.1 KB RAM (97 bytes + 1KB framebuffer)
- **WebUI/Serialization**: ~3 KB RAM (2KB JSON buffer + WebSocket overhead)
- **1-Wire Sensors**: ~150 bytes RAM (event loop overhead)
- **Flash Storage**: ~200 bytes (config files)
- **Code Size**: ~1046 KB flash (53.2% of 1.9 MB partition)
- **RAM Usage**: ~45 KB (13.8% of 320 KB)

### Timing Benchmarks
- **Boot to services ready**: < 2 seconds
- **First network connection**: < 30 seconds
- **Config file parsing**: < 50 ms
- **File I/O (LittleFS)**: < 100 ms

## Contributing

Contributions welcome! Please:
1. Follow the project constitution (`.specify/memory/constitution.md`)
2. Use HAL abstraction for hardware dependencies
3. Add tests for new features (TDD approach)
4. Run QA review before submitting PR
5. Use conventional commits format

## License

MIT License - see [LICENSE](LICENSE) file for details.

## Acknowledgments

- **Libraries**:
  - [NMEA2000](https://github.com/ttlappalainen/NMEA2000) by Timo Lappalainen
  - [NMEA0183](https://github.com/ttlappalainen/NMEA0183) by Timo Lappalainen
  - [ReactESP](https://github.com/mairas/ReactESP) by Matti Airas
  - [ESPAsyncWebServer](https://github.com/ESP32Async/ESPAsyncWebServer)

- **Hardware**: SH-ESP32 board design
- **Testing**: PlatformIO + Unity framework

## Support

- **Documentation**: See [`docs/`](docs/) directory
- **Issues**: [GitHub Issues](https://github.com/yourusername/Poseidon2/issues)
- **Discussions**: [GitHub Discussions](https://github.com/yourusername/Poseidon2/discussions)

---

**Status**: ✅ WiFi Management | ✅ OLED Display | ✅ Loop Frequency Monitoring | ✅ BoatData | ✅ Enhanced BoatData (R005) | ✅ NMEA 0183 | ✅ WebUI Dashboard (R009) | ✅ Source Statistics (R010) | 🚧 NMEA 2000 In Progress

**Last Updated**: 2025-10-13
**Version**: 1.3.0 (WiFi Management + OLED Display + WebUI Dashboard + Source Statistics)
