# Research: OLED Display WiFi Status and Animation Bugs

**Bug ID**: bugfix-001
**Date**: 2025-10-10
**Research Phase**: Phase 0 - Codebase Analysis

## Problem Summary

Two distinct visual bugs on the OLED display:
1. **WiFi Status Bug**: Display shows "WiFi: Disconnected" even when WiFi is connected
2. **Animation Icon Bug**: Spinner shows with square brackets `[ ]` and appears misaligned

## Root Cause Analysis

### Bug #1: WiFi Status Always Shows "Disconnected"

**Location**: `src/components/DisplayManager.cpp`

**Root Cause**: State synchronization gap between `WiFiManager` and `DisplayManager`

**Key Findings**:
1. `DisplayManager._currentStatus` is initialized to `CONN_DISCONNECTED` in constructor (line 29)
2. `DisplayManager` NEVER updates `_currentStatus.wifiStatus` after initialization
3. `StartupProgressTracker` exists as a component within `DisplayManager` but is NOT exposed publicly
4. `onWiFiConnected()` callback in `main.cpp` updates `WiFiManager` state but NOT `DisplayManager`
5. `renderStatusPage()` uses stale `_currentStatus` (line 162)

**Current Architecture**:
```
WiFi Event → onWiFiConnected() → WiFiManager.handleConnectionSuccess()
                                  ↓ (no connection to DisplayManager!)
                              DisplayManager._currentStatus (stale)
```

**Missing Link**: No mechanism exists to notify `DisplayManager` when WiFi state changes.

**Files Involved**:
- `src/components/DisplayManager.h`: No public setter for `_currentStatus`
- `src/components/DisplayManager.cpp:29`: Initial state set to CONN_DISCONNECTED
- `src/components/DisplayManager.cpp:162`: `renderStatusPage()` uses `_currentStatus`
- `src/components/StartupProgressTracker.h`: Has `updateWiFiStatus()` method
- `src/components/StartupProgressTracker.cpp:35`: Implements WiFi status update
- `src/main.cpp:90-122`: `onWiFiConnected()` doesn't notify DisplayManager

### Bug #2: Animation Icon Shows Square Brackets

**Location**: `src/components/DisplayManager.cpp`

**Root Cause**: Hardcoded literal square brackets in rendering code

**Key Findings**:
1. Line 212-217: `renderStatusPage()` prints `"[ "`, icon, then `" ]"`
2. Line 234-239: `updateAnimationIcon()` prints `"[ "`, icon, then `" ]"`
3. User expectation: Only rotating character (/, -, \, |) without brackets

**Code Snippet** (DisplayManager.cpp:212-217):
```cpp
// Line 5: Animation icon (right corner)
_displayAdapter->setCursor(108, getLineY(5));  // 128 - 20 pixels for "[ X ]"
_displayAdapter->print("[ ");
char icon = DisplayFormatter::getAnimationIcon(_currentMetrics.animationState);
char iconStr[2] = {icon, '\0'};
_displayAdapter->print(iconStr);
_displayAdapter->print(" ]");
```

**Expected Behavior**: Remove `"[ "` and `" ]"` prints, only render icon character.

## Component Analysis

### DisplayManager Component

**Responsibilities**:
- Orchestrate OLED display rendering
- Coordinate MetricsCollector, DisplayFormatter, StartupProgressTracker
- Render startup progress and runtime status

**Current State**:
- ✅ Properly uses HAL interfaces (IDisplayAdapter, ISystemMetrics)
- ✅ Static allocation (constitutional compliance)
- ❌ No public API to update WiFi status
- ❌ StartupProgressTracker is private, not accessible from main.cpp

**Internal Components**:
- `_progressTracker`: `StartupProgressTracker*` (private, line 40)
- `_currentStatus`: `SubsystemStatus` (private, line 44)

### StartupProgressTracker Component

**Responsibilities**:
- Track WiFi, filesystem, web server initialization status
- Store SSID, IP address, timestamps

**Current State**:
- ✅ Has `updateWiFiStatus(status, ssid, ip)` method (line 35)
- ✅ Has `getStatus()` accessor (line 68)
- ❌ Instance is private within DisplayManager, not accessible externally

**Key Methods**:
```cpp
void updateWiFiStatus(DisplayConnectionStatus status, const char* ssid = nullptr, const char* ip = nullptr);
const SubsystemStatus& getStatus() const;
```

