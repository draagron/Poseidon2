# Data Model: OLED Display Bugfix

**Bug ID**: bugfix-001
**Date**: 2025-10-10
**Phase**: Design (Data Structures)

## Overview

This bugfix does NOT introduce new data structures. It fixes state synchronization and rendering bugs using existing data types.

## Existing Data Structures (Unchanged)

### SubsystemStatus
**Location**: `src/types/DisplayTypes.h:78-87`

```cpp
struct SubsystemStatus {
    DisplayConnectionStatus wifiStatus;      ///< Current WiFi connection state
    char wifiSSID[33];                       ///< Connected SSID (max 32 + null)
    char wifiIPAddress[16];                  ///< IP address string
    FilesystemStatus fsStatus;               ///< Filesystem state
    WebServerStatus webServerStatus;         ///< Web server state
    unsigned long wifiTimestamp;             ///< millis() when WiFi changed
    unsigned long fsTimestamp;               ///< millis() when FS changed
    unsigned long wsTimestamp;               ///< millis() when WS changed
};
```

**Usage**:
- Stored in `DisplayManager._currentStatus` (private field)
- Updated by `StartupProgressTracker._status` (internal component)
- Passed to `renderStatusPage()` for display

**Memory**: 66 bytes (static allocation)

### DisplayConnectionStatus
**Location**: `src/types/DisplayTypes.h:47-52`

```cpp
enum DisplayConnectionStatus {
    CONN_CONNECTING,    ///< WiFi connection in progress
    CONN_CONNECTED,     ///< WiFi connected successfully
    CONN_DISCONNECTED,  ///< WiFi disconnected
    CONN_FAILED         ///< WiFi connection failed
};
```

**Usage**:
- Used in `SubsystemStatus.wifiStatus`
- Passed to `updateWiFiStatus()` method (NEW in this fix)
- Determines display text: "WiFi: <SSID>" vs "WiFi: Disconnected"

### DisplayMetrics
**Location**: `src/types/DisplayTypes.h:32-39`

```cpp
struct DisplayMetrics {
    uint32_t freeRamBytes;          ///< Free heap (ESP.getFreeHeap())
    uint32_t sketchSizeBytes;       ///< Code size (ESP.getSketchSize())
    uint32_t freeFlashBytes;        ///< Free flash (ESP.getFreeSketchSpace())
    uint8_t  cpuIdlePercent;        ///< CPU idle 0-100%
    uint8_t  animationState;        ///< Icon state: 0=/, 1=-, 2=\, 3=|
    unsigned long lastUpdate;       ///< millis() timestamp
};
```

