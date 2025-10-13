# Contract: FrequencyCalculator

**Component**: Source Statistics Tracking
**Feature**: 012-sources-stats-and
**Version**: 1.0.0

## Purpose

FrequencyCalculator is a stateless utility class that computes rolling-average update frequencies from circular timestamp buffers. It implements the 10-sample frequency calculation specified in FR-005.

## Interface

```cpp
class FrequencyCalculator {
public:
    /**
     * @brief Calculate update frequency from circular timestamp buffer
     *
     * Computes rolling average based on first and last timestamp in buffer.
     * Formula: frequency (Hz) = 1000 / ((last_timestamp - first_timestamp) / (count - 1))
     *
     * @param buffer Circular buffer of millis() timestamps (must contain ≥2 samples)
     * @param count Number of valid timestamps in buffer (2-10)
     *
     * @return Frequency in Hz (0.0 if invalid input)
     *
     * @pre count >= 2 (otherwise frequency undefined)
     * @pre count <= 10 (buffer size limit)
     * @pre buffer[0..count-1] contain valid millis() timestamps
     *
     * @note Stateless function - can be called with any buffer
     * @note Handles millis() rollover gracefully (incorrect for one cycle, self-corrects)
     * @note Returns 0.0 if count < 2 or average interval is 0
     *
     * @example
     * uint32_t buffer[10] = {1000, 1100, 1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900};
     * double freq = FrequencyCalculator::calculate(buffer, 10);
     * // freq = 1000 / ((1900 - 1000) / 9) = 1000 / 100 = 10.0 Hz
     */
    static double calculate(const uint32_t* buffer, uint8_t count);

    /**
     * @brief Add timestamp to circular buffer
     *
     * Inserts timestamp at current index, advances index with wrap-around,
     * sets full flag when 10 samples collected.
     *
     * @param buffer Circular buffer of millis() timestamps (size 10)
     * @param index Current buffer index (0-9), updated by reference
     * @param full Buffer full flag, set to true when 10 samples collected
     * @param timestamp Timestamp to insert (typically millis())
     *
     * @post buffer[index] = timestamp
     * @post index = (index + 1) % 10
     * @post full = true if buffer wrapped around
     *
     * @note Helper function for MessageSource timestamp management
     */
    static void addTimestamp(uint32_t* buffer, uint8_t& index, bool& full,
                            uint32_t timestamp);
};
```

## Implementation Details

### Frequency Calculation Algorithm

**Steps**:
1. Validate `count >= 2` (return 0.0 if invalid)
2. Calculate `totalInterval = buffer[count - 1] - buffer[0]`
3. Calculate `avgInterval = totalInterval / (count - 1)`
4. If `avgInterval == 0`, return 0.0 (guard against division by zero)
5. Return `1000.0 / avgInterval` (convert ms to Hz)

**Precision**:
- Use `double` for intermediate calculations
- Round result to 1 decimal place for display
- Internal storage keeps full precision

**Edge Cases**:
- **count < 2**: Return 0.0 (frequency undefined)
- **avgInterval == 0**: Return 0.0 (all timestamps identical, impossible in practice)
- **millis() rollover**: Incorrect result for one buffer cycle (~1 second at 10 Hz), then self-corrects

### Circular Buffer Management

**Buffer Layout**:
```
buffer[0..9]: Timestamp values
index: Next write position (0-9)
full: true when 10 samples collected
```

**Insert Logic**:
```cpp
buffer[index] = timestamp;
index = (index + 1) % 10;
if (index == 0) full = true;  // Wrapped around
```

**Read Logic** (for frequency calculation):
- If `full == false`: Use first `index` elements (count = index)
- If `full == true`: Use all 10 elements (count = 10)

## Usage Example

```cpp
// In SourceRegistry::recordUpdate()
MessageSource* source = findSourceMutable(sourceId);

// Add new timestamp
uint32_t now = millis();
FrequencyCalculator::addTimestamp(source->timestampBuffer, source->bufferIndex,
                                  source->bufferFull, now);

// Calculate frequency if buffer full
if (source->bufferFull) {
    source->frequency = FrequencyCalculator::calculate(source->timestampBuffer, 10);
} else {
    source->frequency = 0.0;  // Not enough samples yet
}
```

