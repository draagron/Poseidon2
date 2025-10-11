# Quickstart: NMEA 0183 Data Handlers

**Feature**: 006-nmea-0183-handlers
**Purpose**: Validate NMEA 0183 sentence parsing and BoatData integration
**Target**: ESP32 hardware with Serial2 connected to NMEA 0183 source
**Time**: ~15 minutes

---

## Prerequisites

### Hardware Setup
- ✓ ESP32 board (esp32dev or compatible)
- ✓ NMEA 0183 source (autopilot or VHF radio) with RS-232/RS-422 output
- ✓ USB cable for ESP32 programming and power
- ✓ Level shifter (if NMEA source is RS-232/RS-422, ESP32 is 3.3V UART)
- ✓ Wires for Serial2 connection: RX=GPIO25, TX=GPIO27 (TX not used)

### Software Setup
- ✓ PlatformIO installed and configured
- ✓ ESP32 toolchain installed (`pio platform install espressif32`)
- ✓ Serial monitor tool (PlatformIO Monitor or screen/minicom)
- ✓ WebSocket client (Python script at `src/helpers/ws_logger.py`)

### Wiring Diagram
```
NMEA 0183 Source (Autopilot/VHF)       ESP32
    [TX] ────┬──────────────────────> [RX] GPIO25 (Serial2)
             │
    [GND] ───┴──────────────────────> [GND]

Notes:
- Connect NMEA TX to ESP32 RX (GPIO25)
- Do NOT connect NMEA RX (ESP32 TX unused for read-only NMEA)
- Use level shifter if NMEA source is 12V RS-232
- Common ground required
```

---

## Step 1: Build and Upload

### 1.1 Build Firmware
```bash
cd /home/niels/Dev/Poseidon2
pio run -e esp32dev
```

**Expected Output**:
```
Building .pio/build/esp32dev/firmware.bin
RAM:   [=         ]  12.3% (used 40324 bytes from 327680 bytes)
Flash: [===       ]  34.5% (used 451234 bytes from 1310720 bytes)
SUCCESS
```

**Validation**:
- ✓ Flash usage < 80% (constitutional requirement)
- ✓ Build completes without errors
- ✓ NMEA0183Handler compiled and linked

---

### 1.2 Upload to ESP32
```bash
pio run -e esp32dev -t upload
```

**Expected Output**:
```
Configuring upload protocol...
Uploading .pio/build/esp32dev/firmware.bin
Writing at 0x00010000... (100%)
Wrote 451234 bytes (278901 compressed) at 0x00010000
Leaving... Hard resetting via RTS pin...
SUCCESS
```

**Validation**:
- ✓ Upload completes successfully
- ✓ ESP32 auto-resets after upload

---

## Step 2: Connect to WiFi

### 2.1 Configure WiFi (if needed)
```bash
# Upload wifi.conf to LittleFS
echo "YourSSID,YourPassword" > data/wifi.conf
pio run -e esp32dev -t uploadfs
```

### 2.2 Monitor Serial Boot
```bash
pio run -e esp32dev -t monitor
```

**Expected Output**:
```
Poseidon2 Gateway v1.0.0
Initializing WiFi...
WiFi connected: YourSSID
IP address: 192.168.1.100
Initializing OLED display...
Initializing Serial2 for NMEA 0183 (4800 baud)...
Initializing BoatData...
NMEA 0183 handler registered
Starting ReactESP event loops...
Ready.
```

**Validation**:
- ✓ WiFi connects successfully
- ✓ Serial2 initializes at 4800 baud
- ✓ No "NMEA0183Handler init failed" errors

**Note IP Address**: You'll need this for WebSocket logging (e.g., 192.168.1.100)

---

## Step 3: Start WebSocket Logging

### 3.1 Activate Python Environment
```bash
cd src/helpers
source websocket_env/bin/activate
```

### 3.2 Connect to WebSocket Logger
```bash
python3 ws_logger.py <ESP32_IP_ADDRESS>
```

**Example**:
```bash
python3 ws_logger.py 192.168.1.100
```

**Expected Output**:
```
Connecting to ws://192.168.1.100/logs...
Connected to Poseidon2 Gateway
[2025-10-11 14:23:45] [INFO] [Main] System startup complete
[2025-10-11 14:23:45] [DEBUG] [NMEA0183] Handler initialized, listening on Serial2
```

