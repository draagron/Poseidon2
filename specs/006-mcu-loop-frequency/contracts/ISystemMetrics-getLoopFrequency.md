# HAL Interface Contract: ISystemMetrics::getLoopFrequency()

**Interface**: `ISystemMetrics`
**Method**: `getLoopFrequency()`
**Location**: `src/hal/interfaces/ISystemMetrics.h`
**Type**: Query method (read-only)

## Method Signature

```cpp
/**
 * @brief Get current main loop frequency
 *
 * Returns the average loop iteration frequency measured over the last
 * 5-second window. Returns 0 if no measurement has completed yet
 * (first 5 seconds after boot).
 *
 * @return Loop frequency in Hz (0 = not yet measured)
 */
virtual uint32_t getLoopFrequency() = 0;
```

## Contract Specification

### Input
- **Parameters**: None (query method)
- **Preconditions**: None (safe to call at any time)

### Output
- **Return Type**: `uint32_t`
- **Return Value**: Loop frequency in Hz
- **Range**: [0, UINT32_MAX]
  - `0`: No measurement yet (first 5 seconds after boot)
  - `10-2000`: Normal operating range for ESP32 with ReactESP
  - `> 2000`: Unexpected (log warning)

### Behavior Requirements

#### BR-001: Initial State
- **MUST** return `0` if called before first 5-second measurement completes
- **MUST NOT** block or delay (immediate return)

#### BR-002: Measurement Availability
- **MUST** return last calculated frequency after first measurement
- **MUST** update frequency every 5 seconds (±500ms tolerance)

#### BR-003: Value Stability
- **MUST** return same value if called multiple times within 5-second window
- **MUST NOT** return 0 after first measurement completes (unless system hang)

#### BR-004: Thread Safety
- **MUST** be safe to call from any execution context
- **MUST NOT** modify internal state (const method semantics)

#### BR-005: Performance
- **MUST** execute in < 10 microseconds (simple memory read)
- **MUST NOT** perform calculations or I/O operations

### Error Handling
- **No errors possible**: Method always returns valid uint32_t
- **Invalid state**: Returns 0 if measurement not yet available
- **System hang**: Returns 0 if loop stops iterating

## Implementation Requirements

### ESP32SystemMetrics Implementation

```cpp
// src/hal/implementations/ESP32SystemMetrics.h
class ESP32SystemMetrics : public ISystemMetrics {
private:
    LoopPerformanceMonitor _loopMonitor;  // Owned instance

public:
    // ... other methods ...

    /**
     * @brief Get loop frequency from internal monitor
     * @return Loop frequency in Hz (0 if not yet measured)
     */
    uint32_t getLoopFrequency() override {
        return _loopMonitor.getLoopFrequency();
    }

    /**
     * @brief Instrument loop iteration
     *
     * MUST be called from main.cpp loop() every iteration.
     * Updates internal LoopPerformanceMonitor state.
     */
    void instrumentLoop() {
        _loopMonitor.endLoop();
    }
};
```

### MockSystemMetrics Implementation

```cpp
// src/mocks/MockSystemMetrics.h
class MockSystemMetrics : public ISystemMetrics {
private:
    uint32_t _mockLoopFrequency;  // Test-controlled value

public:
    MockSystemMetrics() : _mockLoopFrequency(0) {}

    // Test control method
    void setMockLoopFrequency(uint32_t frequency) {
        _mockLoopFrequency = frequency;
    }

    // Interface implementation
    uint32_t getLoopFrequency() override {
        return _mockLoopFrequency;
    }

    // ... other mock methods ...
};
```

## Usage Examples

### Production Usage (main.cpp)

```cpp
// Global instances
ESP32SystemMetrics* systemMetrics = nullptr;

void setup() {
    // Initialize HAL
    systemMetrics = new ESP32SystemMetrics();

    // ... other setup ...
}

void loop() {
    // Instrument loop iteration
    systemMetrics->instrumentLoop();  // Updates frequency measurement

    // ReactESP processing
    app.tick();

    delay(10);
}
```

### Display Integration (DisplayManager.cpp)

```cpp
void DisplayManager::renderStatusPage() {
    // Collect metrics (includes loop frequency)
    _currentMetrics = _metricsCollector->collectMetrics();

    // ... render display ...

    // Format frequency (handles 0 case)
    String freqStr = DisplayFormatter::formatFrequency(_currentMetrics.loopFrequency);

    _displayAdapter->setCursor(0, getLineY(4));
    _displayAdapter->print(F("Loop: "));
    _displayAdapter->print(freqStr);
    _displayAdapter->print(F(" Hz"));
}
```

### Test Usage (test_performance_contracts)

