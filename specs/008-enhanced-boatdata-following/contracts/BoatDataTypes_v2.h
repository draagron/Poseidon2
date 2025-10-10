/**
 * @file BoatDataTypes_v2.h
 * @brief Enhanced data structure definitions for BoatData v2.0.0
 *
 * CONTRACT SPECIFICATION for Enhanced BoatData feature (R005).
 * This file defines the EXPECTED structure after implementation.
 *
 * Changes from v1.0.0:
 * - GPSData: Added variation field (moved from CompassData)
 * - CompassData: Added rateOfTurn, heelAngle, pitchAngle, heave; removed variation
 * - SpeedData renamed to DSTData with added depth and seaTemperature fields
 * - NEW: EngineData structure
 * - NEW: SaildriveData structure
 * - NEW: BatteryData structure
 * - NEW: ShorePowerData structure
 *
 * @see specs/008-enhanced-boatdata-following/data-model.md
 * @version 2.0.0
 * @date 2025-10-10
 */

#ifndef BOATDATA_TYPES_V2_H
#define BOATDATA_TYPES_V2_H

#include <Arduino.h>

// =============================================================================
// ENUMERATIONS (unchanged from v1.0.0)
// =============================================================================

enum class SensorType : uint8_t {
    GPS = 0,
    COMPASS = 1,
};

enum class ProtocolType : uint8_t {
    NMEA0183 = 0,
    NMEA2000 = 1,
    ONEWIRE = 2,
    UNKNOWN = 255
};

// =============================================================================
// SENSOR DATA STRUCTURES (ENHANCED)
// =============================================================================

/**
 * @brief GPS sensor data (ENHANCED from v1.0.0)
 *
 * CHANGES:
 * - ADDED: double variation (magnetic variation from GPS)
 */
struct GPSData {
    double latitude;           ///< Decimal degrees, positive = North, range [-90, 90]
    double longitude;          ///< Decimal degrees, positive = East, range [-180, 180]
    double cog;                ///< Course over ground, radians, range [0, 2π], true
    double sog;                ///< Speed over ground, knots, range [0, 100]
    double variation;          ///< Magnetic variation, radians, positive = East, negative = West (ADDED v2.0.0)
    bool available;            ///< Data validity flag
    unsigned long lastUpdate;  ///< millis() timestamp of last update
};

/**
 * @brief Compass sensor data (ENHANCED from v1.0.0)
 *
 * CHANGES:
 * - REMOVED: double variation (moved to GPSData)
 * - ADDED: double rateOfTurn (radians/second)
 * - ADDED: double heelAngle (moved from SpeedData/DSTData)
 * - ADDED: double pitchAngle (radians)
 * - ADDED: double heave (meters)
 */
struct CompassData {
    double trueHeading;        ///< Radians, range [0, 2π]
    double magneticHeading;    ///< Radians, range [0, 2π]
    double rateOfTurn;         ///< Radians/second, positive = turning right (ADDED v2.0.0)
    double heelAngle;          ///< Radians, range [-π/2, π/2], positive = starboard (ADDED v2.0.0)
    double pitchAngle;         ///< Radians, range [-π/6, π/6], positive = bow up (ADDED v2.0.0)
    double heave;              ///< Meters, range [-5.0, 5.0], positive = upward (ADDED v2.0.0)
    bool available;            ///< Data validity flag
    unsigned long lastUpdate;  ///< millis() timestamp of last update
};

/**
 * @brief Wind vane sensor data (unchanged from v1.0.0)
 */
struct WindData {
    double apparentWindAngle;  ///< AWA, radians, range [-π, π], positive = starboard
    double apparentWindSpeed;  ///< AWS, knots, range [0, 100]
    bool available;
    unsigned long lastUpdate;
};

/**
 * @brief DST sensor data (RENAMED from SpeedData, ENHANCED)
 *
 * CHANGES:
 * - RENAMED: SpeedData → DSTData
 * - REMOVED: double heelAngle (moved to CompassData)
 * - ADDED: double depth (meters)
 * - ADDED: double seaTemperature (Celsius)
 * - KEPT: double measuredBoatSpeed (m/s, unchanged)
 */
struct DSTData {
    double depth;              ///< Depth below waterline, meters, range [0, 100] (ADDED v2.0.0)
    double measuredBoatSpeed;  ///< Speed through water, m/s, range [0, 25] (unchanged)
    double seaTemperature;     ///< Water temperature, Celsius, range [-10, 50] (ADDED v2.0.0)
    bool available;
    unsigned long lastUpdate;
};

/**
 * @brief Rudder sensor data (unchanged from v1.0.0)
 */
struct RudderData {
    double steeringAngle;      ///< Radians, range [-π/2, π/2], positive = starboard
    bool available;
    unsigned long lastUpdate;
};

/**
 * @brief Engine telemetry data (NEW in v2.0.0)
 */
struct EngineData {
    double engineRev;          ///< Engine RPM, range [0, 6000]
    double oilTemperature;     ///< Oil temperature, Celsius, range [-10, 150]
    double alternatorVoltage;  ///< Alternator output, volts, range [0, 30]
    bool available;
    unsigned long lastUpdate;
};

/**
 * @brief Saildrive engagement status (NEW in v2.0.0)
 */
struct SaildriveData {
    bool saildriveEngaged;     ///< true = deployed/engaged, false = retracted/disengaged
    bool available;
    unsigned long lastUpdate;
};

/**
 * @brief Dual battery bank monitoring (NEW in v2.0.0)
 */
