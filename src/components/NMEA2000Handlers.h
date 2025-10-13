/**
 * @file NMEA2000Handlers.h
 * @brief NMEA2000 PGN message handlers for NMEA 2000 handling feature
 *
 * This file contains handler functions for NMEA2000 Parameter Group Numbers (PGNs)
 * that populate the BoatData structure. Each handler:
 * - Parses the PGN using NMEA2000 library functions
 * - Validates data using DataValidation helpers
 * - Updates the global BoatData instance
 * - Logs events via WebSocket for debugging
 *
 * PGN Handlers (13 total):
 * GPS (4 PGNs):
 * - PGN 129025: Position, Rapid Update → GPSData lat/lon
 * - PGN 129026: COG & SOG, Rapid Update → GPSData cog/sog
 * - PGN 129029: GNSS Position Data → GPSData lat/lon
 * - PGN 127258: Magnetic Variation → GPSData.variation
 *
 * Compass (4 PGNs):
 * - PGN 127250: Vessel Heading → CompassData true/magnetic heading
 * - PGN 127251: Rate of Turn → CompassData.rateOfTurn
 * - PGN 127252: Heave → CompassData.heave
 * - PGN 127257: Attitude → CompassData heel/pitch
 *
 * DST (3 PGNs):
 * - PGN 128267: Water Depth → DSTData.depth
 * - PGN 128259: Speed (Water Referenced) → DSTData.measuredBoatSpeed
 * - PGN 130316: Temperature Extended Range → DSTData.seaTemperature
 *
 * Engine (2 PGNs):
 * - PGN 127488: Engine Parameters, Rapid → EngineData.engineRev
 * - PGN 127489: Engine Parameters, Dynamic → EngineData oil temp/voltage
 *
 * Wind (1 PGN):
 * - PGN 130306: Wind Data → WindData apparent wind angle/speed
 *
 * @see specs/010-nmea-2000-handling/
 * @version 1.0.0
 * @date 2025-10-12
 */

#ifndef NMEA2000_HANDLERS_H
#define NMEA2000_HANDLERS_H

#include <Arduino.h>
#include <NMEA2000.h>
#include <N2kMessages.h>
#include "../components/BoatData.h"
#include "../utils/WebSocketLogger.h"
#include "../utils/DataValidation.h"
#include "../components/SourceRegistry.h"

/**
 * @brief Handle PGN 127251 - Rate of Turn
 *
 * Updates CompassData.rateOfTurn with validated rate of turn value.
 * Sign convention: positive = turning right (starboard)
 *
 * @param N2kMsg NMEA2000 message
 * @param boatData BoatData instance to update
 * @param logger WebSocket logger for debug output
 */
void HandleN2kPGN127251(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger, SourceRegistry* registry);

/**
 * @brief Handle PGN 127252 - Heave
 *
 * Parses NMEA2000 PGN 127252 (Heave) messages to extract vertical displacement data,
 * validates the value against the acceptable range (±5.0 meters), and updates the
 * CompassData.heave field in the BoatData structure. Logs all operations via WebSocket
 * for debugging and monitoring.
 *
 * Sign convention: positive = upward motion (vessel rising above reference plane)
 *
 * Validation:
 * - Valid range: [-5.0, 5.0] meters
 * - Out-of-range values: Clamped to limits with WARN log
 * - Unavailable (N2kDoubleNA): Logged at DEBUG, update skipped
 * - Parse failure: Logged at ERROR, availability set to false
 *
 * @param N2kMsg NMEA2000 message (const reference, not modified)
 * @param boatData BoatData instance to update (must not be nullptr)
 * @param logger WebSocket logger for debug output (must not be nullptr)
 *
 * @pre boatData != nullptr
 * @pre logger != nullptr
 * @post If valid heave parsed: CompassData.heave updated, availability=true, timestamp=millis()
 *
 * @see ParseN2kPGN127252 (NMEA2000 library function)
 * @see DataValidation::clampHeave
 * @see DataValidation::isValidHeave
 */
void HandleN2kPGN127252(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger, SourceRegistry* registry);

/**
 * @brief Handle PGN 127257 - Attitude
 *
 * Updates CompassData with heel, pitch, and heave values.
 * Validates pitch (±30°) and heave (±5m) ranges.
 *
 * Sign conventions:
 * - Heel: positive = starboard tilt
 * - Pitch: positive = bow up
 * - Heave: positive = upward motion
 *
 * @param N2kMsg NMEA2000 message
 * @param boatData BoatData instance to update
 * @param logger WebSocket logger for debug output
 */
void HandleN2kPGN127257(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger, SourceRegistry* registry);

/**
 * @brief Handle PGN 129029 - GNSS Position Data (Enhanced)
 *
 * Updates GPSData with latitude, longitude, altitude, and magnetic variation.
 * This is an enhancement to existing GPS handling to add variation field.
 *
 * @param N2kMsg NMEA2000 message
 * @param boatData BoatData instance to update
 * @param logger WebSocket logger for debug output
 */