```cpp
#include <unity.h>
#include "mocks/MockSystemMetrics.h"

void test_getLoopFrequency_returns_zero_initially() {
    MockSystemMetrics metrics;

    uint32_t freq = metrics.getLoopFrequency();

    TEST_ASSERT_EQUAL_UINT32(0, freq);
}

void test_getLoopFrequency_returns_set_value() {
    MockSystemMetrics metrics;
    metrics.setMockLoopFrequency(212);

    uint32_t freq = metrics.getLoopFrequency();

    TEST_ASSERT_EQUAL_UINT32(212, freq);
}

void test_getLoopFrequency_returns_same_value_multiple_calls() {
    MockSystemMetrics metrics;
    metrics.setMockLoopFrequency(150);

    uint32_t freq1 = metrics.getLoopFrequency();
    uint32_t freq2 = metrics.getLoopFrequency();

    TEST_ASSERT_EQUAL_UINT32(freq1, freq2);
}
```

## Contract Test Scenarios

### CT-001: Initial State
```cpp
void test_contract_initial_state_returns_zero() {
    ESP32SystemMetrics metrics;
    TEST_ASSERT_EQUAL_UINT32(0, metrics.getLoopFrequency());
}
```

### CT-002: Non-Zero After Measurement
```cpp
void test_contract_nonzero_after_first_measurement() {
    ESP32SystemMetrics metrics;

    // Simulate 5 seconds of loop iterations (1000 iterations @ 200 Hz)
    for (int i = 0; i < 1000; i++) {
        metrics.instrumentLoop();
        delayMicroseconds(5000);  // 5ms per iteration
    }

    uint32_t freq = metrics.getLoopFrequency();
    TEST_ASSERT_GREATER_THAN_UINT32(0, freq);
}
```

### CT-003: Value Stability Within Window
```cpp
void test_contract_value_stable_within_window() {
    ESP32SystemMetrics metrics;

    // First measurement (200 Hz expected)
    for (int i = 0; i < 1000; i++) {
        metrics.instrumentLoop();
        delayMicroseconds(5000);
    }

    uint32_t freq1 = metrics.getLoopFrequency();
    delay(100);  // Wait 100ms (still in same 5-second window)
    uint32_t freq2 = metrics.getLoopFrequency();

    TEST_ASSERT_EQUAL_UINT32(freq1, freq2);
}
```

### CT-004: No Side Effects
```cpp
void test_contract_no_side_effects() {
    MockSystemMetrics metrics;
    metrics.setMockLoopFrequency(300);

    // Call multiple times
    for (int i = 0; i < 100; i++) {
        uint32_t freq = metrics.getLoopFrequency();
        TEST_ASSERT_EQUAL_UINT32(300, freq);
    }
}
```

### CT-005: Performance Requirement
```cpp
void test_contract_performance_under_10_microseconds() {
    MockSystemMetrics metrics;
    metrics.setMockLoopFrequency(212);

    uint32_t start = micros();
    for (int i = 0; i < 1000; i++) {
        volatile uint32_t freq = metrics.getLoopFrequency();
    }
    uint32_t duration = micros() - start;

    // Average < 10 microseconds per call
    TEST_ASSERT_LESS_THAN_UINT32(10000, duration);
}
```

## Non-Functional Requirements

### NFR-001: Memory Footprint
- **ESP32SystemMetrics**: +16 bytes (LoopPerformanceMonitor member)
- **MockSystemMetrics**: +4 bytes (uint32_t member)
- **No heap allocation**: All static members

### NFR-002: Execution Time
- **getLoopFrequency()**: < 10 µs (memory read + return)
- **instrumentLoop()**: < 5 µs (increment + conditional)

### NFR-003: Thread Safety
- **getLoopFrequency()**: Read-only, safe to call from interrupts
- **instrumentLoop()**: Must only be called from main loop (not ISR-safe)

### NFR-004: Testability
- **Mockable**: `MockSystemMetrics` provides full test control
- **Deterministic**: Mock returns exact set value
- **No dependencies**: Can test in isolation

## Integration Points

### Upstream (Data Providers)
- `LoopPerformanceMonitor`: Measures frequency, called via `instrumentLoop()`

### Downstream (Data Consumers)
- `MetricsCollector::collectMetrics()`: Queries frequency for DisplayMetrics
- `DisplayManager::renderStatusPage()`: Displays formatted frequency
- `WebSocketLogger`: Logs frequency warnings (< 10 Hz or > 2000 Hz)

## Change History

| Version | Date       | Change Description |
|---------|------------|-------------------|
| 1.0     | 2025-10-10 | Initial contract definition |

## Related Contracts
- `ISystemMetrics::getFreeHeap()` - Similar query method pattern
- `ISystemMetrics::getTotalFlash()` - Similar query method pattern
- `IDisplayAdapter::print()` - Display output method

---
**Contract Status**: ✅ READY FOR IMPLEMENTATION - All requirements specified
