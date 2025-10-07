/**
 * @file BoatDataTypes.h
 * @brief Data structure definitions for BoatData centralized marine data model
 *
 * This file contains all type definitions for sensor data, derived parameters,
 * calibration, and diagnostics. All structures use static allocation.
 *
 * Units:
 * - Angles: radians (except where noted)
 * - Speeds: knots
 * - Coordinates: decimal degrees
 * - Time: milliseconds (millis())
 *
 * @see specs/003-boatdata-feature-as/data-model.md
 * @version 1.0.0
 * @date 2025-10-06
 */

#ifndef BOATDATA_TYPES_H
#define BOATDATA_TYPES_H

#include <Arduino.h>

// =============================================================================
// ENUMERATIONS
// =============================================================================

/**
 * @brief Sensor type enumeration for multi-source management
 */
enum class SensorType : uint8_t {
    GPS = 0,      ///< GPS sensors (latitude, longitude, COG, SOG)
    COMPASS = 1,  ///< Compass sensors (heading, variation)
    // Future: WIND, SPEED (if multi-source support needed)
};

/**
 * @brief Protocol type enumeration for source identification
 */
enum class ProtocolType : uint8_t {
    NMEA0183 = 0,  ///< NMEA 0183 (Serial)
    NMEA2000 = 1,  ///< NMEA 2000 (CAN bus)
    ONEWIRE = 2,   ///< 1-Wire sensors
    UNKNOWN = 255  ///< Unknown/uninitialized protocol
};

// =============================================================================
// SENSOR DATA STRUCTURES
// =============================================================================

/**
 * @brief GPS sensor data
 *
 * Raw GPS data from NMEA0183/NMEA2000 sources.
 * Units: decimal degrees, radians, knots
 */
struct GPSData {
    double latitude;           ///< Decimal degrees, positive = North, range [-90, 90]
    double longitude;          ///< Decimal degrees, positive = East, range [-180, 180]
    double cog;                ///< Course over ground, radians, range [0, 2π], true
    double sog;                ///< Speed over ground, knots, range [0, 100]
    bool available;            ///< Data validity flag
    unsigned long lastUpdate;  ///< millis() timestamp of last update
};

/**
 * @brief Compass sensor data
 *
 * Heading and magnetic variation data.
 * Units: radians
 */
struct CompassData {
    double trueHeading;        ///< Radians, range [0, 2π]
    double magneticHeading;    ///< Radians, range [0, 2π]
    double variation;          ///< Magnetic variation, radians, positive = East, negative = West
    bool available;            ///< Data validity flag
    unsigned long lastUpdate;  ///< millis() timestamp of last update
};

/**
 * @brief Wind vane sensor data (raw apparent wind)
 *
 * Apparent wind angle and speed from masthead sensor.
 * Units: radians, knots
 */
struct WindData {
    double apparentWindAngle;  ///< AWA, radians, range [-π, π], positive = starboard, negative = port
    double apparentWindSpeed;  ///< AWS, knots, range [0, 100]
    bool available;            ///< Data validity flag
    unsigned long lastUpdate;  ///< millis() timestamp of last update
};

/**
 * @brief Speed sensor data (paddle wheel and heel)
 *
 * Measured boat speed and heel angle.
 * Units: radians, knots
 */
struct SpeedData {
    double heelAngle;          ///< Radians, range [-π/2, π/2], positive = starboard, negative = port
    double measuredBoatSpeed;  ///< MSB, knots, from paddle wheel, range [0, 50]
    bool available;            ///< Data validity flag
    unsigned long lastUpdate;  ///< millis() timestamp of last update
};

/**
 * @brief Rudder sensor data
 *
 * Steering angle from rudder position sensor.
 * Units: radians
 */
struct RudderData {
    double steeringAngle;      ///< Radians, range [-π/2, π/2], positive = starboard, negative = port
    bool available;            ///< Data validity flag
    unsigned long lastUpdate;  ///< millis() timestamp of last update
};

// =============================================================================
// CALIBRATION DATA STRUCTURES
// =============================================================================

/**
 * @brief Calibration parameters (persisted to flash)
 *
 * User-configurable calibration values used in calculations.
 * Stored in /calibration.json on LittleFS.
 */
struct CalibrationData {
    double leewayCalibrationFactor;  ///< K factor in leeway formula, range (0, +∞), typical [0.1, 5.0]
    double windAngleOffset;          ///< Radians, range [-2π, 2π], masthead misalignment correction
    bool loaded;                     ///< True if loaded from flash, false if using defaults
};

/**
 * @brief Default calibration values
 */
#define DEFAULT_LEEWAY_K_FACTOR 1.0
#define DEFAULT_WIND_ANGLE_OFFSET 0.0

// =============================================================================
// DERIVED/CALCULATED DATA STRUCTURES
// =============================================================================

/**
 * @brief Calculated sailing parameters (updated every 200ms)
 *
 * Derived parameters calculated from sensor inputs.
 * Units: radians, knots
 */
struct DerivedData {
    // Corrected apparent wind angles
    double awaOffset;          ///< AWA corrected for masthead offset, radians, range [-π, π]
    double awaHeel;            ///< AWA corrected for heel, radians, range [-π, π]

    // Leeway and speed
    double leeway;             ///< Leeway angle, radians, range [-π/4, π/4] (limited to ±45°)
    double stw;                ///< Speed through water, knots, corrected for leeway

