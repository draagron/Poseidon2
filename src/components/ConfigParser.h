/**
 * @file ConfigParser.h
 * @brief WiFi configuration file parser
 *
 * Parses plain text configuration files in comma-separated format.
 * Performs defensive parsing - invalid lines are skipped, valid lines preserved.
 *
 * File format:
 *   SSID1,password1
 *   SSID2,password2
 *   SSID3,
 *
 * Maximum 3 lines enforced. Lines exceeding limit are ignored.
 */

#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <Arduino.h>
#include "WiFiCredentials.h"
#include "WiFiConfigFile.h"
#include "../config.h"

/**
 * @brief Configuration file parser class
 *
 * Stateful parser that tracks errors encountered during parsing.
 * Use reset() to clear error state between parse operations.
 */
class ConfigParser {
private:
    String lastError;    ///< Last error message encountered
    int errorCount;      ///< Number of errors encountered in last parse

public:
    /**
     * @brief Constructor
     */
    ConfigParser();

    /**
     * @brief Parse a single line of configuration
     * @param line Line to parse (format: "SSID,password")
     * @param creds Output credentials structure
     * @return true if line parsed successfully and credentials are valid
     *
     * Behavior:
     * - Splits line on first comma
     * - Trims whitespace from SSID and password
     * - Validates SSID length (1-32 chars)
     * - Validates password length (0 or 8-63 chars)
     * - Sets lastError on failure
     */
    bool parseLine(const String& line, WiFiCredentials& creds);

    /**
     * @brief Parse entire configuration file content
     * @param fileContent File content as string
     * @param config Output configuration structure
     * @return true if at least one valid network was parsed
     *
     * Behavior:
     * - Processes max 3 lines
     * - Invalid lines are skipped (logged as errors)
     * - Valid lines are added to config
     * - Returns true if ANY valid networks found
     * - Handles both LF and CRLF line endings
     */
    bool parseFile(const String& fileContent, WiFiConfigFile& config);

    /**
     * @brief Get last error message
     * @return Error message string (empty if no error)
     */
    String getLastError() const;

    /**
     * @brief Get number of errors encountered in last parse operation
     * @return Error count
     */
    int getErrorCount() const;

    /**
     * @brief Reset parser state (clear errors)
     */
    void reset();

private:
    /**
     * @brief Trim whitespace from both ends of string
     * @param str String to trim
     * @return Trimmed string
     */
    String trimWhitespace(const String& str) const;
};

#endif // CONFIG_PARSER_H
