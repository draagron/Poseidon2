# Data Model: NMEA2000 PGN 127252 Heave Handler

**Feature**: PGN 127252 Heave Handler
**Date**: 2025-10-11
**Status**: Complete ✓

## Overview

This feature does NOT introduce new data structures. It reuses existing data structures from Enhanced BoatData v2.0.0 (R005) to store heave measurements captured from NMEA2000 PGN 127252 messages.

**Key Point**: The `CompassData.heave` field already exists. This feature adds the handler to populate it.

---

## Entities

### 1. PGN 127252 Message (Input)

**Description**: NMEA2000 Parameter Group Number 127252 carries heave (vertical displacement) data transmitted by marine motion sensors.

**Source**: NMEA2000 CAN bus

**Structure** (as parsed by NMEA2000 library):
```cpp
// Parsed by ParseN2kPGN127252(...)
unsigned char SID;              // Sequence ID (ties related PGNs together)
double Heave;                   // Vertical displacement, meters
double Delay;                   // Optional: Delay added by calculations, seconds
tN2kDelaySource DelaySource;    // Optional: Source of delay (enum)
```

**Field Details**:
| Field | Type | Range | Description |
|-------|------|-------|-------------|
| SID | `unsigned char` | 0-255 | Sequence identifier to correlate messages |
| Heave | `double` | ±∞ (unvalidated) | Vertical displacement in meters (signed) |
| Delay | `double` | ≥0 | Optional delay in seconds (rarely used) |
| DelaySource | `tN2kDelaySource` | Enum | Optional delay source (rarely used) |

**Sign Convention**:
- **Positive heave**: Vessel moving upward (above reference plane)
- **Negative heave**: Vessel moving downward (below reference plane)
- **Reference**: Smooth, wave-free water surface on earth

**Validation**:
- Heave values outside [-5.0, 5.0] meters are clamped with warning log
- N2kDoubleNA (not available) is handled gracefully (no update)

**Lifecycle**:
1. Transmitted by NMEA2000 sensor on CAN bus
2. Received by ESP32 NMEA2000 library
3. Parsed by `ParseN2kPGN127252()`
4. Validated by handler
5. Stored in `CompassData.heave`

---

### 2. CompassData Structure (Storage)

**Description**: Vessel motion data container that stores compass heading, motion sensors (rate of turn, heel, pitch), and heave.

**Location**: `src/types/BoatDataTypes.h`

**Structure**:
```cpp
/**
 * @brief Compass and vessel motion data
 *
 * Stores compass headings (true/magnetic), rate of turn, heel, pitch, and heave.
 * All fields updated independently by different PGN handlers.
 */
struct CompassData {
    double trueHeading;        ///< True heading, radians [0, 2π]
    double magneticHeading;    ///< Magnetic heading, radians [0, 2π]
    double rateOfTurn;         ///< Rate of turn, rad/s, positive = starboard (PGN 127251)
    double heelAngle;          ///< Heel angle, radians [-π/2, π/2], positive = starboard (PGN 127257)
    double pitchAngle;         ///< Pitch angle, radians [-π/6, π/6], positive = bow up (PGN 127257)
    double heave;              ///< Vertical displacement, meters [-5.0, 5.0], positive = upward (PGN 127252)
    bool available;            ///< true = at least one field has been updated
    unsigned long lastUpdate;  ///< millis() timestamp of most recent update
};
```

**Field Details**:
| Field | Type | Range | Units | Source PGN | Updated By |
|-------|------|-------|-------|------------|------------|
| trueHeading | `double` | [0, 2π] | radians | Various | Existing handlers |
| magneticHeading | `double` | [0, 2π] | radians | Various | Existing handlers |
| rateOfTurn | `double` | [-π, π] | rad/s | 127251 | HandleN2kPGN127251 |
| heelAngle | `double` | [-π/2, π/2] | radians | 127257 | HandleN2kPGN127257 |
| pitchAngle | `double` | [-π/6, π/6] | radians | 127257 | HandleN2kPGN127257 |
| **heave** | `double` | [-5.0, 5.0] | meters | **127252** | **HandleN2kPGN127252 (THIS FEATURE)** |
| available | `bool` | true/false | - | - | All handlers |
| lastUpdate | `unsigned long` | 0-2^32 | milliseconds | - | All handlers |