void HandleN2kPGN129029(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger, SourceRegistry* registry);

/**
 * @brief Handle PGN 128267 - Water Depth
 *
 * Updates DSTData.depth with depth below waterline.
 * Validates depth is non-negative (rejects sensor above water).
 *
 * @param N2kMsg NMEA2000 message
 * @param boatData BoatData instance to update
 * @param logger WebSocket logger for debug output
 */
void HandleN2kPGN128267(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger, SourceRegistry* registry);

/**
 * @brief Handle PGN 128259 - Speed (Water Referenced)
 *
 * Updates DSTData.measuredBoatSpeed with paddle wheel speed sensor data.
 * Unit: m/s (NMEA2000 native unit)
 *
 * @param N2kMsg NMEA2000 message
 * @param boatData BoatData instance to update
 * @param logger WebSocket logger for debug output
 */
void HandleN2kPGN128259(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger, SourceRegistry* registry);

/**
 * @brief Handle PGN 130316 - Temperature Extended Range
 *
 * Updates DSTData.seaTemperature with water temperature.
 * Converts from Kelvin (NMEA2000 format) to Celsius.
 *
 * @param N2kMsg NMEA2000 message
 * @param boatData BoatData instance to update
 * @param logger WebSocket logger for debug output
 */
void HandleN2kPGN130316(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger, SourceRegistry* registry);

/**
 * @brief Handle PGN 127488 - Engine Parameters, Rapid Update
 *
 * Updates EngineData.engineRev with engine RPM.
 * Validates RPM is within [0, 6000] range.
 *
 * @param N2kMsg NMEA2000 message
 * @param boatData BoatData instance to update
 * @param logger WebSocket logger for debug output
 */
void HandleN2kPGN127488(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger, SourceRegistry* registry);

/**
 * @brief Handle PGN 127489 - Engine Parameters, Dynamic
 *
 * Updates EngineData with oil temperature and alternator voltage.
 * Validates temperature ([-10, 150]°C) and voltage ([0, 30]V) ranges.
 *
 * @param N2kMsg NMEA2000 message
 * @param boatData BoatData instance to update
 * @param logger WebSocket logger for debug output
 */
void HandleN2kPGN127489(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger, SourceRegistry* registry);

/**
 * @brief Handle PGN 129025 - Position, Rapid Update
 *
 * Updates GPSData with latitude and longitude at 10 Hz update rate.
 * This is the primary GPS position source for navigation.
 *
 * @param N2kMsg NMEA2000 message
 * @param boatData BoatData instance to update
 * @param logger WebSocket logger for debug output
 */
void HandleN2kPGN129025(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger, SourceRegistry* registry);

/**
 * @brief Handle PGN 129026 - COG & SOG, Rapid Update
 *
 * Updates GPSData with course over ground and speed over ground.
 * Speed is converted from m/s (NMEA2000) to knots (BoatData).
 *
 * @param N2kMsg NMEA2000 message
 * @param boatData BoatData instance to update
 * @param logger WebSocket logger for debug output
 */
void HandleN2kPGN129026(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger, SourceRegistry* registry);

/**
 * @brief Handle PGN 127250 - Vessel Heading
 *
 * Updates CompassData with true or magnetic heading based on reference type.
 * Routes to trueHeading (N2khr_true) or magneticHeading (N2khr_magnetic).
 *
 * @param N2kMsg NMEA2000 message
 * @param boatData BoatData instance to update
 * @param logger WebSocket logger for debug output
 */
void HandleN2kPGN127250(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger, SourceRegistry* registry);

/**
 * @brief Handle PGN 127258 - Magnetic Variation
 *
 * Updates GPSData.variation with magnetic variation value.
 * Alternative source to PGN 129029 variation field.
 *
 * @param N2kMsg NMEA2000 message
 * @param boatData BoatData instance to update
 * @param logger WebSocket logger for debug output
 */
void HandleN2kPGN127258(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger, SourceRegistry* registry);

/**
 * @brief Handle PGN 130306 - Wind Data
 *
 * Updates WindData with apparent wind angle and speed.
 * Only processes N2kWind_Apparent reference type (ignores true wind).
 * Speed is converted from m/s (NMEA2000) to knots (BoatData).
 *
 * @param N2kMsg NMEA2000 message
 * @param boatData BoatData instance to update
 * @param logger WebSocket logger for debug output
 */
void HandleN2kPGN130306(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger, SourceRegistry* registry);

/**
 * @brief Register all PGN handlers with NMEA2000 library
 *
 * Calls SetN2kPGNHandler() for all implemented handlers.
 * Should be called once during setup() after NMEA2000 initialization.
 *
 * @param nmea2000 NMEA2000 instance
 * @param boatData BoatData instance for handler callbacks
 * @param logger WebSocket logger for handler callbacks
 */
void RegisterN2kHandlers(tNMEA2000* nmea2000, BoatData* boatData, WebSocketLogger* logger, SourceRegistry* registry);

#endif // NMEA2000_HANDLERS_H
