# Implementation Plan: OLED Display Bugfix

**Bug ID**: bugfix-001
**Branch**: `bugfix/001-the-oled-display`
**Status**: READY FOR IMPLEMENTATION
**Created**: 2025-10-10
**Estimated Duration**: 2-3 hours

## Executive Summary

**Problem**: Two distinct visual bugs on OLED display affecting user experience:
1. WiFi status incorrectly shows "Disconnected" even when WiFi is connected
2. Animation icon displays with unwanted square brackets `[ / ]` instead of clean icon

**Root Cause**:
1. State synchronization gap between `WiFiManager` and `DisplayManager`
2. Hardcoded literal brackets in animation rendering code

**Solution**:
1. Add `DisplayManager::updateWiFiStatus()` method to bridge state gap
2. Remove bracket printing and adjust cursor position in animation rendering

**Impact**:
- **User-Facing**: Major improvement (correct WiFi status + cleaner display)
- **Technical**: Minimal (one new method + removed print statements)
- **Risk**: LOW (simple state sync + cosmetic change)
- **Testing**: Comprehensive (6 regression tests + hardware validation)

## Problem Statement

### Bug #1: WiFi Status Shows "Disconnected" When Connected

**Visual Evidence**: `misc/IMG_2483.png` shows:
- Line 0: "WiFi: Disconnected" ← WRONG (device is clearly connected)
- Line 1: "IP: ---" ← WRONG (should show actual IP)
- Lines 2-4: RAM, Flash, CPU metrics visible ← Proves WiFi is actually connected

**Technical Root Cause** (from research.md):
- `DisplayManager._currentStatus.wifiStatus` initialized to `CONN_DISCONNECTED` in constructor
- Never updated when WiFi state changes
- `onWiFiConnected()` callback updates `WiFiManager` but NOT `DisplayManager`
- `renderStatusPage()` uses stale `_currentStatus`

**User Impact**: User sees incorrect WiFi status, cannot trust display information

### Bug #2: Animation Icon Shows Square Brackets

**Visual Evidence**: `misc/IMG_2483.png` shows:
- Bottom of display: `[ +` (square brackets with animation character)

**Technical Root Cause** (from research.md):
- DisplayManager.cpp lines 212-217, 234-239: Hardcoded `"[ "` and `" ]"` prints
- User expectation: Single rotating character (/, -, \, |) without brackets

**User Impact**: Visual clutter, unexpected formatting

## Architecture Overview

### Current Architecture (Broken)

```
┌─────────────────┐
│  WiFi Events    │
│  (ESP32)        │
└────────┬────────┘
         │
         ├─────────────────────┐
         │                     │
         v                     v
┌─────────────────┐   ┌──────────────────┐
│  WiFiManager    │   │  DisplayManager  │
│  (State OK)     │   │  (State STALE)   │
└─────────────────┘   └──────────────────┘
                              │
                              v
                      ┌──────────────────┐
                      │  OLED Display    │
                      │  Shows: WRONG    │
                      └──────────────────┘
```

**Problem**: No communication path from WiFi events to DisplayManager

### Fixed Architecture

```
┌─────────────────┐
│  WiFi Events    │
│  (ESP32)        │
└────────┬────────┘
         │
         ├─────────────────────┬─────────────────────┐
         │                     │                     │
         v                     v                     v
┌─────────────────┐   ┌──────────────────┐  ┌──────────────────┐
│  WiFiManager    │   │  DisplayManager  │  │  WebSocket       │
│  (State)        │   │  updateWiFiStatus│  │  Logger          │
└─────────────────┘   │  (NEW METHOD)    │  └──────────────────┘
                      └──────────┬───────┘
                                 │
                                 v
                      ┌──────────────────────┐
                      │  StartupProgress     │
                      │  Tracker (existing)  │
                      └──────────┬───────────┘
                                 │
                                 v
                      ┌──────────────────┐
                      │  _currentStatus  │
                      │  (synchronized)  │
                      └──────────┬───────┘
                                 │
                                 v
                      ┌──────────────────┐
                      │  OLED Display    │
                      │  Shows: CORRECT  │
                      └──────────────────┘
```

**Solution**: New `updateWiFiStatus()` method provides notification path

## Implementation Strategy

### Approach: Minimal Invasive Fix

**Philosophy**: Leverage existing components, add minimal new code