**Validation Rules** (for heave):
- **Valid Range**: [-5.0, 5.0] meters
- **Clamping**: Out-of-range values clamped to limits (not rejected)
- **Warning**: WARN log if value clamped
- **Sign**: Positive = upward motion, Negative = downward motion

**Access Methods**:
```cpp
// BoatData component provides getters/setters
CompassData getCompassData();                  // Retrieve current data
void setCompassData(const CompassData& data);  // Update all fields
```

**Lifecycle**:
1. **Initial State**: All fields zero, available=false
2. **First Update**: Handler sets heave field, available=true, lastUpdate=millis()
3. **Subsequent Updates**: Only heave, available, lastUpdate modified (other fields unchanged)
4. **Independent Updates**: Other handlers update other fields independently

**Memory Footprint**:
- Size: 8 doubles (8×8=64 bytes) + 1 bool (1 byte) + 1 unsigned long (4 bytes) = 69 bytes
- **No change**: Structure already exists, heave field already allocated

---

## Relationships

### PGN 127252 → CompassData

**Relationship**: PGN 127252 message provides data to populate `CompassData.heave` field

**Cardinality**: Many-to-One (many PGN 127252 messages update single CompassData structure)

**Flow**:
```
NMEA2000 Sensor
    ↓ (transmits PGN 127252 on CAN bus)
ESP32 NMEA2000 Library
    ↓ (receives message, calls handler)
HandleN2kPGN127252
    ↓ (parses, validates, stores)
CompassData.heave
    ↓ (stored in BoatData component)
Available to consumers
```

**Data Transformation**:
1. **Parse**: `ParseN2kPGN127252()` extracts heave from NMEA2000 message
2. **Validate**: `DataValidation::isValidHeave()` checks range
3. **Clamp**: `DataValidation::clampHeave()` if out-of-range
4. **Store**: `setCompassData()` updates BoatData structure

---

## Data Flow Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│ NMEA2000 Marine Sensor (e.g., Motion Reference Unit)           │
└────────────────────────┬────────────────────────────────────────┘
                         │ Transmits PGN 127252 on CAN bus
                         ↓
┌─────────────────────────────────────────────────────────────────┐
│ ESP32 NMEA2000 Library (ttlappalainen/NMEA2000)                │
│ - Receives CAN message                                          │
│ - Identifies PGN 127252                                         │
│ - Calls registered handler                                      │
└────────────────────────┬────────────────────────────────────────┘
                         │ Invokes handler
                         ↓
┌─────────────────────────────────────────────────────────────────┐
│ HandleN2kPGN127252 (This Feature)                              │
│ ┌─────────────────────────────────────────────────────────────┐ │
│ │ 1. Null pointer checks (boatData, logger)                   │ │
│ │ 2. ParseN2kPGN127252() → SID, heave, delay, delaySource    │ │
│ │ 3. Check N2kIsNA(heave) → Log DEBUG if unavailable         │ │
│ │ 4. isValidHeave(heave) → Check [-5.0, 5.0] range           │ │
│ │ 5. clampHeave(heave) → Clamp if out-of-range, log WARN     │ │
│ │ 6. getCompassData() → Retrieve current structure            │ │
│ │ 7. compass.heave = heave                                     │ │
│ │ 8. compass.available = true, compass.lastUpdate = millis()  │ │
│ │ 9. setCompassData(compass) → Store updated structure        │ │
│ │ 10. Log DEBUG with heave value                              │ │
│ │ 11. incrementNMEA2000Count()                                │ │
│ └─────────────────────────────────────────────────────────────┘ │
└────────────────────────┬────────────────────────────────────────┘
                         │ Updates
                         ↓
┌─────────────────────────────────────────────────────────────────┐
│ BoatData Component (src/components/BoatData.h)                 │
│ ┌─────────────────────────────────────────────────────────────┐ │
│ │ struct BoatDataStructure {                                  │ │
│ │     CompassData compass;  ← Updated here                    │ │
│ │     // ... other sensor data ...                            │ │
│ │ }                                                            │ │
│ └─────────────────────────────────────────────────────────────┘ │
└────────────────────────┬────────────────────────────────────────┘
                         │ Available to
                         ↓