    // True wind
    double tws;                ///< True wind speed, knots, range [0, 100]
    double twa;                ///< True wind angle, radians, range [-π, π], relative to boat heading
    double wdir;               ///< Wind direction, radians, range [0, 2π], magnetic (true wind direction)

    // Performance
    double vmg;                ///< Velocity made good, knots, signed (negative = away from wind)

    // Current
    double soc;                ///< Speed of current, knots, range [0, 20]
    double doc;                ///< Direction of current, radians, range [0, 2π], magnetic (direction current flows TO)

    bool available;            ///< True if calculation completed successfully
    unsigned long lastUpdate;  ///< millis() timestamp of last calculation cycle
};

// =============================================================================
// DIAGNOSTIC DATA STRUCTURES
// =============================================================================

/**
 * @brief Diagnostic information (RAM-only, resets on reboot)
 *
 * Counters and performance metrics for system monitoring.
 */
struct DiagnosticData {
    unsigned long nmea0183MessageCount;      ///< Total NMEA 0183 messages received
    unsigned long nmea2000MessageCount;      ///< Total NMEA 2000 messages received
    unsigned long actisenseMessageCount;     ///< Total Actisense messages received
    unsigned long calculationCount;          ///< Total calculation cycles completed
    unsigned long calculationOverruns;       ///< Count of cycles exceeding 200ms
    unsigned long lastCalculationDuration;   ///< Duration of last calculation cycle (microseconds)
};

// =============================================================================
// COMPOSITE BOAT DATA STRUCTURE
// =============================================================================

/**
 * @brief Central boat data repository structure
 *
 * Statically allocated structure containing all sensor data,
 * derived parameters, calibration, and diagnostics.
 *
 * Thread safety: Single-threaded (ReactESP event loop)
 * Memory footprint: ~304 bytes (negligible for ESP32)
 */
struct BoatDataStructure {
    GPSData gps;               ///< GPS sensor data
    CompassData compass;       ///< Compass sensor data
    WindData wind;             ///< Wind vane sensor data
    SpeedData speed;           ///< Speed/heel sensor data
    RudderData rudder;         ///< Rudder position sensor data
    CalibrationData calibration;  ///< Calibration parameters
    DerivedData derived;       ///< Calculated sailing parameters
    DiagnosticData diagnostics;   ///< Diagnostic counters
};

// =============================================================================
// MULTI-SOURCE MANAGEMENT STRUCTURES
// =============================================================================

/**
 * @brief Sensor source metadata for multi-source prioritization
 *
 * Tracks multiple sources for the same sensor type (e.g., GPS-A, GPS-B).
 * Automatic priority based on update frequency, manual override supported.
 *
 * Memory footprint: ~100 bytes per source
 */
struct SensorSource {
    // Identification
    char sourceId[16];             ///< Human-readable ID (e.g., "GPS-NMEA2000", "GPS-NMEA0183")
    SensorType sensorType;         ///< Sensor type (GPS, COMPASS)
    ProtocolType protocolType;     ///< Protocol type (NMEA0183, NMEA2000, ONEWIRE)

    // Priority & Availability
    double updateFrequency;        ///< Measured update rate (Hz), used for automatic priority
    int priority;                  ///< Priority level (0 = highest), -1 = automatic (frequency-based)
    bool manualOverride;           ///< True if user manually set this as preferred source

    // Status
    bool active;                   ///< True if currently selected as primary source
    bool available;                ///< True if recent updates received (within 5 seconds)
    unsigned long lastUpdateTime;  ///< millis() timestamp of last data from this source
    unsigned long updateCount;     ///< Total updates received (for frequency calculation)

    // Statistics (for diagnostics)
    unsigned long rejectedCount;   ///< Count of invalid/outlier readings rejected
    double avgUpdateInterval;      ///< Average time between updates (ms)
};

/**
 * @brief Source management arrays (static allocation)
 */
#define MAX_GPS_SOURCES 5
#define MAX_COMPASS_SOURCES 5

/**
 * @brief Source manager structure
 *
 * Manages multiple sources for GPS and compass sensors.
 * Memory footprint: ~1050 bytes (10 sources * 100 bytes + metadata)
 */
struct SourceManager {
    SensorSource gpsSources[MAX_GPS_SOURCES];       ///< GPS source array
    int gpsSourceCount;                             ///< Active GPS sources (0-5)
    int activeGpsSourceIndex;                       ///< Index of current primary GPS source (-1 = none)

    SensorSource compassSources[MAX_COMPASS_SOURCES];  ///< Compass source array
    int compassSourceCount;                         ///< Active compass sources (0-5)
    int activeCompassSourceIndex;                   ///< Index of current primary compass source (-1 = none)

    unsigned long lastPriorityUpdate;               ///< millis() timestamp of last priority recalculation
};

// =============================================================================
// CALIBRATION PERSISTENCE STRUCTURE
// =============================================================================

/**
 * @brief Calibration parameters with persistence metadata
 *
 * Stored in /calibration.json on LittleFS.
 * Default values used if file missing or invalid.
 *
 * Memory footprint: ~32 bytes
 */
struct CalibrationParameters {
    // === Parameters ===
    double leewayCalibrationFactor;  ///< K factor in leeway formula, range (0, +∞)
    double windAngleOffset;          ///< Masthead misalignment correction, radians, range [-2π, 2π]

    // === Metadata ===
    unsigned long version;           ///< Config format version (1)
    unsigned long lastModified;      ///< Unix timestamp of last update
    bool valid;                      ///< True if loaded from flash successfully
};

#endif // BOATDATA_TYPES_H
