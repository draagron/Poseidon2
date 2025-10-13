/**
 * @file ManufacturerLookup.h
 * @brief NMEA2000 manufacturer code to name mapping utility
 *
 * Provides static lookup table for translating manufacturer codes (uint16_t)
 * to human-readable names. Data stored in PROGMEM (flash) to conserve RAM.
 *
 * @see specs/013-r013-nmea2000-device/contracts/ManufacturerLookupContract.md
 * @version 1.0.0
 * @date 2025-10-13
 */

#ifndef MANUFACTURER_LOOKUP_H
#define MANUFACTURER_LOOKUP_H

#include <stdint.h>

// Platform-specific includes
#ifndef UNIT_TEST
#include <Arduino.h>
#else
// Native test environment - define PROGMEM as empty
#define PROGMEM
#define pgm_read_word(addr) (*(addr))
#define pgm_read_ptr(addr) (*(addr))
#endif

/**
 * @brief Manufacturer code to name mapping entry
 */
struct ManufacturerEntry {
    uint16_t code;          ///< NMEA2000 manufacturer code
    const char* name;       ///< Manufacturer name
};

/**
 * @brief Get manufacturer name from NMEA2000 code
 *
 * @param code NMEA2000 manufacturer code (e.g., 275, 1855)
 * @return Manufacturer name (e.g., "Garmin", "Furuno") or "Unknown (275)" for unrecognized codes
 *
 * @note Returns static string (no allocation), valid for program lifetime
 * @note Thread-safe (read-only data)
 * @note Performance: <50μs for linear search of ~50 entries
 */
const char* getManufacturerName(uint16_t code);

#endif // MANUFACTURER_LOOKUP_H
