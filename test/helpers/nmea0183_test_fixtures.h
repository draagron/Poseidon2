#ifndef NMEA0183_TEST_FIXTURES_H
#define NMEA0183_TEST_FIXTURES_H

/**
 * @file nmea0183_test_fixtures.h
 * @brief Shared test utilities and NMEA 0183 sentence fixtures
 *
 * Provides valid and invalid NMEA 0183 sentence constants for testing parsers
 * and handlers. All valid sentences have correct checksums calculated per NMEA
 * 0183 specification (XOR of all characters between $ and *).
 *
 * Usage:
 * @code
 * MockSerialPort mock;
 * mock.setMockData(VALID_APRSA);
 * // Test sentence processing...
 * @endcode
 */

// ============================================================================
// Valid NMEA 0183 Sentences (Correct Checksums)
// ============================================================================

/**
 * @brief Valid RSA (Rudder Sensor Angle) sentence from autopilot
 *
 * Sentence: $APRSA,15.0,A*3C
 * - Talker ID: AP (autopilot)
 * - Rudder angle: 15.0° starboard
 * - Status: A (valid)
 * - Checksum: 3C (valid)
 */
const char* VALID_APRSA = "$APRSA,15.0,A*3C\r\n";

/**
 * @brief Valid HDM (Heading Magnetic) sentence from autopilot
 *
 * Sentence: $APHDM,045.5,M*2F
 * - Talker ID: AP (autopilot)
 * - Magnetic heading: 045.5° (45.5 degrees)
 * - Reference: M (magnetic)
 * - Checksum: 2F (valid)
 */
const char* VALID_APHDM = "$APHDM,045.5,M*2F\r\n";

/**
 * @brief Valid GGA (GPS Fix Data) sentence from VHF radio
 *
 * Sentence: $VHGGA,123519,5230.5000,N,00507.0000,E,1,08,0.9,545.4,M,46.9,M,,*47
 * - Talker ID: VH (VHF radio)
 * - Time: 12:35:19 UTC
 * - Latitude: 5230.5000 N (52°30.5' North = 52.508333°)
 * - Longitude: 00507.0000 E (5°7' East = 5.116667°)
 * - Fix quality: 1 (GPS fix)
 * - Satellites: 08
 * - HDOP: 0.9
 * - Altitude: 545.4 M
 * - Checksum: 47 (valid)
 */
const char* VALID_VHGGA = "$VHGGA,123519,5230.5000,N,00507.0000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n";

/**
 * @brief Valid RMC (Recommended Minimum Navigation) sentence from VHF radio
 *
 * Sentence: $VHRMC,123519,A,5230.5000,N,00507.0000,E,5.5,054.7,230394,003.1,W*6A
 * - Talker ID: VH (VHF radio)
 * - Time: 12:35:19 UTC
 * - Status: A (valid)
 * - Latitude: 5230.5000 N (52.508333°)
 * - Longitude: 00507.0000 E (5.116667°)
 * - SOG: 5.5 knots
 * - COG: 054.7° true
 * - Date: 23/03/1994
 * - Variation: 003.1° W (3.1° West = -3.1°)
 * - Checksum: 6A (valid)
 */
const char* VALID_VHRMC = "$VHRMC,123519,A,5230.5000,N,00507.0000,E,5.5,054.7,230394,003.1,W*6A\r\n";

/**
 * @brief Valid VTG (Track Made Good) sentence from VHF radio
 *
 * Sentence: $VHVTG,054.7,T,057.9,M,5.5,N,10.2,K*48
 * - Talker ID: VH (VHF radio)
 * - True COG: 054.7° (54.7 degrees true)
 * - Reference: T (true)
 * - Magnetic COG: 057.9° (57.9 degrees magnetic)
 * - Reference: M (magnetic)
 * - SOG: 5.5 N (knots)
 * - SOG: 10.2 K (km/h)
 * - Calculated variation: 054.7 - 057.9 = -3.2° (3.2°W)
 * - Checksum: 48 (valid)
 */
const char* VALID_VHVTG = "$VHVTG,054.7,T,057.9,M,5.5,N,10.2,K*48\r\n";

// ============================================================================
// Invalid NMEA 0183 Sentences (For Error Testing)
// ============================================================================

/**
 * @brief Invalid RSA sentence - bad checksum
 *
 * Sentence: $APRSA,15.0,A*FF (checksum should be *3C)
 * - Expected behavior: Silent discard (FR-024)
 */
const char* INVALID_APRSA_CHECKSUM = "$APRSA,15.0,A*FF\r\n";

