/**
 * @file TalkerIdLookup.cpp
 * @brief NMEA0183 talker ID to device type mapping implementation
 *
 * @see specs/013-r013-nmea2000-device/contracts/TalkerIdLookupContract.md
 * @version 1.0.0
 * @date 2025-10-13
 */

#include "TalkerIdLookup.h"
#include <Arduino.h>
#include <string.h>

/**
 * @brief Static lookup table (stored in PROGMEM to save RAM)
 *
 * Data sources:
 * - IEC 61162-1 standard (NMEA0183)
 * - GPSD documentation: https://gpsd.gitlab.io/gpsd/NMEA.html
 */
const TalkerEntry PROGMEM talkerTable[] = {
    {"AG", "Autopilot (General)"},
    {"AI", "AIS (Automatic Identification System)"},
    {"AP", "Autopilot"},
    {"BD", "BeiDou Receiver"},
    {"CD", "Digital Selective Calling (DSC)"},
    {"CS", "Communications - Satellite"},
    {"CT", "Communications - Radio-Telephone (MF/HF)"},
    {"CV", "Communications - Radio-Telephone (VHF)"},
    {"CX", "Communications - Scanning Receiver"},
    {"DE", "DECCA Navigation"},
    {"DF", "Direction Finder"},
    {"DU", "Duplex Repeater Station"},
    {"EC", "Electronic Chart System"},
    {"EP", "Emergency Position Indicating Beacon"},
    {"ER", "Engine Room Monitoring Systems"},
    {"GA", "Galileo Receiver"},
    {"GB", "BeiDou (China)"},
    {"GI", "NavIC/IRNSS (India)"},
    {"GL", "GLONASS Receiver"},
    {"GN", "GNSS Receiver (Multi-constellation)"},
    {"GP", "GPS Receiver"},
    {"HC", "Heading Compass"},
    {"HE", "Heading - North Seeking Gyro"},
    {"HN", "Heading - Non North Seeking Gyro"},
    {"II", "Integrated Instrumentation"},
    {"IN", "Integrated Navigation"},
    {"LA", "Loran-A"},
    {"LC", "Loran-C"},
    {"MP", "Microwave Positioning System"},
    {"NL", "Navigation Light Controller"},
    {"OM", "OMEGA Navigation System"},
    {"OS", "Distress Alarm System"},
    {"QZ", "QZSS (Japan)"},
    {"RA", "RADAR and/or ARPA"},
    {"SD", "Depth Sounder"},
    {"SN", "Electronic Positioning System"},
    {"SS", "Scanning Sounder"},
    {"TI", "Turn Rate Indicator"},
    {"TR", "TRANSIT Navigation System"},
    {"U0", "User Configured Talker ID (0)"},
    {"U1", "User Configured Talker ID (1)"},
    {"U2", "User Configured Talker ID (2)"},
    {"U3", "User Configured Talker ID (3)"},
    {"U4", "User Configured Talker ID (4)"},
    {"U5", "User Configured Talker ID (5)"},
    {"U6", "User Configured Talker ID (6)"},
    {"U7", "User Configured Talker ID (7)"},
    {"U8", "User Configured Talker ID (8)"},
    {"U9", "User Configured Talker ID (9)"},
    {"UP", "User Configured Talker ID (P)"},
    {"VA", "VHF Data Exchange System (VDES) - ASM"},
    {"VD", "Velocity Sensor - Doppler"},
    {"VH", "VHF Radio"},
    {"VM", "Velocity Sensor - Speed Log, Water, Magnetic"},
    {"VR", "Voyage Data Recorder"},
    {"VS", "VDES Satellite"},
    {"VT", "VDES Terrestrial"},
    {"VW", "Wind Sensor"},
    {"WI", "Weather Instruments"},
    {"YC", "Transducer - Temperature"},
    {"YD", "Transducer - Displacement, Angular or Linear"},
    {"YF", "Transducer - Frequency"},
    {"YL", "Transducer - Level"},
    {"YP", "Transducer - Pressure"},
    {"YR", "Transducer - Flow Rate"},
    {"YT", "Transducer - Tachometer"},
    {"YV", "Transducer - Volume"},
    {"YX", "Transducer (Multi-sensor)"},
    {"ZA", "Timekeeper - Atomic Clock"},
    {"ZC", "Timekeeper - Chronometer"},
    {"ZQ", "Timekeeper - Quartz"},
    {"ZV", "Timekeeper - Radio Update"},
    {"", nullptr}  // Sentinel (end of table)
};

const char* getTalkerDescription(const char* talkerId) {
    // Validate input (handle short strings gracefully)
    if (talkerId == nullptr || talkerId[0] == '\0') {
        return "Unknown NMEA0183 Device";
    }

    // Linear search through PROGMEM table
    for (int i = 0; ; i++) {
        char tableId[3];
        // Read talker ID from PROGMEM
        strcpy_P(tableId, (const char*)pgm_read_ptr(&talkerTable[i].id));
        const char* tableDescription = (const char*)pgm_read_ptr(&talkerTable[i].description);

        // Check for sentinel (end of table)
        if (tableDescription == nullptr || tableId[0] == '\0') {
            break;
        }

        // Found matching talker ID (case-sensitive comparison)
        if (strcmp(tableId, talkerId) == 0) {
            return tableDescription;
        }
    }

    // Not found - return generic unknown device
    return "Unknown NMEA0183 Device";
}
