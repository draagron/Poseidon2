# Quickstart: MCU Loop Frequency Display Validation

**Feature**: MCU Loop Frequency Display
**Date**: 2025-10-10
**Estimated Time**: 10 minutes

## Prerequisites

### Hardware
- ✅ ESP32 device (SH-ESP32 board) with OLED display connected
- ✅ USB cable for programming and serial monitoring
- ✅ OLED display: SSD1306, 128x64, I2C address 0x3C

### Software
- ✅ PlatformIO CLI installed
- ✅ Feature branch checked out: `006-mcu-loop-frequency`
- ✅ All dependencies installed (Adafruit_SSD1306, ReactESP, etc.)

### Network
- ✅ WiFi network credentials configured in `/wifi.conf`
- ✅ Device and computer on same network for WebSocket logging

## Step 1: Build and Upload Firmware

```bash
# Navigate to project root
cd /home/niels/Dev/Poseidon2

# Clean build
pio run -t clean

# Build for ESP32
pio run

# Upload to device
pio run -t upload

# Optional: Monitor serial output during boot
pio run -t monitor
```

**Expected Output**:
```
Poseidon2 WiFi Gateway - Initializing...
LittleFS mounted successfully
WiFi config loaded: X networks
Initializing OLED display...
OLED display initialized successfully
Setup complete - entering main loop
```

**Success Criteria**: Build completes without errors, firmware uploads successfully

## Step 2: Verify Display Shows Placeholder During First 5 Seconds

**Action**: Observe OLED display immediately after device boots

**Expected Display** (within first 5 seconds):
```
WiFi: <Connecting...>
IP: ---
RAM: XXX KB
Flash: XXX/XXX KB
Loop: --- Hz           ← VERIFY: Shows "---" placeholder
<animation icon>
```

**Acceptance Criteria**:
- ✅ Line 4 shows "Loop: --- Hz" (not "CPU Idle: 85%")
- ✅ No numeric frequency displayed yet
- ✅ Display updates smoothly (no crashes)

**Failure Modes**:
- ❌ Display shows "CPU Idle: 85%" → Old code still active
- ❌ Display shows "Loop: 0 Hz" → Should show "---" before first measurement
- ❌ Line 4 blank or garbled → String formatting error

## Step 3: Verify Frequency Appears After 5 Seconds

**Action**: Continue observing display for 5+ seconds

**Expected Display** (after 5 seconds):
```
WiFi: <SSID>
IP: 192.168.X.X
RAM: XXX KB
Flash: XXX/XXX KB
Loop: XXX Hz           ← VERIFY: Shows numeric frequency
<animation icon>
```

**Typical Frequency Range**:
- **Normal**: 100-500 Hz (ESP32 with ReactESP event loops)
- **Warning**: < 50 Hz (potential performance issue)
- **Critical**: < 10 Hz (system overload)

**Acceptance Criteria**:
- ✅ Line 4 shows "Loop: [numeric value] Hz"
- ✅ Frequency is non-zero (> 0 Hz)
- ✅ Frequency updates every 5 seconds
- ✅ Other metrics (WiFi, RAM, Flash) unaffected

**Failure Modes**:
- ❌ Still shows "---" after 10 seconds → Measurement not calculating
- ❌ Shows "Loop: 0 Hz" → Loop hang or calculation error
- ❌ Frequency > 10000 Hz → Unrealistic value, counter overflow?

## Step 4: Verify Frequency Updates Every 5 Seconds

**Action**: Observe display for 30 seconds, watch Line 4 frequency value

**Expected Behavior**:
- Frequency value changes every 5 seconds (±0.5 sec)
- Value stays within 100-500 Hz range (typical)
- Animation icon rotates every 1 second (independent timing)

**Test Cases**:
1. **Idle system**: Frequency should be relatively stable (±10 Hz)
2. **Load test** (optional): Press button, trigger WiFi reconnect → frequency may decrease temporarily
3. **Long-term stability**: Run for 5 minutes → no crashes, continuous updates

**Acceptance Criteria**:
- ✅ Frequency updates at 5-second intervals
- ✅ Display doesn't freeze or reset
- ✅ Frequency values are reasonable (10-2000 Hz range)

