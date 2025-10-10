# Research: WebSocket Loop Frequency Logging

**Feature**: WebSocket Loop Frequency Logging (R007)
**Date**: 2025-10-10
**Status**: Complete

## Objective

Investigate existing WebSocket logging infrastructure and identify integration points for adding loop frequency logging to the existing 5-second display update cycle.

---

## Research Questions

### Q1: Does WebSocketLogger utility exist? What is its API?

**Answer**: ✅ YES - `src/utils/WebSocketLogger.h` exists and is fully functional.

**API Documentation**:
```cpp
class WebSocketLogger {
public:
    // Initialize with AsyncWebServer
    bool begin(AsyncWebServer* server, const char* path = "/logs");

    // Primary logging method
    void broadcastLog(LogLevel level, const char* component,
                      const char* event, const String& data = "");

    // Client information
    uint32_t getClientCount() const;
    bool hasClients() const;
    uint32_t getMessageCount() const;
};
```

**Key Method**: `broadcastLog(LogLevel, component, event, data)`
- **LogLevel**: enum class (DEBUG, INFO, WARN, ERROR, FATAL)
- **component**: String identifier (e.g., "Performance")
- **event**: Event name (e.g., "LOOP_FREQUENCY")
- **data**: Optional JSON string

**LogLevel Enum** (`src/utils/LogEnums.h`):
```cpp
enum class LogLevel {
    DEBUG = 0,   // ← Use for normal frequency
    INFO = 1,
    WARN = 2,    // ← Use for abnormal frequency
    ERROR = 3,
    FATAL = 4
};
```

---

### Q2: How is WebSocketLogger currently used in the codebase?

**Answer**: WebSocketLogger is instantiated as a global `logger` variable and used throughout main.cpp for various logging events.

**Current Usage Pattern** (from codebase):
```cpp
// Typical usage:
logger.broadcastLog(LogLevel::INFO, F("Component"), F("EVENT_NAME"),
                    String("{\"key\":\"value\"}"));
```

**Examples Found**:
1. WiFi connection events
2. Config file operations
3. System initialization events
4. Reboot scheduling

**Key Observations**:
- F() macro used for string literals (component and event names)
- JSON data passed as String (heap allocated, but temporary)
- Logger handles broadcasting to all connected WebSocket clients
- Silent failure if no clients connected (graceful degradation)

---

### Q3: Where is the 5-second OLED display event loop defined?

**Answer**: `src/main.cpp` lines 427-431

**Code Location**:
```cpp
// src/main.cpp:427-431
app.onRepeat(DISPLAY_STATUS_INTERVAL_MS, []() {
    if (displayManager != nullptr) {
        displayManager->renderStatusPage();
    }
});
```

**Constants** (from config or defines):
- `DISPLAY_STATUS_INTERVAL_MS`: 5000 (5 seconds)
- `DISPLAY_ANIMATION_INTERVAL_MS`: 1000 (1 second, separate loop)

**ReactESP Pattern**:
- `app.onRepeat(interval, callback)` - Non-blocking timer
- Callback is lambda function `[]() { ... }`
- Executes every `interval` milliseconds
- No need for manual millis() tracking

**Integration Point**: Add WebSocket log call INSIDE this lambda callback to synchronize with display update (NFR-012).

---

### Q4: How to access loop frequency value in event loop callback?

**Answer**: Use `systemMetrics->getLoopFrequency()` from global `systemMetrics` pointer.

**Data Access Path**:
```
Global variable: systemMetrics (ESP32SystemMetrics*)
    ↓
Method: systemMetrics->getLoopFrequency()
    ↓
Returns: uint32_t (frequency in Hz)
```

**Existing Pattern** (from R006 - Loop Frequency Display):
```cpp
// Global instances (already declared in main.cpp)
extern ESP32SystemMetrics* systemMetrics;
extern WebSocketLogger logger;

// Access pattern in event loop
uint32_t frequency = systemMetrics->getLoopFrequency();
```

**Null Safety**: Check `systemMetrics != nullptr` before accessing (already pattern in display loop).

**Return Values**:
- `0`: No measurement yet (first 5 seconds after boot)
- `10-2000`: Normal frequency range (typical: 100-500 Hz)
- `< 10` or `> 2000`: Abnormal frequency (warning condition)

---

### Q5: What JSON formatting utilities exist?

**Answer**: No dedicated JSON library used - manual String concatenation with proper escaping.

**Current Approach** (from existing code):
```cpp
// Simple JSON with String concatenation
String data = String("{\"key\":\"") + value + "\"}";

// For numeric values (no quotes needed)
String data = String("{\"frequency\":") + frequency + "}";
```