**Usage**:
- Stored in `DisplayManager._currentMetrics`
- Updated by `MetricsCollector::collectMetrics()`
- `animationState` field used for rotating icon rendering (bug #2 affects this)

**Memory**: 21 bytes (static allocation)

## State Flow Diagram

### BEFORE Fix (Broken State Synchronization)

```
WiFi Event
    ↓
onWiFiConnected()
    ↓
WiFiManager.handleConnectionSuccess()  ← Updates WiFiManager state
    ↓
    ✗ NO CONNECTION TO DisplayManager

DisplayManager._currentStatus
    ↓
wifiStatus = CONN_DISCONNECTED  ← STALE! Never updated
    ↓
renderStatusPage()
    ↓
Display: "WiFi: Disconnected"  ← BUG!
```

### AFTER Fix (Correct State Synchronization)

```
WiFi Event
    ↓
onWiFiConnected()
    ├─→ WiFiManager.handleConnectionSuccess()
    │
    └─→ DisplayManager.updateWiFiStatus(CONN_CONNECTED, ssid, ip)  ← NEW
            ↓
        _progressTracker->updateWiFiStatus(...)
            ↓
        _currentStatus = _progressTracker->getStatus()  ← SYNC
            ↓
        wifiStatus = CONN_CONNECTED  ← CORRECT
            ↓
        renderStatusPage()
            ↓
        Display: "WiFi: <SSID>"  ← FIXED!
```

## Animation Icon Rendering

### BEFORE Fix (With Brackets)

```
Cursor position: 108 (128 - 20 pixels for "[ X ]")
Output: "[ / ]"  ← 5 characters
        "[ - ]"
        "[ \ ]"
        "[ | ]"
```

**Visual**: `[ + ]` shown at bottom of display (see misc/IMG_2483.png)

### AFTER Fix (Without Brackets)

```
Cursor position: 118 (128 - 10 pixels for " X ")
Output: "/"  ← 1 character
        "-"
        "\"
        "|"
```

**Visual**: Clean rotating icon without square brackets

## Method Signatures

### NEW: DisplayManager::updateWiFiStatus()

```cpp
/**
 * @brief Update WiFi connection status
 *
 * Updates internal status for display rendering. Should be called
 * when WiFi state changes (connected, disconnected, etc.).
 *
 * @param status New WiFi connection status
 * @param ssid SSID of connected network (optional, nullptr = no update)
 * @param ip IP address string (optional, nullptr = no update)
 */
void updateWiFiStatus(DisplayConnectionStatus status,
                     const char* ssid = nullptr,
                     const char* ip = nullptr);
```

**Implementation Strategy**:
```cpp
void DisplayManager::updateWiFiStatus(DisplayConnectionStatus status,
                                     const char* ssid,
                                     const char* ip) {
    // Delegate to internal StartupProgressTracker
    if (_progressTracker != nullptr) {
        _progressTracker->updateWiFiStatus(status, ssid, ip);

        // CRITICAL: Synchronize _currentStatus with progressTracker state
        _currentStatus = _progressTracker->getStatus();
    }
}
```

**Why This Works**:
1. `StartupProgressTracker::updateWiFiStatus()` already exists (line 35 in StartupProgressTracker.cpp)
2. It handles SSID/IP copying, null termination, timestamp updates
3. DisplayManager just delegates and synchronizes state
4. No duplicated logic, preserves encapsulation

### MODIFIED: renderStatusPage() - Animation Section

**BEFORE** (DisplayManager.cpp:212-217):
```cpp
// Line 5: Animation icon (right corner)
_displayAdapter->setCursor(108, getLineY(5));  // 128 - 20 pixels for "[ X ]"
_displayAdapter->print("[ ");
char icon = DisplayFormatter::getAnimationIcon(_currentMetrics.animationState);
char iconStr[2] = {icon, '\0'};
_displayAdapter->print(iconStr);
_displayAdapter->print(" ]");
```

**AFTER**:
```cpp
// Line 5: Animation icon (right corner)
_displayAdapter->setCursor(118, getLineY(5));  // 128 - 10 pixels for " X "
char icon = DisplayFormatter::getAnimationIcon(_currentMetrics.animationState);
char iconStr[2] = {icon, '\0'};
_displayAdapter->print(iconStr);
```

**Changes**:
- Cursor X-coordinate: 108 → 118 (adjust for removed brackets)
- Remove: `_displayAdapter->print("[ ");`
- Remove: `_displayAdapter->print(" ]");`

## Memory Impact Analysis

### Static Allocation (Constitutional Compliance)

**BEFORE Fix**:
- `DisplayManager._currentStatus`: 66 bytes
- `DisplayManager._currentMetrics`: 21 bytes
- `StartupProgressTracker._status`: 66 bytes
- **Total**: 153 bytes

**AFTER Fix**:
- Same structures, no new allocations
- **Total**: 153 bytes (unchanged)

**Flash Footprint**:
- `updateWiFiStatus()` wrapper method: ~50 bytes
- Animation rendering changes: -20 bytes (removed bracket printing)
- **Net Change**: +30 bytes flash (~0.0015% of 1.9MB partition)

### Constitutional Compliance: Principle II

✅ **Resource-Aware Development**:
- No new heap allocations
- No new global variables
- Uses existing fixed-size buffers (wifiSSID[33], wifiIPAddress[16])
- String operations via strncpy() with bounds checking (already implemented in StartupProgressTracker)

## Call Sequences

### WiFi Connection Event

```cpp
// In main.cpp:90-122 (onWiFiConnected callback)
void onWiFiConnected() {
    String ssid = wifiAdapter->getSSID();
    String ip = wifiAdapter->getIPAddress();

    wifiManager->handleConnectionSuccess(connectionState, ssid);

    // NEW: Update display with WiFi status
    if (displayManager != nullptr) {
        displayManager->updateWiFiStatus(CONN_CONNECTED, ssid.c_str(), ip.c_str());
    }

    // ... rest of callback ...
}
```

### WiFi Disconnection Event

```cpp
// In main.cpp (onWiFiDisconnected callback)
void onWiFiDisconnected() {
    // ... existing logic ...

    // NEW: Update display with disconnection
    if (displayManager != nullptr) {
        displayManager->updateWiFiStatus(CONN_DISCONNECTED);
    }
}
```

### Display Refresh Cycle

```cpp
// ReactESP event loop (main.cpp:417-421)
app.onRepeat(DISPLAY_STATUS_INTERVAL_MS, []() {
    if (displayManager != nullptr) {
        displayManager->renderStatusPage();
            // ↓ uses _currentStatus (now synchronized!)
            // ↓ renders WiFi SSID if CONN_CONNECTED
            // ↓ renders animation icon WITHOUT brackets
    }
});
```

## Data Validation

### WiFi Status Update Validation

**Input Constraints**:
- `status`: Must be valid `DisplayConnectionStatus` enum value
- `ssid`: If provided, must be null-terminated, max 32 chars (enforced by `strncpy` in StartupProgressTracker)
- `ip`: If provided, must be null-terminated, max 15 chars (enforced by `strncpy`)

**Validation Logic** (already exists in StartupProgressTracker.cpp:35-56):
```cpp
void StartupProgressTracker::updateWiFiStatus(DisplayConnectionStatus status,
                                              const char* ssid,
                                              const char* ip) {
    _status.wifiStatus = status;
    _status.wifiTimestamp = millis();

    // Update SSID if provided
    if (ssid != nullptr) {
        strncpy(_status.wifiSSID, ssid, sizeof(_status.wifiSSID) - 1);
        _status.wifiSSID[sizeof(_status.wifiSSID) - 1] = '\0';  // Ensure null termination
    }

    // Update IP if provided
    if (ip != nullptr) {
        strncpy(_status.wifiIPAddress, ip, sizeof(_status.wifiIPAddress) - 1);
        _status.wifiIPAddress[sizeof(_status.wifiIPAddress) - 1] = '\0';
    }

    // Clear SSID and IP on disconnect
    if (status == CONN_DISCONNECTED || status == CONN_FAILED) {
        _status.wifiSSID[0] = '\0';
        _status.wifiIPAddress[0] = '\0';
    }
}
```

**Safety Features**:
- ✅ Null termination guaranteed
- ✅ Buffer overflow prevention via `strncpy` with size limits
- ✅ Automatic clearing of SSID/IP on disconnect

## Edge Cases

### Edge Case 1: DisplayManager Not Initialized
**Scenario**: WiFi connects before `displayManager` created in `main.cpp`

**Mitigation**:
```cpp
if (displayManager != nullptr) {
    displayManager->updateWiFiStatus(...);
}
```

**Result**: No crash, display updated on next refresh cycle after initialization

### Edge Case 2: WiFi Disconnects During Boot
**Scenario**: WiFi connects briefly then disconnects before display initialized

**Mitigation**:
- Both `onWiFiConnected()` and `onWiFiDisconnected()` call `updateWiFiStatus()`
- Latest state always reflected
- If display not ready, graceful degradation (FR-027)

### Edge Case 3: Long SSID (>32 chars)
**Scenario**: Network has SSID longer than 32 characters

**Mitigation**:
- `strncpy(_status.wifiSSID, ssid, 32)` truncates to 32 chars
- Null terminator added at position 32
- Display shows truncated SSID (acceptable behavior)

### Edge Case 4: Animation Update Without Full Refresh
**Scenario**: `updateAnimationIcon()` called independently

**Current Behavior** (buggy):
```cpp
_displayAdapter->setCursor(108, getLineY(5));
_displayAdapter->print("[ ");  // Overwrites previous icon
_displayAdapter->print(iconStr);
_displayAdapter->print(" ]");
_displayAdapter->display();
```

**Fixed Behavior**:
```cpp
_displayAdapter->setCursor(118, getLineY(5));
_displayAdapter->print(iconStr);  // Clean overwrite
_displayAdapter->display();
```

**Note**: Partial update may leave artifacts if previous render used different cursor position. Full page refresh every 5 seconds cleans up.

## References

- DisplayTypes.h: `src/types/DisplayTypes.h`
- DisplayManager: `src/components/DisplayManager.{h,cpp}`
- StartupProgressTracker: `src/components/StartupProgressTracker.{h,cpp}`
- Main WiFi callbacks: `src/main.cpp:90-122`

---
**Data Model Analysis Complete**: 2025-10-10
**Next**: Contracts and quickstart guide