**Why This Approach**:
1. `StartupProgressTracker` already has `updateWiFiStatus()` method
2. Just need to expose it through DisplayManager public API
3. No architectural changes required
4. Preserves encapsulation

### Fix #1: WiFi Status Synchronization

**Design Decision**: Add public method to DisplayManager (Option B from research.md)

**Alternatives Considered**:
- ❌ Option A: Expose `_progressTracker` publicly (breaks encapsulation)
- ✅ **Option B**: Add wrapper method (preserves facade pattern)
- ❌ Option C: Direct `_currentStatus` update (duplicates logic)

**Implementation**:

1. **Add method declaration** (DisplayManager.h):
   ```cpp
   void updateWiFiStatus(DisplayConnectionStatus status,
                        const char* ssid = nullptr,
                        const char* ip = nullptr);
   ```

2. **Implement delegation** (DisplayManager.cpp):
   ```cpp
   void DisplayManager::updateWiFiStatus(...) {
       if (_progressTracker != nullptr) {
           _progressTracker->updateWiFiStatus(status, ssid, ip);
           _currentStatus = _progressTracker->getStatus();  // SYNC!
       }
   }
   ```

3. **Call from WiFi callbacks** (main.cpp):
   ```cpp
   void onWiFiConnected() {
       // ... existing code ...
       if (displayManager != nullptr) {
           displayManager->updateWiFiStatus(CONN_CONNECTED, ssid.c_str(), ip.c_str());
       }
   }
   ```

**Key Insight**: Synchronization step (`_currentStatus = _progressTracker->getStatus()`) is CRITICAL to fix the bug.

### Fix #2: Animation Icon Brackets

**Design Decision**: Remove bracket printing, adjust cursor position

**Implementation**:

**BEFORE** (DisplayManager.cpp:212-217):
```cpp
_displayAdapter->setCursor(108, getLineY(5));  // 20 pixels for "[ X ]"
_displayAdapter->print("[ ");
_displayAdapter->print(iconStr);
_displayAdapter->print(" ]");
```

**AFTER**:
```cpp
_displayAdapter->setCursor(118, getLineY(5));  // 10 pixels for " X "
_displayAdapter->print(iconStr);
```

**Why This Works**:
- Display width: 128 pixels
- Font width: ~6 pixels per character
- Old position: 128 - 20 = 108 (for 5-char sequence `[ / ]`)
- New position: 128 - 10 = 118 (for 1-char icon `/`)

## Testing Strategy

### Test-Driven Development (TDD) Approach

**Constitutional Requirement** (Principle VIII - Bugfix Workflow):
> Regression test MUST be written before fix is applied

**Workflow**:
1. **RED**: Write tests that FAIL (proving bugs exist)
2. **GREEN**: Implement fixes until tests PASS
3. **REFACTOR**: Clean up (not needed for this fix)
4. **VERIFY**: Hardware validation on actual OLED

### Regression Tests

**Test Suite**: `test/test_oled_integration/`

**Test 1: WiFi Status - Connected**
```cpp
void test_wifi_status_shows_ssid_when_connected() {
    dm.updateWiFiStatus(CONN_CONNECTED, "HomeNetwork", "192.168.1.100");
    dm.renderStatusPage();
    TEST_ASSERT_TRUE(mockDisplay.hasCall("print", "WiFi: HomeNetwork"));
}
```

**Test 2: WiFi Status - Disconnected**
```cpp
void test_wifi_status_shows_disconnected_when_not_connected() {
    dm.updateWiFiStatus(CONN_DISCONNECTED);
    dm.renderStatusPage();
    TEST_ASSERT_TRUE(mockDisplay.hasCall("print", "WiFi: Disconnected"));
}
```

**Test 3: WiFi Status - Clearing on Disconnect**
```cpp
void test_wifi_status_clears_ssid_on_disconnect() {
    dm.updateWiFiStatus(CONN_CONNECTED, "TestNet", "192.168.1.50");
    dm.updateWiFiStatus(CONN_DISCONNECTED);
    TEST_ASSERT_EQUAL_STRING("", dm.getCurrentStatus().wifiSSID);
}
```

**Test 4: Animation Icon - No Brackets**
```cpp
void test_animation_icon_has_no_brackets() {
    dm.updateAnimationIcon();
    TEST_ASSERT_FALSE(mockDisplay.hasCall("print", "[ "));
    TEST_ASSERT_FALSE(mockDisplay.hasCall("print", " ]"));
}
```

