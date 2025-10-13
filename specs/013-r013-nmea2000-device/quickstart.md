# Quickstart: NMEA2000 Device Discovery Validation

**Feature**: NMEA2000 Device Discovery and Identification
**Purpose**: Hardware validation guide for verifying device discovery functionality
**Audience**: Developers, QA testers, marine system integrators
**Time**: 15-30 minutes (depending on device count)

## Prerequisites

### Hardware Requirements
- **ESP32 Gateway**: SH-ESP32 board with Poseidon2 firmware flashed
- **NMEA2000 Network**: Active CAN bus with 120Ω termination resistors
- **Test Devices**: 2-3 NMEA2000 devices (recommended: different manufacturers)
  - Example: Garmin GPS 17x, Furuno compass, Raymarine wind sensor
  - Minimum: 1 NMEA2000 device (limited validation)
- **Power**: 12V DC power supply for NMEA2000 network
- **PC/Laptop**: For WebSocket logging and web UI access

### Software Requirements
- **WiFi Network**: ESP32 must be connected to WiFi (check OLED display for IP address)
- **WebSocket Logger**: Python 3.7+ with `websocket-client` library
  ```bash
  pip install websocket-client
  ```
- **Web Browser**: Chrome/Firefox/Safari (for web UI testing)

### Firmware Build
```bash
# Build and upload firmware with device discovery feature
cd /home/niels/Dev/Poseidon2
pio run --target upload
```

## Validation Test Suite

### Test 1: Device Discovery Logging (Core Functionality)

**Objective**: Verify that ESP32 discovers NMEA2000 devices and logs metadata via WebSocket.

**Steps**:
1. Start WebSocket logger:
   ```bash
   python3 src/helpers/ws_logger.py <ESP32_IP>
   ```

2. Power on NMEA2000 network (if not already powered)

3. Wait 10-30 seconds for devices to announce themselves (NMEA2000 startup)

4. Observe WebSocket logs for device discovery events

**Expected Output** (example for Garmin GPS):
```
[DEBUG] [DeviceDiscovery] DEVICE_DISCOVERED {"sourceId":"NMEA2000-42","manufacturerCode":275,"manufacturer":"Garmin","modelId":"GPS 17x","serial":123456789,"softwareVersion":"v2.30"}
```

**Success Criteria**:
- ✅ One `DEVICE_DISCOVERED` log per connected NMEA2000 device
- ✅ Manufacturer code and name match device (e.g., 275="Garmin")
- ✅ Model ID is populated (not empty string)
- ✅ Serial number is non-zero
- ✅ Discovery occurs within 30 seconds of device power-on

**Failure Modes**:
- ❌ No logs appear → Check WiFi connection, verify WebSocket logger connected
- ❌ `DEVICE_TIMEOUT` logs → Device not broadcasting Product Information PGN (non-compliant)
- ❌ Wrong manufacturer name → ManufacturerLookup table incomplete or NMEA2000 library issue

---

### Test 2: WebSocket Full Snapshot (Schema v2)

**Objective**: Verify that WebSocket `/source-stats` endpoint includes device metadata in v2 schema.

**Steps**:
1. Ensure devices are discovered (complete Test 1 first)

2. Connect to WebSocket endpoint using browser console or Python script:
   ```javascript
   // Browser console (open http://<ESP32_IP>:3030/sources first)
   const ws = new WebSocket('ws://<ESP32_IP>:3030/source-stats');
   ws.onmessage = (event) => console.log(JSON.parse(event.data));
   ```

3. Observe initial `fullSnapshot` message

**Expected Output**:
```json
{
  "event": "fullSnapshot",
  "version": 2,
  "timestamp": 1697232000000,
  "sources": [
    {
      "sourceId": "NMEA2000-42",
      "protocol": "NMEA2000",
      "frequency": 10.2,
      "deviceInfo": {
        "hasInfo": true,
        "manufacturerCode": 275,
        "manufacturer": "Garmin",
        "modelId": "GPS 17x",
        "serialNumber": 123456789,
        "softwareVersion": "v2.30"
      }
    }
  ]
}
```

**Success Criteria**:
- ✅ `version: 2` in JSON payload
- ✅ Each NMEA2000 source has `deviceInfo` object
- ✅ `hasInfo: true` for discovered devices
- ✅ `hasInfo: false` with `status: "Discovering..."` for undiscovered sources (if present)
- ✅ NMEA0183 sources have `deviceInfo.description` (e.g., "Autopilot" for talker ID "AP")

