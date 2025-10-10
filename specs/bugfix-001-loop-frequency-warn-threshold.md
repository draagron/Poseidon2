# Bug Fix Plan: Loop Frequency WARN Threshold Incorrect

**Bug ID**: bugfix-001-loop-frequency-warn-threshold
**Feature**: R007 - WebSocket Loop Frequency Logging
**Date**: 2025-10-10
**Severity**: Medium (incorrect log levels, not a functional failure)
**Status**: Ready for implementation

---

## Problem Statement

### Current Behavior (INCORRECT)
The WebSocket loop frequency logging marks frequencies as WARN when they exceed 2000 Hz:
```cpp
LogLevel level = (frequency > 0 && frequency >= 10 && frequency <= 2000)
                 ? LogLevel::DEBUG : LogLevel::WARN;
```

**Issue**: System reports 412,000 Hz (valid high-performance loop), but it's incorrectly marked as WARN.

### Expected Behavior (CORRECT)
Only frequencies below 200 Hz should be marked as WARN (indicating system slowdown or performance issues).

**User Requirement**: "LOOP_FREQUENCY reported on websocket should only be marked WARN when below 200"

---

## Root Cause Analysis

### Incorrect Assumption in Original Specification
The data-model.md assumed ESP32 loop frequencies would be in the range of 10-2000 Hz, based on a misunderstanding of ReactESP event loop timing:
- **Original assumption**: Loop runs at 100-500 Hz (incorrect)
- **Actual behavior**: Loop runs at 400,000+ Hz (ESP32 native loop speed)
- **Error**: Upper threshold (2000 Hz) was too low, causing false WARN logs

### Logic Error
The current logic treats high frequencies (> 2000 Hz) as "unrealistic, possible measurement error" when they are actually the **normal operating range**.

**Correct Logic**:
- **High frequencies (> 200 Hz)**: Normal operation → DEBUG
- **Low frequencies (< 200 Hz)**: Performance degradation → WARN
- **Zero frequency (0 Hz)**: Startup placeholder → DEBUG

---

## Solution Design

### New Log Level Logic

**Proposed Implementation**:
```cpp
LogLevel level = (frequency == 0 || frequency >= 200)
                 ? LogLevel::DEBUG : LogLevel::WARN;
```

**Simplified Logic**:
- If `frequency == 0`: DEBUG (no measurement yet, first 5 seconds)
- If `frequency >= 200`: DEBUG (normal operation)
- Otherwise (1-199 Hz): WARN (performance degradation)

### Updated Frequency Categories

| Category | Frequency Range | Log Level | Reason |
|----------|----------------|-----------|--------|
| **No Measurement** | 0 Hz | DEBUG | First 5 seconds, expected condition |
| **Normal** | ≥ 200 Hz | DEBUG | Normal high-performance operation |
| **Low Warning** | 1-199 Hz | WARN | Performance degradation, system overload |

**Edge Cases**:
- **0 Hz**: DEBUG (startup, no measurement)
- **199 Hz**: WARN (just below threshold)
- **200 Hz**: DEBUG (threshold boundary)
- **412,000 Hz**: DEBUG (normal high-performance operation) ✅

---

## Implementation Plan

### Files to Modify
1. **`src/main.cpp:435`** - Fix log level logic (1 line change)
2. **`specs/007-loop-frequency-should/data-model.md`** - Update frequency categories documentation
3. **Unit tests** - Update test cases for new thresholds
4. **Integration tests** - Update expected log levels

### Step-by-Step Tasks

#### Task 1: Update Implementation (src/main.cpp)
**File**: `src/main.cpp:435`

**Current Code**:
```cpp
LogLevel level = (frequency > 0 && frequency >= 10 && frequency <= 2000)
                 ? LogLevel::DEBUG : LogLevel::WARN;
```

**New Code**:
```cpp
LogLevel level = (frequency == 0 || frequency >= 200)
                 ? LogLevel::DEBUG : LogLevel::WARN;
```