**Validation**:
- ✓ WebSocket connection established
- ✓ Log messages appear in real-time
- ✓ NMEA0183 handler initialization logged

---

## Step 4: Validate Sentence Reception

### 4.1 Power On NMEA 0183 Source

Turn on your autopilot or VHF radio. NMEA sentences should start transmitting.

**Expected WebSocket Logs**:
```
[DEBUG] [NMEA0183] Received RSA: rudder=15.0°, source=NMEA0183-AP
[DEBUG] [NMEA0183] BoatData updated: RudderData.steeringAngle=0.2618 rad
[DEBUG] [NMEA0183] Received HDM: heading=045.5°, source=NMEA0183-AP
[DEBUG] [NMEA0183] BoatData updated: CompassData.magneticHeading=0.7941 rad
[DEBUG] [NMEA0183] Received GGA: lat=52.5083, lon=5.1167, source=NMEA0183-VH
[DEBUG] [NMEA0183] BoatData updated: GPSData (lat/lon)
```

**Validation**:
- ✓ At least one sentence type received (RSA, HDM, GGA, RMC, or VTG)
- ✓ Talker ID is "AP" (autopilot) or "VH" (VHF)
- ✓ BoatData updates logged with correct units (radians, knots, decimal degrees)

---

### 4.2 Check OLED Display

Look at the ESP32's OLED display (if connected).

**Expected Display**:
```
WiFi: YourSSID
IP: 192.168.1.100
RAM: 244KB
Flash: 830/1920KB
CPU Idle: 85%
[ / ]  (rotating icon)
```

**Validation**:
- ✓ WiFi SSID and IP displayed
- ✓ RAM usage < 300KB (leaves headroom)
- ✓ Rotating icon updates every 1 second (system responsive)

---

## Step 5: Test-Specific Sentence Types

### 5.1 Test RSA (Rudder Angle)

**Action**: Turn autopilot rudder control (if available)

**Expected Log**:
```
[DEBUG] [NMEA0183] Received RSA: rudder=+20.0°, source=NMEA0183-AP
[DEBUG] [NMEA0183] BoatData updated: RudderData.steeringAngle=0.3491 rad
```

**Validation**:
- ✓ Rudder angle matches physical rudder position
- ✓ Positive = starboard, negative = port
- ✓ Value in radians = degrees × 0.0174533

---

### 5.2 Test HDM (Magnetic Heading)

**Action**: Rotate compass (or observe heading change)

**Expected Log**:
```
[DEBUG] [NMEA0183] Received HDM: heading=090.0°, source=NMEA0183-AP
[DEBUG] [NMEA0183] BoatData updated: CompassData.magneticHeading=1.5708 rad
```

**Validation**:
- ✓ Heading matches compass display
- ✓ Value in radians (90° = π/2 ≈ 1.5708)

---

### 5.3 Test GGA (GPS Position)

**Action**: Ensure GPS has valid fix (VHF radio with GPS)

**Expected Log**:
```
[DEBUG] [NMEA0183] Received GGA: lat=52.5083, lon=5.1167, fixQuality=1, source=NMEA0183-VH
[DEBUG] [NMEA0183] BoatData updated: GPSData (lat/lon)
```

**Validation**:
- ✓ Latitude/longitude in decimal degrees
- ✓ Fix quality > 0 (1=GPS fix, 2=DGPS fix)
- ✓ Position matches GPS display (if available)

---

### 5.4 Test RMC (GPS + COG/SOG)

**Action**: Observe vessel in motion (or simulated GPS movement)

**Expected Log**:
```
[DEBUG] [NMEA0183] Received RMC: lat=52.5084, lon=5.1168, cog=054.7°, sog=5.5kn, var=3.1°W, source=NMEA0183-VH
[DEBUG] [NMEA0183] BoatData updated: GPSData (lat/lon/cog/sog)
[DEBUG] [NMEA0183] BoatData updated: CompassData.variation=-0.0541 rad
```

**Validation**:
- ✓ COG (course over ground) in radians
- ✓ SOG (speed over ground) in knots
- ✓ Variation converted to radians (West = negative)

---

### 5.5 Test VTG (True/Magnetic COG)

**Action**: Observe VTG sentences (usually transmitted alongside RMC)