## Accuracy Requirements

**Target**: ±10% of actual frequency (SC-002)

**Test Cases**:
- 10 Hz source (100ms intervals): Expected 9.0-11.0 Hz
- 1 Hz source (1000ms intervals): Expected 0.9-1.1 Hz
- Variable frequency (80-120ms): Expected 8.3-12.5 Hz (rolling average smooths)

**Factors Affecting Accuracy**:
- Buffer size (10 samples): Larger buffer = smoother average, slower response to changes
- Update jitter: NMEA sources may have ±20ms jitter, averaged out over 10 samples
- ReactESP timing: Handler may introduce <5ms latency, negligible

## Performance

**`calculate()`**:
- Time complexity: O(1) (2 array accesses, 3 arithmetic ops)
- Memory: 0 bytes (stateless function)
- CPU: <1 µs on ESP32 @ 240 MHz

**`addTimestamp()`**:
- Time complexity: O(1)
- Memory: 0 bytes
- CPU: <1 µs on ESP32 @ 240 MHz

## Testing Strategy

**Unit Tests** (`test/test_source_stats_units/FrequencyCalculatorTest.cpp`):

```cpp
void test_calculate_10hz_source() {
    uint32_t buffer[10] = {1000, 1100, 1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900};
    double freq = FrequencyCalculator::calculate(buffer, 10);
    assert(fabs(freq - 10.0) < 0.1);  // ±10% tolerance
}

void test_calculate_1hz_source() {
    uint32_t buffer[10] = {1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000};
    double freq = FrequencyCalculator::calculate(buffer, 10);
    assert(fabs(freq - 1.0) < 0.1);
}

void test_calculate_insufficient_samples() {
    uint32_t buffer[10] = {1000};
    double freq = FrequencyCalculator::calculate(buffer, 1);
    assert(freq == 0.0);
}

void test_addTimestamp_wraps_at_10() {
    uint32_t buffer[10] = {0};
    uint8_t index = 0;
    bool full = false;

    for (int i = 0; i < 10; i++) {
        FrequencyCalculator::addTimestamp(buffer, index, full, 1000 + i * 100);
    }

    assert(index == 0);  // Wrapped back to 0
    assert(full == true);  // Buffer filled
}
```

**Integration Tests** (`test/test_source_stats_integration/`):
- Mock NMEA source updating at 10 Hz
- Collect 10 samples via `recordUpdate()`
- Verify calculated frequency within 9.0-11.0 Hz

## Dependencies

**Required**:
- `<stdint.h>`: uint32_t, uint8_t
- `<math.h>`: fabs() (for testing only)

**No Dependencies On**:
- Arduino core (can be tested in native environment)
- BoatData structures
- NMEA libraries

## Invariants

1. **Frequency ≥ 0**: `calculate()` never returns negative values
2. **Buffer Size**: `count ≤ 10` always enforced
3. **Index Range**: `index` always in [0, 9] after `addTimestamp()`
4. **Full Flag**: `full == true` implies ≥10 samples have been added

## Error Handling

**No Errors Possible**:
- Stateless functions with validated inputs
- Guard clauses return safe default (0.0)
- No dynamic memory allocation
- No I/O operations

**Defensive Checks**:
- `if (count < 2) return 0.0;`
- `if (avgInterval == 0) return 0.0;`

## Future Enhancements

**Considered but Deferred**:
- Weighted average (recent samples weighted higher)
- Outlier rejection (discard samples >2σ from mean)
- Adaptive buffer size (10 samples for high-freq, 3 for low-freq)
- Rolling median instead of mean (more robust to jitter)

**Rationale for Simple Average**:
- Sufficient accuracy for diagnostic purposes
- Minimal CPU overhead
- Predictable behavior
- Easy to test and validate
