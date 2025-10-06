# Hardware Tests - ESP32 WiFi Connection

This directory contains **minimal hardware tests** that require actual ESP32 hardware and real WiFi networks.

## Constitutional Principle

As per the project constitution:
> "Hardware-dependent tests kept minimal"

The WiFi management feature has **only ONE** hardware test file (`test_wifi_connection.cpp`). All other tests use mocks and can run in the native environment without hardware.

## Prerequisites

### Hardware
- ESP32 development board (ESP32, ESP32-S2, ESP32-C3, or ESP32-S3)
- USB cable for connection
- Computer with PlatformIO installed

### WiFi Networks
You need access to at least one WiFi network for testing:
- **Primary test network**: For basic connection tests
- **Secondary test network** (optional): For failover testing

### Software
- PlatformIO Core or PlatformIO IDE
- USB drivers for ESP32 (CH340, CP2102, etc.)

## Configuration

### Step 1: Edit Test Credentials

Open `test/test_wifi_connection/test_main.cpp` and modify the test configuration section:

```cpp
// Lines 40-45: Update these with your WiFi credentials
#define TEST_WIFI_SSID     "YourTestNetworkSSID"
#define TEST_WIFI_PASSWORD "YourTestNetworkPassword"

// Optional: Secondary network for failover testing
#define TEST_WIFI_SSID_2     "SecondaryTestNetwork"
#define TEST_WIFI_PASSWORD_2 "SecondaryPassword"
```

**Security Note**: Consider using a dedicated test network or guest network instead of your primary WiFi network.

### Step 2: Connect ESP32

1. Connect ESP32 to your computer via USB
2. Verify the device is recognized:
   ```bash
   pio device list
   ```

## Running Hardware Tests

### Method 1: PlatformIO Test Framework (Recommended)

Run the hardware test from the repository root:
```bash
pio test -e esp32dev -f test_wifi_connection
```

Or run all tests (if you have multiple):
```bash
pio test -e esp32dev
```

### Method 2: PlatformIO IDE

1. Open PlatformIO IDE
2. Navigate to the PlatformIO sidebar
3. Select "Test" → "esp32dev"
4. Click "Run Test" for `test_wifi_connection`

### Method 3: Verbose Output (for debugging)

If you encounter issues, use verbose mode:
```bash
pio test -e esp32dev -f test_wifi_connection -vvv
```

## Test Coverage

The hardware test validates:

1. **WiFi Adapter Initialization**
   - ESP32WiFiAdapter creation
   - Initial status validation

2. **Actual WiFi Connection**
   - Connection to real network
   - IP address assignment
   - RSSI (signal strength) reading
   - SSID verification

3. **Disconnect and Reconnect**
   - WiFi disconnect functionality
   - Reconnection after manual disconnect
   - Status transitions

4. **Connection Timeout**
   - Timeout behavior with non-existent network
   - Timeout duration (30 seconds)

5. **LittleFS Operations**
   - File write to flash
   - File read from flash
   - File existence check
   - File deletion

6. **Config Persistence**
   - Save WiFi configuration to flash
   - Load WiFi configuration from flash
   - Verify data integrity

## Expected Output

Successful test run output:
```
========================================
ESP32 WiFi Hardware Test
========================================
Hardware test setup complete

--- Test: WiFi Adapter Initialization ---
Initial WiFi status: 0
✓ WiFi adapter initialized correctly

--- Test: Actual WiFi Connection ---
Attempting connection to: YourTestNetworkSSID
Waiting for connection (max 30 seconds)...
✓ Connected successfully!
  SSID: YourTestNetworkSSID
  IP Address: 192.168.1.100
  Signal Strength: -45 dBm

--- Test: Disconnect and Reconnect ---
Disconnecting...
Status after disconnect: 6
Reconnecting...
✓ Disconnect and reconnect successful

--- Test: LittleFS Operations ---
✓ LittleFS operations successful

--- Test: Config Persistence ---
✓ Config persistence successful

--- Test: Connection Timeout ---
Attempting connection to non-existent network...
Elapsed time: 30124 ms
✓ Timeout behavior correct
Reconnecting to valid network...

Hardware test cleanup complete

-----------------------
6 Tests 0 Failures 0 Ignored
OK
```

## Troubleshooting

### Test Fails: "WiFi should connect within 30 seconds"

**Cause**: Cannot connect to configured WiFi network

**Solutions**:
1. Verify SSID and password are correct in `test_wifi_connection.cpp`
2. Check that ESP32 is within range of the WiFi network
3. Verify the network is 2.4 GHz (ESP32 classic doesn't support 5 GHz)
4. Check if network requires additional authentication (enterprise WPA2)

### Test Fails: "LittleFS mount failed"

**Cause**: Flash filesystem not formatted or corrupted

**Solutions**:
1. Format LittleFS partition:
   ```bash
   pio run -e esp32dev -t uploadfs
   ```
2. Or erase flash completely:
   ```bash
   pio run -e esp32dev -t erase
   ```
3. Then re-upload test firmware

### Device Not Found

**Cause**: ESP32 not recognized by computer

**Solutions**:
1. Install USB drivers (CH340 or CP2102 depending on board)
2. Try a different USB cable (must support data transfer)
3. Check Device Manager (Windows) or `ls /dev/tty*` (macOS/Linux)
4. Try a different USB port

### Upload Fails: "Permission denied"

**Cause**: Serial port locked by another application

**Solutions**:
1. Close any serial monitors or terminals
2. Close PlatformIO monitor if running
3. Disconnect and reconnect ESP32
4. On Linux, add user to `dialout` group:
   ```bash
   sudo usermod -a -G dialout $USER
   ```
   (logout and login required)

## Test Duration

- **Total test time**: ~2-3 minutes
  - Connection tests: ~60 seconds
  - Timeout test: ~30 seconds
  - Other tests: ~30 seconds

## Continuous Integration

Hardware tests are **not run** in CI/CD pipelines because they require:
- Physical ESP32 hardware
- Real WiFi network access
- Manual configuration

Run hardware tests:
- Before major releases
- After hardware-related code changes
- During manual QA validation

For CI/CD, use the **unit tests** (mocked) and **integration tests** instead:
```bash
# Unit tests (no hardware required)
pio test -e native

# Integration tests (no hardware required)
pio test -e native --filter "test_*_config" --filter "test_*_failover"
```

## Additional Notes

### Test Isolation

Each test is independent and cleans up after itself:
- WiFi is disconnected after tests
- Test files are deleted from LittleFS
- State is reset between tests

### Network Impact

Tests perform minimal network operations:
- Connection/disconnection cycles
- UDP logging (if enabled)
- No large data transfers
- No internet connectivity required (local WiFi only)

### Safety

Tests are safe to run repeatedly:
- No destructive operations on ESP32
- No persistent state changes
- Flash wear is minimal (< 10 write cycles per test run)

---

**Last Updated**: 2025-10-06
**Test Version**: 1.0.0
**Compatible with**: ESP32 Arduino Core 2.x+
