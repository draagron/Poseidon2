---
**⚠️ LEGACY IMPLEMENTATION NOTICE**

This specification describes the initial WiFi management implementation which used UDP broadcast logging (port 4444). The system has since been migrated to WebSocket logging for improved reliability.

**Current Implementation**: WebSocket logging via `ws://<device-ip>/logs`
**Historical Implementation** (described below): UDP broadcast logging on port 4444

This document is preserved for historical reference and architectural decision context. For current logging setup, see [README.md](../../README.md) and [CLAUDE.md](../../CLAUDE.md).

---

# Quickstart: WiFi Network Management Foundation

This guide validates the WiFi configuration and connection management feature.

## Prerequisites

- ESP32 device with Poseidon Gateway firmware installed
- Access to 3 different WiFi networks for testing
- Computer connected to same network as device
- `curl` or web browser for HTTP requests
- UDP listener (e.g., `netcat`, `socat`) for debug logs

## Test Environment Setup

### 1. Prepare WiFi Networks

You'll need access to 3 test networks:
- **Network 1**: Your primary network (e.g., home WiFi)
- **Network 2**: Secondary network (e.g., mobile hotspot)
- **Network 3**: Tertiary network (e.g., guest network)

Note: Networks don't need to be simultaneously available.

### 2. Start UDP Log Listener

```bash
# Listen for debug logs on port 4444
nc -ul 4444
```

Keep this terminal open to monitor WiFi connection events.

### 3. Factory Reset Device (Optional)

If testing from scratch:
```bash
# Connect via serial (for this one-time operation)
# Delete existing config
# This step is manual - use PlatformIO monitor or similar
```

---

## User Story 1: First-Time Configuration

**Scenario**: Configure WiFi on a device without existing configuration.

### Step 1: Create Configuration File

Create `wifi.conf` with your test networks:
```
MyHome_WiFi,mypassword123
MobileHotspot,hotspotpass
GuestNetwork,guestpass
```

**Validation**: File has exactly 3 lines, comma-separated SSID,password format.

### Step 2: Power On Device

- Device boots with no existing configuration
- Watch UDP logs: Should see `CONFIG_LOADED: false` or similar
- Device attempts to connect but has no networks
- Device reboots after failing all attempts (reboot loop until configured)

**Expected UDP Logs**:
```json
{"level":"ERROR","event":"CONFIG_LOADED","data":{"success":false,"reason":"File not found"}}
{"level":"WARN","event":"REBOOT_SCHEDULED","data":{"delay_seconds":5}}
```

### Step 3: Upload Configuration

Connect device via Ethernet/USB initially, or pre-configure one network manually.

Once device has network access:
```bash
curl -X POST \
  -F "config=@wifi.conf" \
  http://<device-ip>/upload-wifi-config
```

**Expected Response**:
```json
{
  "status": "success",
  "message": "Configuration uploaded successfully. Device will reboot in 5 seconds.",
  "networks_count": 3
}
```

**Expected UDP Logs**:
```json
{"level":"INFO","event":"CONFIG_SAVED","data":{"networks_count":3}}
{"level":"INFO","event":"REBOOT_SCHEDULED","data":{"delay_seconds":5}}
```

### Step 4: Verify Reboot and Connection

- Device reboots automatically after 5 seconds
- Attempts to connect to networks in order
- First successful network (e.g., MyHome_WiFi) connects

**Expected UDP Logs**:
```json
{"level":"INFO","event":"CONNECTION_ATTEMPT","data":{"ssid":"MyHome_WiFi","attempt":1,"timeout_seconds":30}}
{"level":"INFO","event":"CONNECTION_SUCCESS","data":{"ssid":"MyHome_WiFi","ip":"192.168.1.100"}}
```

### Step 5: Verify Status

```bash
curl http://<device-ip>/wifi-status
```