## Dependencies

### External Dependencies
- `WiFiManager`: Manages WiFi connection state (separate from DisplayManager)
- `ESP32WiFiAdapter`: Provides WiFi events and connection status
- `main.cpp`: Receives WiFi events (`onWiFiConnected`, `onWiFiDisconnected`)

### Internal Dependencies
- `MetricsCollector`: Collects system metrics (RAM, Flash, CPU)
- `DisplayFormatter`: Formats data for display (animation icons, bytes, percentages)
- `StartupProgressTracker`: Tracks subsystem status (but not exposed)

## Fix Strategy Options

### Option A: Expose StartupProgressTracker Publicly
**Approach**: Add public accessor for `_progressTracker` in DisplayManager

**Pros**:
- Minimal code changes
- Uses existing `StartupProgressTracker::updateWiFiStatus()` method
- Preserves existing architecture

**Cons**:
- Breaks encapsulation (exposes internal component)
- Caller must know to call `displayManager->getProgressTracker()->updateWiFiStatus()`

### Option B: Add WiFi Status Update Method to DisplayManager (RECOMMENDED)
**Approach**: Add `DisplayManager::updateWiFiStatus(status, ssid, ip)` method that delegates to internal `_progressTracker`

**Pros**:
- ✅ Preserves encapsulation (DisplayManager remains facade)
- ✅ Clean public API: `displayManager->updateWiFiStatus(...)`
- ✅ Consistent with component responsibilities
- ✅ Future-proof for additional state updates (filesystem, web server)

**Cons**:
- Requires additional wrapper method (minimal effort)

### Option C: Direct _currentStatus Update
**Approach**: Add setter method to update `_currentStatus` directly

**Pros**:
- Simplest implementation

**Cons**:
- Bypasses `StartupProgressTracker` component
- Duplicates logic (timestamp management, SSID/IP updates)
- Inconsistent with existing architecture

## Recommended Solution

**Fix Strategy**: **Option B** - Add public WiFi status update method to DisplayManager

### Implementation Plan

**Bug #1: WiFi Status Synchronization**

1. Add public method to DisplayManager.h:
   ```cpp
   /**
    * @brief Update WiFi connection status
    *
    * Updates internal status for display rendering. Should be called
    * when WiFi state changes (connected, disconnected, etc.).
    *
    * @param status New WiFi connection status
    * @param ssid SSID of connected network (optional)
    * @param ip IP address string (optional)
    */
   void updateWiFiStatus(DisplayConnectionStatus status, const char* ssid = nullptr, const char* ip = nullptr);
   ```

2. Implement in DisplayManager.cpp:
   ```cpp
   void DisplayManager::updateWiFiStatus(DisplayConnectionStatus status, const char* ssid, const char* ip) {
       if (_progressTracker != nullptr) {
           _progressTracker->updateWiFiStatus(status, ssid, ip);
           // Synchronize _currentStatus with progressTracker state
           _currentStatus = _progressTracker->getStatus();
       }
   }
   ```

3. Update main.cpp `onWiFiConnected()` callback (after line 102):
   ```cpp
   // Update display with WiFi connection status
   if (displayManager != nullptr) {
       displayManager->updateWiFiStatus(CONN_CONNECTED, ssid.c_str(), ip.c_str());
   }
   ```

4. Update main.cpp `onWiFiDisconnected()` callback:
   ```cpp
   // Update display with WiFi disconnection
   if (displayManager != nullptr) {
       displayManager->updateWiFiStatus(CONN_DISCONNECTED);
   }
   ```

**Bug #2: Remove Animation Brackets**

1. Update DisplayManager.cpp line 212-217 (in `renderStatusPage`):
   ```cpp
   // Line 5: Animation icon (right corner)
   _displayAdapter->setCursor(118, getLineY(5));  // 128 - 10 pixels for " X "
   char icon = DisplayFormatter::getAnimationIcon(_currentMetrics.animationState);
   char iconStr[2] = {icon, '\0'};
   _displayAdapter->print(iconStr);
   ```

2. Update DisplayManager.cpp line 234-239 (in `updateAnimationIcon`):
   ```cpp
   _displayAdapter->setCursor(118, getLineY(5));
   char icon = DisplayFormatter::getAnimationIcon(metrics.animationState);
   char iconStr[2] = {icon, '\0'};
   _displayAdapter->print(iconStr);
   ```

