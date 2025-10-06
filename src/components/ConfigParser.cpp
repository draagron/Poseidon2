/**
 * @file ConfigParser.cpp
 * @brief Implementation of WiFi configuration file parser
 */

#include "ConfigParser.h"

ConfigParser::ConfigParser() : lastError(""), errorCount(0) {
}

bool ConfigParser::parseLine(const String& line, WiFiCredentials& creds) {
    // Trim whitespace from entire line
    String trimmedLine = trimWhitespace(line);

    // Reject empty lines
    if (trimmedLine.length() == 0) {
        lastError = F("Empty line");
        errorCount++;
        return false;
    }

    // Find comma separator
    int commaPos = trimmedLine.indexOf(',');
    if (commaPos < 0) {
        lastError = F("Missing comma separator");
        errorCount++;
        return false;
    }

    // Split on first comma
    String ssid = trimmedLine.substring(0, commaPos);
    String password = trimmedLine.substring(commaPos + 1);

    // Trim whitespace from SSID and password
    ssid = trimWhitespace(ssid);
    password = trimWhitespace(password);

    // Create credentials
    creds.ssid = ssid;
    creds.password = password;

    // Validate credentials
    if (!creds.isValid()) {
        lastError = creds.getValidationError();
        errorCount++;
        return false;
    }

    // Clear error on success
    lastError = "";
    return true;
}

bool ConfigParser::parseFile(const String& fileContent, WiFiConfigFile& config) {
    // Reset state
    reset();
    config.clear();

    int lineStart = 0;
    int lineEnd = 0;
    int linesParsed = 0;
    int validNetworks = 0;

    // Process lines until end of file or max networks reached
    while (lineStart < fileContent.length() && validNetworks < MAX_NETWORKS) {
        // Find end of line (handle both LF and CRLF)
        lineEnd = fileContent.indexOf('\n', lineStart);
        if (lineEnd == -1) {
            lineEnd = fileContent.length();
        }

        // Extract line
        String line = fileContent.substring(lineStart, lineEnd);

        // Remove trailing CR if present (CRLF line ending)
        if (line.length() > 0 && line.charAt(line.length() - 1) == '\r') {
            line = line.substring(0, line.length() - 1);
        }

        // Trim whitespace
        line = trimWhitespace(line);

        // Parse line if not empty
        if (line.length() > 0) {
            WiFiCredentials creds;

            // Store current error count
            int prevErrorCount = errorCount;

            if (parseLine(line, creds)) {
                // Valid line - add to config
                config.addNetwork(creds);
                validNetworks++;
            }
            // Invalid lines are silently skipped
            // (error already logged by parseLine)

            linesParsed++;
        }

        // Move to next line
        lineStart = lineEnd + 1;
    }

    // Return true if at least one valid network was parsed
    return validNetworks > 0;
}

String ConfigParser::getLastError() const {
    return lastError;
}

int ConfigParser::getErrorCount() const {
    return errorCount;
}

void ConfigParser::reset() {
    lastError = "";
    errorCount = 0;
}

String ConfigParser::trimWhitespace(const String& str) const {
    if (str.length() == 0) {
        return str;
    }

    int start = 0;
    int end = str.length() - 1;

    // Find first non-whitespace character
    while (start <= end && isspace(str.charAt(start))) {
        start++;
    }

    // Find last non-whitespace character
    while (end >= start && isspace(str.charAt(end))) {
        end--;
    }

    // Return substring
    if (start > end) {
        return ""; // All whitespace
    }

    return str.substring(start, end + 1);
}