/**
 * @brief Invalid RSA sentence - out of range rudder angle
 *
 * Sentence: $APRSA,120.0,A*XX (120° exceeds ±90° limit)
 * - Expected behavior: Parser succeeds, handler rejects (FR-026)
 */
const char* INVALID_APRSA_RANGE = "$APRSA,120.0,A*25\r\n";

/**
 * @brief Invalid RSA sentence - wrong talker ID
 *
 * Sentence: $VHRSA,15.0,A*XX (talker ID "VH" not "AP")
 * - Expected behavior: Parser returns false, silent ignore
 */
const char* INVALID_APRSA_TALKER = "$VHRSA,15.0,A*07\r\n";

/**
 * @brief Invalid RSA sentence - invalid status
 *
 * Sentence: $APRSA,15.0,V*XX (status 'V' = invalid)
 * - Expected behavior: Parser returns false, silent discard
 */
const char* INVALID_APRSA_STATUS = "$APRSA,15.0,V*3E\r\n";

/**
 * @brief Invalid GGA sentence - wrong talker ID
 *
 * Sentence: $GPGGA,... (talker ID "GP" not "VH")
 * - Expected behavior: Handler ignores (wrong talker ID filter)
 */
const char* INVALID_GGA_TALKER = "$GPGGA,123519,5230.5000,N,00507.0000,E,1,08,0.9,545.4,M,46.9,M,,*42\r\n";

/**
 * @brief Invalid VTG sentence - variation out of range
 *
 * Sentence: $VHVTG,054.7,T,090.0,M,5.5,N,10.2,K*XX
 * - Calculated variation: 054.7 - 090.0 = -35.3° (exceeds ±30° limit)
 * - Expected behavior: Handler rejects (FR-026)
 */
const char* INVALID_VTG_VARIATION = "$VHVTG,054.7,T,090.0,M,5.5,N,10.2,K*42\r\n";

/**
 * @brief Malformed sentence - non-numeric angle
 *
 * Sentence: $APRSA,INVALID,A*XX (angle field is text not number)
 * - Expected behavior: Parser returns false or extracts 0.0, silent discard
 */
const char* MALFORMED_APRSA = "$APRSA,INVALID,A*39\r\n";

/**
 * @brief Unsupported sentence type - MWV (wind data)
 *
 * Sentence: $APMWV,045.0,R,5.5,N,A*XX
 * - Message type: MWV (not in supported list)
 * - Expected behavior: Silent ignore (FR-007)
 */
const char* UNSUPPORTED_APMWV = "$APMWV,045.0,R,5.5,N,A*3E\r\n";

// ============================================================================
// Test Helper Functions
// ============================================================================

/**
 * @brief Calculate NMEA 0183 checksum for sentence
 *
 * Calculates XOR checksum of all characters between $ and * (exclusive).
 *
 * @param sentence Null-terminated NMEA sentence string
 * @return Checksum as unsigned char (0-255)
 */
inline unsigned char calculateNMEAChecksum(const char* sentence) {
    unsigned char checksum = 0;
    const char* ptr = sentence;

    // Skip leading $
    if (*ptr == '$') {
        ptr++;
    }

    // XOR all characters until * or end
    while (*ptr != '*' && *ptr != '\0' && *ptr != '\r' && *ptr != '\n') {
        checksum ^= static_cast<unsigned char>(*ptr);
        ptr++;
    }

    return checksum;
}

/**
 * @brief Verify NMEA 0183 sentence checksum
 *
 * Validates that sentence checksum matches calculated value.
 *
 * @param sentence Null-terminated NMEA sentence string with checksum
 * @return true if checksum valid, false otherwise
 */
inline bool verifyNMEAChecksum(const char* sentence) {
    const char* checksumStart = nullptr;

    // Find * character
    for (const char* ptr = sentence; *ptr != '\0'; ptr++) {
        if (*ptr == '*') {
            checksumStart = ptr + 1;
            break;
        }
    }

    if (checksumStart == nullptr) {
        return false;  // No checksum found
    }

    // Calculate expected checksum
    unsigned char calculated = calculateNMEAChecksum(sentence);

    // Parse checksum from sentence (hex string)
    unsigned int sentenceChecksum = 0;
    if (sscanf(checksumStart, "%2X", &sentenceChecksum) != 1) {
        return false;  // Invalid checksum format
    }

    return calculated == static_cast<unsigned char>(sentenceChecksum);
}

#endif // NMEA0183_TEST_FIXTURES_H
