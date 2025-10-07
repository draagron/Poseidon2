# Data Model: WiFi Network Management Foundation

## Entities

### WiFiCredentials
Represents a single WiFi network configuration entry.

**Attributes**:
- `ssid`: String (1-32 characters) - Network SSID
- `password`: String (0-63 characters) - WPA2 password (empty for open networks)

**Validation Rules**:
- SSID must not be empty
- SSID length: 1-32 characters (WiFi spec)
- Password length: 0 (open) or 8-63 characters (WPA2 spec)
- No newline characters allowed in either field

**Relationships**: Member of WiFiConfigFile

---

### WiFiConfigFile
Collection of WiFi network credentials with priority ordering.

**Attributes**:
- `networks`: Array of WiFiCredentials (max size: 3)
- `count`: Integer (0-3) - Number of valid networks loaded

**Validation Rules**:
- Maximum 3 networks allowed
- Networks ordered by priority (index 0 = highest priority)
- Duplicate SSIDs allowed (different locations, same name)

**Persistence**:
- File path: `/wifi.conf` (LittleFS)
- Format: Plain text, one network per line, comma-separated
- Example:
  ```
  HomeNetwork,mypassword123
  Marina_Guest,
  Alternate,backuppass
  ```

**State Transitions**:
1. Uninitialized → Loaded (successful file read)
2. Uninitialized → Empty (file missing/corrupted)
3. Loaded → Updated (user uploads new config)

**Relationships**: Contains multiple WiFiCredentials

---

### WiFiConnectionState
Tracks current connection attempt state and progress.

**Attributes**:
- `status`: Enum {DISCONNECTED, CONNECTING, CONNECTED, FAILED}
- `currentNetworkIndex`: Integer (0-2) - Which network from config being attempted
- `attemptStartTime`: Unsigned long (milliseconds) - When current attempt started
- `connectedSSID`: String - SSID of currently connected network (empty if not connected)
- `retryCount`: Integer - Number of times retried current network

**State Transitions**:
```
Boot → DISCONNECTED
DISCONNECTED → CONNECTING (start connection attempt)
CONNECTING → CONNECTED (WiFi.status() == WL_CONNECTED)
CONNECTING → FAILED (timeout after 30 seconds)
FAILED → CONNECTING (try next network)
FAILED → DISCONNECTED (all networks exhausted, prepare reboot)
CONNECTED → DISCONNECTED (connection lost)
DISCONNECTED → CONNECTING (retry same network)
```

**Lifecycle**:
- Created at system boot
- Persists throughout device runtime
- Not stored in flash (runtime state only)

**Relationships**:
- References WiFiConfigFile to determine next network to try
- Provides status to other gateway services

---

## Component Relationships

```
┌─────────────────┐
│ WiFiConfigFile  │ (Persistent)
│ /wifi.conf      │
└────────┬────────┘
         │ contains
         │ 0..3
         ▼
┌─────────────────┐
│ WiFiCredentials │
│ (SSID,Password) │
└─────────────────┘

┌─────────────────────┐
│ WiFiConnectionState │ (Runtime)
│ - status            │
│ - currentIndex      │
│ - attemptStart      │
└──────────┬──────────┘
           │ references
           │ read-only
           ▼
     WiFiConfigFile
```

---

## Data Flow

### Boot Sequence
1. WiFiConfigFile loads from `/wifi.conf`
2. WiFiConnectionState initialized to DISCONNECTED
3. Connection manager starts attempting networks[0]

### Connection Attempt
1. State → CONNECTING, record attemptStartTime
2. WiFi.begin(networks[currentIndex].ssid, networks[currentIndex].password)
3. Wait for event or timeout (30s)
4. On success: State → CONNECTED, store connectedSSID
5. On failure: State → FAILED, increment currentIndex

### Connection Loss
1. WiFi disconnect event received
2. State → DISCONNECTED
3. Retry same network (connectedSSID) without changing currentIndex

### Configuration Update
1. User uploads new file via HTTP POST
2. Parse and validate new WiFiConfigFile
3. If valid: Write to `/wifi.conf`, trigger reboot
4. If invalid: Return error, keep existing config

---

## Memory Footprint Estimate

**WiFiCredentials** (per network):
- ssid: ~32 bytes (max)
- password: ~63 bytes (max)
- Total: ~95 bytes per network

**WiFiConfigFile**:
- Array of 3 WiFiCredentials: ~285 bytes
- count: 4 bytes
- Total: ~289 bytes

**WiFiConnectionState**:
- status: 1 byte (enum)
- currentNetworkIndex: 4 bytes
- attemptStartTime: 4 bytes
- connectedSSID: ~32 bytes
- retryCount: 4 bytes
- Total: ~45 bytes

**Grand Total**: ~334 bytes (well within ESP32 SRAM limits)

**Flash Storage**: ~200 bytes for config file (3 networks × ~65 bytes avg)

---

## Validation Constraints Summary

| Field | Constraint | Error Handling |
|-------|-----------|----------------|
| SSID | 1-32 chars, not empty | Reject line, log error |
| Password | 0 or 8-63 chars | Reject line, log error |
| Line format | Must contain comma | Reject line, log error |
| File size | Max 3 lines | Truncate, log warning |
| Total networks | Max 3 | Use first 3, log warning |

**Defensive Parsing**: Invalid lines skipped, valid lines preserved
