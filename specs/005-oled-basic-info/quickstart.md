# Quickstart: OLED Basic Info Display

**Feature**: OLED Basic Info Display
**Date**: 2025-10-08
**Purpose**: Manual and automated validation steps to verify OLED display functionality

---

## Prerequisites

### Hardware
- ESP32 development board (SH-ESP32 recommended)
- 128x64 SSD1306 OLED display module (I2C interface)
- I2C connection:
  - OLED SDA → ESP32 GPIO21
  - OLED SCL → ESP32 GPIO22
  - OLED VCC → 3.3V
  - OLED GND → GND

### Software
- PlatformIO Core installed
- USB cable for ESP32 connection
- WiFi network with known SSID/password configured in `/data/wifi.conf`

### Verification
```bash
# Verify PlatformIO installation
pio --version

# Verify ESP32 connection
pio device list

# Verify wifi.conf exists
ls -l /home/niels/Dev/Poseidon2/data/wifi.conf
```

---

## Manual Test Steps

### Step 1: Flash Firmware

```bash
cd /home/niels/Dev/Poseidon2

# Build and upload firmware
pio run -e esp32dev -t upload

# Expected output:
# - Compiling source files
# - Linking firmware image
# - Uploading to ESP32
# - "SUCCESS" message
```

### Step 2: Observe Startup Sequence

**Power on the ESP32 and observe the OLED display:**

**Frame 1 (t=0s)**: Boot screen
```
+----------------------------+
| Poseidon2 Gateway          |
| Booting...                 |
| [ ] WiFi                   |
| [ ] Filesystem             |
| [ ] WebServer              |
|                            |
+----------------------------+
```

**Frame 2 (t=0-30s)**: WiFi connecting
```
+----------------------------+
| Poseidon2 Gateway          |
| Booting...                 |
| [~] WiFi: MyHomeNetwork    |
| Connecting... 5s           | ← Timer increments
| [ ] Filesystem             |
| [ ] WebServer              |
+----------------------------+
```

**Frame 3 (t=5-15s)**: WiFi connected, FS mounting
```
+----------------------------+
| Poseidon2 Gateway          |
| Booting...                 |
| [✓] WiFi: Connected        |
| IP: 192.168.1.100          |
| [~] Filesystem: Mounting   |
| [ ] WebServer              |
+----------------------------+
```

**Frame 4 (t=15-20s)**: All subsystems started
```
+----------------------------+
| Poseidon2 Gateway          |
| Booting complete!          |
| [✓] WiFi: Connected        |
| [✓] Filesystem: Mounted    |
| [✓] WebServer: Running     |
|                            |
+----------------------------+
```

**Expected Results**:
- ✓ Boot screen appears within 1 second of power-on
- ✓ WiFi status shows "Connecting..." with incrementing timer
- ✓ WiFi connects within 30 seconds (or shows "FAILED" if no network available)
- ✓ Filesystem shows "Mounting" → "Mounted" (or "FAILED" if LittleFS issue)
- ✓ WebServer shows "Starting" → "Running"

### Step 3: Verify Runtime Status Display

**After boot complete, display transitions to runtime status:**

```
+----------------------------+
| WiFi: MyHomeNetwork        | ← SSID (or "Disconnected")
| IP: 192.168.1.100          | ← IP address
| RAM: 245KB                 | ← Free heap memory
| Flash: 850/1920KB          | ← Sketch size / Total flash
| CPU Idle: 87%              | ← FreeRTOS idle %
| [ / ]                      | ← Rotating icon (corner)
+----------------------------+
```

**Expected Results**:
- ✓ WiFi SSID and IP address displayed correctly
- ✓ RAM value is reasonable (50-280KB typical)
- ✓ Flash usage matches build output (~800-1000KB typical)
- ✓ CPU idle % is 70-95% (system lightly loaded)
- ✓ Rotating icon appears in lower right corner

### Step 4: Verify Animation Update (1 Second)

**Observe the rotating icon in the corner:**