**Test 5: Animation Icon - Cursor Position**
```cpp
void test_animation_icon_cursor_position() {
    dm.updateAnimationIcon();
    TEST_ASSERT_TRUE(mockDisplay.hasCall("setCursor", 118, 50));
}
```

**Test 6: Animation Icon - State Cycling**
```cpp
void test_animation_cycles_through_states() {
    const char* expectedIcons[] = {"/", "-", "\\", "|", "/"};
    for (int i = 0; i < 5; i++) {
        dm.updateAnimationIcon();
        TEST_ASSERT_TRUE(mockDisplay.hasCall("print", expectedIcons[i]));
    }
}
```

**Total**: 6 regression tests

### Hardware Validation

**Platform**: ESP32 + SSD1306 OLED (128×64, I2C address 0x3C)

**Manual Test Cases**:

1. **WiFi Connection Test**:
   - Boot ESP32
   - Wait for WiFi connection
   - ✅ Expected: Display shows `WiFi: <SSID>` and `IP: <address>`
   - ❌ Current: Display shows `WiFi: Disconnected`

2. **WiFi Disconnection Test**:
   - Power off router
   - Wait for disconnect
   - ✅ Expected: Display shows `WiFi: Disconnected`

3. **Animation Icon Test**:
   - Observe bottom right corner
   - Wait 4 seconds (full cycle)
   - ✅ Expected: `/` → `-` → `\` → `|` (no brackets)
   - ❌ Current: `[ / ]` → `[ - ]` → `[ \ ]` → `[ | ]`

**Documentation**:
- **BEFORE**: `misc/IMG_2483.png` (existing screenshot showing bugs)
- **AFTER**: `misc/IMG_2483_AFTER.png` (to be captured after fix)

## Files to Modify

### Primary Changes

1. **src/components/DisplayManager.h** (+ 12 lines)
   - Add `updateWiFiStatus()` method declaration
   - Doxygen comment explaining purpose

2. **src/components/DisplayManager.cpp** (+ 9 lines, - 4 lines)
   - Implement `updateWiFiStatus()` method
   - Remove bracket prints in `renderStatusPage()` (lines 212-217)
   - Remove bracket prints in `updateAnimationIcon()` (lines 234-239)

3. **src/main.cpp** (+ 8 lines)
   - Call `displayManager->updateWiFiStatus()` in `onWiFiConnected()`
   - Call `displayManager->updateWiFiStatus()` in `onWiFiDisconnected()`

### Test Files (NEW)

4. **test/test_oled_integration/test_main.cpp** (NEW)
   - Unity test runner for all OLED integration tests

5. **test/test_oled_integration/test_wifi_status_display.cpp** (NEW)
   - 3 tests for WiFi status display

6. **test/test_oled_integration/test_animation_icon.cpp** (NEW)
   - 3 tests for animation icon rendering

### Documentation Updates

7. **specs/bugfix-001-the-oled-display/bug-report.md**
   - Update status to "Fixed"
   - Check verification checklist

8. **misc/IMG_2483_AFTER.png** (NEW)
   - Screenshot showing fixed display

## Risk Assessment

### Risk Level: **LOW**

### Identified Risks

**Risk 1: State Synchronization Timing**
- **Scenario**: WiFi event fires before DisplayManager initialized
- **Impact**: Nullptr dereference if displayManager not created
- **Mitigation**: Existing nullptr check: `if (displayManager != nullptr)`
- **Likelihood**: LOW (DisplayManager initialized before WiFi attempts)

**Risk 2: Animation Icon Positioning**
- **Scenario**: Cursor position calculation wrong for different font sizes
- **Impact**: Icon appears off-screen or misaligned
- **Mitigation**: Hardcoded position validated for default font size 1
- **Likelihood**: VERY LOW (font size always 1 in current code)

**Risk 3: _currentStatus and _progressTracker Drift**
- **Scenario**: Future code updates `_currentStatus` directly, bypassing tracker
- **Impact**: State inconsistency returns
- **Mitigation**: Code comments + contract documentation
- **Likelihood**: LOW (encapsulation discourages direct access)

### Risk Mitigation Summary

| Risk | Mitigation | Residual Risk |
|------|------------|---------------|
| Timing | Nullptr checks | VERY LOW |
| Positioning | Hardware validation | VERY LOW |
| State drift | Documentation + tests | LOW |

**Overall Risk**: LOW - Safe to implement

## Resource Impact

### Memory Footprint

**Static Allocation** (Constitutional Principle II):
- **RAM**: 0 bytes (no new data structures)
- **Flash**: +30 bytes net (+50 new method, -20 removed prints)

**Impact**: 0.0015% of 1.9MB flash partition - NEGLIGIBLE

### Performance Impact

**Rendering Performance**:
- **BEFORE**: 3 print calls for animation icon (~15 μs)
- **AFTER**: 1 print call for animation icon (~5 μs)
- **Improvement**: 67% faster (10 μs saved per update)

**WiFi Callback Impact**:
- **New code**: `updateWiFiStatus()` call in `onWiFiConnected()`
- **Execution time**: ~10 μs (string copy + struct copy)
- **Impact**: NEGLIGIBLE (callback already takes >100 μs)

### Constitutional Compliance

| Principle | Requirement | Compliance | Evidence |
|-----------|-------------|------------|----------|
| **I. HAL** | Hardware abstraction | ✅ PASS | Uses IDisplayAdapter interface |
| **II. Resource-Aware** | Static allocation, no heap | ✅ PASS | No new allocations, uses existing SubsystemStatus (66 bytes) |
| **III. QA-First** | Code review required | ✅ PASS | TDD approach, 6 regression tests |
| **IV. Modular Design** | Single responsibility | ✅ PASS | DisplayManager scope unchanged |
| **V. Network Debugging** | WebSocket logging | ✅ PASS | No changes to logging (existing logs remain) |
| **VI. Always-On** | No sleep modes | ✅ PASS | No power management changes |
| **VII. Fail-Safe** | Graceful degradation | ✅ PASS | Nullptr checks, no new failure modes |
| **VIII. Workflow** | Bugfix workflow | ✅ PASS | Regression tests BEFORE fix |

**Conclusion**: Fully compliant with all constitutional principles

## Implementation Timeline

### Phase 1: Test Infrastructure (30 min)
- T001: Create test directory
- T002: Verify MockDisplayAdapter exists

### Phase 2: Regression Tests (50 min)
- T003: Write WiFi status tests (3 tests)
- T004: Write animation icon tests (3 tests)
- **Gate**: Tests written and FAILING

### Phase 3: WiFi Status Fix (20 min)
- T005: Add method declaration (DisplayManager.h)
- T006: Implement method (DisplayManager.cpp)
- T007: Update onWiFiConnected() (main.cpp)
- T008: Update onWiFiDisconnected() (main.cpp)

### Phase 4: Animation Icon Fix (15 min)
- T009: Remove brackets from renderStatusPage()
- T010: Remove brackets from updateAnimationIcon()

### Phase 5: Verification (15 min)
- T011: Run regression tests (should PASS)
- T012: Run full test suite (no regressions)
- **Gate**: All tests passing

### Phase 6: Hardware Validation (20 min)
- T013: Build and upload to ESP32
- T014: Visual verification - WiFi status
- T015: Visual verification - animation icon
- T016: Take screenshot for documentation
- **Gate**: Visual confirmation on hardware

### Phase 7: Documentation (20 min)
- T017: Update bug-report.md status
- T018: Update CHANGELOG (if exists)
- T019: Commit with descriptive message
- T020: Push to remote (optional)

**Total Estimated Time**: 2-3 hours

## Success Criteria

### Functional Requirements

✅ **FR-1**: Display shows correct WiFi SSID when connected
- Verify: Display line 0 shows `WiFi: <SSID>` not "Disconnected"

✅ **FR-2**: Display shows IP address when connected
- Verify: Display line 1 shows `IP: <address>` not "---"

✅ **FR-3**: Display shows "Disconnected" when WiFi actually disconnected
- Verify: Disconnect WiFi, display updates correctly

✅ **FR-4**: Animation icon shows without square brackets
- Verify: Display shows `/`, not `[ / ]`

✅ **FR-5**: Animation icon cycles through 4 states
- Verify: `/` → `-` → `\` → `|` → `/` (1 second intervals)

### Quality Requirements

✅ **QR-1**: All regression tests passing
- 6 tests in test_oled_integration/ all pass

✅ **QR-2**: No test regressions
- All existing tests still pass

✅ **QR-3**: Hardware validation complete
- Visual confirmation on ESP32 + OLED

✅ **QR-4**: Code reviewed and documented
- Doxygen comments complete
- Bug report updated

✅ **QR-5**: Constitutional compliance verified
- All 8 principles satisfied

## Rollback Plan

**If critical issues discovered**:

1. **Immediate Rollback** (git):
   ```bash
   git revert HEAD
   git push origin bugfix/001-the-oled-display
   ```

2. **Preserve Tests**:
   - Keep regression tests (cherry-pick if needed)
   - Tests are valuable for future fix attempts

3. **Re-investigate**:
   - Review failed test output
   - Check hardware logs
   - Consult research.md for alternative approaches

4. **Re-attempt**:
   - Fix identified issues
   - Re-run from Phase 5 (Verification)

**Rollback Risk**: VERY LOW (simple changes, comprehensive tests)

## Dependencies

### External Dependencies
- WiFi events (ESP32 WiFi.onEvent)
- ReactESP event loops (display refresh timing)
- Hardware: ESP32 + SSD1306 OLED

### Internal Dependencies
- StartupProgressTracker (existing, no changes needed)
- IDisplayAdapter (existing, no changes needed)
- DisplayFormatter::getAnimationIcon() (existing, no changes needed)

### Build Dependencies
- PlatformIO (pio test, pio run)
- Unity test framework (already in use)
- Adafruit_SSD1306 library (already installed)

**No new dependencies introduced** ✅

## Monitoring & Validation

### Build-Time Checks
```bash
# Compile for native (tests)
pio run -e native
# Expected: Clean build, no errors