**Failure Modes**:
- ❌ Frequency never changes → Stuck at first measurement
- ❌ Frequency changes every 1 second → Wrong update interval
- ❌ Device reboots periodically → Watchdog timeout or crash

## Step 5: Verify Display Format for Edge Cases

### Test Case 5A: Normal Frequency (100-999 Hz)

**Expected Format**: `"Loop: 212 Hz"` (3 digits, no decimal)

✅ **PASS**: Frequency displays as integer
❌ **FAIL**: Shows decimal places or "k" abbreviation

### Test Case 5B: Low Frequency (< 10 Hz)

**Trigger**: Cause system overload (not recommended for production validation)

**Expected Format**: `"Loop: 5 Hz"` (1-2 digits)

✅ **PASS**: Single digit displays correctly
❌ **FAIL**: Shows "---" or garbled text

### Test Case 5C: High Frequency (> 999 Hz)

**Trigger**: Remove event loops temporarily (testing only)

**Expected Format**: `"Loop: 1.2k Hz"` (abbreviated with "k")

✅ **PASS**: Shows abbreviated format
❌ **FAIL**: Overflows line limit or shows full 4 digits

### Test Case 5D: Character Width Limit

**Action**: Measure Line 4 text length visually

**Expected**: Text fits within display line (21 characters max)

✅ **PASS**: No text cut off or wrapping
❌ **FAIL**: "Hz" missing or text overflows to next line

## Step 6: Verify WebSocket Logging (Optional)

**Action**: Connect to WebSocket logs from development machine

```bash
# Activate Python virtual environment
source src/helpers/websocket_env/bin/activate

# Connect to device WebSocket logs
python3 src/helpers/ws_logger.py <device-ip>
```

**Expected Log Events**:
```json
{"level":"INFO","component":"Main","event":"DISPLAY_INIT_SUCCESS"}
{"level":"DEBUG","component":"Performance","event":"LOOP_FREQUENCY","data":{"frequency":212,"status":"measured"}}
```

**Acceptance Criteria**:
- ✅ Log event every 5 seconds with frequency data
- ✅ No ERROR or FATAL events related to performance monitoring
- ✅ Warnings logged if frequency < 10 Hz or > 2000 Hz

**Failure Modes**:
- ❌ No frequency log events → Logging not implemented
- ❌ ERROR events → Calculation or display errors
- ❌ WARN for normal frequency → Threshold misconfigured

## Step 7: Verify Resource Usage

**Action**: Check compilation output and serial monitor for memory stats

```bash
# Build output shows flash usage
pio run

# Look for:
# RAM:   [=         ]  13.5% (used XXXXX bytes from XXXXXX bytes)
# Flash: [====      ]  47.0% (used XXXXXX bytes from XXXXXX bytes)
```

**Acceptance Criteria**:
- ✅ Flash usage increase < 5% (< 100 KB for feature)
- ✅ RAM usage increase < 1% (< 3 KB for feature)
- ✅ No heap fragmentation warnings

**Failure Modes**:
- ❌ Flash > 80% → Constitutional limit exceeded
- ❌ RAM > 50% → Memory leak or excessive allocation

## Step 8: Verify Graceful Degradation

### Test Case 8A: Display Init Failure

**Action**: Disconnect I2C wires (or test with MockDisplayAdapter returning false)

**Expected Behavior**:
- Device boots normally
- Serial output: "WARNING: OLED display initialization failed"
- WebSocket log: ERROR event for display init
- Other features (WiFi, BoatData) continue working

✅ **PASS**: System continues without display
❌ **FAIL**: Device crashes or reboots

### Test Case 8B: Frequency Measurement Failure

**Action**: Comment out `instrumentLoop()` call in main loop (testing only)

**Expected Behavior**:
- Display shows "Loop: --- Hz" indefinitely
- No crashes or errors
- Other metrics (RAM, Flash) still update

✅ **PASS**: Graceful degradation, display continues
❌ **FAIL**: Display freezes or shows error

## Validation Checklist

