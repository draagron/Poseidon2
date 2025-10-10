# Quickstart: WebSocket Loop Frequency Logging Validation

**Feature**: WebSocket Loop Frequency Logging (R007)
**Date**: 2025-10-10
**Estimated Time**: 10 minutes

## Prerequisites

### Hardware
- ✅ ESP32 device (SH-ESP32 board) with OLED display connected
- ✅ USB cable for programming
- ✅ Computer with WebSocket client capability

### Software
- ✅ PlatformIO CLI installed
- ✅ Feature branch checked out: `007-loop-frequency-should`
- ✅ Python 3.x installed (for WebSocket client)
- ✅ WebSocket client script: `src/helpers/ws_logger.py`

### Network
- ✅ WiFi network credentials configured in `/wifi.conf`
- ✅ Device and computer on same network
- ✅ WebSocket port 80 accessible (no firewall blocking)

### Dependencies
- ✅ R006 (Loop Frequency Display) must be implemented
- ✅ WebSocket logging infrastructure operational

---

## Step 1: Build and Upload Firmware

```bash
# Navigate to project root
cd /home/niels/Dev/Poseidon2

# Clean build
pio run -t clean

# Build for ESP32
pio run -e esp32dev

# Upload to device
pio run -e esp32dev -t upload

# Optional: Monitor serial output during boot
pio run -e esp32dev -t monitor
```

**Expected Serial Output**:
```
Poseidon2 WiFi Gateway - Initializing...
LittleFS mounted successfully
WiFi config loaded: X networks
Connecting to WiFi...
WiFi connected: <SSID>
IP address: 192.168.X.X
Initializing OLED display...
OLED display initialized successfully
Setup complete - entering main loop
```

**Success Criteria**:
- ✅ Build completes without errors
- ✅ Firmware uploads successfully
- ✅ Device boots and connects to WiFi
- ✅ OLED display shows system status

---

## Step 2: Connect to WebSocket Logs

**Action**: Open terminal and connect WebSocket client to device

### Activate Python Virtual Environment

```bash
# Activate virtual environment for ws_logger.py
source src/helpers/websocket_env/bin/activate
```

### Connect to Device WebSocket

```bash
# Replace <device-ip> with actual IP from serial monitor or OLED display
python3 src/helpers/ws_logger.py <device-ip>

# Example:
python3 src/helpers/ws_logger.py 192.168.1.100
```

**Expected Output**:
```
Connecting to WebSocket: ws://192.168.1.100/logs
Connected successfully
Listening for log messages...
```

**Success Criteria**:
- ✅ WebSocket connection established
- ✅ No connection errors
- ✅ Client shows "Connected" message

**Troubleshooting**:
- ❌ Connection refused → Check device IP address, verify device on network
- ❌ Timeout → Check firewall settings, verify WiFi connected
- ❌ 404 Not Found → WebSocket server not running, check firmware upload

---

## Step 3: Verify Log Messages Appear Every 5 Seconds

**Action**: Observe WebSocket log output for periodic messages

**Expected Log Messages**:
```json
{
  "timestamp": 5123,
  "level": "DEBUG",
  "component": "Performance",
  "event": "LOOP_FREQUENCY",
  "data": {"frequency": 0}
}

{
  "timestamp": 10456,
  "level": "DEBUG",
  "component": "Performance",
  "event": "LOOP_FREQUENCY",
  "data": {"frequency": 212}
}

{
  "timestamp": 15789,
  "level": "DEBUG",
  "component": "Performance",
  "event": "LOOP_FREQUENCY",
  "data": {"frequency": 218}
}
```

**Acceptance Criteria**:
- ✅ Log messages appear every ~5 seconds (±500ms tolerance)
- ✅ First message (within 5s of boot) shows `"frequency": 0` (no measurement yet)
- ✅ Subsequent messages show non-zero frequency values
- ✅ Frequency values are realistic (10-2000 Hz range)

**Failure Modes**:
- ❌ No log messages → Implementation not complete, check main.cpp modification
- ❌ Messages appear every 1 second → Wrong event loop used (animation loop instead of status loop)
- ❌ Irregular timing → Timing issue, check DISPLAY_STATUS_INTERVAL_MS constant

---

## Step 4: Verify JSON Format

**Action**: Inspect individual log message structure

**Expected JSON Structure**:
```json
{
  "timestamp": <uint32_t>,
  "level": "DEBUG" | "WARN",
  "component": "Performance",
  "event": "LOOP_FREQUENCY",
  "data": {"frequency": <uint32_t>}
}
```

**Field Validation**:
- ✅ `timestamp`: Present, numeric, increases over time
- ✅ `level`: Either "DEBUG" or "WARN"
- ✅ `component`: Exactly "Performance" (case-sensitive)
- ✅ `event`: Exactly "LOOP_FREQUENCY" (case-sensitive)
- ✅ `data.frequency`: Numeric, no quotes, >= 0

**Success Criteria**:
- ✅ JSON is valid (parseable)
- ✅ All required fields present
- ✅ Field names exactly match specification
- ✅ Frequency is single numeric field (minimal schema)

