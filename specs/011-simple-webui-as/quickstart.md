# Quickstart: Simple WebUI Validation Guide

**Feature**: 011-simple-webui-as (Simple WebUI for BoatData Streaming)
**Purpose**: Step-by-step guide to validate implementation is working correctly
**Audience**: Developers implementing and testing this feature

## Prerequisites

Before starting validation, ensure you have:

1. **Hardware**:
   - ESP32 development board (SH-ESP32 or compatible)
   - USB cable for serial connection
   - Computer with WiFi capability

2. **Software**:
   - PlatformIO CLI installed
   - Python 3.7+ with `websocket-client` package (for WebSocket logger)
   - Modern web browser (Chrome 88+, Firefox 78+, or Safari 14+)

3. **Project State**:
   - All Phase 2 implementation tasks completed
   - Code compiles without errors: `pio run`
   - All tests pass: `pio test -e native`

4. **Network**:
   - WiFi network accessible to both ESP32 and computer
   - ESP32 has known IP address (check serial monitor or use mDNS: `poseidon2.local`)

## Step 1: Build and Upload Firmware

### 1.1 Compile Firmware

```bash
cd /home/niels/Dev/Poseidon2
pio run
```

**Expected Output**:
```
Environment Status    Duration
---------------------  ------------  ------------
esp32dev              SUCCESS       00:01:23
```

**Troubleshooting**:
- If compilation fails, check for syntax errors in new components
- Verify all required includes present in `main.cpp`
- Check PlatformIO dependencies (ArduinoJson should be auto-installed)

### 1.2 Upload Firmware to ESP32

```bash
pio run --target upload
```

**Expected Output**:
```
Uploading .pio/build/esp32dev/firmware.bin
...
Writing at 0x00010000... (100%)
Wrote 850000 bytes in 10.5 seconds
Leaving... Hard resetting via RTS pin...
```

### 1.3 Create HTML Dashboard File

Create `data/stream.html` following the HTMLDashboardContract.md specification.

**Quick Template** (for initial testing):
```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Poseidon2 BoatData</title>
    <style>
        body { font-family: Arial, sans-serif; background: #0a1929; color: #e3f2fd; padding: 20px; }
        .status { font-size: 18px; margin-bottom: 20px; }
        .connected { color: #4caf50; }
        .disconnected { color: #f44336; }
        pre { background: #132f4c; padding: 10px; border-radius: 5px; }
    </style>
</head>
<body>
    <div class="status" id="status">Disconnected</div>
    <pre id="data">Waiting for data...</pre>
    <script>
        const ws = new WebSocket('ws://' + location.host + '/boatdata');
        const statusElem = document.getElementById('status');
        const dataElem = document.getElementById('data');

        ws.onopen = () => {
            statusElem.textContent = 'Connected';
            statusElem.className = 'status connected';
        };

        ws.onmessage = (event) => {
            const data = JSON.parse(event.data);
            dataElem.textContent = JSON.stringify(data, null, 2);
        };

        ws.onclose = () => {
            statusElem.textContent = 'Disconnected';
            statusElem.className = 'status disconnected';
            setTimeout(() => ws = new WebSocket('ws://' + location.host + '/boatdata'), 5000);
        };
    </script>
</body>
</html>
```

**Save to**: `data/stream.html`

### 1.4 Upload LittleFS Filesystem

```bash
pio run --target uploadfs
```

**Expected Output**:
```
Building LittleFS filesystem
/stream.html
Looking for upload port...
Uploading .pio/build/esp32dev/littlefs.bin
...
Wrote 290816 bytes in 5.2 seconds
```

**Troubleshooting**:
- If upload fails, check USB cable connection
- Verify `board_build.filesystem = littlefs` in platformio.ini
- Check data/ directory exists and contains stream.html

## Step 2: Verify Boot Sequence

### 2.1 Connect to Serial Monitor

```bash
pio device monitor
```

**Expected Boot Sequence**:
1. ESP32 boot messages
2. WiFi connection attempt
3. "WebSocketLogger started on /logs"
4. "BoatData WebSocket started on /boatdata"
5. "HTTP server started on port 80"
6. IP address displayed (e.g., "Connected to WiFi: 192.168.1.100")

**Success Criteria**:
- ✅ No error messages during boot
- ✅ WiFi connected within 10 seconds
- ✅ All WebSocket endpoints registered
- ✅ IP address obtained from DHCP

**Troubleshooting**:
- If WiFi fails: Check credentials in config.h
- If WebSocket startup fails: Check logger initialization order
- If HTTP server fails: Check port 80 not already in use