**Failure Modes**:
- ❌ `version: 1` → Schema not upgraded, code deployment issue
- ❌ `deviceInfo` missing → SourceStatsSerializer not updated
- ❌ `hasInfo: false` for all sources → DeviceInfoCollector not running or failed to correlate

---

### Test 3: Discovery Timeout Behavior (60-Second Timeout)

**Objective**: Verify that non-compliant devices (not broadcasting Product Info) are marked as "Unknown (timeout)" after 60 seconds.

**Setup**:
1. Identify a source without device metadata (check logs or WebSocket for `hasInfo: false`)
   - If all devices are compliant, simulate by:
     - Sending NMEA2000 messages from unknown SID (e.g., use Actisense NGT-1 to inject PGN 129025 from SID 99)
     - OR: Temporarily modify DeviceInfoCollector to skip a specific SID

2. Start timer at first message from undiscovered source

**Steps**:
1. Monitor WebSocket logs for 65 seconds

2. At T+60s, expect `DEVICE_TIMEOUT` log:
   ```
   [DEBUG] [DeviceDiscovery] DEVICE_TIMEOUT {"sourceId":"NMEA2000-99","reason":"discovery_timeout"}
   ```

3. Verify WebSocket delta update sent to clients:
   ```json
   {
     "event": "sourceUpdate",
     "sourceId": "NMEA2000-99",
     "changes": {
       "deviceInfo": {
         "hasInfo": false,
         "reason": "discovery_timeout",
         "status": "Unknown (timeout)"
       }
     }
   }
   ```

**Success Criteria**:
- ✅ Timeout log appears at T+60s (±2s tolerance)
- ✅ WebSocket delta update sent to connected clients
- ✅ `deviceInfo.hasInfo` remains `false`
- ✅ Source continues to function (frequency tracking, staleness detection)

**Failure Modes**:
- ❌ No timeout log → Discovery timeout logic not implemented
- ❌ Timeout at wrong time (e.g., T+30s) → `discoveryTimeout` constant incorrect

---

### Test 4: Web UI Device Metadata Display

**Objective**: Verify that `/sources` web page displays device metadata in expandable sections.

**Steps**:
1. Open web browser: `http://<ESP32_IP>:3030/sources`

2. Locate NMEA2000 sources in the GPS or Compass category

3. Click/tap to expand device details section

**Expected Display** (example for Garmin GPS):
```
📡 NMEA2000-42 [10.2 Hz, Fresh]
  ▼ Device Details
    Manufacturer: Garmin (275)
    Model: GPS 17x
    Serial: 123456789
    Software: v2.30
```

**Success Criteria**:
- ✅ Device metadata section expandable (collapsible UI)
- ✅ Manufacturer name and code displayed (e.g., "Garmin (275)")
- ✅ Model ID, serial number, software version populated
- ✅ Undiscovered sources show "Device info: Discovering..." placeholder
- ✅ Timed-out sources show "Device info: Unknown (timeout)" placeholder
- ✅ NMEA0183 sources show device type description (e.g., "Autopilot" for AP talker ID)
- ✅ Responsive layout on mobile (test at 320px viewport width)

**Failure Modes**:
- ❌ No device details section → UI not updated
- ❌ "N/A" or empty strings → SourceStatsSerializer or JavaScript handling issue
- ❌ Placeholder text missing → UI logic incomplete

---

### Test 5: Hot-Plug Device Discovery

**Objective**: Verify that newly connected devices are discovered without requiring ESP32 reboot.

**Steps**:
1. With ESP32 running and WebSocket logger active, disconnect one NMEA2000 device from bus (unplug)

2. Wait 60+ seconds (device transitions to stale/garbage collected)

3. Reconnect same device to bus

4. Observe WebSocket logs for re-discovery

**Expected Output**:
```
[DEBUG] [DeviceDiscovery] DEVICE_DISCOVERED {"sourceId":"NMEA2000-42","manufacturer":"Garmin",...}
```

**Success Criteria**:
- ✅ Device re-discovered within 30 seconds of reconnection
- ✅ WebSocket delta update sent to clients
- ✅ Web UI updates to show device metadata (refresh page if WebSocket client reconnection needed)
- ✅ Message statistics history preserved (frequency buffer, update count)

**Failure Modes**:
- ❌ Device not re-discovered → tN2kDeviceList not handling dynamic bus changes
- ❌ Duplicate source entries → SID correlation logic incorrect

