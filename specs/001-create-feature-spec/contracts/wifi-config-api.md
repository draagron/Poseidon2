---
**⚠️ LEGACY IMPLEMENTATION NOTICE**

This specification describes the initial WiFi management implementation which used UDP broadcast logging (port 4444). The system has since been migrated to WebSocket logging for improved reliability.

**Current Implementation**: WebSocket logging via `ws://<device-ip>/logs`
**Historical Implementation** (described below): UDP broadcast logging on port 4444

This document is preserved for historical reference and architectural decision context. For current logging setup, see [README.md](../../../README.md) and [CLAUDE.md](../../../CLAUDE.md).

---

# API Contract: WiFi Configuration Management

## HTTP Endpoint: Upload Configuration

### POST /upload-wifi-config

Uploads a new WiFi configuration file to the device.

**Request**:
```http
POST /upload-wifi-config HTTP/1.1
Host: <device-ip>
Content-Type: multipart/form-data; boundary=----WebKitFormBoundary

------WebKitFormBoundary
Content-Disposition: form-data; name="config"; filename="wifi.conf"
Content-Type: text/plain

HomeNetwork,mypassword123
Marina_Guest,guestpass
------WebKitFormBoundary--
```

**Validation Rules**:
- File must be plain text
- Maximum 3 lines
- Each line format: `SSID,password`
- SSID: 1-32 characters, non-empty
- Password: 0 (empty) or 8-63 characters

**Response** (Success):
```http
HTTP/1.1 200 OK
Content-Type: application/json

{
  "status": "success",
  "message": "Configuration uploaded successfully. Device will reboot in 5 seconds.",
  "networks_count": 2
}
```

**Response** (Validation Error):
```http
HTTP/1.1 400 Bad Request
Content-Type: application/json

{
  "status": "error",
  "message": "Invalid configuration file",
  "errors": [
    "Line 1: SSID exceeds 32 characters",
    "Line 3: Password must be 8-63 characters for WPA2"
  ]
}
```

**Response** (Server Error):
```http
HTTP/1.1 500 Internal Server Error
Content-Type: application/json

{
  "status": "error",
  "message": "Failed to write configuration to flash storage"
}
```

---

## HTTP Endpoint: Get Current Configuration

### GET /wifi-config

Retrieves the current WiFi configuration (SSIDs only, passwords redacted).

**Request**:
```http
GET /wifi-config HTTP/1.1
Host: <device-ip>
```

**Response**:
```http
HTTP/1.1 200 OK
Content-Type: application/json

{
  "networks": [
    {"ssid": "HomeNetwork", "priority": 1},
    {"ssid": "Marina_Guest", "priority": 2}
  ],
  "max_networks": 3,
  "current_connection": "HomeNetwork"
}
```

---

## HTTP Endpoint: Get Connection Status

### GET /wifi-status

Returns current WiFi connection state.

**Request**:
```http
GET /wifi-status HTTP/1.1
Host: <device-ip>
```

**Response** (Connected):
```http
HTTP/1.1 200 OK
Content-Type: application/json

{
  "status": "CONNECTED",
  "ssid": "HomeNetwork",
  "ip_address": "192.168.1.100",
  "signal_strength": -45,
  "uptime_seconds": 3600
}
```

**Response** (Connecting):
```http
HTTP/1.1 200 OK
Content-Type: application/json

{
  "status": "CONNECTING",
  "current_attempt": "Marina_Guest",
  "attempt_number": 2,
  "time_remaining_seconds": 15
}
```

**Response** (Disconnected):
```http
HTTP/1.1 200 OK
Content-Type: application/json

{
  "status": "DISCONNECTED",
  "retry_count": 5,
  "next_reboot_in_seconds": 3
}
```

---

## Internal Interface: WiFiManager Component

### WiFiManager::loadConfig()

Loads WiFi configuration from persistent storage.

**Signature**:
```cpp
bool loadConfig(WiFiConfigFile& config)
```

**Returns**:
- `true`: Config loaded successfully
- `false`: File missing, corrupted, or parse errors

