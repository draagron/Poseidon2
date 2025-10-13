/**
 * @file ManufacturerLookup.cpp
 * @brief NMEA2000 manufacturer code to name mapping implementation
 *
 * @see specs/013-r013-nmea2000-device/contracts/ManufacturerLookupContract.md
 * @version 1.0.0
 * @date 2025-10-13
 */

#include "ManufacturerLookup.h"
#include <stdio.h>

#ifndef UNIT_TEST
#include <Arduino.h>
#endif

/**
 * @brief Static lookup table (stored in PROGMEM to save RAM)
 *
 * Sorted by code for future binary search optimization (if needed).
 * Data sources:
 * - NMEA organization: https://www.nmea.org/nmea-2000.html
 * - Canboat project: https://github.com/canboat/canboat/blob/master/analyzer/pgns.json
 */
const ManufacturerEntry PROGMEM manufacturerTable[] = {
    {78, "Navionics"},
    {80, "Hemisphere GPS"},
    {85, "Siemens"},
    {88, "SailorMade"},
    {116, "Blue Water Data"},
    {135, "B&G"},
    {137, "Airmar"},
    {140, "Lowrance"},
    {144, "Mercury Marine"},
    {147, "Nautibus"},
    {154, "Volvo Penta"},
    {161, "Maretron"},
    {163, "Fusion Electronics"},
    {165, "Raymarine"},
    {168, "Navico (Simrad)"},
    {172, "Yanmar"},
    {174, "Actia Corporation"},
    {192, "Furuno"},
    {193, "NovAtel"},
    {198, "Mystic Valley Comm"},
    {199, "Xantrex/Schneider"},
    {201, "Garmin"},
    {203, "Hamilton Jet"},
    {211, "Coelmo"},
    {215, "Veethree Electronics"},
    {224, "SeaTalk"},
    {229, "Advansea"},
    {233, "ComNav"},
    {257, "Honda"},
    {273, "Victron Energy"},
    {274, "Transas"},
    {275, "Garmin"},  // Duplicate intentional - Garmin has multiple codes
    {283, "L3 Technologies"},
    {285, "Nobeltec"},
    {286, "BEP Marine"},
    {295, "Offshore Systems"},
    {304, "Mercury Marine"},  // Duplicate code (different product lines)
    {307, "Yanmar"},  // Duplicate code
    {315, "Cummins"},
    {355, "Honda"},  // Duplicate code
    {356, "Groco"},
    {358, "Empir Bus"},
    {370, "Capi 2"},
    {373, "ComNav"},  // Duplicate code
    {374, "Northstar Technologies"},
    {378, "Raymarine"},  // Duplicate code
    {381, "Sea Recovery"},
    {384, "Hemisphere GPS"},  // Duplicate code
    {394, "EmpirBus"},
    {396, "Ocean Sat"},
    {419, "Intellian"},
    {437, "Furuno"},  // Duplicate code (different product lines)
    {459, "Twin Disc"},
    {467, "Kohler"},
    {470, "Mastervolt"},
    {471, "Fischer Panda"},
    {475, "Watcheye"},
    {476, "Ocean LED"},
    {481, "Yacht Control"},
    {493, "KVH Industries"},
    {517, "Lenco Marine"},
    {529, "Victron Energy"},  // Duplicate code
    {573, "SailorMade"},  // Duplicate code
    {578, "Navico"},
    {580, "McMurdo (Orolia)"},
    {586, "Northern Lights"},
    {591, "West Marine"},
    {595, "Digital Yacht"},
    {600, "BEP Marine"},  // Duplicate code
    {743, "Egersund Marine"},
    {799, "Volvo Penta"},  // Duplicate code
    {838, "BlueSeas Systems"},
    {847, "Parker Hannifin"},
    {862, "Evinrude/BRP"},
    {890, "Teleflex"},
    {894, "Yacht Devices"},
    {896, "Mercury Marine"},  // Duplicate code
    {905, "Litton"},
    {909, "Humminbird"},
    {911, "Johnson Outdoors"},
    {1850, "Navico (B&G/Simrad/Lowrance)"},
    {1851, "Simrad"},
    {1852, "Lowrance"},
    {1853, "Mercury Marine"},  // Duplicate code
    {1854, "Furuno USA"},
    {1855, "Furuno"},  // Main Furuno code
    {1856, "Navico"},  // Duplicate code
    {1857, "Navico (Simrad/Lowrance/B&G)"},
    {1858, "Northstar Technologies"},  // Duplicate code
    {1859, "B&G"},  // Duplicate code
    {1860, "Simrad"},  // Duplicate code
    {1861, "Honda"},  // Duplicate code
    {1862, "Garmin"},  // Duplicate code
    {1863, "Hemisphere GPS"},  // Duplicate code
    {1870, "Yacht Monitoring Solutions"},
    {0, nullptr}  // Sentinel (end of table)
};

/**
 * @brief Fallback buffer for unknown codes
 *
 * Uses static buffer to avoid dynamic allocation.
 * Not thread-safe, but acceptable for single-threaded ESP32 application.
 */
static char unknownBuffer[32];

const char* getManufacturerName(uint16_t code) {
    // Linear search through PROGMEM table
    for (int i = 0; ; i++) {
        uint16_t tableCode = pgm_read_word(&manufacturerTable[i].code);
        const char* tableName = (const char*)pgm_read_ptr(&manufacturerTable[i].name);

        // Check for sentinel (end of table)
        if (tableName == nullptr) {
            break;
        }

        // Found matching code
        if (tableCode == code) {
            return tableName;
        }
    }

    // Not found - return "Unknown (code)" format
    snprintf(unknownBuffer, sizeof(unknownBuffer), "Unknown (%u)", code);
    return unknownBuffer;
}
