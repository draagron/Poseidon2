# Contract: BoatDataSerializer

**Component**: BoatDataSerializer
**Type**: Static utility class
**Purpose**: Convert BoatData C++ structures to JSON format for WebSocket streaming
**Location**: `src/components/BoatDataSerializer.{h,cpp}`

## Interface Contract

### Public Static Methods

#### `String toJSON(BoatData* boatData)`

**Purpose**: Serialize complete BoatData snapshot to JSON string

**Parameters**:
- `boatData` (BoatData*): Pointer to centralized BoatData repository

**Returns**:
- `String`: JSON-formatted string representing complete BoatData snapshot
- Empty string (`""`) on serialization failure

**Preconditions**:
- `boatData` must not be null
- BoatData structures must be initialized (pointers valid)

**Postconditions**:
- Returns valid JSON string conforming to data-model.md schema
- JSON size does not exceed 2048 bytes
- All 9 sensor groups included (gps, compass, wind, dst, rudder, engine, saildrive, battery, shorePower)
- Root-level `timestamp` field present (millis() value)

**Error Handling**:
- If `boatData == nullptr`: Log ERROR, return empty string
- If JSON buffer overflows: Log WARN, return truncated JSON with `"incomplete": true` flag
- If ArduinoJson serialization fails: Log WARN, return empty string

**Performance**:
- Execution time: <50 ms (measured with micros())
- Memory: Uses static 2048-byte buffer (no heap allocation)
- No blocking operations

**Example Usage**:
```cpp
BoatData* boatData = getBoatDataInstance();
String json = BoatDataSerializer::toJSON(boatData);

if (json.length() > 0) {
    wsBoatData.textAll(json);  // Broadcast to all WebSocket clients
} else {
    logger.broadcastLog(LogLevel::WARN, "BoatDataSerializer", "SERIALIZATION_FAILED",
        F("{\"reason\":\"Empty JSON output\"}"));
}
```

## JSON Output Format

### Root Structure

```json
{
  "timestamp": <millis()>,
  "gps": { ... },
  "compass": { ... },
  "wind": { ... },
  "dst": { ... },
  "rudder": { ... },
  "engine": { ... },
  "saildrive": { ... },
  "battery": { ... },
  "shorePower": { ... }
}
```

**Field Requirements**:
- All 9 sensor groups MUST be present (even if `available: false`)
- `timestamp` MUST be present at root level
- Each sensor group MUST include `available` and `lastUpdate` fields
- Field names MUST match data-model.md specification exactly
- Numeric values MUST be JSON numbers (not strings)
- Boolean values MUST be JSON booleans (not strings/numbers)

### Example Complete Output

See [data-model.md](../data-model.md) section "Complete Message Structure" for full example.

## Implementation Requirements

### Memory Management

**MUST USE**:
- `StaticJsonDocument<2048>` for JSON creation (stack allocation)
- No dynamic memory allocation (no `new`, `malloc`, etc.)
- No heap-based String operations inside serialization

**Buffer Overflow Handling**:
```cpp
StaticJsonDocument<2048> doc;

// ... populate JSON ...

if (doc.overflowed()) {
    logger.broadcastLog(LogLevel::WARN, "BoatDataSerializer", "BUFFER_OVERFLOW",
        String(F("{\"capacity\":2048,\"usage\":")) + doc.memoryUsage() + F("}"));
    doc["incomplete"] = true;  // Mark as incomplete
}

String output;
serializeJson(doc, output);
return output;
```

### ArduinoJson Usage Pattern

**Nested Object Creation**:
```cpp
// GPS group
JsonObject gps = doc.createNestedObject("gps");
GPSData gpsData = boatData->getGPSData();
gps["latitude"] = gpsData.latitude;
gps["longitude"] = gpsData.longitude;
gps["cog"] = gpsData.cog;
gps["sog"] = gpsData.sog;
gps["variation"] = gpsData.variation;
gps["available"] = gpsData.available;
gps["lastUpdate"] = gpsData.lastUpdate;

// Compass group
JsonObject compass = doc.createNestedObject("compass");
CompassData compassData = boatData->getCompassData();
compass["trueHeading"] = compassData.trueHeading;
// ... etc
```

### Performance Monitoring

**Timing Measurement**:
```cpp
unsigned long startMicros = micros();

// Serialization code here

unsigned long durationMicros = micros() - startMicros;

if (durationMicros > 50000) {  // 50 ms = 50000 µs
    logger.broadcastLog(LogLevel::WARN, "BoatDataSerializer", "SLOW_SERIALIZATION",
        String(F("{\"duration_ms\":")) + (durationMicros / 1000) + F("}"));
}
```

## Testing Contract

### Unit Tests (test/test_webui_units/test_json_format.cpp)