### 2.2 Verify WebSocket Logging

Open a second terminal and connect to WebSocket logger:

```bash
cd /home/niels/Dev/Poseidon2/src/helpers
python3 ws_logger.py <ESP32_IP>
```

**Expected Output**:
```json
{"level":"INFO","component":"WebSocketLogger","event":"CLIENT_CONNECTED","data":{"clientId":123}}
{"level":"INFO","component":"BoatDataStream","event":"BROADCAST_STARTED","data":{"interval_ms":1000}}
```

**Success Criteria**:
- ✅ WebSocket connection established
- ✅ Log messages appear in JSON format
- ✅ No ERROR or FATAL level messages

## Step 3: Verify HTTP Endpoint

### 3.1 Test /stream Endpoint

Open browser and navigate to:
```
http://<ESP32_IP>/stream
```

**Expected Result**:
- ✅ HTML page loads within 2 seconds
- ✅ Page title: "Poseidon2 BoatData" (or similar)
- ✅ Status shows "Connecting..." or "Connected"
- ✅ No 404 or 500 errors

**Troubleshooting**:
- **404 Not Found**: LittleFS upload failed or file path incorrect
  - Solution: Re-run `pio run --target uploadfs`
- **500 Internal Server Error**: LittleFS read error
  - Solution: Check LittleFS.begin() in main.cpp
- **Page doesn't load**: Wrong IP address or ESP32 not on network
  - Solution: Check serial monitor for correct IP

### 3.2 Verify Content-Type Header

Open browser developer tools (F12) → Network tab → Reload page

**Check Headers**:
```
Status: 200 OK
Content-Type: text/html
Content-Length: <size>
```

**Success Criteria**:
- ✅ Status code: 200
- ✅ Content-Type: text/html (not application/octet-stream)
- ✅ Content-Length matches file size

## Step 4: Verify WebSocket Connection

### 4.1 Check Connection Status in Browser

**In browser at http://<ESP32_IP>/stream**:

**Expected Behavior**:
1. Status indicator shows "Connecting..." (yellow) for <1 second
2. Status changes to "Connected" (green)
3. Data appears within 2 seconds (first broadcast at 1 Hz)

**Success Criteria**:
- ✅ WebSocket connects automatically on page load
- ✅ Connection status updates correctly
- ✅ No JavaScript errors in browser console (F12 → Console)

### 4.2 Check WebSocket Messages in Browser DevTools

**Browser DevTools** (F12) → Network tab → Filter: WS (WebSocket)

**Expected**:
- ✅ Connection to `ws://<ESP32_IP>/boatdata` shows status 101 (Switching Protocols)
- ✅ Messages tab shows JSON messages arriving every ~1 second
- ✅ Message size: ~1400-1800 bytes

**Troubleshooting**:
- **No WebSocket connection**: Check wsBoatData global variable exists in main.cpp
- **Connection refused**: Verify WebSocket endpoint registered with server
- **No messages**: Check ReactESP broadcast timer is running

### 4.3 Verify WebSocket Logs on ESP32

**In WebSocket logger terminal** (`ws_logger.py`):

**Expected Logs**:
```json
{"level":"INFO","component":"BoatDataStream","event":"CLIENT_CONNECTED","data":{"clientId":456,"totalClients":1}}
{"level":"DEBUG","component":"BoatDataStream","event":"BROADCAST","data":{"clients":1,"size":1567}}
{"level":"DEBUG","component":"BoatDataStream","event":"BROADCAST","data":{"clients":1,"size":1567}}
...
```

**Success Criteria**:
- ✅ CLIENT_CONNECTED event logged when browser opens
- ✅ BROADCAST events logged at ~1 Hz
- ✅ JSON size reported as ~1400-1800 bytes

## Step 5: Verify JSON Data Flow

### 5.1 Inspect JSON Message Structure

**In browser console** (F12 → Console), type:
```javascript
ws.onmessage = (event) => { console.log(JSON.parse(event.data)); };
```

**Expected JSON Structure**:
```json
{
  "timestamp": 1697123456789,
  "gps": {
    "latitude": 37.774929,
    "longitude": -122.419418,
    "cog": 1.5708,
    "sog": 5.5,
    "variation": 0.2618,
    "available": true,
    "lastUpdate": 1697123456789
  },
  "compass": { /* 8 fields */ },
  "wind": { /* 4 fields */ },
  "dst": { /* 5 fields */ },
  "rudder": { /* 3 fields */ },
  "engine": { /* 5 fields */ },
  "saildrive": { /* 3 fields */ },
  "battery": { /* 12 fields */ },
  "shorePower": { /* 4 fields */ }
}
```

