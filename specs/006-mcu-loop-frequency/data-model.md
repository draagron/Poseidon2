# Phase 1: Data Model

**Feature**: MCU Loop Frequency Display
**Date**: 2025-10-10

## Overview

This document defines the data structures used for loop performance measurement and display. All structures use static allocation (constitutional Principle II) with primitive types for efficiency.

## Core Data Structures

### 1. PerformanceMetrics (Internal State)

**Purpose**: Track loop iteration metrics for frequency calculation

**Location**: `src/utils/LoopPerformanceMonitor.h` (private members)

**Fields**:
```cpp
class LoopPerformanceMonitor {
private:
    uint32_t _loopCount;              // Accumulated iterations in current window
    uint32_t _lastReportTime;         // millis() timestamp of last report
    uint32_t _currentFrequency;       // Last calculated frequency (Hz)
    bool _hasFirstMeasurement;        // False until first 5-second window completes

    // Not needed per requirements (removed from reference code):
    // uint32_t _loopStartTime;       // micros() - would add overhead
    // uint32_t _maxLoopTime;         // max duration - not displayed
    // uint32_t _totalLoopTime;       // sum of durations - not used
};
```

**Memory Footprint**: 13 bytes (3× uint32_t + 1× bool) + 3 bytes padding = 16 bytes aligned

**Lifecycle**:
- Initialized in constructor: all counters zero, `_hasFirstMeasurement = false`
- Updated every loop iteration via `endLoop()`
- Reset every 5 seconds after frequency calculation

### 2. DisplayMetrics (Extended)

**Purpose**: System metrics collected for display rendering

**Location**: `src/types/DisplayTypes.h` (existing struct, add new field)

**Before** (existing):
```cpp
struct DisplayMetrics {
    uint32_t freeRAM;        // Free heap in bytes
    uint32_t totalRAM;       // Total heap in bytes
    uint32_t usedFlash;      // Used flash in KB
    uint32_t totalFlash;     // Total flash in KB
    uint8_t cpuIdlePercent;  // CPU idle percentage (0-100) ← TO BE REMOVED
    uint8_t animationState;  // Animation icon state (0-3)
};
```

**After** (modified):
```cpp
struct DisplayMetrics {
    uint32_t freeRAM;        // Free heap in bytes
    uint32_t totalRAM;       // Total heap in bytes
    uint32_t usedFlash;      // Used flash in KB
    uint32_t totalFlash;     // Total flash in KB
    uint32_t loopFrequency;  // Loop frequency in Hz (0 = not yet measured) ← NEW
    uint8_t animationState;  // Animation icon state (0-3)
};
```

**Changes**:
- **REMOVE**: `uint8_t cpuIdlePercent` (replaced by loop frequency per FR-044)
- **ADD**: `uint32_t loopFrequency` (supports 0-4,294,967,295 Hz range)

**Memory Impact**: +3 bytes (uint32_t replaces uint8_t, but struct already 4-byte aligned)

**Validation Rules**:
- `loopFrequency == 0`: Indicates no measurement yet (display "---")
- `loopFrequency < 10`: Warning - critically low performance
- `loopFrequency 100-500`: Normal operating range for ESP32 with ReactESP
- `loopFrequency > 1000`: Unexpected - log warning, display as "X.Xk Hz"

### 3. HAL Interface Extension

**Purpose**: Abstract loop frequency access for testing

**Location**: `src/hal/interfaces/ISystemMetrics.h` (existing interface, add method)

**Before** (existing methods):
```cpp
class ISystemMetrics {
public:
    virtual ~ISystemMetrics() = default;

    virtual uint32_t getFreeHeap() = 0;
    virtual uint32_t getTotalHeap() = 0;
    virtual uint32_t getUsedFlash() = 0;
    virtual uint32_t getTotalFlash() = 0;
    virtual uint8_t getCPUIdlePercent() = 0;  ← TO BE REMOVED
};
```

**After** (modified):
```cpp
class ISystemMetrics {
public:
    virtual ~ISystemMetrics() = default;

    virtual uint32_t getFreeHeap() = 0;
    virtual uint32_t getTotalHeap() = 0;
    virtual uint32_t getUsedFlash() = 0;
    virtual uint32_t getTotalFlash() = 0;
    virtual uint32_t getLoopFrequency() = 0;  ← NEW (replaces getCPUIdlePercent)
};
```

**Rationale**: Replace unused CPU idle metric with loop frequency (one metric per line constraint)

## State Transitions

### Loop Frequency Measurement State Machine

