# Data Model: Simple WebUI for BoatData Streaming

**Feature**: 011-simple-webui-as
**Date**: 2025-10-13
**Status**: Complete

## Overview

This document defines the complete data model for WebSocket-based BoatData streaming, including the JSON message schema, field mappings from C++ structures to JSON, unit conversions, and data flow architecture.

## Data Flow Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           ESP32 Backend (C++)                           │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  BoatData* (Central Repository)                                        │
│  ├── getGPSData() → GPSData struct                                     │
│  ├── getCompassData() → CompassData struct                             │
│  ├── getWindData() → WindData struct                                   │
│  ├── getDSTData() → DSTData struct                                     │
│  ├── getRudderData() → RudderData struct                               │
│  ├── getEngineData() → EngineData struct                               │
│  ├── getSaildriveData() → SaildriveData struct                         │
│  ├── getBatteryData() → BatteryData struct                             │
│  └── getShorePowerData() → ShorePowerData struct                       │
│                                                                         │
│                            ↓                                            │
│                                                                         │
│  BoatDataSerializer::toJSON(boatData)                                  │
│  ├── Create StaticJsonDocument<2048>                                   │
│  ├── Add timestamp (millis())                                          │
│  ├── Serialize each sensor group to nested JSON object                 │
│  └── Convert to String (~1500-1800 bytes)                              │
│                                                                         │
│                            ↓                                            │
│                                                                         │
│  ReactESP Timer (1 Hz / 1000ms interval)                               │
│  ├── Check wsBoatData.count() > 0 (clients connected?)                 │
│  ├── Call BoatDataSerializer::toJSON()                                 │
│  └── wsBoatData.textAll(jsonString) - broadcast to all                 │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
                                    ↓
                                    │ WebSocket TCP/IP
                                    ↓