---

### Test 6: Multi-Device Differentiation

**Objective**: Verify that multiple devices of same model (e.g., 2x GPS units) are distinguished by serial number.

**Setup**: Requires 2+ devices with same manufacturer and model (e.g., 2x Garmin GPS 17x)

**Steps**:
1. Connect both devices to NMEA2000 bus

2. Verify both devices discovered via WebSocket logs

3. Check web UI displays both sources with different serial numbers

**Expected Display**:
```
NMEA2000-42 - Garmin GPS 17x (Serial: 123456789)
NMEA2000-7  - Garmin GPS 17x (Serial: 987654321)
```

**Success Criteria**:
- ✅ Both devices discovered with unique SIDs
- ✅ Serial numbers differ and correctly displayed
- ✅ Source prioritization logic considers frequency (10 Hz device prioritized over 1 Hz)

**Failure Modes**:
- ❌ Only one device discovered → tN2kDeviceList capacity exceeded or SID collision

---

### Test 7: Performance Validation (Poll Cycle Time)

**Objective**: Verify that device discovery polling completes in <10ms per FR-022.

**Steps**:
1. Enable performance logging (modify DeviceInfoCollector.cpp):
   ```cpp
   unsigned long start = millis();
   pollDeviceList();
   unsigned long duration = millis() - start;
   logger->broadcastLog(LogLevel::DEBUG, "DeviceDiscovery", "POLL_CYCLE_TIME",
       String(F("{\"duration\":")) + duration + F("}"));
   ```

2. Rebuild and upload firmware

3. Monitor WebSocket logs for `POLL_CYCLE_TIME` events

**Expected Output**:
```
[DEBUG] [DeviceDiscovery] POLL_CYCLE_TIME {"duration":4}
```

**Success Criteria**:
- ✅ Poll cycle duration <10ms consistently (>95% of polls)
- ✅ No message processing delays observed (GPS frequency remains 10 Hz)

**Failure Modes**:
- ❌ Poll duration >10ms → Optimize device iteration or lookup algorithms

---

## Troubleshooting

### Problem: No devices discovered after 60 seconds

**Possible Causes**:
1. NMEA2000 devices not broadcasting Product Information PGN 126996
   - **Solution**: Verify devices are NMEA2000-compliant (check manufacturer documentation)
   - **Workaround**: Devices still function, just marked as "Unknown device"

2. `tN2kDeviceList` not initialized or attached to `NMEA2000` object
   - **Solution**: Check `main.cpp` initialization sequence, ensure `deviceList = new tN2kDeviceList(nmea2000)` called after NMEA2000 setup

3. DeviceInfoCollector ReactESP timer not registered
   - **Solution**: Verify `deviceCollector->init(app)` called in `setup()`

### Problem: Wrong manufacturer names (e.g., "Unknown (275)" instead of "Garmin")

**Possible Causes**:
1. ManufacturerLookup table incomplete
   - **Solution**: Add missing manufacturer codes to `ManufacturerLookup.cpp`

2. PROGMEM string read issue (ESP32-specific)
   - **Solution**: Use `pgm_read_word()` and `strcpy_P()` for PROGMEM access

### Problem: WebSocket clients not receiving device metadata

**Possible Causes**:
1. Schema version not incremented to v2
   - **Solution**: Verify `SOURCE_STATS_SCHEMA_VERSION == 2` in SourceStatsSerializer.h

2. SourceStatsSerializer not serializing `deviceInfo`
   - **Solution**: Add JSON serialization logic for `deviceInfo` object

3. `hasChanges` flag not set when device discovered
   - **Solution**: Ensure `SourceRegistry::updateDeviceInfo()` sets `hasChanges_ = true`

## Success Checklist

- [ ] **Test 1**: Device discovery logs appear within 30 seconds
- [ ] **Test 2**: WebSocket full snapshot includes v2 schema with `deviceInfo`
- [ ] **Test 3**: Discovery timeout triggers after 60 seconds
- [ ] **Test 4**: Web UI displays device metadata in expandable sections
- [ ] **Test 5**: Hot-plugged devices re-discovered without reboot
- [ ] **Test 6**: Multiple identical devices differentiated by serial number
- [ ] **Test 7**: Poll cycle time <10ms consistently

**When all tests pass**: Feature R013 (NMEA2000 Device Discovery) is validated and ready for production deployment.

---

**Last Updated**: 2025-10-13
**Quickstart Version**: 1.0
