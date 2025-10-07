/**
 * @file BoatData.h
 * @brief Central boat data repository implementation
 *
 * This class implements both IBoatDataStore and ISensorUpdate interfaces,
 * providing centralized storage for all marine sensor data with validation,
 * multi-source management, and diagnostic tracking.
 *
 * Features:
 * - Implements IBoatDataStore for read/write access to all sensor data
 * - Implements ISensorUpdate for NMEA/1-Wire message handlers
 * - Validates all incoming data (range + rate-of-change)
 * - Integrates with SourcePrioritizer for multi-source GPS/compass
 * - Tracks diagnostic counters (message counts, rejections)
 * - Thread-safe for single-threaded ReactESP event loop
 *
 * @see specs/003-boatdata-feature-as/data-model.md lines 26-126
 * @see test/contract/test_iboatdatastore_contract.cpp
 * @see test/contract/test_isensorupdate_contract.cpp
 * @version 1.0.0
 * @date 2025-10-06
 */

#ifndef BOAT_DATA_H
#define BOAT_DATA_H

#include "../hal/interfaces/IBoatDataStore.h"
#include "../hal/interfaces/ISensorUpdate.h"
#include "../hal/interfaces/ISourcePrioritizer.h"
#include "../utils/DataValidator.h"
#include "../types/BoatDataTypes.h"

/**
 * @brief Central boat data repository
 *
 * Statically allocated singleton providing centralized access to all
 * marine sensor data, derived parameters, calibration, and diagnostics.
 *
 * Usage:
 * @code
 * BoatData boatData(sourcePrioritizer);
 *
 * // NMEA handler updates GPS
 * boatData.updateGPS(40.7128, -74.0060, 1.571, 5.5, "GPS-NMEA0183");
 *
 * // Display reads GPS
 * GPSData gps = boatData.getGPSData();
 * if (gps.available) {
 *     Serial.printf("Lat: %.4f, Lon: %.4f\n", gps.latitude, gps.longitude);
 * }
 * @endcode
 */
class BoatData : public IBoatDataStore, public ISensorUpdate {
public:
    /**
     * @brief Constructor
     *
     * @param prioritizer Source prioritizer for multi-source GPS/compass management
     */
    BoatData(ISourcePrioritizer* prioritizer);

    // =========================================================================
    // IBoatDataStore IMPLEMENTATION
    // =========================================================================

    GPSData getGPSData() override;
    void setGPSData(const GPSData& data) override;

    CompassData getCompassData() override;
    void setCompassData(const CompassData& data) override;

    WindData getWindData() override;
    void setWindData(const WindData& data) override;

    SpeedData getSpeedData() override;
    void setSpeedData(const SpeedData& data) override;

    RudderData getRudderData() override;
    void setRudderData(const RudderData& data) override;

    DerivedData getDerivedData() override;
    void setDerivedData(const DerivedData& data) override;

    CalibrationData getCalibration() override;
    void setCalibration(const CalibrationData& data) override;

    // =========================================================================
    // ISensorUpdate IMPLEMENTATION
    // =========================================================================

    bool updateGPS(double lat, double lon, double cog, double sog, const char* sourceId) override;
    bool updateCompass(double trueHdg, double magHdg, double variation, const char* sourceId) override;
    bool updateWind(double awa, double aws, const char* sourceId) override;
    bool updateSpeed(double heelAngle, double boatSpeed, const char* sourceId) override;
    bool updateRudder(double angle, const char* sourceId) override;

    // =========================================================================
    // ADDITIONAL PUBLIC METHODS
    // =========================================================================

    /**
     * @brief Get diagnostic data
     *
     * @return Diagnostic counters (message counts, calculation stats)
     */
    DiagnosticData getDiagnostics();

    /**
     * @brief Increment NMEA0183 message counter
     */
    void incrementNMEA0183Count();

    /**
     * @brief Increment NMEA2000 message counter
     */
    void incrementNMEA2000Count();

    /**
     * @brief Increment Actisense message counter
     */
    void incrementActisenseCount();

private:
    // Central data structure
    BoatDataStructure data;

    // Source prioritizer for GPS/compass
    ISourcePrioritizer* sourcePrioritizer;

    /**
     * @brief Validate and update GPS data with rate-of-change check
     *
     * @param lat Latitude (degrees)
     * @param lon Longitude (degrees)
     * @param cog Course over ground (radians)
     * @param sog Speed over ground (knots)
     * @return true if valid and accepted, false if rejected
     */
    bool validateAndUpdateGPS(double lat, double lon, double cog, double sog);

    /**
     * @brief Validate and update compass data with rate-of-change check
     *
     * @param trueHdg True heading (radians)
     * @param magHdg Magnetic heading (radians)
     * @param variation Magnetic variation (radians)
     * @return true if valid and accepted, false if rejected
     */
    bool validateAndUpdateCompass(double trueHdg, double magHdg, double variation);
};

#endif // BOAT_DATA_H