**Expected Log**:
```
[DEBUG] [NMEA0183] Received VTG: trueCOG=054.7°, magCOG=057.9°, sog=5.5kn, source=NMEA0183-VH
[DEBUG] [NMEA0183] Calculated variation: -3.2° (trueCOG - magCOG)
[DEBUG] [NMEA0183] BoatData updated: GPSData (cog/sog)
[DEBUG] [NMEA0183] BoatData updated: CompassData.variation=-0.0558 rad
```

**Validation**:
- ✓ Variation calculated from COG difference
- ✓ Variation in typical range (±30°)
- ✓ Both true and magnetic COG logged

---

## Step 6: Test Error Handling

### 6.1 Disconnect NMEA Source

**Action**: Unplug NMEA 0183 TX wire from ESP32 RX (GPIO25)

**Expected Behavior**:
- ✓ No new NMEA logs appear
- ✓ System continues operating (no crash or hang)
- ✓ BoatData retains last valid values

**Expected Log**:
```
[DEBUG] [NMEA0183] No sentences received for 5 seconds
[INFO] [BoatData] GPS source NMEA0183-VH marked stale (no updates >5s)
```

---

### 6.2 Send Malformed Sentence

**Action**: Use NMEA simulator to send sentence with invalid checksum

**Example**:
```
$APRSA,15.0,A*FF  (invalid checksum, should be *3C)
```

**Expected Behavior**:
- ✓ Sentence silently discarded (no log, per FR-024)
- ✓ No BoatData update
- ✓ Next valid sentence processed normally

**Validation**: Check WebSocket logs for NO error messages (silent discard)

---

### 6.3 Send Out-of-Range Data

**Action**: Send RSA with rudder angle > 90°

**Example**:
```
$APRSA,120.0,A*XX  (120° exceeds ±90° limit)
```

**Expected Behavior**:
- ✓ Sentence silently discarded (no log, per FR-026)
- ✓ No BoatData update
- ✓ No error counter incremented

**Validation**: Check BoatData diagnostics show no rejection (silent discard)

---

## Step 7: Validate Multi-Source Priority

### 7.1 Add NMEA 2000 GPS Source (if available)

**Action**: Connect NMEA 2000 GPS transmitting at 10 Hz

**Expected Behavior**:
- ✓ NMEA 2000 source registered with higher frequency (10 Hz vs NMEA 0183 1 Hz)
- ✓ BoatData automatically prioritizes NMEA 2000 source
- ✓ NMEA 0183 data still received but not used (lower priority)

**Expected Log**:
```
[INFO] [BoatData] GPS source priority changed: NMEA2000-GPS (10.0 Hz) > NMEA0183-VH (1.0 Hz)
[DEBUG] [NMEA0183] Received GGA (NMEA0183-VH) - rejected (lower priority source)
```

---

## Step 8: Performance Validation

### 8.1 Check Sentence Processing Time

**Action**: Enable performance logging (if implemented)

**Expected Log**:
```
[DEBUG] [NMEA0183] Processing time: RSA=8ms, HDM=6ms, GGA=12ms, RMC=14ms, VTG=11ms
```

**Validation**:
- ✓ All processing times < 50ms (FR-027)
- ✓ Typical time < 15ms per sentence

---

### 8.2 Check CPU Utilization

**Action**: Observe OLED display CPU idle percentage

**Expected Display**:
```
CPU Idle: 85%
```

**Validation**:
- ✓ CPU idle > 80% (system not overloaded)
- ✓ NMEA 0183 processing not blocking other tasks

---

## Step 9: Diagnostic Query (Optional)

### 9.1 Query BoatData Diagnostics

**Action**: Send HTTP GET request (if web API implemented)

```bash
curl http://192.168.1.100/api/diagnostics
```

**Expected Response**:
```json
{
  "gps": {
    "activeSource": "NMEA2000-GPS",
    "sources": [
      {"id": "NMEA2000-GPS", "frequency": 10.0, "priority": 1},
      {"id": "NMEA0183-VH", "frequency": 1.0, "priority": 2}
    ]
  },
  "compass": {
    "activeSource": "NMEA0183-AP",
    "sources": [
      {"id": "NMEA0183-AP", "frequency": 1.0, "priority": 1}
    ]
  },
  "rejectionCount": 0,
  "validationFailures": 0
}
```

**Validation**:
- ✓ NMEA0183-AP and NMEA0183-VH sources registered
- ✓ Rejection count = 0 (all sentences valid)

