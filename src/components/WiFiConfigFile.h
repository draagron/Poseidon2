/**
 * @file WiFiConfigFile.h
 * @brief WiFi configuration file data structure
 *
 * Manages a collection of up to 3 WiFi network credentials with priority ordering.
 * Supports serialization to/from plain text format for persistent storage.
 *
 * File format (plain text, comma-separated):
 *   SSID1,password1
 *   SSID2,password2
 *   SSID3,
 *
 * Memory footprint: ~289 bytes per instance
 */

#ifndef WIFI_CONFIG_FILE_H
#define WIFI_CONFIG_FILE_H

#include <Arduino.h>
#include "WiFiCredentials.h"
#include "../config.h"

/**
 * @brief WiFi configuration file structure
 *
 * Stores up to MAX_NETWORKS (3) WiFi credentials with priority ordering.
 * Index 0 = highest priority, index 2 = lowest priority.
 */
struct WiFiConfigFile {
    WiFiCredentials networks[MAX_NETWORKS]; ///< Array of network credentials
    int count;                              ///< Number of valid networks (0-3)

    /**
     * @brief Default constructor
     */
    WiFiConfigFile() : count(0) {}

    /**
     * @brief Copy constructor
     * @param other WiFiConfigFile to copy from
     */
    WiFiConfigFile(const WiFiConfigFile& other) : count(other.count) {
        for (int i = 0; i < count; i++) {
            networks[i] = other.networks[i];
        }
    }

    /**
     * @brief Assignment operator
     * @param other WiFiConfigFile to assign from
     * @return Reference to this object
     */
    WiFiConfigFile& operator=(const WiFiConfigFile& other) {
        if (this != &other) {
            count = other.count;
            for (int i = 0; i < count; i++) {
                networks[i] = other.networks[i];
            }
        }
        return *this;
    }

    /**
     * @brief Check if config file is empty
     * @return true if no networks configured
     */
    bool isEmpty() const {
        return count == 0;
    }

    /**
     * @brief Check if config file is full (3 networks)
     * @return true if at maximum capacity
     */
    bool isFull() const {
        return count >= MAX_NETWORKS;
    }

    /**
     * @brief Add a network to the configuration
     * @param creds WiFi credentials to add
     * @return true if added successfully, false if full or invalid
     */
    bool addNetwork(const WiFiCredentials& creds) {
        if (isFull()) {
            return false; // Already at max capacity
        }

        if (!creds.isValid()) {
            return false; // Invalid credentials
        }

        networks[count] = creds;
        count++;
        return true;
    }

    /**
     * @brief Get network credentials by index
     * @param index Network index (0-2)
     * @return Pointer to credentials, or nullptr if invalid index
     */
    WiFiCredentials* getNetwork(int index) {
        if (index < 0 || index >= count) {
            return nullptr;
        }
        return &networks[index];
    }

    /**
     * @brief Get network credentials by index (const version)
     * @param index Network index (0-2)
     * @return Pointer to credentials, or nullptr if invalid index
     */
    const WiFiCredentials* getNetwork(int index) const {
        if (index < 0 || index >= count) {
            return nullptr;
        }
        return &networks[index];
    }

    /**
     * @brief Remove network by index
     * @param index Network index to remove (0-2)
     * @return true if removed successfully
     */
    bool removeNetwork(int index) {
        if (index < 0 || index >= count) {
            return false;
        }

        // Shift remaining networks down
        for (int i = index; i < count - 1; i++) {
            networks[i] = networks[i + 1];
        }

        count--;
        return true;
    }

    /**
     * @brief Clear all networks from configuration
     */
    void clear() {
        count = 0;
        // Clear network data
        for (int i = 0; i < MAX_NETWORKS; i++) {
            networks[i] = WiFiCredentials();
        }
    }

    /**
     * @brief Serialize configuration to plain text format
     * @return Plain text string (line-separated SSID,password)
     *
     * Format:
     *   SSID1,password1\n
     *   SSID2,password2\n
     *   SSID3,\n
     */
    String toPlainText() const {
        String result = "";

        for (int i = 0; i < count; i++) {
            result += networks[i].ssid;
            result += ",";
            result += networks[i].password;
            result += "\n";
        }

        return result;
    }

    /**
     * @brief Parse configuration from plain text format
     * @param plainText Plain text string to parse
     * @return true if at least one valid network was parsed
     *
     * Behavior:
     * - Invalid lines are skipped
     * - Valid lines are parsed and added
     * - Maximum 3 networks (truncates if more)
     * - Returns true if ANY valid networks parsed
     */
    bool fromPlainText(const String& plainText) {
        clear();

        int lineStart = 0;
        int lineEnd = 0;
        int linesParsed = 0;

        while (lineStart < plainText.length() && linesParsed < MAX_NETWORKS) {
            // Find end of line
            lineEnd = plainText.indexOf('\n', lineStart);
            if (lineEnd == -1) {
                lineEnd = plainText.length();
            }

            // Extract line
            String line = plainText.substring(lineStart, lineEnd);
            line.trim();

            // Parse line if not empty
            if (line.length() > 0) {
                int commaPos = line.indexOf(',');
                if (commaPos > 0) {
                    String ssid = line.substring(0, commaPos);
                    String password = line.substring(commaPos + 1);

                    WiFiCredentials creds(ssid, password);
                    if (creds.isValid()) {
                        addNetwork(creds);
                        linesParsed++;
                    }
                    // Invalid credentials are silently skipped
                }
                // Lines without comma are silently skipped
            }

            lineStart = lineEnd + 1;
        }

        return count > 0; // Success if at least one network parsed
    }
};

#endif // WIFI_CONFIG_FILE_H