struct BatteryData {
    // Battery A (House Bank)
    double voltageA;           ///< Volts, range [0, 30]
    double amperageA;          ///< Amperes, range [-200, 200], positive = charging
    double stateOfChargeA;     ///< Percent, range [0.0, 100.0]
    bool shoreChargerOnA;      ///< true = shore charger active for Battery A
    bool engineChargerOnA;     ///< true = alternator charging Battery A

    // Battery B (Starter Bank)
    double voltageB;           ///< Volts, range [0, 30]
    double amperageB;          ///< Amperes, range [-200, 200], positive = charging
    double stateOfChargeB;     ///< Percent, range [0.0, 100.0]
    bool shoreChargerOnB;      ///< true = shore charger active for Battery B
    bool engineChargerOnB;     ///< true = alternator charging Battery B

    // Metadata
    bool available;
    unsigned long lastUpdate;
};

/**
 * @brief Shore power connection and consumption (NEW in v2.0.0)
 */
struct ShorePowerData {
    bool shorePowerOn;         ///< true = shore power connected and available
    double power;              ///< Shore power draw, watts, range [0, 5000]
    bool available;
    unsigned long lastUpdate;
};

// =============================================================================
// CALIBRATION DATA STRUCTURES (unchanged from v1.0.0)
// =============================================================================

struct CalibrationData {
    double leewayCalibrationFactor;
    double windAngleOffset;
    bool loaded;
};

#define DEFAULT_LEEWAY_K_FACTOR 1.0
#define DEFAULT_WIND_ANGLE_OFFSET 0.0

// =============================================================================
// DERIVED/CALCULATED DATA STRUCTURES (unchanged from v1.0.0)
// =============================================================================

struct DerivedData {
    double awaOffset;
    double awaHeel;
    double leeway;
    double stw;
    double tws;
    double twa;
    double wdir;
    double vmg;
    double soc;
    double doc;
    bool available;
    unsigned long lastUpdate;
};

// =============================================================================
// DIAGNOSTIC DATA STRUCTURES (unchanged from v1.0.0)
// =============================================================================

struct DiagnosticData {
    unsigned long nmea0183MessageCount;
    unsigned long nmea2000MessageCount;
    unsigned long actisenseMessageCount;
    unsigned long calculationCount;
    unsigned long calculationOverruns;
    unsigned long lastCalculationDuration;
};

// =============================================================================
// COMPOSITE BOAT DATA STRUCTURE (ENHANCED)
// =============================================================================

/**
 * @brief Central boat data repository structure (v2.0.0)
 *
 * CHANGES:
 * - RENAMED: SpeedData speed → DSTData dst
 * - ADDED: EngineData engine
 * - ADDED: SaildriveData saildrive
 * - ADDED: BatteryData battery
 * - ADDED: ShorePowerData shorePower
 *
 * Memory footprint: ~560 bytes (up from ~304 bytes in v1.0.0)
 */
struct BoatDataStructure {
    GPSData gps;                   ///< GPS sensor data (enhanced with variation)
    CompassData compass;           ///< Compass sensor data (enhanced with attitude)
    WindData wind;                 ///< Wind vane sensor data (unchanged)
    DSTData dst;                   ///< Depth/Speed/Temperature (replaces SpeedData)
    RudderData rudder;             ///< Rudder position sensor data (unchanged)
    EngineData engine;             ///< Engine telemetry (NEW)
    SaildriveData saildrive;       ///< Saildrive status (NEW)
    BatteryData battery;           ///< Dual battery monitoring (NEW)
    ShorePowerData shorePower;     ///< Shore power status (NEW)
    CalibrationData calibration;   ///< Calibration parameters (unchanged)
    DerivedData derived;           ///< Calculated sailing parameters (unchanged)
    DiagnosticData diagnostics;    ///< Diagnostic counters (unchanged)
};

// =============================================================================
// MULTI-SOURCE MANAGEMENT STRUCTURES (unchanged from v1.0.0)
// =============================================================================

struct SensorSource {
    char sourceId[16];
    SensorType sensorType;
    ProtocolType protocolType;
    double updateFrequency;
    int priority;
    bool manualOverride;
    bool active;
    bool available;
    unsigned long lastUpdateTime;
    unsigned long updateCount;
    unsigned long rejectedCount;
    double avgUpdateInterval;
};

#define MAX_GPS_SOURCES 5
#define MAX_COMPASS_SOURCES 5

struct SourceManager {
    SensorSource gpsSources[MAX_GPS_SOURCES];
    int gpsSourceCount;
    int activeGpsSourceIndex;
    SensorSource compassSources[MAX_COMPASS_SOURCES];
    int compassSourceCount;
    int activeCompassSourceIndex;
    unsigned long lastPriorityUpdate;
};

// =============================================================================
// CALIBRATION PERSISTENCE STRUCTURE (unchanged from v1.0.0)
// =============================================================================

struct CalibrationParameters {
    double leewayCalibrationFactor;
    double windAngleOffset;
    unsigned long version;
    unsigned long lastModified;
    bool valid;
};

// =============================================================================
// BACKWARD COMPATIBILITY (temporary during migration)
// =============================================================================

/**
 * @brief Temporary typedef for backward compatibility
 *
 * Allows legacy code to compile during migration phase.
 * DEPRECATED: Will be removed in v2.1.0
 */
typedef DSTData SpeedData;

#endif // BOATDATA_TYPES_V2_H
