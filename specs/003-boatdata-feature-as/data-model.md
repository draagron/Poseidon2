# Data Model: BoatData

**Feature**: BoatData - Centralized Marine Data Model
**Date**: 2025-10-06
**Status**: Design Complete

## Overview
This document defines the data structures for the BoatData centralized marine sensor repository. All structures use static allocation (constitutional requirement: Principle II). Units follow FR-041 through FR-044 (angles in radians, speeds in knots, coordinates in decimal degrees).

---

## Entity 1: BoatData

**Purpose**: Central repository for all marine sensor data, calculated parameters, calibration, and diagnostics.

**Scope**: Single global instance (static allocation), no dynamic memory.

**Access Pattern**:
- Write: NMEA/1-Wire message handlers (sensor updates)
- Read: Calculation engine, web API, display, logging
- Update frequency: Sensors (variable), Calculations (200ms cycle)

### Structure Definition

```cpp
/**
 * @brief Central boat data repository
 *
 * Statically allocated structure containing all sensor data,
 * derived parameters, calibration, and diagnostics.
 *
 * Units:
 * - Angles: radians (except where noted)
 * - Speeds: knots
 * - Coordinates: decimal degrees
 * - Time: milliseconds (millis())
 *
 * Thread safety: Single-threaded (ReactESP event loop)
 */
struct BoatData {
    // === GPS Data (FR-001) ===
    struct GPSData {
        double latitude;           // Decimal degrees, positive = North, range [-90, 90]
        double longitude;          // Decimal degrees, positive = East, range [-180, 180]
        double cog;                // Course over ground, radians, range [0, 2π], true
        double sog;                // Speed over ground, knots, range [0, 100]
        bool available;            // Data validity flag
        unsigned long lastUpdate;  // millis() timestamp of last update
    } gps;

    // === Compass Data (FR-001) ===
    struct CompassData {
        double trueHeading;        // Radians, range [0, 2π]
        double magneticHeading;    // Radians, range [0, 2π]
        double variation;          // Magnetic variation, radians, positive = East, negative = West
        bool available;
        unsigned long lastUpdate;
    } compass;

    // === Wind Vane Data (FR-001) - Raw Sensor ===
    struct WindData {
        double apparentWindAngle;  // AWA, radians, range [-π, π], positive = starboard, negative = port
        double apparentWindSpeed;  // AWS, knots, range [0, 100]
        bool available;
        unsigned long lastUpdate;
    } wind;

    // === Paddle Wheel Data (FR-001) - Raw Sensor ===
    struct SpeedData {
        double heelAngle;          // Radians, range [-π/2, π/2], positive = starboard, negative = port
        double measuredBoatSpeed;  // MSB, knots, from paddle wheel, range [0, 50]
        bool available;
        unsigned long lastUpdate;
    } speed;

    // === Rudder Data (FR-001) ===
    struct RudderData {
        double steeringAngle;      // Radians, range [-π/2, π/2], positive = starboard, negative = port
        bool available;
        unsigned long lastUpdate;
    } rudder;

    // === Calibration Parameters (FR-003) ===
    struct CalibrationData {
        double leewayCalibrationFactor;  // K factor, range (0, +∞), typical [0.1, 5.0]
        double windAngleOffset;          // Radians, range [-2π, 2π], masthead misalignment correction
        bool loaded;                     // True if loaded from flash, false if using defaults
    } calibration;

    // === Calculated Sailing Parameters (FR-002) ===
    struct DerivedData {
        // Corrected apparent wind angles
        double awaOffset;          // AWA corrected for masthead offset, radians, range [-π, π]
        double awaHeel;            // AWA corrected for heel, radians, range [-π, π]

        // Leeway and speed
        double leeway;             // Leeway angle, radians, range [-π/4, π/4] (limited to ±45°)
        double stw;                // Speed through water, knots, corrected for leeway

        // True wind
        double tws;                // True wind speed, knots, range [0, 100]
        double twa;                // True wind angle, radians, range [-π, π], relative to boat heading
        double wdir;               // Wind direction, radians, range [0, 2π], magnetic (true wind direction)

        // Performance
        double vmg;                // Velocity made good, knots, signed (negative = away from wind)

        // Current
        double soc;                // Speed of current, knots, range [0, 20]
        double doc;                // Direction of current, radians, range [0, 2π], magnetic (direction current flows TO)

        bool available;            // True if calculation completed successfully
        unsigned long lastUpdate;  // millis() timestamp of last calculation cycle
    } derived;

    // === Diagnostics (FR-004) - RAM only, resets on reboot ===
    struct DiagnosticData {
        unsigned long nmea0183MessageCount;  // Total NMEA 0183 messages received
        unsigned long nmea2000MessageCount;  // Total NMEA 2000 messages received
        unsigned long actisenseMessageCount; // Total Actisense messages received
        unsigned long calculationCount;      // Total calculation cycles completed
        unsigned long calculationOverruns;   // Count of cycles exceeding 200ms
        unsigned long lastCalculationDuration; // Duration of last calculation cycle (ms)
    } diagnostics;
};
```

