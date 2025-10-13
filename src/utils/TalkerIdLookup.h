/**
 * @file TalkerIdLookup.h
 * @brief NMEA0183 talker ID to device type mapping utility
 *
 * Provides static lookup table for translating 2-character talker IDs
 * to human-readable device type descriptions. Data stored in PROGMEM (flash)
 * to conserve RAM.
 *
 * @see specs/013-r013-nmea2000-device/contracts/TalkerIdLookupContract.md
 * @version 1.0.0
 * @date 2025-10-13
 */

#ifndef TALKER_ID_LOOKUP_H
#define TALKER_ID_LOOKUP_H

#include <Arduino.h>

/**
 * @brief Talker ID to description mapping entry
 */
struct TalkerEntry {
    char id[3];                 ///< 2-char talker ID + null terminator
    const char* description;    ///< Device type description
};

/**
 * @brief Get device type description from NMEA0183 talker ID
 *
 * @param talkerId 2-character talker ID (e.g., "AP", "GP", "VH")
 * @return Device type description (e.g., "Autopilot", "GPS Receiver") or "Unknown NMEA0183 Device"
 *
 * @pre talkerId is null-terminated string (length ≥ 2)
 * @note Returns static string (no allocation), valid for program lifetime
 * @note Case-sensitive (NMEA0183 talker IDs are uppercase by standard)
 * @note Thread-safe (read-only data)
 * @note Performance: <20μs for linear search of ~15 entries
 */
const char* getTalkerDescription(const char* talkerId);

#endif // TALKER_ID_LOOKUP_H