- t=0s: `[ / ]`
- t=1s: `[ - ]`
- t=2s: `[ \ ]`
- t=3s: `[ | ]`
- t=4s: `[ / ]` (cycles back)

**Expected Results**:
- ✓ Icon changes every 1 second
- ✓ Smooth cycling through 4 states (/, -, \, |)
- ✓ No visible flicker or tearing

### Step 5: Verify Status Update (5 Seconds)

**Wait 5 seconds and observe RAM/CPU values:**

- Values should update every 5 seconds
- RAM may fluctuate slightly (±10KB typical)
- CPU idle % may vary (70-95% typical)

**Expected Results**:
- ✓ RAM value updates every 5 seconds
- ✓ Flash values remain constant (no update needed)
- ✓ CPU idle % updates every 5 seconds
- ✓ No visible lag or blocking during updates

### Step 6: Test WiFi Disconnect Handling

**Disconnect WiFi access point (power off router or block MAC address):**

**Within 1 second**, display should update:
```
+----------------------------+
| WiFi: Disconnected         | ← Status changed
| IP: ---                    | ← IP cleared
| RAM: 245KB                 |
| Flash: 850/1920KB          |
| CPU Idle: 87%              |
| [ - ]                      |
+----------------------------+
```

**Expected Results**:
- ✓ Display shows "Disconnected" within 1 second (FR-009, FR-017)
- ✓ IP address cleared or shows "---"
- ✓ RAM/CPU metrics continue updating (system still running)
- ✓ Rotating icon continues animating (system responsive)

### Step 7: Test WiFi Reconnect

**Reconnect WiFi access point:**

**Display should show reconnection progress:**
```
+----------------------------+
| WiFi: Connecting...        |
| Attempting... 3s           | ← Timer increments
| RAM: 245KB                 |
| Flash: 850/1920KB          |
| CPU Idle: 87%              |
| [ \ ]                      |
+----------------------------+
```

**After connection succeeds:**
```
+----------------------------+
| WiFi: MyHomeNetwork        | ← Back to connected
| IP: 192.168.1.100          | ← IP restored
| RAM: 245KB                 |
| Flash: 850/1920KB          |
| CPU Idle: 87%              |
| [ | ]                      |
+----------------------------+
```

**Expected Results**:
- ✓ Display shows "Connecting..." with timer
- ✓ Connection restores within 30 seconds
- ✓ SSID and IP reappear after successful reconnect
- ✓ System continues operating during reconnection (graceful handling)

### Step 8: Test Graceful Degradation (Optional)

**Disconnect OLED hardware (unplug I2C wires) and reboot:**

**Expected Results**:
- ✓ System boots normally (no crash or hang)
- ✓ WebSocket logs show "OLED init failed" ERROR message
- ✓ WiFi, filesystem, web server continue operating
- ✓ Device accessible via WebSocket logs and HTTP API

**WebSocket log output**:
```bash
source src/helpers/websocket_env/bin/activate
python3 src/helpers/ws_logger.py <device-ip>

# Expected log entry:
{"level":"ERROR","component":"DisplayAdapter","event":"INIT_FAILED","message":"I2C communication error"}
```

---

## Automated Quickstart Test

### Test: test_quickstart_validation.cpp

**Location**: `test/test_oled_integration/test_quickstart_validation.cpp`

**Purpose**: Automated verification of quickstart steps 1-7 using mocked hardware

**Test Cases**:
1. **Boot Sequence**: Verify startup progress display for WiFi, FS, web server
2. **Status Display**: Verify runtime status shows WiFi, RAM, flash, CPU, animation
3. **Animation Cycle**: Verify icon updates every 1 second (/, -, \, |)
4. **Status Refresh**: Verify metrics update every 5 seconds
5. **WiFi Disconnect**: Verify display updates within 1 second
6. **WiFi Reconnect**: Verify reconnection progress and status restoration

**Run Test**:
```bash
cd /home/niels/Dev/Poseidon2

# Run quickstart validation test (native platform, mocked hardware)
pio test -e native -f test_oled_integration -v

# Expected output:
# - test_quickstart_validation.cpp: PASSED
# - All 6 test cases passed
```