```
[BOOT]
  │
  ├─> PerformanceMetrics initialized
  │   (loopCount = 0, hasFirstMeasurement = false)
  │
  ├─> [MEASURING] (0-5 seconds)
  │   │
  │   ├─> endLoop() called each iteration
  │   │   (loopCount++, no frequency calculated yet)
  │   │
  │   └─> Display shows "Loop: --- Hz"
  │
  ├─> [FIRST_MEASUREMENT] (at 5 seconds)
  │   │
  │   ├─> frequency = loopCount / 5
  │   ├─> hasFirstMeasurement = true
  │   ├─> loopCount reset to 0
  │   └─> Display shows "Loop: XXX Hz"
  │
  └─> [STEADY_STATE] (5+ seconds)
      │
      ├─> Repeat every 5 seconds:
      │   ├─> Calculate new frequency
      │   ├─> Update display
      │   └─> Reset loopCount
      │
      └─> Handle edge cases:
          ├─> millis() overflow: Detect wrap, continue measurement
          ├─> Low frequency (< 10 Hz): Log warning
          └─> High frequency (> 999 Hz): Abbreviate display format
```

### Display Update Flow

```
[ReactESP 5-second timer fires]
  │
  ├─> DisplayManager::renderStatusPage()
  │   │
  │   ├─> MetricsCollector::collectMetrics()
  │   │   │
  │   │   ├─> ISystemMetrics::getLoopFrequency()
  │   │   │   │
  │   │   │   ├─> ESP32SystemMetrics::getLoopFrequency()
  │   │   │   │   │
  │   │   │   │   └─> LoopPerformanceMonitor::getLoopFrequency()
  │   │   │   │       (returns _currentFrequency)
  │   │   │   │
  │   │   │   └─> Returns frequency to MetricsCollector
  │   │   │
  │   │   └─> Returns DisplayMetrics struct
  │   │
  │   ├─> DisplayFormatter::formatFrequency(metrics.loopFrequency)
  │   │   │
  │   │   ├─> If frequency == 0: return "---"
  │   │   ├─> If frequency < 1000: return "XXX"
  │   │   └─> If frequency >= 1000: return "X.Xk"
  │   │
  │   └─> IDisplayAdapter::print("Loop: " + formatted + " Hz")
  │
  └─> Display updated
```

## Data Flow Diagram

```
┌─────────────────────────────────────────────────────────────┐
│ main.cpp loop()                                             │
│                                                             │
│  monitor.endLoop() ──> LoopPerformanceMonitor              │
│                        ├─ _loopCount++                      │
│                        ├─ Check 5-second boundary           │
│                        └─ Calculate _currentFrequency       │
└─────────────────────────────────────────────────────────────┘
                                   │
                                   │ (every 5 seconds)
                                   ▼
┌─────────────────────────────────────────────────────────────┐
│ ReactESP event: app.onRepeat(5000, renderStatusPage)       │
└─────────────────────────────────────────────────────────────┘
                                   │
                                   ▼
┌─────────────────────────────────────────────────────────────┐
│ DisplayManager::renderStatusPage()                          │
│  ├─ MetricsCollector::collectMetrics()                      │
│  │  └─ ISystemMetrics::getLoopFrequency() ──────────┐      │
│  │                                                    │      │
│  ├─ DisplayFormatter::formatFrequency(freq) ─────┐  │      │
│  │                                               │  │      │
│  └─ IDisplayAdapter::print("Loop: " + fmt)      │  │      │
└─────────────────────────────────────────────────┼──┼──────┘
                                                  │  │
                                                  │  │
┌─────────────────────────────────────────────────┘  │
│ DisplayFormatter (src/components/DisplayFormatter.h) │
│  ├─ Input: uint32_t frequency                       │
│  ├─ Logic:                                           │
│  │  ├─ if (freq == 0) return "---"                  │
│  │  ├─ if (freq < 1000) return String(freq)         │
│  │  └─ if (freq >= 1000) return String(freq/1000.0, 1) + "k" │
│  └─ Output: String (right-padded to 3 chars)        │
└────────────────────────────────────────────────────┘
                                                  │
┌─────────────────────────────────────────────────┘
│ ESP32SystemMetrics (src/hal/implementations/ESP32SystemMetrics.h) │
│  ├─ Member: LoopPerformanceMonitor _loopMonitor    │
│  ├─ Method: uint32_t getLoopFrequency()             │
│  │  └─ return _loopMonitor.getLoopFrequency();     │
│  │                                                   │
│  └─ Method: void instrumentLoop()                   │
│     └─ Called from main.cpp loop() every iteration │
└────────────────────────────────────────────────────┘
                                                  │
┌─────────────────────────────────────────────────┘
│ LoopPerformanceMonitor (src/utils/LoopPerformanceMonitor.h) │
│  ├─ State: _loopCount, _lastReportTime, _currentFrequency   │
│  ├─ Method: void endLoop()                                  │
│  │  ├─ _loopCount++                                         │
│  │  ├─ if (millis() - _lastReportTime >= 5000)             │
│  │  │  ├─ _currentFrequency = _loopCount / 5               │
│  │  │  ├─ _loopCount = 0                                    │
│  │  │  └─ _lastReportTime = millis()                        │
│  │  └─ return                                                │
│  │                                                           │
│  └─ Method: uint32_t getLoopFrequency()                     │
│     └─ return _hasFirstMeasurement ? _currentFrequency : 0  │
└──────────────────────────────────────────────────────────────┘
```