---

## Troubleshooting

### Issue: No NMEA sentences received

**Symptoms**: No logs after "Handler initialized"

**Checks**:
1. Verify NMEA source is powered on and transmitting
2. Check wiring: NMEA TX → ESP32 RX (GPIO25), common ground
3. Verify baud rate is 4800 (NMEA 0183 standard)
4. Use logic analyzer or oscilloscope to verify NMEA TX signal
5. Check Serial2 initialization in serial monitor: "Serial2 initialized at 4800 baud"

**Fix**:
- Re-check wiring connections
- Verify NMEA source is configured for NMEA 0183 output (not NMEA 2000 or Seatalk)
- Ensure level shifter (if used) is powered and correct polarity

---

### Issue: Sentences received but BoatData not updated

**Symptoms**: Logs show "Received XYZ" but no "BoatData updated"

**Checks**:
1. Check talker ID: Must be "AP" or "VH" (not "GP", "II", etc.)
2. Check message type: Must be RSA, HDM, GGA, RMC, or VTG
3. Check for out-of-range values in logs (silently discarded per FR-026)
4. Verify BoatData initialized before NMEA0183Handler

**Fix**:
- If talker ID is wrong: Reconfigure NMEA source or add talker ID to filter (requires code change)
- If message type unsupported: Feature only handles 5 sentence types (XDR, MWV, etc. not implemented)

---

### Issue: WebSocket logger not connecting

**Symptoms**: "Connection refused" or timeout

**Checks**:
1. Verify ESP32 WiFi connected (check serial monitor)
2. Ping ESP32 IP address: `ping 192.168.1.100`
3. Check firewall allows HTTP/WebSocket port 80
4. Verify Python virtual environment activated

**Fix**:
- Ensure ESP32 and computer on same network
- Disable firewall temporarily to test
- Use `python3 ws_logger.py <ip> --reconnect` for auto-reconnect

---

### Issue: ESP32 crashes or reboots

**Symptoms**: Watchdog timer reset or stack overflow

**Checks**:
1. Check serial monitor for crash dump
2. Look for "Stack overflow" or "Guru Meditation Error"
3. Verify sentence processing time < 50ms (performance budget)

**Fix**:
- Review crash dump for function causing overflow
- Increase stack size in ReactESP task (if using RTOS tasks)
- Report to developer if consistent crashes

---

## Success Criteria

### Minimum Success (Acceptance)
- [ ] ESP32 boots and connects to WiFi
- [ ] Serial2 initialized at 4800 baud
- [ ] At least one NMEA 0183 sentence type received (RSA, HDM, GGA, RMC, or VTG)
- [ ] BoatData updated with parsed values
- [ ] WebSocket logs show DEBUG messages for valid sentences
- [ ] Malformed sentences silently discarded (no error logs)

### Full Success (All Features)
- [ ] All 5 sentence types tested (RSA, HDM, GGA, RMC, VTG)
- [ ] Talker ID filtering works (AP and VH only)
- [ ] Unit conversion correct (degrees→radians, DDMM→decimal)
- [ ] Variation calculation from VTG correct
- [ ] Multi-source prioritization works (NMEA 2000 wins over NMEA 0183)
- [ ] Out-of-range data rejected (no BoatData corruption)
- [ ] Processing time < 50ms per sentence
- [ ] CPU idle > 80% (non-blocking operation)

---

## Cleanup

### Stop WebSocket Logger
```bash
# Press Ctrl+C in ws_logger.py terminal
deactivate  # Exit Python virtual environment
```

### Stop Serial Monitor
```bash
# Press Ctrl+C in pio monitor terminal
```

### Power Off ESP32
```bash
# Unplug USB cable (or use ESPHome power switch if integrated)
```

---

## Next Steps

After successful quickstart:
1. **Integration Testing**: Run full test suite (`pio test -e native -f test_nmea0183_*`)
2. **Hardware Validation**: Run ESP32 timing tests (`pio test -e esp32dev_test -f test_nmea0183_hardware`)
3. **Production Deployment**: Update firmware version, tag release, deploy to vessel
4. **Monitoring**: Enable production logging (ERROR/FATAL only), monitor for 48 hours

---

**Quickstart Version**: 1.0.0
**Last Updated**: 2025-10-11
**Estimated Time**: 15 minutes (assuming hardware already wired)