**Failure Modes**:
- ❌ Invalid JSON → String formatting error, check JSON construction
- ❌ `data` field is string not object → JSON not parsed, check WebSocketLogger
- ❌ Extra fields in `data` → Wrong schema, should be `{"frequency": XXX}` only
- ❌ `frequency` in quotes → Wrong format, should be numeric (no quotes)

---

## Step 5: Verify Component and Event Identifiers

**Action**: Confirm component and event fields match specification

**Expected Values**:
- **Component**: "Performance"
- **Event**: "LOOP_FREQUENCY"

**Verification**:
```bash
# Filter logs for LOOP_FREQUENCY events
python3 src/helpers/ws_logger.py <device-ip> | grep "LOOP_FREQUENCY"
```

**Acceptance Criteria**:
- ✅ All loop frequency logs have `"component": "Performance"`
- ✅ All loop frequency logs have `"event": "LOOP_FREQUENCY"`
- ✅ Case matches exactly (not "performance" or "loop_frequency")

**Failure Modes**:
- ❌ Wrong component name → Check F() macro in broadcastLog call
- ❌ Wrong event name → Check F() macro in broadcastLog call
- ❌ Case mismatch → Strings must be exact, case-sensitive

---

## Step 6: Verify Log Level (DEBUG vs WARN)

**Action**: Monitor log levels for different frequency ranges

### Normal Frequency (DEBUG Level)

**Expected**: Most logs should be DEBUG level

**Example**:
```json
{
  "level": "DEBUG",
  "data": {"frequency": 212}
}
```

**Criteria**:
- ✅ Frequency 10-2000 Hz → DEBUG level
- ✅ Frequency 0 Hz (first 5 seconds) → DEBUG level

### Abnormal Frequency (WARN Level)

**Trigger**: Simulate system overload or measurement error (optional)

**Expected**:
```json
{
  "level": "WARN",
  "data": {"frequency": 5}
}
```

**Criteria**:
- ✅ Frequency < 10 Hz → WARN level
- ✅ Frequency > 2000 Hz → WARN level

**Test Procedure** (Optional):
1. Cause system overload: Run blocking code in main loop
2. Observe frequency drop below 10 Hz
3. Verify log level changes to WARN

**Acceptance Criteria**:
- ✅ Normal frequencies (10-2000 Hz) logged at DEBUG
- ✅ Abnormal frequencies (<10 Hz or >2000 Hz) logged at WARN
- ✅ Zero frequency (0 Hz, first 5 seconds) logged at DEBUG

**Failure Modes**:
- ❌ All logs are DEBUG → Log level logic not implemented
- ❌ All logs are WARN → Logic inverted, check conditional
- ❌ Zero frequency shows WARN → Zero handling missing

---

## Step 7: Verify Frequency Matches OLED Display

**Action**: Compare WebSocket log frequency with OLED display value

**Procedure**:
1. Read frequency from OLED display Line 4: "Loop: XXX Hz"
2. Read frequency from most recent WebSocket log: `"frequency": XXX`
3. Compare values

**Expected**: Values should match exactly (±1 Hz due to timing)

**Example**:
- **OLED Display**: `Loop: 212 Hz`
- **WebSocket Log**: `"frequency": 212`
- **Match**: ✅ PASS

**Acceptance Criteria**:
- ✅ Frequency values match within ±1 Hz
- ✅ Both values update synchronously (every 5 seconds)
- ✅ Zero frequency on OLED ("Loop: --- Hz") matches zero in log

**Failure Modes**:
- ❌ Values differ significantly → Different data sources, check systemMetrics access
- ❌ OLED updates but logs don't → WebSocket log not implemented
- ❌ Logs update but OLED doesn't → Separate issue, check R006 implementation

---

## Step 8: Verify Graceful Degradation

**Action**: Test system behavior when WebSocket fails

### Test Case 8A: Disconnect WebSocket Client

**Procedure**:
1. Stop `ws_logger.py` client (Ctrl+C)
2. Observe OLED display continues updating
3. Observe no serial errors or crashes
4. Reconnect client

**Expected Behavior**:
- ✅ Device continues normal operation
- ✅ OLED display still updates every 5 seconds
- ✅ Loop frequency measurement unaffected
- ✅ No errors in serial monitor
- ✅ Upon reconnection, logs resume immediately

**Acceptance Criteria**: ✅ Graceful degradation (FR-059)

### Test Case 8B: Multiple WebSocket Clients

**Procedure**:
1. Open two terminal windows
2. Connect two `ws_logger.py` clients to device
3. Verify both receive log messages

**Expected Behavior**:
- ✅ Both clients receive identical log messages
- ✅ Messages appear simultaneously on both clients
- ✅ No message loss or duplication

**Acceptance Criteria**: ✅ Broadcast to all clients (FR-060)

### Test Case 8C: WiFi Disconnection

**Procedure**:
1. Disconnect WiFi temporarily
2. Observe OLED display continues showing frequency
3. Reconnect WiFi
4. Reconnect WebSocket client

**Expected Behavior**:
- ✅ OLED display continues during WiFi outage
- ✅ Frequency measurement continues
- ✅ Logs resume after WiFi reconnects