┌─────────────────────────────────────────────────────────────────────────┐
│                        Browser Client (JavaScript)                      │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  WebSocket Connection (ws://192.168.1.100/boatdata)                    │
│  ├── onopen: Set connection status indicator                           │
│  ├── onmessage: Receive JSON string (~1500-1800 bytes)                 │
│  ├── onerror: Log error, show disconnected status                      │
│  └── onclose: Attempt reconnect after 5 seconds                        │
│                                                                         │
│                            ↓                                            │
│                                                                         │
│  JSON.parse(message) → JavaScript Object                               │
│  ├── data.timestamp → Update "Last Updated" display                    │
│  ├── data.gps → Update GPS section DOM elements                        │
│  ├── data.compass → Update Compass section DOM elements                │
│  ├── data.wind → Update Wind section DOM elements                      │
│  ├── ... (etc for all sensor groups)                                   │
│  └── Check available flags → Gray out unavailable sensors              │
│                                                                         │
│                            ↓                                            │
│                                                                         │
│  DOM Updates (getElementById / innerHTML)                              │
│  ├── Convert radians → degrees for display                             │
│  ├── Format numbers (latitude: 6 decimals, heading: 1 decimal)         │
│  ├── Apply units (°, knots, meters, RPM, etc.)                         │
│  └── Update visual indicators (green/red, N/A text)                    │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

## JSON Message Schema

### Complete Message Structure

```json
{
  "timestamp": 1697123456789,

  "gps": {
    "latitude": 37.774929,
    "longitude": -122.419418,
    "cog": 1.5708,
    "sog": 5.5,
    "variation": 0.2618,
    "available": true,
    "lastUpdate": 1697123456789
  },

  "compass": {
    "trueHeading": 1.5708,
    "magneticHeading": 1.5708,
    "rateOfTurn": 0.1,
    "heelAngle": 0.0524,
    "pitchAngle": 0.0175,
    "heave": 0.15,
    "available": true,
    "lastUpdate": 1697123456789
  },

  "wind": {
    "apparentWindAngle": 0.7854,
    "apparentWindSpeed": 12.5,
    "available": true,
    "lastUpdate": 1697123456789
  },

  "dst": {
    "depth": 15.5,
    "measuredBoatSpeed": 2.83,
    "seaTemperature": 18.5,
    "available": true,
    "lastUpdate": 1697123456789
  },

  "rudder": {
    "steeringAngle": 0.1745,
    "available": true,
    "lastUpdate": 1697123456789
  },

  "engine": {
    "engineRev": 1500.0,
    "oilTemperature": 85.0,
    "alternatorVoltage": 14.2,
    "available": true,
    "lastUpdate": 1697123456789
  },

  "saildrive": {
    "saildriveEngaged": true,
    "available": true,
    "lastUpdate": 1697123456789
  },

  "battery": {
    "voltageA": 12.8,
    "amperageA": 5.2,
    "stateOfChargeA": 85.5,
    "shoreChargerOnA": false,
    "engineChargerOnA": true,
    "voltageB": 12.6,
    "amperageB": 0.5,
    "stateOfChargeB": 78.3,
    "shoreChargerOnB": false,
    "engineChargerOnB": false,
    "available": true,
    "lastUpdate": 1697123456789
  },

  "shorePower": {
    "shorePowerOn": false,
    "power": 0.0,
    "available": true,
    "lastUpdate": 1697123456789
  }
}
```

### Message Size Analysis

**Estimated JSON Size**:
- GPS: ~180 bytes (7 fields + metadata)
- Compass: ~200 bytes (8 fields + metadata)
- Wind: ~110 bytes (4 fields + metadata)
- DST: ~140 bytes (5 fields + metadata)
- Rudder: ~90 bytes (3 fields + metadata)
- Engine: ~140 bytes (5 fields + metadata)
- Saildrive: ~90 bytes (3 fields + metadata)
- Battery: ~320 bytes (12 fields + metadata)
- ShorePower: ~100 bytes (4 fields + metadata)
- Root timestamp + structure: ~50 bytes

**Total**: ~1420 bytes (uncompressed, with whitespace ~1600 bytes)
**Buffer Size**: 2048 bytes (29% margin for future expansion)

## Field Mapping: C++ to JSON

### GPS Data (src/types/BoatDataTypes.h lines 69-77)

| C++ Field | C++ Type | C++ Unit | JSON Field | JSON Type | JSON Unit | Notes |
|-----------|----------|----------|------------|-----------|-----------|-------|
| `latitude` | double | decimal degrees | `latitude` | number | decimal degrees | [-90, 90] |
| `longitude` | double | decimal degrees | `longitude` | number | decimal degrees | [-180, 180] |
| `cog` | double | radians | `cog` | number | radians | [0, 2π] |
| `sog` | double | knots | `sog` | number | knots | [0, 100] |
| `variation` | double | radians | `variation` | number | radians | Magnetic variation |
| `available` | bool | - | `available` | boolean | - | Data validity flag |
| `lastUpdate` | unsigned long | milliseconds | `lastUpdate` | number | milliseconds | millis() timestamp |

**JSON Serialization Code**:
```cpp
JsonObject gps = doc.createNestedObject("gps");
GPSData gpsData = boatData->getGPSData();
gps["latitude"] = gpsData.latitude;
gps["longitude"] = gpsData.longitude;
gps["cog"] = gpsData.cog;
gps["sog"] = gpsData.sog;
gps["variation"] = gpsData.variation;
gps["available"] = gpsData.available;
gps["lastUpdate"] = gpsData.lastUpdate;
```

### Compass Data (src/types/BoatDataTypes.h lines 92-101)

| C++ Field | C++ Type | C++ Unit | JSON Field | JSON Type | JSON Unit | Notes |
|-----------|----------|----------|------------|-----------|-----------|-------|
| `trueHeading` | double | radians | `trueHeading` | number | radians | [0, 2π] |
| `magneticHeading` | double | radians | `magneticHeading` | number | radians | [0, 2π] |
| `rateOfTurn` | double | rad/sec | `rateOfTurn` | number | rad/sec | +starboard |
| `heelAngle` | double | radians | `heelAngle` | number | radians | +starboard |
| `pitchAngle` | double | radians | `pitchAngle` | number | radians | +bow up |
| `heave` | double | meters | `heave` | number | meters | +upward |
| `available` | bool | - | `available` | boolean | - | Data validity |
| `lastUpdate` | unsigned long | milliseconds | `lastUpdate` | number | milliseconds | Timestamp |

### Wind Data (src/types/BoatDataTypes.h lines 109-114)

| C++ Field | C++ Type | C++ Unit | JSON Field | JSON Type | JSON Unit | Notes |
|-----------|----------|----------|------------|-----------|-----------|-------|
| `apparentWindAngle` | double | radians | `apparentWindAngle` | number | radians | [-π, π] +starboard |
| `apparentWindSpeed` | double | knots | `apparentWindSpeed` | number | knots | [0, 100] |
| `available` | bool | - | `available` | boolean | - | Data validity |
| `lastUpdate` | unsigned long | milliseconds | `lastUpdate` | number | milliseconds | Timestamp |

### DST Data (src/types/BoatDataTypes.h lines 129-135)

| C++ Field | C++ Type | C++ Unit | JSON Field | JSON Type | JSON Unit | Notes |
|-----------|----------|----------|------------|-----------|-----------|-------|
| `depth` | double | meters | `depth` | number | meters | [0, 100] |
| `measuredBoatSpeed` | double | m/s | `measuredBoatSpeed` | number | m/s | [0, 25] |
| `seaTemperature` | double | Celsius | `seaTemperature` | number | Celsius | [-10, 50] |
| `available` | bool | - | `available` | boolean | - | Data validity |
| `lastUpdate` | unsigned long | milliseconds | `lastUpdate` | number | milliseconds | Timestamp |

### Rudder Data (src/types/BoatDataTypes.h lines 143-147)

| C++ Field | C++ Type | C++ Unit | JSON Field | JSON Type | JSON Unit | Notes |
|-----------|----------|----------|------------|-----------|-----------|-------|
| `steeringAngle` | double | radians | `steeringAngle` | number | radians | [-π/2, π/2] +starboard |
| `available` | bool | - | `available` | boolean | - | Data validity |
| `lastUpdate` | unsigned long | milliseconds | `lastUpdate` | number | milliseconds | Timestamp |

### Engine Data (src/types/BoatDataTypes.h lines 155-161)

| C++ Field | C++ Type | C++ Unit | JSON Field | JSON Type | JSON Unit | Notes |
|-----------|----------|----------|------------|-----------|-----------|-------|
| `engineRev` | double | RPM | `engineRev` | number | RPM | [0, 6000] |
| `oilTemperature` | double | Celsius | `oilTemperature` | number | Celsius | [-10, 150] |
| `alternatorVoltage` | double | volts | `alternatorVoltage` | number | volts | [0, 30] |
| `available` | bool | - | `available` | boolean | - | Data validity |
| `lastUpdate` | unsigned long | milliseconds | `lastUpdate` | number | milliseconds | Timestamp |

### Saildrive Data (src/types/BoatDataTypes.h lines 169-173)

| C++ Field | C++ Type | C++ Unit | JSON Field | JSON Type | JSON Unit | Notes |
|-----------|----------|----------|------------|-----------|-----------|-------|
| `saildriveEngaged` | bool | - | `saildriveEngaged` | boolean | - | true=deployed |
| `available` | bool | - | `available` | boolean | - | Data validity |
| `lastUpdate` | unsigned long | milliseconds | `lastUpdate` | number | milliseconds | Timestamp |

### Battery Data (src/types/BoatDataTypes.h lines 181-199)

| C++ Field | C++ Type | C++ Unit | JSON Field | JSON Type | JSON Unit | Notes |
|-----------|----------|----------|------------|-----------|-----------|-------|
| `voltageA` | double | volts | `voltageA` | number | volts | [0, 30] |
| `amperageA` | double | amperes | `amperageA` | number | amperes | [-200, 200] |
| `stateOfChargeA` | double | percent | `stateOfChargeA` | number | percent | [0, 100] |
| `shoreChargerOnA` | bool | - | `shoreChargerOnA` | boolean | - | Shore charging |
| `engineChargerOnA` | bool | - | `engineChargerOnA` | boolean | - | Alternator charging |
| `voltageB` | double | volts | `voltageB` | number | volts | [0, 30] |
| `amperageB` | double | amperes | `amperageB` | number | amperes | [-200, 200] |
| `stateOfChargeB` | double | percent | `stateOfChargeB` | number | percent | [0, 100] |
| `shoreChargerOnB` | bool | - | `shoreChargerOnB` | boolean | - | Shore charging |
| `engineChargerOnB` | bool | - | `engineChargerOnB` | boolean | - | Alternator charging |
| `available` | bool | - | `available` | boolean | - | Data validity |
| `lastUpdate` | unsigned long | milliseconds | `lastUpdate` | number | milliseconds | Timestamp |

### Shore Power Data (src/types/BoatDataTypes.h lines 207-213)

| C++ Field | C++ Type | C++ Unit | JSON Field | JSON Type | JSON Unit | Notes |
|-----------|----------|----------|------------|-----------|-----------|-------|
| `shorePowerOn` | bool | - | `shorePowerOn` | boolean | - | Shore power status |
| `power` | double | watts | `power` | number | watts | [0, 5000] |
| `available` | bool | - | `available` | boolean | - | Data validity |
| `lastUpdate` | unsigned long | milliseconds | `lastUpdate` | number | milliseconds | Timestamp |

## Unit Conversions

### Server-Side (C++ → JSON)

**NO CONVERSIONS** - Server sends raw values in native BoatData units.

**Rationale**:
- Keep server logic simple (minimal CPU overhead)
- Preserve data accuracy (no rounding errors)
- Enable flexible client-side formatting
- Support future internationalization (metric/imperial toggle)

### Client-Side (JSON → Display)

**JavaScript Conversion Functions**:

```javascript
// Radians to degrees
function radToDeg(rad) {
    return (rad * 180.0 / Math.PI).toFixed(1);  // 1 decimal place
}

// Radians to degrees (signed, -180 to +180)
function radToSignedDeg(rad) {
    let deg = rad * 180.0 / Math.PI;
    if (deg > 180) deg -= 360;
    if (deg < -180) deg += 360;
    return deg.toFixed(1);
}

// Meters/second to knots
function msToKnots(ms) {
    return (ms * 1.94384).toFixed(1);  // 1 decimal place
}

// Format GPS coordinates (6 decimal places)
function formatLatLon(value) {
    return value.toFixed(6);
}

// Format timestamp as "HH:MM:SS ago"
function formatTimestamp(timestamp) {
    const ageSeconds = Math.floor((Date.now() - timestamp) / 1000);
    if (ageSeconds < 60) return ageSeconds + "s ago";
    if (ageSeconds < 3600) return Math.floor(ageSeconds / 60) + "m ago";
    return Math.floor(ageSeconds / 3600) + "h ago";
}
```

**Display Format Table**:

| Field Type | JSON Value | Display Format | Example |
|------------|------------|----------------|---------|
| Heading (radians) | 1.5708 | "90.0°" | `radToDeg(1.5708) + "°"` |
| Wind angle (radians) | 0.7854 | "45.0°" | `radToSignedDeg(0.7854) + "°"` |
| Latitude | 37.774929 | "37.774929° N" | `formatLatLon(lat) + "° N"` |
| Speed (m/s) | 2.83 | "5.5 knots" | `msToKnots(2.83) + " knots"` |
| Speed (knots) | 5.5 | "5.5 knots" | `value.toFixed(1) + " knots"` |
| Temperature (°C) | 18.5 | "18.5°C" | `value.toFixed(1) + "°C"` |
| Voltage | 12.8 | "12.8V" | `value.toFixed(1) + "V"` |
| Timestamp | 1697123456789 | "5s ago" | `formatTimestamp(ts)` |

## Data Staleness Handling

### Server Behavior

- Server always broadcasts the latest BoatData snapshot (even if data is stale)
- `lastUpdate` timestamp indicates data freshness
- `available` flag indicates sensor validity

### Client Behavior

**Visual Indicators**:
1. **Fresh data** (< 2 seconds old): Normal display, green indicator
2. **Stale data** (2-5 seconds old): Yellow indicator
3. **Very stale data** (> 5 seconds old): Gray out values, red indicator
4. **Unavailable** (`available: false`): Display "N/A", gray background

**JavaScript Logic**:
```javascript
function updateSensorDisplay(sensorName, data) {
    const age = Date.now() - data.lastUpdate;
    const element = document.getElementById(sensorName);

    if (!data.available) {
        element.classList.add('unavailable');
        element.innerHTML = 'N/A';
        return;
    }

    if (age > 5000) {
        element.classList.add('stale');
    } else if (age > 2000) {
        element.classList.add('aging');
    } else {
        element.classList.remove('stale', 'aging');
    }

    // Update value...
}
```

## Error Handling

### Server-Side Errors

| Error Condition | Handling | WebSocket Log Level |
|----------------|----------|---------------------|
| BoatData pointer null | Skip broadcast | ERROR |
| JSON buffer overflow | Truncate, mark incomplete | WARN |
| ArduinoJson serialization failure | Skip broadcast, retry next cycle | WARN |
| No WebSocket clients | Skip serialization (optimization) | DEBUG (first occurrence only) |

### Client-Side Errors

| Error Condition | Handling | User Experience |
|----------------|----------|-----------------|
| WebSocket connection failure | Auto-reconnect after 5 seconds | Show "Connecting..." status |
| WebSocket disconnection | Auto-reconnect after 5 seconds | Show "Disconnected" status |
| Invalid JSON message | Log error, display last valid data | Console error, no UI change |
| Missing JSON fields | Display "N/A" for missing fields | Partial data displayed |

## Memory Footprint

### Backend (ESP32 C++)

| Component | Type | Size | Lifecycle |
|-----------|------|------|-----------|
| `StaticJsonDocument<2048>` | Stack | 2048 bytes | Transient (serialization only) |
| JSON String | Stack | ~1600 bytes | Transient (broadcast only) |
| AsyncWebSocket handler | Heap | ~100 bytes | Persistent |
| ReactESP timer | Heap | ~50 bytes | Persistent |
| **Total Runtime** | - | **~150 bytes** | **Persistent** |
| **Peak Transient** | - | **~3700 bytes** | **During serialization** |

### Frontend (Browser JavaScript)

| Component | Size | Notes |
|-----------|------|-------|
| HTML structure | ~5-8 KB | Static, loaded once |
| Inline CSS | ~2-3 KB | Static, loaded once |
| JavaScript code | ~5-8 KB | Static, loaded once |
| JSON message buffer | ~1.6 KB | Per message, garbage collected |
| **Total page size** | **~15-20 KB** | **Single HTTP request** |

## Performance Characteristics

### Serialization Performance

**Benchmark** (ESP32 @ 240 MHz):
- `createNestedObject()`: ~3 µs per object × 9 objects = ~27 µs
- Field assignment: ~0.5 µs per field × 70 fields = ~35 µs
- `serializeJson()`: ~3 ms for 1600-byte output
- **Total**: ~3.1 ms (6.2% of 50ms budget)

**Headroom**: 93.8% of budget available for future expansion

### Network Performance

**Broadcast Latency**:
- Serialization: ~3 ms
- ESPAsyncWebServer queue: <5 ms
- WiFi transmission (1.6 KB): <10 ms @ 20 Mbps
- **Total**: <20 ms server-to-client (requirement: <100 ms)

**Throughput** (5 concurrent clients):
- Payload: 1.6 KB × 5 clients = 8 KB per broadcast
- Frequency: 1 Hz
- Bandwidth: 8 KB/s = 64 kbps
- WiFi capacity: 20 Mbps typical
- **Utilization**: 0.3% of WiFi bandwidth

## Testing Validation Points

### JSON Schema Validation

**Test Cases**:
1. All 9 sensor groups present in JSON
2. Each group has correct field names
3. Numeric fields are JSON numbers (not strings)
4. Boolean fields are JSON booleans
5. Timestamp is positive integer
6. `available` flag is boolean in all groups
7. `lastUpdate` timestamp present in all groups

**Validation Tool**: JSON Schema validator (online or library)

### Data Integrity

**Test Cases**:
1. C++ value `latitude = 37.774929` → JSON `"latitude": 37.774929`
2. C++ value `available = true` → JSON `"available": true`
3. C++ value `available = false` → JSON `"available": false`
4. C++ value `heelAngle = 0.0` → JSON `"heelAngle": 0.0` (not omitted)
5. All fields present even when `available = false`

### Performance Validation

**Test Cases**:
1. Serialization completes in <50 ms (measure with `micros()`)
2. Broadcast to 5 clients completes in <100 ms
3. Memory usage stable over 1000 broadcasts (no leaks)
4. JSON output size <2048 bytes (buffer size)

## Future Extensions

### Potential Additions (Future Features)

1. **Calculated/Derived Data**: Add `derived` object for TWS, TWA, VMG, leeway
2. **Diagnostics**: Add `diagnostics` object for message counts, overruns
3. **Selective Updates**: Client subscribes to specific sensor groups only
4. **Binary Protocol**: Use MessagePack for smaller payloads (research needed)
5. **Compression**: gzip compression for WebSocket messages (research needed)

**Note**: Current design supports up to ~400 additional bytes before buffer resize needed.