**Advantages**:
- Zero library overhead
- Minimal heap allocation (temporary String)
- Simple for small JSON payloads

**Escaping Rules**:
- Numeric values: No quotes (e.g., `"frequency": 212`)
- String values: Use quotes and escape special chars
- Backslashes: Use `\\` for literal backslash
- Quotes: Use `\"` for literal quote

**For This Feature**:
- Single numeric field: `{"frequency": <value>}`
- No escaping needed (frequency is uint32_t)
- Example: `String("{\"frequency\":") + 212 + "}"`
- Result: `"{"frequency":212}"`

---

## Integration Point Summary

### Exact Modification Location

**File**: `src/main.cpp`
**Lines**: 427-431 (inside DISPLAY_STATUS_INTERVAL_MS event loop)

**Before** (existing code):
```cpp
app.onRepeat(DISPLAY_STATUS_INTERVAL_MS, []() {
    if (displayManager != nullptr) {
        displayManager->renderStatusPage();
    }
});
```

**After** (with WebSocket logging):
```cpp
app.onRepeat(DISPLAY_STATUS_INTERVAL_MS, []() {
    if (displayManager != nullptr) {
        displayManager->renderStatusPage();
    }

    // R007: WebSocket loop frequency logging
    if (systemMetrics != nullptr) {
        uint32_t frequency = systemMetrics->getLoopFrequency();
        LogLevel level = (frequency > 0 && frequency >= 10 && frequency <= 2000)
                         ? LogLevel::DEBUG : LogLevel::WARN;
        String data = String("{\"frequency\":") + frequency + "}";
        logger.broadcastLog(level, F("Performance"), F("LOOP_FREQUENCY"), data);
    }
});
```

**Code Size Estimate**:
- Lambda function: +8 lines
- Compiled code: ~500 bytes flash
- Stack usage: ~20 bytes (temporary variables)
- Heap usage: ~30 bytes (String data, temporary, freed after broadcast)

---

## Dependencies Verified

### Existing Infrastructure (No New Dependencies)

1. **WebSocketLogger** ✅
   - Location: `src/utils/WebSocketLogger.h/cpp`
   - Status: Fully implemented and operational
   - API: `broadcastLog(LogLevel, component, event, data)`

2. **LogLevel Enum** ✅
   - Location: `src/utils/LogEnums.h`
   - Values: DEBUG, INFO, WARN, ERROR, FATAL
   - Status: Complete

3. **ESP32SystemMetrics** ✅
   - Location: `src/hal/implementations/ESP32SystemMetrics.h/cpp`
   - Method: `getLoopFrequency()` (implemented in R006)
   - Status: Ready for use

4. **ReactESP Event Loop** ✅
   - Library: ReactESP (already included)
   - Usage: `app.onRepeat(interval, callback)`
   - Status: Active, 5-second loop exists

5. **LoopPerformanceMonitor** ✅
   - Location: `src/utils/LoopPerformanceMonitor.h/cpp`
   - Status: Implemented in R006, owned by ESP32SystemMetrics
   - Measurement window: 5 seconds (synchronized with display)

---

## Technical Constraints

### Performance Constraints (NFR-010, NFR-011, NFR-012)

**NFR-010: Log emission overhead < 1ms**
- WebSocket broadcast: Typically ~100-200 µs for small JSON
- String allocation: ~50 µs
- Total estimated: ~250 µs ✅ (well under 1ms limit)

**NFR-011: Message size < 200 bytes**
- JSON payload: `{"frequency":9999}` = 19 bytes
- WebSocketLogger adds metadata: timestamp, level, component, event
- Total estimated: ~150 bytes ✅ (well under 200 byte limit)

**NFR-012: Synchronized timing**
- Solution: Add log call INSIDE existing 5-second event loop
- No new timer needed
- Guaranteed synchronization ✅

### Memory Constraints

**Static Allocation**: None (no new global variables)
**Heap Allocation**: ~30 bytes temporary (String data), freed after broadcast
**Stack Usage**: ~20 bytes (local variables in lambda)
**Flash Impact**: ~500 bytes (estimated compiled code size)

---

## Existing Log Message Format

**WebSocketLogger Output Format** (from existing logs):
```json
{
  "timestamp": 12345678,
  "level": "DEBUG",
  "component": "Performance",
  "event": "LOOP_FREQUENCY",
  "data": {"frequency": 212}
}
```

**Fields**:
- `timestamp`: millis() value (uint32_t)
- `level`: Log level string (e.g., "DEBUG", "WARN")
- `component`: Component identifier (e.g., "Performance")
- `event`: Event name (e.g., "LOOP_FREQUENCY")
- `data`: User-provided JSON (parsed from String parameter)