**Expected Response**:
```json
{
  "status": "CONNECTED",
  "ssid": "MyHome_WiFi",
  "ip_address": "192.168.1.100",
  "signal_strength": -45,
  "uptime_seconds": 60
}
```

**✅ Success Criteria**:
- Configuration uploaded successfully
- Device rebooted
- Connected to highest-priority available network
- Status query returns CONNECTED

---

## User Story 2: Network Failover

**Scenario**: First network unavailable, device connects to second.

### Step 1: Disable Primary Network

Turn off "MyHome_WiFi" (router off, or move device out of range).

### Step 2: Reboot Device

```bash
curl -X POST http://<device-ip>/reboot
```

Or power cycle manually.

### Step 3: Monitor Connection Attempts

Watch UDP logs:

**Expected Sequence**:
```json
{"event":"CONNECTION_ATTEMPT","data":{"ssid":"MyHome_WiFi","attempt":1}}
// ... 30 seconds pass ...
{"event":"CONNECTION_FAILED","data":{"ssid":"MyHome_WiFi","reason":"Timeout"}}
{"event":"CONNECTION_ATTEMPT","data":{"ssid":"MobileHotspot","attempt":1}}
{"event":"CONNECTION_SUCCESS","data":{"ssid":"MobileHotspot"}}
```

### Step 4: Verify Connection

```bash
curl http://<device-ip>/wifi-status
```

**Expected**: Connected to "MobileHotspot" (second network).

**✅ Success Criteria**:
- First network skipped after 30-second timeout
- Second network connected successfully
- Total time to connect: ~30-60 seconds

---

## User Story 3: Connection Loss Recovery

**Scenario**: Device loses connection after successful connection.

### Step 1: Establish Connection

Ensure device is connected to a network (use status query to verify).

### Step 2: Simulate Connection Loss

- Move device out of WiFi range, OR
- Disable WiFi router temporarily

### Step 3: Monitor Reconnection Attempts

Watch UDP logs:

**Expected**:
```json
{"event":"CONNECTION_LOST","data":{"ssid":"MobileHotspot"}}
{"event":"CONNECTION_ATTEMPT","data":{"ssid":"MobileHotspot","attempt":1}}
// Retries every 30 seconds on same network
{"event":"CONNECTION_ATTEMPT","data":{"ssid":"MobileHotspot","attempt":2}}
```

### Step 4: Restore Network Access

Re-enable router or move device back in range.

**Expected**:
```json
{"event":"CONNECTION_SUCCESS","data":{"ssid":"MobileHotspot"}}
```

**✅ Success Criteria**:
- Device retries the SAME network (no failover to other networks)
- Reconnects automatically when network available
- No reboot triggered (only reboots if ALL networks fail)

---

## User Story 4: All Networks Unavailable

**Scenario**: Device cannot connect to any configured network.

### Step 1: Disable All Networks

Turn off all 3 configured WiFi networks.

### Step 2: Reboot Device

Power cycle or use reboot endpoint.

### Step 3: Monitor Reboot Loop

**Expected UDP Logs**:
```json
{"event":"CONNECTION_ATTEMPT","data":{"ssid":"MyHome_WiFi","attempt":1}}
{"event":"CONNECTION_FAILED","data":{"ssid":"MyHome_WiFi","reason":"Timeout"}}
{"event":"CONNECTION_ATTEMPT","data":{"ssid":"MobileHotspot","attempt":1}}
{"event":"CONNECTION_FAILED","data":{"ssid":"MobileHotspot","reason":"Timeout"}}
{"event":"CONNECTION_ATTEMPT","data":{"ssid":"GuestNetwork","attempt":1}}
{"event":"CONNECTION_FAILED","data":{"ssid":"GuestNetwork","reason":"Timeout"}}
{"event":"REBOOT_SCHEDULED","data":{"delay_seconds":5}}
// Device reboots and repeats...
```

### Step 4: Restore One Network

Enable "MyHome_WiFi" again.

**Expected**: Device connects on next boot cycle.