**Success Criteria**:
- ✅ All 9 sensor groups present (gps, compass, wind, dst, rudder, engine, saildrive, battery, shorePower)
- ✅ Root `timestamp` field present
- ✅ Each sensor group has `available` and `lastUpdate` fields
- ✅ Numeric values are JSON numbers (not strings)
- ✅ Boolean values are JSON booleans (true/false, not "true"/"false")

### 5.2 Verify Data Updates

**Monitor timestamp field** in browser console:

**Expected**:
- ✅ Timestamp updates every ~1 second
- ✅ Timestamp values increase monotonically
- ✅ No duplicate messages (timestamps always advance)

**Calculate Update Rate**:
```javascript
let lastTime = 0;
ws.onmessage = (event) => {
    const data = JSON.parse(event.data);
    if (lastTime > 0) {
        const delta = data.timestamp - lastTime;
        console.log('Update interval:', delta, 'ms');
    }
    lastTime = data.timestamp;
};
```

**Expected Interval**: 1000 ms ±50 ms

### 5.3 Verify Data Accuracy

**With live NMEA data** (if available):

1. Compare GPS coordinates in dashboard with known position
2. Compare compass heading with physical heading
3. Compare wind data with anemometer readings
4. Verify engine RPM matches tachometer

**Without live data** (mock/test data):

1. Set BoatData fields to known values
2. Verify JSON contains exact values (no conversion on server side)
3. Verify client-side conversions correct (radians→degrees, m/s→knots)

## Step 6: Verify Multi-Client Support

### 6.1 Open Multiple Browser Windows

Open 5 browser tabs/windows, all navigating to:
```
http://<ESP32_IP>/stream
```

**Expected Behavior**:
- ✅ All 5 clients connect successfully
- ✅ All clients receive data simultaneously
- ✅ No disconnections or errors

**In WebSocket logger**:
```json
{"level":"INFO","component":"BoatDataStream","event":"CLIENT_CONNECTED","data":{"clientId":456,"totalClients":1}}
{"level":"INFO","component":"BoatDataStream","event":"CLIENT_CONNECTED","data":{"clientId":457,"totalClients":2}}
{"level":"INFO","component":"BoatDataStream","event":"CLIENT_CONNECTED","data":{"clientId":458,"totalClients":3}}
{"level":"INFO","component":"BoatDataStream","event":"CLIENT_CONNECTED","data":{"clientId":459,"totalClients":4}}
{"level":"INFO","component":"BoatDataStream","event":"CLIENT_CONNECTED","data":{"clientId":460,"totalClients":5}}
```

**Success Criteria**:
- ✅ All 5 clients show "Connected" status
- ✅ All clients receive data at 1 Hz
- ✅ ESP32 logs show 5 concurrent clients
- ✅ No performance degradation

### 6.2 Test Client Limit (10 clients)

Open 11 browser windows.

**Expected Behavior**:
- ✅ Clients 1-10 connect successfully
- ✅ Client 11 is rejected with status code 1011 (Server overload)
- ✅ WebSocket logger shows MAX_CLIENTS_EXCEEDED warning

**Troubleshooting**:
- If all 11 clients connect: Check max client limit enforcement in setupBoatDataWebSocket()
- If limit is <10: Check ESPAsyncWebServer configuration

## Step 7: Verify Error Handling

### 7.1 Test Disconnection and Reconnect

**Test Steps**:
1. Open dashboard in browser (connected)
2. Restart ESP32: `pio device monitor --target upload`
3. Observe browser behavior

**Expected Behavior**:
- ✅ Status changes to "Disconnected" (red) when ESP32 reboots
- ✅ Status changes to "Connecting..." (yellow) after 5 seconds
- ✅ Status changes to "Connected" (green) when ESP32 boots
- ✅ Data resumes flowing within 2 seconds

**Success Criteria**:
- ✅ Auto-reconnect works without page reload
- ✅ No JavaScript errors in console

### 7.2 Test File Missing (404 Error)

**Test Steps**:
1. Navigate to `http://<ESP32_IP>/nonexistent.html`

**Expected Response**:
- ✅ HTTP 404 Not Found
- ✅ Plain text error message: "Dashboard file not found in LittleFS"

**In WebSocket logger**:
```json
{"level":"ERROR","component":"HTTPFileServer","event":"FILE_NOT_FOUND","data":{"path":"/nonexistent.html"}}
```