**Acceptance Criteria**: ✅ System resilience

---

## Step 9: Performance Validation

**Action**: Verify log emission overhead

### Timing Measurement

**Procedure**:
1. Monitor loop frequency on OLED display
2. Record baseline frequency (before R007 implementation)
3. Measure frequency with WebSocket logging active
4. Calculate overhead

**Expected Results**:
- **Baseline** (R006 only): ~220 Hz (typical)
- **With Logging** (R007): ~218 Hz (typical)
- **Overhead**: < 2 Hz (< 1%)

**Acceptance Criteria**: ✅ Overhead < 1ms per message (NFR-010)

### Message Size Measurement

**Procedure**:
1. Capture WebSocket message
2. Measure JSON string length

**Example Message**:
```json
{"timestamp":12345,"level":"DEBUG","component":"Performance","event":"LOOP_FREQUENCY","data":{"frequency":212}}
```

**Size**: ~130-150 bytes

**Acceptance Criteria**: ✅ Message size < 200 bytes (NFR-011)

---

## Validation Checklist

### Functional Requirements
- [ ] **FR-051**: Log message emitted every 5 seconds ✅
- [ ] **FR-052**: Frequency value matches OLED display ✅
- [ ] **FR-053**: 5-second interval synchronized with display update ✅
- [ ] **FR-054**: Log emitted even when frequency is 0 Hz ✅
- [ ] **FR-055**: DEBUG level for normal (10-2000 Hz), WARN for abnormal ✅
- [ ] **FR-056**: Component identifier is "Performance" ✅
- [ ] **FR-057**: Event identifier is "LOOP_FREQUENCY" ✅
- [ ] **FR-058**: JSON format with minimal schema `{"frequency": XXX}` ✅
- [ ] **FR-059**: System continues if WebSocket logging fails ✅
- [ ] **FR-060**: All WebSocket clients receive broadcast ✅

### Non-Functional Requirements
- [ ] **NFR-010**: Log emission overhead < 1ms ✅
- [ ] **NFR-011**: Message size < 200 bytes ✅
- [ ] **NFR-012**: Timing synchronized with OLED display ✅

### Constitution Compliance
- [ ] Hardware abstraction maintained (no direct hardware access) ✅
- [ ] Memory footprint within limits (zero static, ~30 bytes heap temporary) ✅
- [ ] Graceful degradation implemented (FR-059) ✅
- [ ] WebSocket logging functional (Principle V) ✅

---

## Troubleshooting

### Issue: No WebSocket Logs Appear

**Possible Causes**:
1. WebSocket client not connected
2. Implementation not complete (main.cpp not modified)
3. systemMetrics is nullptr

**Solutions**:
1. Verify WebSocket connection: `ws_logger.py <device-ip>`
2. Check main.cpp for broadcastLog call in 5-second event loop
3. Check serial monitor for initialization errors

### Issue: Wrong Frequency Values

**Possible Causes**:
1. Different data source (not systemMetrics->getLoopFrequency())
2. Timing mismatch (wrong event loop)

**Solutions**:
1. Verify systemMetrics->getLoopFrequency() call
2. Ensure log call is in DISPLAY_STATUS_INTERVAL_MS loop (5 seconds, not 1 second)

### Issue: Wrong Log Level

**Possible Causes**:
1. Log level logic incorrect
2. Frequency threshold wrong

**Solutions**:
1. Check conditional: `(frequency > 0 && frequency >= 10 && frequency <= 2000) ? DEBUG : WARN`
2. Verify thresholds: 10 Hz (lower), 2000 Hz (upper)

### Issue: JSON Parse Error

**Possible Causes**:
1. String formatting incorrect
2. Missing escapes for quotes

**Solutions**:
1. Check JSON construction: `String("{\"frequency\":") + frequency + "}"`
2. Verify backslash escaping: `\"` for literal quote

### Issue: WebSocket Connection Refused

**Possible Causes**:
1. Wrong IP address
2. Firewall blocking port 80
3. Device not on network

**Solutions**:
1. Verify IP from OLED display or serial monitor
2. Check firewall settings (allow HTTP/WebSocket on port 80)
3. Ping device to verify network connectivity

---

## Success Metrics

**Definition of Done**:
- ✅ All validation checklist items pass
- ✅ WebSocket logs appear every 5 seconds
- ✅ Frequency matches OLED display value
- ✅ Log level correct (DEBUG for normal, WARN for abnormal)
- ✅ JSON format valid and minimal
- ✅ Graceful degradation verified
- ✅ Performance overhead < 1%
- ✅ Message size < 200 bytes

**Acceptance**: Feature ready for production deployment

---

## Next Steps

After successful validation:
1. ✅ Run full test suite: `pio test -e native -f test_websocket_frequency_*`
2. ✅ Run hardware tests: `pio test -e esp32dev_test -f test_websocket_frequency_hardware`
3. ✅ Create pull request from branch `007-loop-frequency-should`
4. ✅ QA review and approval
5. ✅ Merge to `main` branch

---

**Quickstart Version**: 1.0 | **Last Updated**: 2025-10-10
