# Quickstart Validation Guide: Source Statistics Tracking

**Feature**: 012-sources-stats-and
**Version**: 1.0.0
**Date**: 2025-10-13
**Time to Complete**: 15-20 minutes

## Purpose

This guide provides a step-by-step validation workflow to verify the source statistics tracking feature is working correctly. It covers local testing with real NMEA data, WebSocket connectivity, and dashboard functionality.

## Prerequisites

**Hardware**:
- ESP32 device (SH-ESP32 board)
- NMEA 2000 device (e.g., GPS unit with SID 42)
- NMEA 0183 device (e.g., autopilot with talker ID "AP") [optional]
- USB cable for programming and power

**Software**:
- PlatformIO installed
- Python 3.8+ (for WebSocket logger script)
- Node.js 16+ (for proxy server) [optional]
- Web browser (Chrome/Firefox/Safari)

**Network**:
- WiFi network with SSID configured in ESP32
- Computer on same network as ESP32

## Quick Start (5 minutes)

### 1. Build and Upload

```bash
# From repository root
cd /Users/nielsnorgaard/Dev/Poseidon2

# Build firmware
pio run

# Upload to ESP32
pio run --target upload

# Monitor serial output for IP address
pio device monitor
```

**Expected Output**:
```
[WiFi] Connected to <SSID>
[WiFi] IP address: 192.168.1.100
[SourceRegistry] Initialized with 19 message types
[WebSocket] Source stats endpoint: ws://192.168.1.100/source-stats
[HTTP] Dashboard available: http://192.168.1.100:3030/sources
```

### 2. Verify WebSocket Endpoint

```bash
# Use Python WebSocket logger to connect
python3 src/helpers/ws_logger.py 192.168.1.100 --endpoint /source-stats
```

**Expected Output** (initial full snapshot):
```json
{
  "event": "fullSnapshot",
  "version": 1,
  "timestamp": 12345,
  "sources": {}
}
```

(Empty on first connect - sources appear as NMEA messages arrive)

### 3. Check Dashboard

Open browser to: `http://192.168.1.100:3030/sources`

**Expected Display**:
- Dashboard HTML loads
- WebSocket connection status: "Connected"
- Categories displayed (GPS, Compass, Wind, etc.)
- Initially empty (no sources yet)

✅ **If all three steps succeed, proceed to detailed validation**

---

## Detailed Validation (15 minutes)

### Scenario 1: NMEA 2000 Source Discovery (P1)

**Objective**: Verify NMEA 2000 sources are discovered and tracked

**Steps**:
1. Connect NMEA 2000 GPS unit (transmitting PGN 129025 at 10 Hz)
2. Wait 5 seconds for source discovery
3. Check WebSocket logger output

**Expected WebSocket Message**:
```json
{
  "event": "deltaUpdate",
  "timestamp": 23456,
  "changes": [
    {
      "sourceId": "NMEA2000-42",
      "frequency": 10.1,
      "timeSinceLast": 98,
      "isStale": false
    }
  ]
}
```

**Expected Dashboard Update**:
- GPS category expands
- New row: "PGN129025" → "NMEA2000-42" → "10.1 Hz" → "98ms" → 🟢 (green)

**Success Criteria**:
- ✅ Source ID format correct: "NMEA2000-<SID>"
- ✅ Frequency calculated: 9.0-11.0 Hz (±10% tolerance)
- ✅ Update latency <200ms (visual update after message arrival)
- ✅ Staleness indicator green (fresh)

**Troubleshooting**:
- **No source appears**: Check NMEA2000 CAN bus connection, verify PGN handler registered
- **Frequency 0.0 Hz**: Source discovered but <10 samples collected, wait 1 second
- **Wrong SID**: Check NMEA 2000 device configuration

---

### Scenario 2: NMEA 0183 Source Discovery (P1)

**Objective**: Verify NMEA 0183 sources are discovered and tracked

**Steps**:
1. Connect NMEA 0183 autopilot (transmitting RSA sentences at 1 Hz with talker ID "AP")
2. Wait 10 seconds for 10 samples (frequency calculation requires full buffer)
3. Check WebSocket logger output

**Expected WebSocket Message**:
```json
{
  "event": "deltaUpdate",
  "timestamp": 34567,
  "changes": [
    {
      "sourceId": "NMEA0183-AP",
      "frequency": 1.0,
      "timeSinceLast": 985,
      "isStale": false
    }
  ]
}
```