### Display Behavior
- [ ] Line 4 shows "Loop: --- Hz" during first 5 seconds
- [ ] Line 4 shows "Loop: XXX Hz" after 5 seconds (numeric frequency)
- [ ] Frequency updates every 5 seconds
- [ ] Frequency value is reasonable (10-2000 Hz range)
- [ ] Display format fits within line limit (21 characters)
- [ ] Animation icon continues rotating independently
- [ ] Other metrics (WiFi, RAM, Flash) unaffected

### Functional Requirements
- [ ] FR-041: Loop iteration count measured over 5-second window ✅
- [ ] FR-042: Frequency calculated as (count / 5.0) Hz ✅
- [ ] FR-043: Display updated every 5 seconds ✅
- [ ] FR-044: "CPU Idle: 85%" replaced with "Loop: XXX Hz" ✅
- [ ] FR-045: Frequency shown as integer (no decimals, unless > 999 Hz) ✅
- [ ] FR-046: Loop counter reset after each measurement period ✅
- [ ] FR-047: No serial port output (display-only) ✅
- [ ] FR-048: Counter overflow handled gracefully ✅
- [ ] FR-049: Placeholder "---" shown before first measurement ✅
- [ ] FR-050: Measurement accuracy within ±5 Hz ✅

### Non-Functional Requirements
- [ ] NFR-007: Measurement overhead < 1% (frequency degradation < 2 Hz)
- [ ] NFR-008: Display update completes within 5-second cycle (no delays)
- [ ] NFR-009: Memory footprint within limits (< 50 bytes RAM, < 5% flash)

### Constitution Compliance
- [ ] Principle I: Hardware abstraction via ISystemMetrics ✅
- [ ] Principle II: Static allocation, no heap usage ✅
- [ ] Principle IV: Modular design, single responsibility ✅
- [ ] Principle V: WebSocket logging implemented ✅
- [ ] Principle VII: Graceful degradation on failure ✅

## Troubleshooting

### Issue: Display shows "CPU Idle: 85%" instead of "Loop: XXX Hz"

**Cause**: Old code not replaced

**Fix**:
1. Verify `DisplayTypes.h` has `loopFrequency` field (not `cpuIdlePercent`)
2. Verify `ISystemMetrics` has `getLoopFrequency()` method (not `getCPUIdlePercent()`)
3. Rebuild with `pio run -t clean && pio run`

### Issue: Display shows "Loop: --- Hz" indefinitely

**Cause**: `instrumentLoop()` not called or measurement not calculating

**Fix**:
1. Verify `main.cpp loop()` calls `systemMetrics->instrumentLoop()`
2. Check serial output for errors during LoopPerformanceMonitor initialization
3. Verify 5 seconds elapsed (use millis() debug output)

### Issue: Frequency shows "0 Hz" after 5 seconds

**Cause**: Loop counter not incrementing or division error

**Fix**:
1. Verify `endLoop()` increments `_loopCount`
2. Check for integer division: use `loop_count / 5` (not `loop_count / 5.0` with rounding)
3. Add serial debug: `Serial.printf("Loop count: %lu\n", _loopCount);`

### Issue: Device reboots every ~49 days

**Cause**: `millis()` overflow not handled

**Fix**:
1. Verify `endLoop()` checks `(now < last_report)` for wrap detection
2. Test overflow scenario in unit tests

### Issue: Frequency > 10000 Hz (unrealistic)

**Cause**: Counter overflow or measurement window too short

**Fix**:
1. Verify 5000ms window (not 500ms)
2. Check `millis()` vs `micros()` usage (should use `millis()` for timing)
3. Add bounds checking: `if (freq > 9999) freq = 9999;`

## Success Metrics

**Definition of Done**:
- ✅ All validation checklist items pass
- ✅ Device runs for 5 minutes without crashes
- ✅ Frequency updates reliably every 5 seconds
- ✅ Display format correct for all edge cases tested
- ✅ Memory and flash usage within constitutional limits
- ✅ WebSocket logs show no ERROR/FATAL events

**Acceptance**: Feature is ready for integration testing and QA review

---
**Quickstart Version**: 1.0 | **Last Updated**: 2025-10-10