### 7.3 Test Serialization Failure

**Test Steps** (requires code modification for testing):
1. Temporarily modify BoatDataSerializer::toJSON() to return empty string
2. Upload firmware
3. Observe behavior

**Expected Behavior**:
- ✅ No WebSocket messages sent (wsBoatData.textAll() skipped)
- ✅ WebSocket logger shows SERIALIZATION_FAILED warning
- ✅ System continues operating (no crash)

## Step 8: Performance Validation

### 8.1 Measure Page Load Time

**Browser DevTools** (F12) → Network tab → Reload page

**Check Timing**:
- DNS lookup: <10 ms
- Connection: <10 ms
- Request/Response: <50 ms
- DOM Content Loaded: <500 ms
- WebSocket connection: <100 ms
- First data message: <2000 ms

**Success Criteria**:
- ✅ Total page load time: <2 seconds
- ✅ No resources fail to load

### 8.2 Measure Update Latency

**Browser Console**:
```javascript
ws.onmessage = (event) => {
    const data = JSON.parse(event.data);
    const now = Date.now();
    const latency = now - data.timestamp;
    console.log('Latency:', latency, 'ms');
};
```

**Expected Latency**: <100 ms (typically 20-50 ms)

**Success Criteria**:
- ✅ Latency consistently <100 ms
- ✅ No latency spikes >200 ms

### 8.3 Monitor Memory Usage

**In WebSocket logger**, watch for memory warnings:

**Expected**:
- ✅ No "Low heap" warnings
- ✅ No "Buffer overflow" warnings
- ✅ No "Out of memory" errors

**To check manually** (requires additional logging in main.cpp):
```cpp
app.onRepeat(10000, []() {
    logger.broadcastLog(LogLevel::DEBUG, "System", "HEAP_STATUS",
        String(F("{\"free\":")) + ESP.getFreeHeap() +
        F(",\"total\":") + ESP.getHeapSize() + F("}"));
});
```

**Expected Free Heap**: >200 KB (with 5 clients connected)

## Step 9: Browser Compatibility Testing

### 9.1 Desktop Browsers

Test on all available browsers:

- [ ] **Chrome 88+** (Windows/Mac/Linux)
- [ ] **Firefox 78+** (Windows/Mac/Linux)
- [ ] **Safari 14+** (Mac only)
- [ ] **Edge 88+** (Windows/Mac)

**For Each Browser**:
1. Load `http://<ESP32_IP>/stream`
2. Verify page layout correct
3. Verify WebSocket connects
4. Verify data displays correctly
5. Check console for errors

### 9.2 Mobile Browsers

Test on available devices:

- [ ] **Safari (iOS 14+)** - iPad/iPhone
- [ ] **Chrome (Android 88+)** - Phone/Tablet

**For Each Device**:
1. Verify responsive layout (1-2 columns depending on screen size)
2. Verify touch scrolling works
3. Verify data readable (font size appropriate)
4. Verify no layout overflow or horizontal scrolling

## Step 10: Integration Testing

### 10.1 Run Automated Integration Tests

```bash
pio test -e native -f test_webui_integration
```

**Expected Output**:
```
Test Summary:
test_html_serving (8/8 passed)
test_websocket_connection (5/5 passed)
test_json_data_flow (7/7 passed)
test_multi_client (5/5 passed)

Total: 25 tests, 25 passed, 0 failed
```

**Success Criteria**:
- ✅ All tests pass
- ✅ No memory leaks detected
- ✅ No assertion failures

### 10.2 Run Unit Tests

```bash
pio test -e native -f test_webui_units
```

**Expected Output**:
```
Test Summary:
test_json_format (17/17 passed)
test_serialization_performance (4/4 passed)
test_throttling (3/3 passed)

Total: 24 tests, 24 passed, 0 failed
```

## Success Criteria Checklist

### Core Functionality
- [ ] Firmware compiles without errors
- [ ] LittleFS upload succeeds
- [ ] ESP32 boots and connects to WiFi
- [ ] HTTP endpoint `/stream` serves HTML file
- [ ] WebSocket endpoint `/boatdata` accepts connections
- [ ] JSON messages broadcast at 1 Hz
- [ ] All 9 sensor groups present in JSON
- [ ] Browser displays data correctly

### Performance
- [ ] Page loads in <2 seconds
- [ ] WebSocket latency <100 ms
- [ ] Update rate 1 Hz ±50 ms
- [ ] No memory warnings
- [ ] Free heap >200 KB with 5 clients