**Test Cases**:
1. **TC-SER-001**: Verify all 9 sensor groups present in output JSON
2. **TC-SER-002**: Verify root `timestamp` field present and is number
3. **TC-SER-003**: Verify each sensor group has `available` boolean field
4. **TC-SER-004**: Verify each sensor group has `lastUpdate` number field
5. **TC-SER-005**: Verify GPS group has 7 fields (latitude, longitude, cog, sog, variation, available, lastUpdate)
6. **TC-SER-006**: Verify compass group has 8 fields
7. **TC-SER-007**: Verify wind group has 4 fields
8. **TC-SER-008**: Verify dst group has 5 fields
9. **TC-SER-009**: Verify rudder group has 3 fields
10. **TC-SER-010**: Verify engine group has 5 fields
11. **TC-SER-011**: Verify saildrive group has 3 fields
12. **TC-SER-012**: Verify battery group has 12 fields
13. **TC-SER-013**: Verify shorePower group has 4 fields
14. **TC-SER-014**: Verify numeric fields are JSON numbers (not strings)
15. **TC-SER-015**: Verify boolean fields are JSON booleans (true/false, not "true"/"false")
16. **TC-SER-016**: Verify null BoatData pointer returns empty string
17. **TC-SER-017**: Verify output size does not exceed 2048 bytes

### Performance Tests (test/test_webui_units/test_serialization_performance.cpp)

**Test Cases**:
1. **TC-PERF-001**: Measure serialization time, assert <50 ms
2. **TC-PERF-002**: Verify no heap allocation during serialization (if tooling available)
3. **TC-PERF-003**: Verify JSON output size is reasonable (~1400-1800 bytes)
4. **TC-PERF-004**: Benchmark 1000 consecutive serializations, verify no memory leaks

### Integration Tests (test/test_webui_integration/test_json_data_flow.cpp)

**Test Cases**:
1. **TC-INT-001**: Verify GPS data values match BoatData values exactly
2. **TC-INT-002**: Verify compass data values match BoatData values exactly
3. **TC-INT-003**: Verify wind data values match BoatData values exactly
4. **TC-INT-004**: Verify dst data values match BoatData values exactly
5. **TC-INT-005**: Verify battery data values match BoatData values exactly
6. **TC-INT-006**: Verify `available: false` is serialized correctly
7. **TC-INT-007**: Verify `lastUpdate` timestamp is recent (within last second)

## Logging Contract

### WebSocket Logging Requirements

**Component Name**: "BoatDataSerializer"

**Event Types**:
- `SERIALIZATION_SUCCESS`: DEBUG level, logged on successful serialization (optional, may be too verbose)
- `SERIALIZATION_FAILED`: WARN level, logged when toJSON() returns empty string
- `BUFFER_OVERFLOW`: WARN level, logged when StaticJsonDocument overflows
- `SLOW_SERIALIZATION`: WARN level, logged when serialization takes >50 ms
- `NULL_BOATDATA`: ERROR level, logged when boatData pointer is null

**Log Format** (JSON):
```json
// SERIALIZATION_SUCCESS (optional)
{"component":"BoatDataSerializer","event":"SERIALIZATION_SUCCESS","data":{"size_bytes":1567}}

// SERIALIZATION_FAILED
{"component":"BoatDataSerializer","event":"SERIALIZATION_FAILED","data":{"reason":"Empty JSON output"}}

// BUFFER_OVERFLOW
{"component":"BoatDataSerializer","event":"BUFFER_OVERFLOW","data":{"capacity":2048,"usage":2100}}

// SLOW_SERIALIZATION
{"component":"BoatDataSerializer","event":"SLOW_SERIALIZATION","data":{"duration_ms":65}}

// NULL_BOATDATA
{"component":"BoatDataSerializer","event":"NULL_BOATDATA","data":{"reason":"BoatData pointer is null"}}
```

## Dependencies

### Required Headers

```cpp
#include <Arduino.h>
#include <ArduinoJson.h>
#include "../types/BoatDataTypes.h"
#include "BoatData.h"
#include "../utils/WebSocketLogger.h"
```

### Required Libraries

- ArduinoJson v6.21.0+ (already in platformio.ini)

## Constitutional Compliance

### Principle II: Resource-Aware Development

- ✅ Static memory allocation (StaticJsonDocument<2048>)
- ✅ No heap allocation
- ✅ Memory footprint documented (2048 bytes stack)
- ✅ Performance budget respected (<50 ms)

### Principle IV: Modular Component Design

- ✅ Single responsibility (JSON serialization only)
- ✅ Static class (no instance state)
- ✅ Pure function (no side effects except logging)
- ✅ Dependency injection (BoatData pointer, WebSocketLogger)

### Principle V: Network-Based Debugging

- ✅ WebSocket logging for errors
- ✅ JSON log format
- ✅ Sufficient detail for diagnosis

### Principle VI: Always-On Operation

- ✅ Non-blocking (<50 ms execution time)
- ✅ No delays or blocking I/O

### Principle VII: Fail-Safe Operation

- ✅ Null pointer checks
- ✅ Buffer overflow handling
- ✅ Returns empty string on failure (caller handles gracefully)
- ✅ System continues operation on serialization failure

## Version History

- **v1.0.0** (2025-10-13): Initial contract definition
