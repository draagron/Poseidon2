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
- 🚧 **NMEA 2000** (CAN bus): Bidirectional message handling
- 🚧 **NMEA 0183** (Serial): Read-only sentence parsing
- 🚧 **SignalK Integration**: Real-time data streaming
- 🚧 **1-Wire Sensors**: Temperature, humidity monitoring

### System Features
- ✅ **Always-On Operation**: No sleep modes, 24/7 uptime
- ✅ **WebSocket Logging**: Reliable network-based debugging
- ✅ **Hardware Abstraction**: Testable via mocks (HAL pattern)
- ✅ **ReactESP**: Event-driven architecture for responsive operation
- 🚧 **OLED Display**: Real-time status and diagnostics
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

## Development

### Project Structure
```
Poseidon2/
├── src/
│   ├── main.cpp                    # Entry point
│   ├── config.h                    # Compile-time configuration
│   ├── hal/                        # Hardware Abstraction Layer
│   │   ├── interfaces/             # HAL interfaces (IWiFiAdapter, IFileSystem)
│   │   └── implementations/        # ESP32-specific implementations
│   ├── components/                 # Feature components
│   │   ├── WiFiManager.cpp         # WiFi connection orchestrator
│   │   ├── ConfigParser.cpp        # Config file parser
│   │   ├── ConnectionStateMachine.cpp # State: DISCONNECTED→CONNECTING→CONNECTED
│   │   └── ConfigWebServer.cpp     # HTTP API endpoints
│   ├── utils/                      # Utility functions
│   │   ├── WebSocketLogger.cpp          # WebSocket logging
│   │   └── TimeoutManager.cpp     # ReactESP timeout tracking
│   └── mocks/                      # Mock implementations for testing
├── test/                           # PlatformIO tests
│   ├── unit/                       # Unit tests (mocked, native)
│   ├── integration/                # Integration tests (mocked)
│   └── test_wifi_connection/       # Hardware tests (ESP32 required)
├── examples/poseidongw/            # Reference implementation
└── platformio.ini                  # PlatformIO configuration
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
```

See [`test/test_wifi_connection/README.md`](test/test_wifi_connection/README.md) for hardware test setup.

### WebSocket Debug Logging

All WiFi and system events are logged via WebSocket for reliable debugging:

```bash
# Connect to WebSocket logs (requires Python 3 + websockets library)
pip3 install websockets
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

## Troubleshooting

### Device Not Connecting to WiFi

**Symptoms**: Reboot loop, no connection

**Solutions**:
1. Verify `wifi.conf` format (SSID,password per line)
2. Check SSID and password are correct
3. Ensure network is 2.4 GHz (ESP32 classic doesn't support 5 GHz)
4. Verify device is within WiFi range
5. Monitor WebSocket logs for detailed error messages: `python3 src/helpers/ws_logger.py <ip>`

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
4. Verify Python websockets library installed: `pip3 install websockets`
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
- **Flash Storage**: ~200 bytes (config file)
- **Code Size**: ~850 KB flash (43% of 1.9 MB partition)
- **RAM Usage**: ~45 KB (13.9% of 320 KB)

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

**Status**: ✅ WiFi Management Complete | 🚧 Marine Protocols In Progress

**Last Updated**: 2025-10-06
**Version**: 1.0.0 (WiFi Management Foundation)