**Side Effects**:
- Reads `/wifi.conf` from LittleFS
- Logs parse errors via UDP if any
- Populates config parameter with valid networks

**Error Handling**:
- Missing file: Returns false, logs "Config file not found"
- Parse error: Returns partial config if any valid lines exist
- No valid lines: Returns false, logs "No valid networks in config"

---

### WiFiManager::saveConfig()

Saves WiFi configuration to persistent storage.

**Signature**:
```cpp
bool saveConfig(const WiFiConfigFile& config)
```

**Returns**:
- `true`: Config saved successfully
- `false`: Flash write error

**Preconditions**:
- config.count ≤ 3
- All networks in config are pre-validated

**Side Effects**:
- Writes `/wifi.conf` to LittleFS
- Overwrites existing file
- Logs success/failure via UDP

---

### WiFiManager::connect()

Attempts to connect to next available network.

**Signature**:
```cpp
void connect(WiFiConnectionState& state, const WiFiConfigFile& config)
```

**Behavior**:
- Uses state.currentNetworkIndex to select network
- Calls WiFi.begin() with credentials
- Sets state.status = CONNECTING
- Records state.attemptStartTime
- Registers timeout callback (30 seconds)

**Side Effects**:
- Initiates WiFi connection attempt
- Updates connection state
- Logs attempt details via UDP

---

### WiFiManager::checkTimeout()

Checks if current connection attempt has exceeded timeout.

**Signature**:
```cpp
bool checkTimeout(const WiFiConnectionState& state)
```

**Returns**:
- `true`: Timeout exceeded (30 seconds elapsed)
- `false`: Still within timeout window

**Usage**: Called periodically from ReactESP event loop

---

### WiFiManager::handleDisconnect()

Handles WiFi disconnection event.

**Signature**:
```cpp
void handleDisconnect(WiFiConnectionState& state)
```

**Behavior**:
- Sets state.status = DISCONNECTED
- Keeps state.currentNetworkIndex unchanged (retry same network)
- Increments state.retryCount
- Schedules reconnection attempt

**Side Effects**:
- Logs disconnection via UDP
- Triggers status update to other services

---

## UDP Logging Protocol

All WiFi events broadcast via UDP to port 4444.

**Log Format**:
```json
{
  "timestamp": 1234567890,
  "level": "INFO",
  "component": "WiFiManager",
  "event": "CONNECTION_ATTEMPT",
  "data": {
    "ssid": "HomeNetwork",
    "attempt": 1,
    "timeout_seconds": 30
  }
}
```

**Event Types**:
- `CONNECTION_ATTEMPT`: Starting connection to network
- `CONNECTION_SUCCESS`: Successfully connected
- `CONNECTION_FAILED`: Timeout or authentication failure
- `CONNECTION_LOST`: Disconnected after successful connection
- `CONFIG_LOADED`: Configuration file read from flash
- `CONFIG_SAVED`: Configuration file written to flash
- `CONFIG_INVALID`: Validation errors in config file
- `REBOOT_SCHEDULED`: All networks exhausted, rebooting

---

## Contract Test Scenarios

### Test 1: Valid Config Upload
**Given**: Device is online
**When**: POST /upload-wifi-config with valid 3-network file
**Then**: Response 200, config saved, reboot scheduled

### Test 2: Invalid SSID Length
**Given**: Device is online
**When**: POST with SSID > 32 characters
**Then**: Response 400, error details in JSON

### Test 3: Max Networks Exceeded
**Given**: Device is online
**When**: POST with 5 networks
**Then**: Response 400, "Maximum 3 networks allowed"

### Test 4: Connection Status Query
**Given**: Device is connected to "HomeNetwork"
**When**: GET /wifi-status
**Then**: Response 200, status="CONNECTED", ssid="HomeNetwork"

### Test 5: Config Retrieval (Passwords Redacted)
**Given**: Config contains passwords
**When**: GET /wifi-config
**Then**: Response contains SSIDs only, no passwords
