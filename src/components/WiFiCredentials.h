/**
 * @file WiFiCredentials.h
 * @brief WiFi network credentials data structure
 *
 * Represents a single WiFi network with SSID and password.
 * Validates credentials according to WiFi specification:
 * - SSID: 1-32 characters, non-empty
 * - Password: 0 (open network) or 8-63 characters (WPA2)
 *
 * Memory footprint: ~95 bytes per instance
 */

#ifndef WIFI_CREDENTIALS_H
#define WIFI_CREDENTIALS_H

#include <Arduino.h>

/**
 * @brief WiFi network credentials structure
 *
 * Stores SSID and password for a single WiFi network with validation.
 */
struct WiFiCredentials {
    String ssid;        ///< Network SSID (1-32 characters)
    String password;    ///< WPA2 password (0 or 8-63 characters)

    /**
     * @brief Default constructor
     */
    WiFiCredentials() : ssid(""), password("") {}

    /**
     * @brief Constructor with parameters
     * @param ssid Network SSID
     * @param password Network password
     */
    WiFiCredentials(const char* ssid, const char* password)
        : ssid(ssid), password(password) {}

    /**
     * @brief Constructor with String parameters
     * @param ssid Network SSID
     * @param password Network password
     */
    WiFiCredentials(const String& ssid, const String& password)
        : ssid(ssid), password(password) {}

    /**
     * @brief Copy constructor
     * @param other WiFiCredentials to copy from
     */
    WiFiCredentials(const WiFiCredentials& other)
        : ssid(other.ssid), password(other.password) {}

    /**
     * @brief Assignment operator
     * @param other WiFiCredentials to assign from
     * @return Reference to this object
     */
    WiFiCredentials& operator=(const WiFiCredentials& other) {
        if (this != &other) {
            ssid = other.ssid;
            password = other.password;
        }
        return *this;
    }

    /**
     * @brief Validate credentials according to WiFi specification
     * @return true if credentials are valid
     *
     * Validation rules:
     * - SSID must not be empty
     * - SSID length must be 1-32 characters
     * - SSID must not contain newline characters
     * - Password must be 0 (open) or 8-63 characters (WPA2)
     * - Password must not contain newline characters
     */
    bool isValid() const {
        // Check SSID
        if (ssid.length() == 0) {
            return false; // SSID cannot be empty
        }

        if (ssid.length() > 32) {
            return false; // SSID max length is 32 chars
        }

        if (ssid.indexOf('\n') >= 0 || ssid.indexOf('\r') >= 0) {
            return false; // SSID cannot contain newlines
        }

        // Check password
        size_t passLen = password.length();

        if (passLen > 0 && passLen < 8) {
            return false; // Password must be 0 (open) or >= 8 (WPA2)
        }

        if (passLen > 63) {
            return false; // Password max length is 63 chars
        }

        if (password.indexOf('\n') >= 0 || password.indexOf('\r') >= 0) {
            return false; // Password cannot contain newlines
        }

        return true;
    }

    /**
     * @brief Check if this represents an open network (no password)
     * @return true if password is empty
     */
    bool isOpenNetwork() const {
        return password.length() == 0;
    }

    /**
     * @brief Get validation error message if credentials are invalid
     * @return Error message string (empty if valid)
     */
    String getValidationError() const {
        if (ssid.length() == 0) {
            return F("SSID cannot be empty");
        }

        if (ssid.length() > 32) {
            return F("SSID exceeds 32 characters");
        }

        if (ssid.indexOf('\n') >= 0 || ssid.indexOf('\r') >= 0) {
            return F("SSID cannot contain newline characters");
        }

        size_t passLen = password.length();

        if (passLen > 0 && passLen < 8) {
            return F("Password must be 8-63 characters for WPA2");
        }

        if (passLen > 63) {
            return F("Password exceeds 63 characters");
        }

        if (password.indexOf('\n') >= 0 || password.indexOf('\r') >= 0) {
            return F("Password cannot contain newline characters");
        }

        return "";
    }
};

#endif // WIFI_CREDENTIALS_H