### Error Handling
- [ ] Auto-reconnect works (5s delay)
- [ ] 404 error for missing files
- [ ] Graceful degradation on serialization failure
- [ ] No crashes on errors

### Multi-Client
- [ ] 5 concurrent clients supported
- [ ] 10 client limit enforced
- [ ] All clients receive same data simultaneously

### Browser Compatibility
- [ ] Works in Chrome/Edge 88+
- [ ] Works in Firefox 78+
- [ ] Works in Safari 14+ (if available)
- [ ] Mobile responsive layout (iOS/Android)

### Testing
- [ ] All unit tests pass (24/24)
- [ ] All integration tests pass (25/25)
- [ ] Manual browser tests complete

## Common Issues and Solutions

### Issue: WebSocket Not Connecting

**Symptoms**: Status stuck on "Connecting...", no data appears

**Causes**:
1. WebSocket endpoint not registered in main.cpp
2. Wrong WebSocket URL (should be `ws://` not `wss://`)
3. Firewall blocking WebSocket traffic
4. ESPAsyncWebServer not started

**Solutions**:
1. Verify `setupBoatDataWebSocket(webServer->getServer())` called in onWiFiConnected()
2. Check browser console for WebSocket URL (F12 → Network → WS)
3. Try from another device on same network
4. Check serial monitor for "HTTP server started" message

### Issue: Empty or Invalid JSON

**Symptoms**: Data shows "--" for all fields, or "JSON parse error" in console

**Causes**:
1. BoatDataSerializer::toJSON() returning empty string
2. BoatData not initialized (all values null)
3. ArduinoJson buffer overflow
4. Serialization taking >50ms (timeout)

**Solutions**:
1. Check WebSocket logs for SERIALIZATION_FAILED warnings
2. Verify BoatData initialized with mock data (if no NMEA sources)
3. Increase StaticJsonDocument size to 2048
4. Check for blocking code in serialization

### Issue: Page Loads But No Data

**Symptoms**: HTML displays, "Connected" status, but no data updates

**Causes**:
1. ReactESP broadcast timer not running
2. wsBoatData.textAll() not called
3. No clients connected to wsBoatData
4. Broadcast timer frequency incorrect

**Solutions**:
1. Verify ReactESP event loop in main.cpp: `app.onRepeat(1000, ...)`
2. Add debug log in broadcast timer callback
3. Check ESP32 logs for BROADCAST events
4. Verify timer interval is 1000ms (not 10ms or 10000ms)

### Issue: High Memory Usage / Crashes

**Symptoms**: ESP32 reboots randomly, "Low heap" warnings in logs

**Causes**:
1. Too many WebSocket clients (>10)
2. Memory leak in serialization
3. JSON buffer too large (heap allocation)
4. String concatenation in loop

**Solutions**:
1. Enforce 10 client limit in setupBoatDataWebSocket()
2. Use StaticJsonDocument<2048> (not DynamicJsonDocument)
3. Check for String operations in hot paths
4. Monitor free heap with `ESP.getFreeHeap()`

### Issue: Data Not Updating

**Symptoms**: JSON timestamp not incrementing, data appears stale

**Causes**:
1. BoatData not being updated (no NMEA sources)
2. ReactESP event loop blocked
3. Broadcast timer stopped
4. WebSocket connection dropped

**Solutions**:
1. Verify NMEA handlers are processing messages (check logs)
2. Check for blocking delays in main loop
3. Restart ESP32 and monitor serial output
4. Check browser console for WebSocket close events

## Next Steps

After successful validation:

1. **Document Issues**: Record any deviations from expected behavior in GitHub issues
2. **Run QA Review**: Submit PR for QA subagent review (constitutional requirement)
3. **Create Full-Featured Dashboard**: Replace minimal HTML with production dashboard (per HTMLDashboardContract.md)
4. **Performance Tuning**: Optimize JSON size, adjust broadcast rate if needed
5. **Documentation**: Update CLAUDE.md with Simple WebUI integration guide

## References

- **Feature Specification**: `specs/011-simple-webui-as/spec.md`
- **Implementation Plan**: `specs/011-simple-webui-as/plan.md`
- **Data Model**: `specs/011-simple-webui-as/data-model.md`
- **Contracts**: `specs/011-simple-webui-as/contracts/`
  - BoatDataSerializerContract.md
  - WebSocketEndpointContract.md
  - HTTPFileServerContract.md
  - HTMLDashboardContract.md

---
**Quickstart Version**: 1.0.0 | **Last Updated**: 2025-10-13