**Rationale**: Simplified logic that correctly identifies low performance (< 200 Hz) as WARN.

---

#### Task 2: Update Unit Tests
**File**: `test/test_websocket_frequency_units/test_main.cpp`

**Tests to Update**:
1. `test_log_level_lower_boundary` - Change from 10 Hz to 200 Hz
2. `test_log_level_below_threshold` - Change from 9 Hz to 199 Hz
3. `test_log_level_above_threshold` - Change from 2001 Hz to 201 Hz (or higher)
4. Remove upper boundary test (2000 Hz limit no longer exists)

**New Test Cases to Add**:
- **Test**: 199 Hz → WARN (just below threshold)
- **Test**: 200 Hz → DEBUG (threshold boundary)
- **Test**: 412,000 Hz → DEBUG (high-performance normal)

---

#### Task 3: Update Integration Tests
**File**: `test/test_websocket_frequency_integration/test_main.cpp`

**Tests to Update**:
- `test_log_emission_low_frequency` - Use frequency < 200 Hz for WARN test
- `test_log_emission_high_frequency` - Use frequency ≥ 200 Hz for DEBUG test
- `test_metadata_log_level_abnormal` - Update expected WARN frequencies

---

#### Task 4: Update Documentation
**File**: `specs/007-loop-frequency-should/data-model.md:120-157`

**Update Frequency Categories Table**:
```markdown
| Category | Frequency Range | Log Level | Reason |
|----------|----------------|-----------|--------|
| **No Measurement** | 0 Hz | DEBUG | First 5 seconds, expected condition |
| **Normal** | ≥ 200 Hz | DEBUG | Normal high-performance operation |
| **Low Warning** | 1-199 Hz | WARN | Performance degradation, system overload |
```

**Update Decision Logic**:
```cpp
LogLevel determineLogLevel(uint32_t frequency) {
    if (frequency == 0) {
        return LogLevel::DEBUG;  // No measurement yet
    }
    if (frequency >= 200) {
        return LogLevel::DEBUG;  // Normal operation
    }
    return LogLevel::WARN;  // Low performance (< 200 Hz)
}
```

**Update Edge Cases**:
- **0 Hz**: DEBUG (no measurement)
- **199 Hz**: WARN (just below threshold)
- **200 Hz**: DEBUG (threshold boundary)
- **412,000 Hz**: DEBUG (high-performance normal)

---

#### Task 5: Update Quickstart Validation
**File**: `specs/007-loop-frequency-should/quickstart.md`

**Update Step 6**: Verify log level expectations
- Normal: ≥ 200 Hz → DEBUG
- Warning: 1-199 Hz → WARN

---

## Testing Strategy

### Regression Test (TDD Approach)
**Before fixing the bug, create a failing test that demonstrates the issue:**

```cpp
void test_high_frequency_should_be_debug() {
    // Test that 412,000 Hz is correctly marked as DEBUG
    uint32_t frequency = 412000;
    LogLevel level = (frequency == 0 || frequency >= 200)
                     ? LogLevel::DEBUG : LogLevel::WARN;
    TEST_ASSERT_EQUAL(LogLevel::DEBUG, level);
}
```

**Expected Result**: This test should FAIL with current code, PASS after fix.

### Test Coverage
1. **Boundary Tests**:
   - 0 Hz → DEBUG ✓
   - 199 Hz → WARN ✓
   - 200 Hz → DEBUG ✓
   - 412,000 Hz → DEBUG ✓

2. **Range Tests**:
   - 1-199 Hz → WARN
   - 200-999,999 Hz → DEBUG

3. **Integration Tests**:
   - MockSystemMetrics returns 100 Hz → broadcastLog(WARN, ...)
   - MockSystemMetrics returns 500 Hz → broadcastLog(DEBUG, ...)
   - MockSystemMetrics returns 412,000 Hz → broadcastLog(DEBUG, ...)

---

## Validation Checklist