### Validation Rules

| Field | Range Check | Rate-of-Change Limit |
|-------|-------------|----------------------|
| `gps.latitude` | [-90.0, 90.0] | 0.1°/sec |
| `gps.longitude` | [-180.0, 180.0] | 0.1°/sec |
| `gps.cog` | [0, 2π] | π rad/sec |
| `gps.sog` | [0, 100] | 10 knots/sec |
| `compass.trueHeading` | [0, 2π] | π rad/sec |
| `compass.magneticHeading` | [0, 2π] | π rad/sec |
| `compass.variation` | [-π, π] | (static) |
| `wind.apparentWindAngle` | [-π, π] | 2π rad/sec |
| `wind.apparentWindSpeed` | [0, 100] | 30 knots/sec |
| `speed.heelAngle` | [-π/2, π/2] | π/4 rad/sec |
| `speed.measuredBoatSpeed` | [0, 50] | 5 knots/sec |
| `rudder.steeringAngle` | [-π/2, π/2] | π/3 rad/sec |
| `calibration.leewayCalibrationFactor` | (0, +∞) | N/A |
| `calibration.windAngleOffset` | [-2π, 2π] | N/A |

### State Transitions

```
Initial State: available = false (all sensor groups)
                    ↓
Sensor Update → Validation (range + rate-of-change)
                    ↓
             Valid? ───Yes──→ Update field, available = true, lastUpdate = millis()
                    │
                   No
                    ↓
             Log rejection, retain last valid value
                    ↓
Stale Detection (lastUpdate > 5000ms ago)
                    ↓
             available = false (trigger source failover)
```

---

## Entity 2: SensorSource

**Purpose**: Track multiple data sources for a sensor type (GPS, compass) with automatic priority and failover.

**Scope**: Fixed-size arrays per sensor type (MAX_SOURCES = 5 per spec clarification).

### Structure Definition

```cpp
/**
 * @brief Sensor source metadata for multi-source prioritization
 *
 * Tracks multiple sources for the same sensor type (e.g., GPS-A, GPS-B).
 * Automatic priority based on update frequency, manual override supported.
 */
struct SensorSource {
    // Identification
    char sourceId[16];             // Human-readable ID (e.g., "GPS-NMEA2000", "GPS-NMEA0183")
    SensorType sensorType;         // Enum: GPS, COMPASS
    ProtocolType protocolType;     // Enum: NMEA0183, NMEA2000, ONEWIRE

    // Priority & Availability
    double updateFrequency;        // Measured update rate (Hz), used for automatic priority
    int priority;                  // Priority level (0 = highest), -1 = automatic (frequency-based)
    bool manualOverride;           // True if user manually set this as preferred source

    // Status
    bool active;                   // True if currently selected as primary source
    bool available;                // True if recent updates received (within 5 seconds)
    unsigned long lastUpdateTime;  // millis() timestamp of last data from this source
    unsigned long updateCount;     // Total updates received (for frequency calculation)

    // Statistics (for diagnostics)
    unsigned long rejectedCount;   // Count of invalid/outlier readings rejected
    double avgUpdateInterval;      // Average time between updates (ms)
};

/**
 * @brief Sensor type enumeration
 */
enum class SensorType : uint8_t {
    GPS = 0,
    COMPASS = 1,
    // Future: WIND, SPEED (if multi-source support needed)
};

/**
 * @brief Protocol type enumeration
 */
enum class ProtocolType : uint8_t {
    NMEA0183 = 0,
    NMEA2000 = 1,
    ONEWIRE = 2,
    UNKNOWN = 255
};

/**
 * @brief Source management arrays (static allocation)
 */
#define MAX_GPS_SOURCES 5
#define MAX_COMPASS_SOURCES 5

struct SourceManager {
    SensorSource gpsSources[MAX_GPS_SOURCES];
    int gpsSourceCount;                       // Active count (0-5)
    int activeGpsSourceIndex;                 // Index of current primary GPS source (-1 = none)

    SensorSource compassSources[MAX_COMPASS_SOURCES];
    int compassSourceCount;
    int activeCompassSourceIndex;

    unsigned long lastPriorityUpdate;         // millis() timestamp of last priority recalculation
};
```

### Priority Algorithm

```
1. Manual override check:
   IF any source has manualOverride = true:
       Set that source as active (highest priority)
       RETURN

2. Automatic priority (frequency-based):
   FOR each available source:
       Calculate updateFrequency = updateCount / (millis() - firstUpdateTime)

   Sort sources by updateFrequency (descending)
   Set highest frequency source as active

3. Failover on stale detection:
   IF active source lastUpdateTime > 5000ms:
       Mark source.available = false
       Select next highest priority available source
       IF no sources available:
           Mark sensor data.available = false (degraded mode)
```

### State Transitions

```
Source Registration:
    New source → Add to array → available = false

First Update:
    available = true → Trigger priority recalculation

Active Selection:
    Priority calculation → Set active = true for highest priority
                        → Set active = false for others

Stale Detection:
    lastUpdateTime > 5000ms → available = false → Trigger failover

Manual Override:
    User sets source → manualOverride = true → Force active
                    → Disable automatic priority until reboot (FR-013)
```