## Constraints and Invariants

### Memory Constraints
1. **Total static allocation**: 16 bytes for LoopPerformanceMonitor
2. **No heap allocation**: All counters are primitive types (uint32_t, bool)
3. **DisplayMetrics size**: Increased by 3 bytes (24 → 27 bytes, rounds to 28 with alignment)
4. **Total feature RAM impact**: < 50 bytes (well within constitutional limits)

### Timing Constraints
1. **Measurement window**: Exactly 5000ms (±1ms tolerance due to ReactESP scheduling)
2. **Update frequency**: Every 5 seconds, synchronized with display refresh
3. **First measurement delay**: 5 seconds after boot (displays "---" until then)
4. **Overflow handling**: `millis()` wrap at ~49.7 days detected and handled

### Data Validity Constraints
1. **Frequency range**: 0-9999 Hz (display supports up to "9.9k Hz")
2. **Zero frequency**: Valid state during first 5 seconds (not yet measured)
3. **Frequency < 10 Hz**: Warning logged via WebSocket (potential system hang)
4. **Frequency > 2000 Hz**: Warning logged (unexpected for ESP32 + ReactESP)

### Interface Contracts
1. **ISystemMetrics::getLoopFrequency()**: MUST return 0 if no measurement yet
2. **LoopPerformanceMonitor::endLoop()**: MUST be called every loop iteration
3. **DisplayFormatter::formatFrequency()**: MUST handle 0 (return "---")
4. **ESP32SystemMetrics**: MUST own LoopPerformanceMonitor instance lifecycle

## Validation Rules

### Input Validation
- **endLoop() calls**: No input validation needed (no parameters)
- **millis() overflow**: Detected via `(current < last)` comparison

### Output Validation
- **getLoopFrequency()**: Returns uint32_t in range [0, UINT32_MAX]
- **formatFrequency()**: Returns String with length ≤ 7 chars ("X.XXk Hz")

### State Consistency
- **Invariant 1**: `_loopCount` reset to 0 after each frequency calculation
- **Invariant 2**: `_hasFirstMeasurement` set to true after first 5-second window
- **Invariant 3**: `_currentFrequency` only updated every 5 seconds
- **Invariant 4**: Display shows "---" if `_hasFirstMeasurement == false`

## Error Handling

### Error Scenarios
1. **millis() overflow**: Detected, frequency calculated normally, counters reset
2. **Zero loop count after 5 sec**: Display "0 Hz", log FATAL error (system hang)
3. **Display init failure**: Already handled by DisplayManager (graceful degradation)
4. **Invalid frequency (> 10000 Hz)**: Clamp to "9.9k Hz", log warning

### Recovery Strategies
- **No recovery needed**: Loop frequency measurement is non-critical
- **Continue operation**: Other display metrics (RAM, Flash, WiFi) unaffected
- **WebSocket logging**: Operator alerted to performance anomalies

## Testing Validation

### Unit Test Scenarios (DisplayMetrics)
1. Default initialization: `loopFrequency == 0`
2. Assignment: Set to 212, verify value persists
3. Boundary: Set to UINT32_MAX, verify no overflow

### Integration Test Scenarios
1. **First 5 seconds**: Display shows "Loop: --- Hz"
2. **After 5 seconds**: Display shows "Loop: XXX Hz" with measured value
3. **Frequency update**: Value changes every 5 seconds
4. **millis() overflow**: Frequency calculated correctly across wrap boundary
5. **Low frequency**: < 10 Hz triggers warning log
6. **High frequency**: > 999 Hz displays as "X.Xk Hz"
7. **Zero frequency**: Displays "0 Hz" if loop hangs

### Contract Test Scenarios
1. **ISystemMetrics::getLoopFrequency()**: Returns uint32_t
2. **MockSystemMetrics**: Can set arbitrary frequency for testing
3. **ESP32SystemMetrics**: Delegates to LoopPerformanceMonitor

---
**Phase 1 Data Model Status**: ✅ COMPLETE - All data structures defined and validated