# Compile for ESP32 (firmware)
pio run -e esp32dev
# Expected: Clean build, no errors
```

### Test-Time Checks
```bash
# Run regression tests
pio test -e native -f test_oled_integration
# Expected: 6 tests, 0 failures

# Run full test suite
pio test -e native
# Expected: All tests pass, no regressions
```

### Runtime Checks
- Serial monitor: WiFi connection events logged
- WebSocket logs: Display manager events logged
- OLED display: Visual confirmation

### Verification Checklist

**Before Commit**:
- [ ] All regression tests passing
- [ ] Full test suite passing
- [ ] Build clean for native and esp32dev
- [ ] Hardware validation complete
- [ ] Screenshots captured

**Before Merge**:
- [ ] Bug report updated
- [ ] Documentation complete
- [ ] Code reviewed (self or peer)
- [ ] Constitutional compliance verified

## Related Documentation

### Project Documentation
- **Constitution**: `.specify/memory/constitution.md` v1.2.0
- **CLAUDE.md**: OLED Display Management section
- **User Requirements**: `user_requirements/R004 - OLED Basic Info Display.md`

### Bugfix Documentation
- **Bug Report**: `specs/bugfix-001-the-oled-display/bug-report.md`
- **Research**: `specs/bugfix-001-the-oled-display/research.md`
- **Data Model**: `specs/bugfix-001-the-oled-display/data-model.md`
- **Contracts**: `specs/bugfix-001-the-oled-display/contracts/`
- **Quickstart**: `specs/bugfix-001-the-oled-display/quickstart.md`
- **Tasks**: `specs/bugfix-001-the-oled-display/tasks.md`

### Component Documentation
- DisplayManager: `src/components/DisplayManager.{h,cpp}`
- StartupProgressTracker: `src/components/StartupProgressTracker.{h,cpp}`
- DisplayTypes: `src/types/DisplayTypes.h`

### Visual Evidence
- **BEFORE**: `misc/IMG_2483.png` (showing bugs)
- **AFTER**: `misc/IMG_2483_AFTER.png` (to be captured)

## Approval & Sign-Off

**Plan Status**: ✅ APPROVED FOR IMPLEMENTATION

**Approvals**:
- [x] Technical feasibility validated (research complete)
- [x] Risk assessment complete (LOW risk)
- [x] Constitutional compliance verified
- [x] Test strategy defined (TDD approach)
- [x] Resource impact acceptable (negligible)

**Ready for**: `/implement` command or manual execution per tasks.md

---

**Plan Version**: 1.0.0
**Created**: 2025-10-10
**Branch**: bugfix/001-the-oled-display
**Workflow**: Bugfix (regression tests required)
**Risk**: LOW
**Priority**: HIGH

🤖 Plan generated with [Claude Code](https://claude.com/claude-code)