**✅ Success Criteria**:
- All networks attempted in sequence (90 seconds total)
- Device reboots after all fail
- Reboot loop continues until network available
- 5-second delay between reboots

---

## User Story 5: Invalid Configuration Handling

**Scenario**: User uploads configuration with errors.

### Step 1: Create Invalid Config

`invalid-wifi.conf`:
```
ThisSSIDIsWayTooLongAndExceedsTheMaximumOf32Characters,password
ValidNetwork,short
TooManyCommas,pass,word,extra
```

### Step 2: Upload Invalid Config

```bash
curl -X POST \
  -F "config=@invalid-wifi.conf" \
  http://<device-ip>/upload-wifi-config
```

**Expected Response** (400 Bad Request):
```json
{
  "status": "error",
  "message": "Invalid configuration file",
  "errors": [
    "Line 1: SSID exceeds 32 characters",
    "Line 2: Password must be 8-63 characters for WPA2",
    "Line 3: Invalid format - multiple commas detected"
  ]
}
```

### Step 3: Verify Existing Config Unchanged

```bash
curl http://<device-ip>/wifi-config
```

**Expected**: Previous valid configuration still intact.

**✅ Success Criteria**:
- Invalid config rejected with detailed error messages
- Existing config remains unchanged
- Device does not reboot

---

## Integration Test: Services Run Independently

**Scenario**: Verify WiFi connection does not block other gateway services.

### Setup

Modify test environment to log timestamps from multiple services.

### Step 1: Disable WiFi Networks

All networks unavailable.

### Step 2: Monitor Service Startup

Watch logs from multiple gateway components:
- NMEA2000 handler
- NMEA0183 parser
- UDP logger
- OLED display

**Expected**:
- All services start immediately (within 1-2 seconds of boot)
- WiFi connection attempts run in parallel
- No service waits for WiFi connection

**✅ Success Criteria**:
- Service start times independent of WiFi status
- NMEA handlers operational even without WiFi
- UDP logger buffering messages even before WiFi connected

---

## Performance Validation

### Connection Timing Test

| Scenario | Expected Time | Actual Time | Pass/Fail |
|----------|--------------|-------------|-----------|
| First network available | <30s | | |
| First network fails, second succeeds | 30-60s | | |
| All networks fail → reboot | ~95s (90s attempts + 5s delay) | | |
| Reconnect after disconnect | <30s | | |

### Memory Usage Test

Check memory before and after WiFi configuration loaded:

```cpp
// Log free heap
Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
```

**Expected**: <500 bytes difference (config footprint ~334 bytes).

---

## Troubleshooting

### Issue: Device Not Rebooting After Config Upload

**Check**: UDP logs for "REBOOT_SCHEDULED" event
**Fix**: Verify reboot delay code executed, check for exceptions

### Issue: Connection Timeout Not Working

**Check**: ReactESP event loop running, timeout callback registered
**Fix**: Verify millis() timing logic, check for blocking code

### Issue: Config File Not Persisting

**Check**: LittleFS mounted correctly, write permissions
**Fix**: Format LittleFS if corrupted, check partition table

### Issue: UDP Logs Not Received

**Check**: Device IP address, firewall rules, listener port
**Fix**: Verify device on same network segment, check UDP broadcast address

---

## Acceptance Criteria Checklist

- [ ] Config file uploads successfully via HTTP
- [ ] Device reboots after valid config upload
- [ ] Connects to first available network in priority order
- [ ] Skips unavailable networks after 30-second timeout
- [ ] Retries same network after connection loss (no failover)
- [ ] Reboots after all networks fail
- [ ] 5-second delay between reboot cycles
- [ ] Invalid config rejected with error details
- [ ] Services start independently of WiFi status
- [ ] UDP logs provide visibility into all connection events
- [ ] Status endpoint returns accurate connection state
- [ ] Maximum 3 networks enforced
- [ ] SSID/password validation enforced (1-32, 8-63)

**All items must pass for feature acceptance.**