**Expected Dashboard Update**:
- Rudder category expands (RSA sentences map to Rudder)
- New row: "RSA" → "NMEA0183-AP" → "1.0 Hz" → "985ms" → 🟢 (green)

**Success Criteria**:
- ✅ Source ID format correct: "NMEA0183-<TalkerID>"
- ✅ Frequency calculated: 0.9-1.1 Hz
- ✅ Category assignment correct (RSA → Rudder)

**Troubleshooting**:
- **No source appears**: Check Serial2 wiring (RX=GPIO25, TX=GPIO27), verify 4800 baud
- **Wrong talker ID**: Verify autopilot configuration (should broadcast as "AP")

---

### Scenario 3: Staleness Detection (P1)

**Objective**: Verify sources are marked stale after 5 seconds without updates

**Steps**:
1. With NMEA 2000 GPS active (from Scenario 1), disconnect GPS or disable transmission
2. Wait 5-6 seconds
3. Observe WebSocket logger and dashboard

**Expected WebSocket Message** (after 5 seconds):
```json
{
  "event": "deltaUpdate",
  "timestamp": 45678,
  "changes": [
    {
      "sourceId": "NMEA2000-42",
      "timeSinceLast": 5123,
      "isStale": true
    }
  ]
}
```

**Expected Dashboard Update**:
- GPS → PGN129025 → NMEA2000-42: Indicator changes 🟢 → 🔴 (red)
- Time since last update continues incrementing

**Success Criteria**:
- ✅ Staleness triggers within 5.5 seconds (5.0s threshold + 0.5s tolerance)
- ✅ `isStale` flag changes to `true`
- ✅ Visual indicator changes color (green → red)

**Troubleshooting**:
- **Staleness never triggers**: Check `updateStaleFlags()` is called every 500ms
- **Triggers too early**: Verify threshold is 5000ms, not 500ms

---

### Scenario 4: Source Reactivation (P1)

**Objective**: Verify stale sources become fresh when transmission resumes

**Steps**:
1. With stale GPS source (from Scenario 3), reconnect GPS or enable transmission
2. Wait for first message arrival
3. Observe WebSocket logger and dashboard

**Expected WebSocket Message** (immediately after message):
```json
{
  "event": "deltaUpdate",
  "timestamp": 56789,
  "changes": [
    {
      "sourceId": "NMEA2000-42",
      "timeSinceLast": 102,
      "isStale": false
    }
  ]
}
```

**Expected Dashboard Update**:
- GPS → PGN129025 → NMEA2000-42: Indicator changes 🔴 → 🟢 (green)
- Time since last update resets to ~100ms

**Success Criteria**:
- ✅ `isStale` flag changes to `false` immediately
- ✅ No duplicate source entry created
- ✅ Frequency resumes calculating after 10 new samples

---

### Scenario 5: Multiple Sources per Message Type (P1)

**Objective**: Verify multiple sources for same PGN/sentence are tracked independently

**Steps**:
1. Connect two NMEA 2000 GPS units with different SIDs (e.g., SID 42 and SID 10)
2. Both transmit PGN 129025 at 10 Hz
3. Wait 5 seconds for discovery
4. Check dashboard

**Expected Dashboard Display**:
```
GPS
  PGN129025
    NMEA2000-42    10.1 Hz    98ms    🟢
    NMEA2000-10    10.0 Hz    100ms   🟢
```

**Success Criteria**:
- ✅ Both sources listed under same message type (PGN129025)
- ✅ Frequencies calculated independently
- ✅ Staleness tracked independently (disconnect one, verify only that one goes stale)

---

### Scenario 6: Dashboard Responsiveness (P2)

**Objective**: Verify dashboard updates in real-time without page refresh

**Steps**:
1. Open dashboard in browser
2. Connect/disconnect NMEA source multiple times
3. Observe dashboard updates without refreshing page

**Success Criteria**:
- ✅ New sources appear within 200ms of WebSocket message (SC-006)
- ✅ Staleness indicator changes without page refresh
- ✅ Time since last update increments every 500ms
- ✅ No page flicker or layout jumps

**Performance Check**:
- Open browser developer tools (F12)
- Monitor Network tab → WebSocket messages
- Verify messages received every 500ms (batched delta updates)
- Verify no dropped frames in rendering

---

### Scenario 7: Garbage Collection (P1)

**Objective**: Verify sources are removed after 5 minutes of inactivity

