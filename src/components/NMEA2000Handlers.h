/**
 * @file NMEA2000Handlers.h
 * @brief NMEA2000 PGN message handlers for Enhanced BoatData v2.0.0
 *
 * This file contains handler functions for NMEA2000 Parameter Group Numbers (PGNs)
 * that populate the enhanced BoatData structure. Each handler:
 * - Parses the PGN using NMEA2000 library functions
 * - Validates data using DataValidation helpers
 * - Updates the global BoatData instance
 * - Logs events via WebSocket for debugging
 *
 * PGN Handlers (9 new + 1 enhanced):
 * - PGN 127251: Rate of Turn → CompassData.rateOfTurn
 * - PGN 127252: Heave → CompassData.heave
 * - PGN 127257: Attitude → CompassData heel/pitch (heave is N/A in this PGN)
 * - PGN 129029: GNSS Position (enhanced) → GPSData.variation
 * - PGN 128267: Water Depth → DSTData.depth
 * - PGN 128259: Speed (Water Referenced) → DSTData.measuredBoatSpeed
 * - PGN 130316: Temperature Extended Range → DSTData.seaTemperature
 * - PGN 127488: Engine Parameters, Rapid → EngineData.engineRev
 * - PGN 127489: Engine Parameters, Dynamic → EngineData oil temp/voltage
 *
 * @see specs/008-enhanced-boatdata-following/research.md lines 260-313
 * @see specs/008-enhanced-boatdata-following/data-model.md
 * @version 2.0.0
 * @date 2025-10-10
 */

#ifndef NMEA2000_HANDLERS_H
#define NMEA2000_HANDLERS_H

#include <Arduino.h>
#include <NMEA2000.h>
#include <N2kMessages.h>
#include "../components/BoatData.h"
#include "../utils/WebSocketLogger.h"
#include "../utils/DataValidation.h"

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
void HandleN2kPGN127251(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger);

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
void HandleN2kPGN127252(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger);

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
void HandleN2kPGN127257(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger);

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
void HandleN2kPGN129029(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger);

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
void HandleN2kPGN128267(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger);

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
void HandleN2kPGN128259(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger);

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
void HandleN2kPGN130316(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger);

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
void HandleN2kPGN127488(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger);

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
void HandleN2kPGN127489(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger);

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
void RegisterN2kHandlers(tNMEA2000* nmea2000, BoatData* boatData, WebSocketLogger* logger);

#endif // NMEA2000_HANDLERS_H