**Test Implementation Pattern**:
```cpp
#include <unity.h>
#include "mocks/MockDisplayAdapter.h"
#include "mocks/MockSystemMetrics.h"
#include "components/DisplayManager.h"

void test_boot_sequence() {
    MockDisplayAdapter mockDisplay;
    MockSystemMetrics mockMetrics;
    DisplayManager manager(&mockDisplay, &mockMetrics);

    // Simulate boot sequence
    SubsystemStatus status = {CONN_CONNECTING, "MyHomeNetwork", "", FS_MOUNTING, WS_STARTING, 0, 0, 0};
    manager.renderStartupProgress(status);

    // Verify display calls
    TEST_ASSERT_TRUE(mockDisplay.wasCleared());
    TEST_ASSERT_TRUE(mockDisplay.wasTextRendered("WiFi: MyHomeNetwork"));
    TEST_ASSERT_TRUE(mockDisplay.wasTextRendered("Connecting..."));
}
```

---

## Troubleshooting

### Issue: OLED display remains blank

**Causes**:
- I2C wiring incorrect (check SDA=21, SCL=22)
- OLED I2C address mismatch (try 0x3D instead of 0x3C)
- OLED module faulty or requires external power

**Diagnosis**:
```bash
# Check WebSocket logs for I2C errors
source src/helpers/websocket_env/bin/activate
python3 src/helpers/ws_logger.py <device-ip>

# Look for:
{"level":"ERROR","component":"DisplayAdapter","event":"INIT_FAILED"}
```

**Solutions**:
1. Verify I2C wiring with multimeter (continuity test)
2. Try alternate I2C address: Change `0x3C` to `0x3D` in ESP32DisplayAdapter.cpp
3. Test OLED module with Arduino I2C scanner sketch

### Issue: Display flickers or shows garbage

**Causes**:
- I2C bus speed too fast (interference or long wires)
- Framebuffer corruption (memory issue)

**Solutions**:
1. Reduce I2C clock speed: Change `400000` to `100000` in ESP32DisplayAdapter.cpp
2. Add pull-up resistors (4.7kΩ) on SDA and SCL lines
3. Check ESP32 RAM usage (ensure no heap exhaustion)

### Issue: Animation stops or freezes

**Causes**:
- ReactESP event loop blocked (long-running callback)
- System crash or watchdog trigger

**Diagnosis**:
```bash
# Monitor WebSocket logs for WARN/ERROR
python3 src/helpers/ws_logger.py <device-ip> --filter WARN

# Check for:
{"level":"WARN","component":"CalculationEngine","event":"OVERRUN"}
```

**Solutions**:
1. Verify `app.tick()` is called frequently in main loop (every 10ms)
2. Check for blocking operations in ReactESP callbacks (should be <10ms)
3. Review WebSocket logs for watchdog resets

### Issue: Status values incorrect or frozen

**Causes**:
- FreeRTOS stats disabled (CPU idle % shows 0%)
- Metrics collection failing (ISystemMetrics error)

**Solutions**:
1. Verify `CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS=y` in sdkconfig
2. Check WebSocket logs for MetricsCollector errors
3. Validate ESP.getFreeHeap() / ESP.getSketchSize() return reasonable values

---

## Success Criteria

**All manual test steps (1-7) pass:**
- ✓ Startup sequence displays subsystem status
- ✓ Runtime status shows WiFi, RAM, flash, CPU, animation
- ✓ Animation updates every 1 second
- ✓ Status updates every 5 seconds
- ✓ WiFi disconnect/reconnect handled gracefully
- ✓ Graceful degradation if OLED fails to initialize

**Automated quickstart test passes:**
- ✓ `pio test -e native -f test_oled_integration` exits with code 0
- ✓ All 6 test cases in test_quickstart_validation.cpp pass

**Performance validation:**
- ✓ Display updates complete within 50ms (no visible lag)
- ✓ System remains responsive during display updates (rotating icon smooth)
- ✓ RAM usage stable (~1.1KB for display, acceptable)

---

*Quickstart validation guide: 2025-10-08*
