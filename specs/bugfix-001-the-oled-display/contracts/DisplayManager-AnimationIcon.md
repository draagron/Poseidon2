# Contract: DisplayManager Animation Icon Rendering

**Component**: DisplayManager
**Modified Methods**: `renderStatusPage()`, `updateAnimationIcon()`
**Bug Fix**: bugfix-001 (Animation icon brackets removal)
**Date**: 2025-10-10

## Contract Overview

This contract defines the behavioral changes to animation icon rendering in `DisplayManager` to remove square brackets and fix alignment.

## Bug Description

**Current Behavior** (buggy):
- Animation icon renders as: `[ / ]`, `[ - ]`, `[ \ ]`, `[ | ]`
- Square brackets are literal characters, not part of the icon
- Cursor position: X=108 (for 20-pixel width including brackets)

**Expected Behavior** (fixed):
- Animation icon renders as: `/`, `-`, `\`, `|` (single character, no brackets)
- Cursor position: X=118 (for 10-pixel width, single character)

## Modified Method: renderStatusPage()

### Current Implementation (Buggy)

**Location**: `src/components/DisplayManager.cpp:212-217`

```cpp
// Line 5: Animation icon (right corner)
_displayAdapter->setCursor(108, getLineY(5));  // 128 - 20 pixels for "[ X ]"
_displayAdapter->print("[ ");
char icon = DisplayFormatter::getAnimationIcon(_currentMetrics.animationState);
char iconStr[2] = {icon, '\0'};
_displayAdapter->print(iconStr);
_displayAdapter->print(" ]");
```

### Fixed Implementation

```cpp
// Line 5: Animation icon (right corner)
_displayAdapter->setCursor(118, getLineY(5));  // 128 - 10 pixels for " X "
char icon = DisplayFormatter::getAnimationIcon(_currentMetrics.animationState);
char iconStr[2] = {icon, '\0'};
_displayAdapter->print(iconStr);
```

### Changes

1. **Cursor X-Position**: 108 → 118
   - Calculation: 128 (display width) - 10 pixels (single char + margin)
   - Aligns icon to right side without brackets

2. **Removed Lines**:
   - `_displayAdapter->print("[ ");` - DELETED
   - `_displayAdapter->print(" ]");` - DELETED

3. **Preserved**:
   - Icon character generation: `DisplayFormatter::getAnimationIcon()`
   - Icon string formatting: `char iconStr[2] = {icon, '\0'};`

## Modified Method: updateAnimationIcon()

### Current Implementation (Buggy)

**Location**: `src/components/DisplayManager.cpp:234-239`

```cpp
_displayAdapter->setCursor(108, getLineY(5));
_displayAdapter->print("[ ");
char icon = DisplayFormatter::getAnimationIcon(metrics.animationState);
char iconStr[2] = {icon, '\0'};
_displayAdapter->print(iconStr);
_displayAdapter->print(" ]");
_displayAdapter->display();
```

### Fixed Implementation

```cpp
_displayAdapter->setCursor(118, getLineY(5));
char icon = DisplayFormatter::getAnimationIcon(metrics.animationState);
char iconStr[2] = {icon, '\0'};
_displayAdapter->print(iconStr);
_displayAdapter->display();
```

### Changes

Same as `renderStatusPage()`:
1. Cursor X-Position: 108 → 118
2. Removed bracket printing lines

## Preconditions

**Same as before** (no new preconditions):
1. Display initialized: `_displayAdapter->init()` called
2. Display ready: `_displayAdapter->isReady() == true`
3. Valid animation state: `metrics.animationState` ∈ {0, 1, 2, 3}

## Postconditions

### Visual Output (CHANGED)

**BEFORE Fix**:
```
Line 5 (bottom right): "[ / ]"
```

**AFTER Fix**:
```
Line 5 (bottom right): "/"
```

### Display Buffer State

**Cursor Position** (CHANGED):
- X-coordinate: 118 (was 108)
- Y-coordinate: `getLineY(5)` (unchanged)

**Characters Written**:
- BEFORE: 5 characters (`[ `, `/`, ` ]`)
- AFTER: 1 character (`/`)

**Pixel Usage**:
- BEFORE: ~20 pixels width (5 chars × ~4 pixels)
- AFTER: ~10 pixels width (1 char + margin)

### Animation State Progression (UNCHANGED)

**Cycle**: 0 → 1 → 2 → 3 → 0 (repeats)

| State | Icon Character |
|-------|----------------|
| 0 | `/` |
| 1 | `-` |
| 2 | `\` |
| 3 | `|` |

**Update Frequency**: 1 second (1000ms) via ReactESP `app.onRepeat(DISPLAY_ANIMATION_INTERVAL_MS, ...)`

## Invariants

### Icon Character Set
**Invariant**: Icon MUST be one of: `/`, `-`, `\`, `|`

**Validation**:
```cpp
char icon = DisplayFormatter::getAnimationIcon(animationState);
assert(icon == '/' || icon == '-' || icon == '\\' || icon == '|');
```

### Cursor Positioning
**Invariant**: Cursor X-position MUST be 118 (right-aligned for single character)

**Rationale**:
- Display width: 128 pixels
- Font width: ~6 pixels per character
- Right margin: ~4 pixels
- Calculation: 128 - 6 - 4 = 118

### No Brackets
**Invariant**: Display output MUST NOT contain `[` or `]` characters

**Validation** (integration test):
```cpp
TEST(DisplayManagerIntegration, AnimationIconNoBrackets) {
    MockDisplayAdapter mockDisplay;
    DisplayManager dm(&mockDisplay, &mockMetrics);

    dm.renderStatusPage();

    // Verify no bracket characters rendered
    ASSERT_FALSE(mockDisplay.hasCall("print", "["));
    ASSERT_FALSE(mockDisplay.hasCall("print", "]"));
}
```

## Performance Impact

### Rendering Time (IMPROVED)

**BEFORE**:
- 3 `print()` calls per animation update
- ~15 μs per update (3 × 5 μs)

**AFTER**:
- 1 `print()` call per animation update
- ~5 μs per update (1 × 5 μs)

**Improvement**: 67% faster (~10 μs saved per update)

**Annual Savings** (for fun):
- Update frequency: 1 Hz
- Updates per year: 31,536,000
- Time saved: 315 seconds/year (~5 minutes)
- **Conclusion**: Negligible, but cleaner code is the real win

### Memory Impact (IMPROVED)

**Flash Footprint**:
- BEFORE: ~80 bytes (3 print calls)
- AFTER: ~60 bytes (1 print call)
- **Savings**: -20 bytes flash

**RAM**: No change (same stack usage for local variables)

## Edge Cases

### Edge Case 1: Display Partially Rendered

**Scenario**: Display refresh interrupted between status page and animation update

**BEFORE Behavior**:
- Status page shows: `[ / ]` (full 5-char sequence)
- Animation update overwrites: `[ - ]`
- **Problem**: If interrupted, may see `[ / ]` or `[ -` artifacts

**AFTER Behavior**:
- Status page shows: `/`
- Animation update overwrites: `-`
- **Improvement**: Single character always atomic, no partial artifacts

### Edge Case 2: Cursor Position Drift

**Scenario**: Previous rendering moved cursor unexpectedly

**Mitigation**:
- `setCursor(118, getLineY(5))` explicitly set before every print
- No assumption about current cursor position
- **Result**: Robust against cursor drift

### Edge Case 3: Font Size Change

**Scenario**: Future code changes `setTextSize(2)` before animation

**Current Behavior**:
- Font size not reset in `updateAnimationIcon()`
- Would render large icon (2× size)

**Mitigation** (not in this fix, but noted):
- Consider adding `_displayAdapter->setTextSize(1);` in `updateAnimationIcon()`
- Currently safe: main code always uses size 1

## Test Validation

### Unit Test: Animation Icon No Brackets

```cpp
TEST(DisplayManager, AnimationIconNoBrackets) {
    MockDisplayAdapter mockDisplay;
    MockSystemMetrics mockMetrics;
    DisplayManager dm(&mockDisplay, &mockMetrics);

    dm.init();

    // Update animation icon
    DisplayMetrics metrics;
    metrics.animationState = 0;
    dm.updateAnimationIcon(metrics);

    // Verify no brackets printed
    auto calls = mockDisplay.getAllCalls();
    for (const auto& call : calls) {
        ASSERT_NE(call.text, "[ ");
        ASSERT_NE(call.text, " ]");
    }

    // Verify icon character printed
    ASSERT_TRUE(mockDisplay.hasCall("print", "/"));
}
```

### Unit Test: Cursor Position

```cpp
TEST(DisplayManager, AnimationIconCursorPosition) {
    MockDisplayAdapter mockDisplay;
    MockSystemMetrics mockMetrics;
    DisplayManager dm(&mockDisplay, &mockMetrics);

    dm.init();

    // Update animation
    dm.updateAnimationIcon();

    // Verify cursor set to X=118, Y=50 (line 5)
    ASSERT_TRUE(mockDisplay.hasCall("setCursor", 118, 50));
}
```

### Integration Test: Visual Regression

**Manual Test** (hardware required):
1. Flash firmware to ESP32 with OLED
2. Wait for WiFi connection
3. Observe animation icon at bottom right
4. **Expected**: Single rotating character (`/`, `-`, `\`, `|`)
5. **Expected**: No square brackets visible
6. **Expected**: Icon positioned ~4 pixels from right edge

**Validation**:
- Compare with screenshot `misc/IMG_2483.png` (BEFORE - shows `[ +`)
- New screenshot should show clean icon without brackets

## Visual Specification

### Display Layout (Line 5)

**BEFORE**:
```
Line 5: <empty>                              [ / ]
        ↑                                    ↑   ↑
        X=0                                 X=108 X=128
```

**AFTER**:
```
Line 5: <empty>                                /
        ↑                                      ↑ ↑
        X=0                                  X=118 X=128
```

### Character Spacing

**Font**: Adafruit GFX default (5×7 pixel characters)
**Width**: ~6 pixels per character (5px glyph + 1px spacing)

**BEFORE**:
- `[` at X=108: 6 pixels
- ` ` at X=114: 6 pixels (space)
- `/` at X=120: 6 pixels
- (Total exceeds 128 - likely clipped!)

**AFTER**:
- `/` at X=118: 6 pixels
- Right margin: 4 pixels
- Total: 118 + 6 = 124 (fits within 128 ✅)

## Dependencies

### Internal Dependencies
- `DisplayFormatter::getAnimationIcon()`: Unchanged, returns char for state 0-3
- `IDisplayAdapter::setCursor()`: Unchanged, sets cursor position
- `IDisplayAdapter::print()`: Unchanged, renders string to display

### External Dependencies
- ReactESP timer: `app.onRepeat(1000, ...)` triggers `updateAnimationIcon()` every 1s
- Display hardware: SSD1306 OLED (128×64 resolution)

### Constitutional Dependencies
- **Principle II (Resource-Aware)**: Flash savings (-20 bytes) ✅
- **Principle VII (Fail-Safe)**: No new failure modes ✅

## Backward Compatibility

**Breaking Changes**: None (internal rendering change, no API changes)

**Visual Change**: User-visible improvement (removes unwanted brackets)

**Testing Impact**:
- Tests checking for bracket output will FAIL (expected)
- Update test expectations to match new behavior

## Rollback Plan

**If fix causes issues**:

1. Revert cursor position: 118 → 108
2. Re-add bracket printing:
   ```cpp
   _displayAdapter->print("[ ");
   _displayAdapter->print(iconStr);
   _displayAdapter->print(" ]");
   ```

**Likelihood**: Very low (simple rendering change, no logic changes)

## References

- DisplayManager.cpp: `src/components/DisplayManager.cpp:212-217, 234-239`
- DisplayFormatter: `src/components/DisplayFormatter.h` (getAnimationIcon method)
- Bug Screenshot: `misc/IMG_2483.png` (shows `[ +` at bottom)

---
**Contract Version**: 1.0.0
**Status**: Ready for implementation
**Expected Impact**: User-visible improvement, no regressions