┌─────────────────────────────────────────────────────────────────┐
│ Consumers (Future Features)                                     │
│ - WebSocket data streaming                                      │
│ - OLED display (motion status)                                  │
│ - Data logging / export                                         │
│ - Alarm/alert systems                                           │
└─────────────────────────────────────────────────────────────────┘
```

---

## Validation Rules

### Heave Value Validation

**Function**: `DataValidation::isValidHeave(double heave)`

**Rule**: Heave must be within [-5.0, 5.0] meters

**Rationale**: Typical marine sensor operating range for vertical displacement

**Actions**:
| Condition | Action | Log Level | Details |
|-----------|--------|-----------|---------|
| heave ≥ -5.0 AND heave ≤ 5.0 | Accept value | DEBUG | Normal operation |
| heave < -5.0 | Clamp to -5.0 | WARN | Out-of-range, clamped |
| heave > 5.0 | Clamp to 5.0 | WARN | Out-of-range, clamped |
| N2kIsNA(heave) | Skip update | DEBUG | Sensor not available |

**Example Log Messages**:
```json
// Valid heave
{"level":"DEBUG","component":"NMEA2000","event":"PGN127252_UPDATE","data":"{\"heave\":2.5,\"meters\":true}"}

// Out-of-range (clamped)
{"level":"WARN","component":"NMEA2000","event":"PGN127252_OUT_OF_RANGE","data":"{\"heave\":6.2,\"clamped\":5.0}"}

// Not available
{"level":"DEBUG","component":"NMEA2000","event":"PGN127252_NA","data":"{\"reason\":\"Heave not available\"}"}

// Parse failure
{"level":"ERROR","component":"NMEA2000","event":"PGN127252_PARSE_FAILED","data":"{\"reason\":\"Failed to parse PGN 127252\"}"}
```

---

## Storage Considerations

### Memory Footprint

**RAM Impact**: **0 bytes** (no new allocations)
- CompassData.heave field already allocated in Enhanced BoatData v2.0.0
- Handler uses stack-local variables (cleaned up after execution)
- No global variables added

**Flash Impact**: **~2KB** (handler code only)
- Handler function: ~1,200 bytes
- Function declaration + Doxygen comments: ~300 bytes
- Handler registration: ~100 bytes
- Total: ~1,600 bytes

**Stack Usage**: **~200 bytes** (during handler execution)
- Local variables:
  - `unsigned char SID` (1 byte)
  - `double heave` (8 bytes)
  - `double delay` (8 bytes)
  - `tN2kDelaySource delaySource` (4 bytes, enum)
  - `CompassData compass` (69 bytes, struct copy)
  - Function call overhead (~100 bytes)
- Total: ~200 bytes (well within 8KB task stack limit)

### Data Retention

**Persistence**: In-memory only (CompassData structure in RAM)
- Heave value lost on reboot (no flash storage)
- Consumers responsible for logging/export if persistence needed

**Update Frequency**: Event-driven (depends on sensor transmission rate)
- Typical: 1-10 Hz (motion sensors)
- Handler processes each message independently
- No inter-message dependencies (stateless)

---

## Implementation Notes

### No New Data Structures Required

This feature does NOT add new data structures because:
1. **CompassData.heave** already exists (added in Enhanced BoatData v2.0.0)
2. **Validation functions** already implemented (clampHeave, isValidHeave)
3. **WebSocket logging** already integrated
4. **BoatData component** already supports CompassData updates

### Reuse of Existing Infrastructure

**100% reuse** of:
- Data structures (CompassData)
- Validation utilities (DataValidation.h)
- Logging infrastructure (WebSocketLogger)
- Storage mechanism (BoatData component)
- Access methods (getCompassData, setCompassData)

**Only addition**: Handler function to populate existing field

### Alignment with Enhanced BoatData v2.0.0

This feature completes the heave integration started in Enhanced BoatData v2.0.0:
- **R005 added**: CompassData.heave field, validation functions, test infrastructure
- **R005 noted**: PGN 127257 (Attitude) does not provide heave (comment in handler)
- **This feature adds**: PGN 127252 handler to populate heave field

**Result**: Complete heave data flow from NMEA2000 sensor to BoatData storage

---

## Data Model Status

**Status**: ✅ COMPLETE

**Entities**: 2 (PGN 127252 Message, CompassData)

**New Structures**: 0 (reuses existing)

**Validation Rules**: 1 (heave range [-5.0, 5.0] meters)

**Memory Impact**: 0 bytes RAM, ~2KB flash

**Ready for**: Contract definition (Phase 1 continued)