**Note**: WebSocketLogger automatically constructs the outer JSON structure. The `data` parameter we provide (`{"frequency":212}`) gets embedded as the `data` field.

---

## Testing Strategy

### Unit Tests (Native Platform)

**Test Group**: `test_websocket_frequency_units/`

**Tests**:
1. JSON formatting: Verify `{"frequency":212}` format
2. Log level selection: DEBUG for normal, WARN for abnormal
3. Placeholder handling: 0 Hz handled correctly
4. Edge cases: Boundary values (10 Hz, 2000 Hz)

**Mocking**: No hardware needed, pure string/logic tests

### Integration Tests (Native Platform)

**Test Group**: `test_websocket_frequency_integration/`

**Tests**:
1. Log emission: Verify WebSocketLogger.broadcastLog() called
2. Timing: Verify 5-second interval (mock millis())
3. Metadata: Verify component="Performance", event="LOOP_FREQUENCY"
4. Graceful degradation: Verify system continues if WebSocket fails

**Mocking**:
- MockWebSocketLogger (capture broadcastLog calls)
- MockSystemMetrics (provide test frequency values)
- Mock millis() for timing tests

### Hardware Tests (ESP32 Required)

**Test Group**: `test_websocket_frequency_hardware/`

**Tests**:
1. Actual WebSocket emission on ESP32
2. Timing accuracy (±500ms tolerance)
3. Log content matches OLED display

**Requirements**: ESP32 device, WebSocket client (Python ws_logger.py)

---

## Risks and Mitigation

### Risk 1: String Heap Allocation

**Issue**: Temporary String allocation for JSON data
**Impact**: ~30 bytes heap per log (every 5 seconds)
**Mitigation**: String is freed immediately after broadcast, no accumulation

### Risk 2: WebSocket Failure

**Issue**: WebSocket server failure or no clients
**Impact**: broadcastLog() fails silently
**Mitigation**: Graceful degradation (FR-059) - system continues, OLED display unaffected

### Risk 3: Log Spam

**Issue**: Log message every 5 seconds could be noisy
**Impact**: Developer experience (log volume)
**Mitigation**: DEBUG level (can be filtered), only 12 messages/minute

---

## Alternatives Considered

### Alternative 1: Separate Event Loop (REJECTED)

**Approach**: Create new `app.onRepeat(5000, ...)` for logging only

**Pros**: Clean separation of concerns
**Cons**:
- Violates NFR-012 (must synchronize with display update)
- Two timers might drift over time
- Unnecessary complexity

**Decision**: REJECTED - Reuse existing display update loop

### Alternative 2: ArduinoJson Library (REJECTED)

**Approach**: Use ArduinoJson for JSON serialization

**Pros**: Proper JSON escaping, no manual concatenation
**Cons**:
- Adds library dependency
- Increases flash usage (~15KB)
- Overkill for single numeric field

**Decision**: REJECTED - Manual string concatenation sufficient

### Alternative 3: Log on Every Display Update (REJECTED)

**Approach**: Log both animation (1s) and status (5s) updates

**Pros**: More frequent updates
**Cons**:
- Violates FR-053 (MUST be 5-second interval)
- 5x more log volume
- No user value (frequency doesn't change every second)

**Decision**: REJECTED - Stick to 5-second interval

---

## Recommendations

### Implementation Approach

1. **Minimal Change**: Add 6-8 lines to existing event loop
2. **No New Files**: All logic in main.cpp lambda
3. **Reuse Everything**: WebSocketLogger, ReactESP, LoopPerformanceMonitor
4. **TDD**: Write tests BEFORE modifying main.cpp

### Code Quality

1. Use `F()` macro for string literals (component, event)
2. Check `systemMetrics != nullptr` before access
3. Use inline log level determination (ternary operator)
4. Keep lambda function simple and readable

### Testing Priority

1. **High**: Integration tests (log emission, timing)
2. **Medium**: Unit tests (JSON formatting)
3. **Low**: Hardware tests (validation only, not regression)

---

## Conclusion

**Feasibility**: ✅ **TRIVIAL** - All infrastructure exists, minimal change required

**Risk**: ✅ **LOW** - No new dependencies, graceful degradation, extensive test coverage

**Complexity**: ✅ **MINIMAL** - Single file modification, ~8 lines of code

**Readiness**: ✅ **READY FOR IMPLEMENTATION** - All research complete, integration point identified

---

**Research Version**: 1.0 | **Completed**: 2025-10-10
