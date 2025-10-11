#ifndef NMEA0183PARSERS_H
#define NMEA0183PARSERS_H

#include <NMEA0183Msg.h>

/**
 * @file NMEA0183Parsers.h
 * @brief Custom NMEA 0183 sentence parsers
 *
 * Provides custom parser functions for NMEA 0183 sentences not included in the
 * ttlappalainen/NMEA0183 library. Follows library parser pattern:
 * - Function signature: bool NMEA0183Parse<MessageCode>(const tNMEA0183Msg& msg, <output params>)
 * - Returns true on successful parse, false on validation failure
 * - Validates talker ID, message code, and required fields
 * - Does not perform range validation (caller's responsibility)
 *
 * Reference: examples/poseidongw/src/NMEA0183Handlers.cpp:78-86 (RSA parser)
 */

/**
 * @brief Parse RSA (Rudder Sensor Angle) sentence
 *
 * RSA sentence format: $APRSA,<starboard_angle>,A,<port_angle>,A*<checksum>
 * Where:
 * - Field 0: Starboard rudder angle (degrees, positive=starboard)
 * - Field 1: Status ('A'=valid, 'V'=invalid)
 * - Field 2: Port rudder angle (degrees, unused in single-rudder systems)
 * - Field 3: Status ('A'=valid, 'V'=invalid)
 *
 * This parser extracts the starboard rudder angle (Field 0) for single-rudder
 * autopilot systems. Validates talker ID is "AP" (autopilot) and status is 'A'.
 *
 * Example sentence: $APRSA,15.0,A,15.0,A*3C
 * Result: rudderAngle = 15.0 degrees
 *
 * @param msg NMEA0183 message object (from tNMEA0183 library)
 * @param rudderAngle Output: rudder angle in degrees (positive=starboard, negative=port)
 * @return true if successfully parsed and talker ID is "AP" with status 'A', false otherwise
 *
 * @note Does not perform range validation (caller must check ±90° limit)
 * @note Custom parser - RSA not included in NMEA0183 library
 * @note Reference: examples/poseidongw/src/NMEA0183Handlers.cpp:78-86
 */
bool NMEA0183ParseRSA(const tNMEA0183Msg& msg, double& rudderAngle);

#endif // NMEA0183PARSERS_H