**Steps**:
1. With active GPS source, disconnect device
2. Wait 5 minutes and 10 seconds
3. Check WebSocket logger for removal event

**Expected WebSocket Message** (after 5 minutes):
```json
{
  "event": "sourceRemoved",
  "sourceId": "NMEA2000-42",
  "timestamp": 301234,
  "reason": "stale"
}
```

**Expected Dashboard Update**:
- GPS → PGN129025 → NMEA2000-42: Row disappears from table

**Success Criteria**:
- ✅ Removal event sent to WebSocket clients (FR-021)
- ✅ Source removed from registry
- ✅ Memory reclaimed (verify via diagnostics endpoint if available)

**Shortcut for Testing** (modify temporarily):
- Reduce GC threshold from 300000ms to 10000ms (10 seconds) in SourceRegistry.cpp
- Test with 10-second wait instead of 5 minutes
- Restore original threshold before production build

---

### Scenario 8: 50-Source Limit (P1)

**Objective**: Verify eviction when source limit reached

**Steps**:
1. Simulate 50 active sources (use script or mock)
2. Add 51st source
3. Check WebSocket logger for eviction

**Expected Behavior**:
- Oldest inactive source (highest `timeSinceLast`) is removed
- Removal event sent: `{"event": "sourceRemoved", "sourceId": "<oldest>", "reason": "evicted"}`
- New source added successfully

**Mock Script** (for testing):
```python
# test_50_sources.py
import websocket
import json
import time

ws = websocket.WebSocket()
ws.connect("ws://192.168.1.100/source-stats")

# Send mock updates for 51 sources
for sid in range(51):
    # Simulate NMEA2000 message with unique SID
    # (requires test harness, not directly mockable via WebSocket)
    pass
```

**Alternative**: Reduce MAX_SOURCES to 5 temporarily for easier testing

**Success Criteria**:
- ✅ Registry never exceeds 50 sources (FR-019)
- ✅ Eviction removes oldest inactive source
- ✅ Removal event includes reason="evicted"

---

### Scenario 9: Node.js Proxy (P3, Optional)

**Objective**: Verify Node.js proxy relays source statistics

**Steps**:
1. Start Node.js proxy:
   ```bash
   cd nodejs-boatdata-viewer
   ESP32_IP=192.168.1.100 node server.js
   ```

2. Open proxy dashboard: `http://localhost:3000/sources.html`

3. Verify same data as direct ESP32 connection

**Expected Behavior**:
- Proxy connects to ESP32 WebSocket (`ws://192.168.1.100/source-stats`)
- Browser connects to proxy (`ws://localhost:3000/source-stats`)
- Dashboard displays same sources as direct connection
- Multiple browser clients supported (connect 3+ browsers simultaneously)

**Success Criteria**:
- ✅ Proxy relays full snapshot on connect
- ✅ Proxy relays delta updates every 500ms
- ✅ Proxy relays removal events
- ✅ Multiple clients receive updates without ESP32 overload

**Troubleshooting**:
- **Proxy can't connect to ESP32**: Verify ESP32 IP in config.json, check firewall
- **Browser can't connect to proxy**: Verify Node.js server running, check port 3000

---

## Performance Validation

### Memory Footprint (SC-007)

**Check ESP32 heap usage**:
1. Add diagnostic endpoint in main.cpp:
   ```cpp
   server.on("/diagnostics", HTTP_GET, [](AsyncWebServerRequest *request){
       String json = "{\"freeHeap\":" + String(ESP.getFreeHeap()) +
                     ",\"sourcesCount\":" + String(sourceRegistry.getTotalSourceCount()) + "}";
       request->send(200, "application/json", json);
   });
   ```

2. Query endpoint: `curl http://192.168.1.100/diagnostics`

3. Calculate: `sourceStatsMemory = baselineHeap - currentHeap`

**Success Criteria**:
- ✅ Memory usage <10KB for 20 sources
- ✅ Memory usage <20KB for 50 sources

### WebSocket Throughput (SC-004, SC-006)

**Measure update latency**:
1. Open browser developer tools (F12)
2. Network tab → Filter "WS" (WebSocket)
3. Select WebSocket connection
4. Monitor Messages tab for timestamps

**Calculate latency**:
- ESP32 timestamp (from JSON): `message.timestamp`
- Browser receive time: `performance.now()` when message handler called
- Latency = browser receive time - ESP32 timestamp (approximate, assuming clock sync)