## Testing Strategy

### Regression Tests (BEFORE applying fix)

**Test 1: WiFi Status Display - Connected State**
- Location: `test/test_oled_integration/test_wifi_status_display.cpp`
- Scenario: WiFi connects, then `updateWiFiStatus(CONN_CONNECTED, "HomeNet", "192.168.1.100")` called
- Expected: Display shows "WiFi: HomeNet" and "IP: 192.168.1.100"
- Current: FAILS (shows "WiFi: Disconnected")

**Test 2: WiFi Status Display - Disconnected State**
- Scenario: WiFi disconnects, then `updateWiFiStatus(CONN_DISCONNECTED)` called
- Expected: Display shows "WiFi: Disconnected" and "IP: ---"
- Current: PASSES (already shows disconnected)

**Test 3: Animation Icon Format**
- Scenario: Render status page with animation state
- Expected: Icon appears without brackets (only character: /, -, \, |)
- Current: FAILS (shows "[ / ]", "[ - ]", etc.)

### Unit Tests

**Test 4: DisplayManager::updateWiFiStatus() Method**
- Test: Call `updateWiFiStatus()` and verify internal state updated
- Verify: `getCurrentStatus()` returns updated WiFi status, SSID, IP

**Test 5: Animation Icon Position**
- Test: Verify cursor position for animation icon
- Expected: X-coordinate adjusted from 108 to 118 (after removing brackets)

## Constitutional Compliance

### Principle I: Hardware Abstraction Layer
- ✅ Fix maintains HAL interfaces (IDisplayAdapter)
- ✅ No new hardware dependencies introduced

### Principle II: Resource-Aware Development
- ✅ No new heap allocations (method just updates existing fields)
- ✅ Static allocation preserved (SubsystemStatus already exists)
- ✅ String operations use fixed-size buffers (wifiSSID[33], wifiIPAddress[16])

### Principle IV: Modular Component Design
- ✅ DisplayManager remains responsible for display orchestration
- ✅ Single responsibility maintained (display state management)
- ✅ Dependency injection unchanged

### Principle V: Network-Based Debugging
- ✅ No changes to logging infrastructure
- ✅ Existing WebSocket logging remains intact

### Principle VII: Fail-Safe Operation
- ✅ Graceful degradation maintained (nullptr checks)
- ✅ No new failure modes introduced

### Principle VIII: Workflow Selection
- ✅ Bugfix workflow correctly selected (regression test required)
- ✅ Root cause identified before fix
- ✅ Prevention strategy defined

## Memory Impact

**New Code**:
- `DisplayManager::updateWiFiStatus()` method: ~50 bytes flash (wrapper method)
- No new RAM allocation (uses existing `_currentStatus` field)

**Total Impact**:
- Flash: +50 bytes (~0.002% of 1.9MB partition)
- RAM: 0 bytes (no new allocations)

## Files to Modify

1. **src/components/DisplayManager.h**: Add `updateWiFiStatus()` public method declaration
2. **src/components/DisplayManager.cpp**: Implement `updateWiFiStatus()`, remove bracket prints
3. **src/main.cpp**: Call `displayManager->updateWiFiStatus()` in WiFi event callbacks
4. **test/test_oled_integration/test_wifi_status_display.cpp**: Add regression tests (NEW FILE)

## Risk Assessment

**Risk Level**: LOW

**Risks**:
1. **State Synchronization**: `_currentStatus` and `_progressTracker` may become out of sync
   - **Mitigation**: Always synchronize after calling `_progressTracker->updateWiFiStatus()`

2. **Animation Icon Positioning**: Removing brackets may cause visual misalignment
   - **Mitigation**: Adjust X-coordinate from 108 to 118 (center icon without brackets)

3. **Timing**: WiFi event callbacks may fire before DisplayManager initialized
   - **Mitigation**: Existing nullptr check (`if (displayManager != nullptr)`)

**Breaking Changes**: None (public API extended, not modified)

## References

- Bug Report: `specs/bugfix-001-the-oled-display/bug-report.md`
- OLED Display Feature: `specs/005-oled-basic-info/` (R004)
- WiFi Management: CLAUDE.md WiFi Management section
- Constitution: `.specify/memory/constitution.md` v1.2.0

---
**Research completed**: 2025-10-10
**Next Phase**: Design (contracts, data model, quickstart)