---

## Entity 3: CalibrationParameters

**Purpose**: Store user-configurable calibration values with persistence.

**Scope**: Single instance, persisted to LittleFS flash filesystem.

### Structure Definition

```cpp
/**
 * @brief Calibration parameters with persistence
 *
 * Stored in `/calibration.json` on LittleFS.
 * Default values used if file missing or invalid.
 */
struct CalibrationParameters {
    // === Parameters ===
    double leewayCalibrationFactor;  // K factor in leeway formula, range (0, +∞)
    double windAngleOffset;          // Masthead misalignment correction, radians, range [-2π, 2π]

    // === Metadata ===
    unsigned long version;           // Config format version (1)
    unsigned long lastModified;      // Unix timestamp of last update
    bool valid;                      // True if loaded from flash successfully
};

/**
 * @brief Default calibration values (used if file missing)
 */
#define DEFAULT_LEEWAY_K_FACTOR 1.0
#define DEFAULT_WIND_ANGLE_OFFSET 0.0
```

### Persistence Format

**File**: `/calibration.json` on LittleFS

**JSON Schema**:
```json
{
    "version": 1,
    "leewayKFactor": 0.65,
    "windAngleOffset": 0.087,
    "lastModified": 1696608000
}
```

**Validation Rules**:
- `leewayKFactor` > 0 (reject if ≤ 0)
- `windAngleOffset` in range [-2π, 2π] (reject if outside)
- `version` == 1 (reject if mismatch for future compatibility)

### Atomic Update Protocol

To prevent calculation errors during calibration updates (clarification decision: atomic application):

```
1. Web API receives calibration update request
2. Validate new values (range checks)
3. Pause calculation cycle (set flag)
4. Update CalibrationParameters in memory
5. Write to flash (async, non-blocking)
6. Resume calculation cycle
7. Next cycle uses new values
```

**Implementation**: Use ReactESP `app.onDelay(0, applyCalibration)` to defer update until after current calculation cycle.

---

## Data Flow Diagram

```
┌─────────────────┐
│ NMEA0183 Handler│──updateGPS()──┐
└─────────────────┘               │
                                  ▼
┌─────────────────┐         ┌──────────┐
│ NMEA2000 Handler│──────→  │ BoatData │  (Static global)
└─────────────────┘         └──────────┘
                                  │
┌─────────────────┐               │ read()
│  1-Wire Handler │──────────────┘
└─────────────────┘               ▼
                            ┌────────────────┐
                            │CalculationEngine│ (200ms cycle)
                            └────────────────┘
                                  │
                                  │ write derived parameters
                                  ▼
                            ┌──────────┐
                            │ BoatData │
                            └──────────┘
                                  │
         ┌────────────────────────┼────────────────────────┐
         │                        │                        │
         ▼                        ▼                        ▼
    ┌────────┐            ┌──────────┐            ┌──────────┐
    │Display │            │  Logger  │            │  Web API │
    └────────┘            └──────────┘            └──────────┘
```

---

## Memory Footprint Estimation

```
BoatData structure:
- GPS Data: 5 doubles + 1 bool + 1 ulong = 58 bytes
- Compass Data: 3 doubles + 1 bool + 1 ulong = 37 bytes
- Wind Data: 2 doubles + 1 bool + 1 ulong = 29 bytes
- Speed Data: 2 doubles + 1 bool + 1 ulong = 29 bytes
- Rudder Data: 1 double + 1 bool + 1 ulong = 17 bytes
- Calibration: 2 doubles + 1 bool = 17 bytes
- Derived: 10 doubles + 1 bool + 1 ulong = 93 bytes
- Diagnostics: 6 ulongs = 24 bytes
TOTAL: ~304 bytes

SensorSource structure: ~100 bytes per source
SourceManager: 10 sources * 100 + metadata = ~1050 bytes

CalibrationParameters: ~32 bytes

TOTAL STATIC ALLOCATION: ~1400 bytes (negligible for ESP32 with 320KB RAM)
```

**Constitutional Compliance**: All structures use static allocation (Principle II), well within ESP32 8KB stack limit per task.

---

## Relationships Summary

```
BoatData (1) ─┐
              ├─── Updated by ──→ NMEA Handlers (N)
              ├─── Updated by ──→ 1-Wire Handlers (N)
              ├─── Read by ────→ CalculationEngine (1)
              ├─── Read by ────→ Display (1)
              ├─── Read by ────→ Logger (1)
              └─── Read by ────→ Web API (1)

SourceManager (1) ─┐
                   ├─── Manages ──→ SensorSource (N, max 5 per type)
                   └─── Selects ──→ Active Source (1 per sensor type)

CalibrationParameters (1) ─┐
                            ├─── Persisted to ──→ LittleFS (/calibration.json)
                            ├─── Read by ──────→ CalculationEngine (1)
                            └─── Updated by ───→ CalibrationWebServer (1)
```

---

**Data Model Status**: ✅ Complete
**Next Artifact**: contracts/ (C++ interface definitions)