**Success Criteria**:
- ✅ Delta updates sent every 500ms ±50ms (SC-004)
- ✅ Visual updates <200ms after WebSocket receive (SC-006)

### Dashboard Load Time (SC-005)

**Measure page load**:
1. Open browser developer tools
2. Network tab → Disable cache
3. Hard refresh page (Ctrl+Shift+R)
4. Check "Finish" time in Network tab

**Success Criteria**:
- ✅ Dashboard loads <2 seconds (SC-005)

---

## Troubleshooting Guide

### WebSocket Connection Fails

**Symptoms**: Browser shows "Disconnected" status

**Checks**:
1. Verify ESP32 IP address: `ping 192.168.1.100`
2. Verify WebSocket endpoint: `curl http://192.168.1.100/source-stats` (should fail with HTTP 400, WebSocket only)
3. Check ESP32 serial logs for WebSocket handler registration
4. Verify firewall allows WebSocket (port 80)

**Solution**: Restart ESP32, verify network connectivity

### Sources Not Appearing

**Symptoms**: Dashboard empty despite NMEA devices connected

**Checks**:
1. Verify NMEA handlers call `SourceRegistry::recordUpdate()`
2. Check Serial2/CAN bus connections
3. Monitor WebSocketLogger for NMEA parse events
4. Verify PGN/sentence handlers registered

**Solution**: Add debug logs to NMEA handlers, verify hardware connections

### Frequency Always 0.0 Hz

**Symptoms**: Source discovered but frequency not calculated

**Checks**:
1. Verify source has received ≥10 messages (buffer must be full)
2. Check `bufferFull` flag in source structure
3. Verify FrequencyCalculator logic (unit tests)

**Solution**: Wait for 10 samples (1 second at 10 Hz), check buffer logic

### Dashboard Not Updating

**Symptoms**: Initial data loads but no real-time updates

**Checks**:
1. Verify WebSocket connection active (browser dev tools)
2. Check `updateStaleFlags()` called every 500ms (ReactESP timer)
3. Verify `hasChanges()` flag set correctly
4. Check browser console for JavaScript errors

**Solution**: Hard refresh page, check ReactESP event loop running

---

## Success Checklist

Before marking feature complete, verify all scenarios:

**Priority 1 (P1) - Must Pass**:
- [ ] Scenario 1: NMEA 2000 source discovery
- [ ] Scenario 2: NMEA 0183 source discovery
- [ ] Scenario 3: Staleness detection
- [ ] Scenario 4: Source reactivation
- [ ] Scenario 5: Multiple sources per message type
- [ ] Scenario 7: Garbage collection
- [ ] Scenario 8: 50-source limit enforcement
- [ ] Memory footprint <10KB (20 sources)
- [ ] Frequency accuracy ±10%
- [ ] Staleness detection within 5.5s

**Priority 2 (P2) - Should Pass**:
- [ ] Scenario 6: Dashboard responsiveness
- [ ] Dashboard loads <2 seconds
- [ ] Visual updates <200ms latency

**Priority 3 (P3) - Optional**:
- [ ] Scenario 9: Node.js proxy integration

---

## Next Steps

After validation passes:
1. **Code Review**: Submit PR for QA subagent review
2. **Documentation**: Update README.md with usage examples
3. **Deployment**: Tag release with firmware version
4. **Field Testing**: Deploy to boat, test with real marine environment

## Appendix: Test Data Sources

### Mock NMEA 2000 Messages

If no physical NMEA 2000 devices available, use actisense_reader tool:

```bash
# Send mock PGN 129025 (GPS Position) at 10 Hz
actisense_sender --pgn 129025 --sid 42 --rate 10 \
  --data "lat:37.8,-122.4,lon:37.8,-122.4"
```

### Mock NMEA 0183 Sentences

```bash
# Send mock RSA sentences at 1 Hz via Serial2
echo -e '$APRSA,-10.5,A,,V*3A\r\n' > /dev/ttyUSB0
# Repeat every 1 second
```

### Example NMEA 0183 Log Files

Use log files from `examples/nmea0183-logs/` (if available) with replay tool:

```bash
python3 scripts/replay_nmea0183.py examples/nmea0183-logs/autopilot.log --port /dev/ttyUSB0
```

---

**Document Version**: 1.0.0
**Last Updated**: 2025-10-13
**Tested On**: ESP32-DevKitC, ESP32-S3