### Pre-Implementation
- [ ] Review current test failures (identify tests broken by old logic)
- [ ] Create regression test demonstrating the bug (412,000 Hz → WARN incorrectly)
- [ ] Document expected behavior for user validation

### Implementation
- [ ] Update src/main.cpp:435 (1 line change)
- [ ] Update unit tests (test_websocket_frequency_units)
- [ ] Update integration tests (test_websocket_frequency_integration)
- [ ] Update data-model.md documentation
- [ ] Update quickstart.md validation steps

### Post-Implementation
- [ ] All unit tests pass (28 tests)
- [ ] All integration tests pass
- [ ] ESP32 firmware builds successfully
- [ ] Regression test passes (412,000 Hz → DEBUG)
- [ ] Manual validation: Upload to ESP32, verify WARN only for < 200 Hz

---

## Risk Assessment

**Risk Level**: LOW

**Risks**:
1. **Test Failures**: Many existing tests assume 10-2000 Hz range
   - **Mitigation**: Update all tests before implementation

2. **Documentation Inconsistency**: Multiple docs reference old thresholds
   - **Mitigation**: Update all references to frequency thresholds

3. **User Confusion**: Users may have adapted to old WARN threshold
   - **Mitigation**: Update CHANGELOG.md, document breaking change

**Breaking Change**: YES (log levels will change for frequencies 200-2000 Hz)
- **Old behavior**: 2001 Hz → WARN
- **New behavior**: 2001 Hz → DEBUG
- **Impact**: Less noise in logs (fewer false WARN messages)

---

## Estimated Effort

- **Implementation**: 15 minutes (1 line code change + test updates)
- **Testing**: 20 minutes (update 8-10 test cases)
- **Documentation**: 15 minutes (update 3 files)
- **Validation**: 10 minutes (build + manual test on ESP32)
- **Total**: ~60 minutes

---

## Success Criteria

### Functional
- [ ] 412,000 Hz frequency reports as DEBUG (not WARN)
- [ ] Frequencies below 200 Hz report as WARN
- [ ] Frequency 0 Hz still reports as DEBUG (startup)

### Technical
- [ ] All 28 unit/integration tests pass
- [ ] ESP32 firmware builds without errors
- [ ] No memory footprint increase
- [ ] No performance degradation

### Documentation
- [ ] data-model.md reflects new thresholds
- [ ] quickstart.md validation updated
- [ ] CHANGELOG.md documents bug fix

---

## Rollout Plan

### Development
1. Create branch: `bugfix/001-loop-frequency-warn-threshold`
2. Update tests first (TDD - tests should fail)
3. Implement fix in main.cpp
4. Verify all tests pass
5. Update documentation

### Testing
1. Run native tests: `pio test -e native -f test_websocket_frequency_*`
2. Build ESP32 firmware: `pio run -e esp32dev`
3. Upload to ESP32: `pio run -e esp32dev -t upload`
4. Monitor WebSocket logs: Verify 412,000 Hz → DEBUG

### Deployment
1. Create PR from bugfix branch
2. QA review
3. Merge to main
4. Tag as patch release (if warranted)

---

## Related Issues

- **Original Specification**: R007 WebSocket Loop Frequency Logging
- **Affected Files**: src/main.cpp, tests, documentation
- **User Report**: "LOOP_FREQUENCY at 412000 hz should not be WARN"

---

## Notes

### Why 200 Hz Threshold?
- **User Requirement**: "should only be marked WARN when below 200"
- **Performance Indicator**: < 200 Hz suggests system slowdown
- **ESP32 Typical**: Normal operation is 100,000-500,000 Hz range
- **Conservative Threshold**: 200 Hz provides early warning before severe degradation

### Alternative Thresholds Considered
- **100 Hz**: Too aggressive, may cause false positives
- **500 Hz**: May miss early performance degradation
- **200 Hz**: Balanced threshold matching user requirement

---

**Bug Fix Plan Version**: 1.0
**Created**: 2025-10-10
**Ready for Implementation**: YES
**Requires User Approval**: NO (user requested this fix)
