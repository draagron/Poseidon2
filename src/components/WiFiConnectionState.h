/**
 * @file WiFiConnectionState.h
 * @brief WiFi connection state machine
 *
 * Tracks current WiFi connection state and manages state transitions.
 * Enforces valid state transitions and tracks connection attempts.
 *
 * State transition diagram:
 *   DISCONNECTED → CONNECTING → CONNECTED
 *                ↓ (timeout)
 *              FAILED → CONNECTING (retry)
 *                     ↓ (all networks exhausted)
 *                  DISCONNECTED (reboot)
 *
 * Memory footprint: ~45 bytes per instance
 */

#ifndef WIFI_CONNECTION_STATE_H
#define WIFI_CONNECTION_STATE_H

#include <Arduino.h>

/**
 * @brief WiFi connection status enumeration
 */
enum class ConnectionStatus {
    DISCONNECTED = 0,  ///< Not connected, no attempt in progress
    CONNECTING = 1,    ///< Connection attempt in progress
    CONNECTED = 2,     ///< Successfully connected to network
    FAILED = 3         ///< Connection attempt failed (timeout/auth)
};

/**
 * @brief WiFi connection state structure
 *
 * Runtime state for WiFi connection management.
 * Not persisted to flash - recreated on each boot.
 */
struct WiFiConnectionState {
    ConnectionStatus status;        ///< Current connection status
    int currentNetworkIndex;        ///< Index of network being attempted (0-2)
    unsigned long attemptStartTime; ///< When current attempt started (millis)
    String connectedSSID;           ///< SSID of connected network (empty if not connected)
    int retryCount;                 ///< Number of retry attempts

    /**
     * @brief Default constructor - initializes to DISCONNECTED
     */
    WiFiConnectionState()
        : status(ConnectionStatus::DISCONNECTED),
          currentNetworkIndex(0),
          attemptStartTime(0),
          connectedSSID(""),
          retryCount(0) {}

    /**
     * @brief Copy constructor
     * @param other WiFiConnectionState to copy from
     */
    WiFiConnectionState(const WiFiConnectionState& other)
        : status(other.status),
          currentNetworkIndex(other.currentNetworkIndex),
          attemptStartTime(other.attemptStartTime),
          connectedSSID(other.connectedSSID),
          retryCount(other.retryCount) {}

    /**
     * @brief Assignment operator
     * @param other WiFiConnectionState to assign from
     * @return Reference to this object
     */
    WiFiConnectionState& operator=(const WiFiConnectionState& other) {
        if (this != &other) {
            status = other.status;
            currentNetworkIndex = other.currentNetworkIndex;
            attemptStartTime = other.attemptStartTime;
            connectedSSID = other.connectedSSID;
            retryCount = other.retryCount;
        }
        return *this;
    }

    /**
     * @brief Transition to a new state
     * @param newStatus Target status
     * @param ssid SSID to set when transitioning to CONNECTED (optional)
     * @return true if transition is valid and completed
     *
     * Valid transitions:
     * - DISCONNECTED → CONNECTING
     * - CONNECTING → CONNECTED
     * - CONNECTING → FAILED
     * - FAILED → CONNECTING
     * - CONNECTED → DISCONNECTED
     */
    bool transitionTo(ConnectionStatus newStatus, const String& ssid = "") {
        // Validate transition
        if (!isValidTransition(status, newStatus)) {
            return false;
        }

        // Execute transition
        status = newStatus;

        switch (newStatus) {
            case ConnectionStatus::CONNECTING:
                attemptStartTime = millis();
                break;

            case ConnectionStatus::CONNECTED:
                connectedSSID = ssid;
                retryCount = 0; // Reset retry count on success
                break;

            case ConnectionStatus::FAILED:
                retryCount++;
                break;

            case ConnectionStatus::DISCONNECTED:
                connectedSSID = "";
                break;
        }

        return true;
    }

    /**
     * @brief Move to next network in configuration
     */
    void moveToNextNetwork() {
        currentNetworkIndex++;
    }

    /**
     * @brief Check if all networks have been exhausted
     * @param totalNetworks Total number of configured networks
     * @return true if all networks tried
     */
    bool allNetworksExhausted(int totalNetworks) const {
        return currentNetworkIndex >= totalNetworks;
    }

    /**
     * @brief Reset network index to 0 (after reboot)
     */
    void resetNetworkIndex() {
        currentNetworkIndex = 0;
    }

    /**
     * @brief Check if currently connected
     * @return true if status is CONNECTED
     */
    bool isConnected() const {
        return status == ConnectionStatus::CONNECTED;
    }

    /**
     * @brief Check if connection attempt in progress
     * @return true if status is CONNECTING
     */
    bool isConnecting() const {
        return status == ConnectionStatus::CONNECTING;
    }

    /**
     * @brief Get elapsed time since current attempt started
     * @return Elapsed time in milliseconds
     */
    unsigned long getElapsedTime() const {
        if (attemptStartTime == 0) {
            return 0;
        }
        return millis() - attemptStartTime;
    }

    /**
     * @brief Check if timeout has been exceeded
     * @param timeoutMs Timeout threshold in milliseconds
     * @return true if elapsed time exceeds timeout
     */
    bool isTimeoutExceeded(unsigned long timeoutMs) const {
        return getElapsedTime() >= timeoutMs;
    }

    /**
     * @brief Get status as human-readable string
     * @return Status string
     */
    String getStatusString() const {
        switch (status) {
            case ConnectionStatus::DISCONNECTED:
                return "DISCONNECTED";
            case ConnectionStatus::CONNECTING:
                return "CONNECTING";
            case ConnectionStatus::CONNECTED:
                return "CONNECTED";
            case ConnectionStatus::FAILED:
                return "FAILED";
            default:
                return "UNKNOWN";
        }
    }

private:
    /**
     * @brief Check if state transition is valid
     * @param from Current status
     * @param to Target status
     * @return true if transition is allowed
     */
    bool isValidTransition(ConnectionStatus from, ConnectionStatus to) const {
        // Same state is always valid (no-op)
        if (from == to) {
            return true;
        }

        switch (from) {
            case ConnectionStatus::DISCONNECTED:
                return to == ConnectionStatus::CONNECTING;

            case ConnectionStatus::CONNECTING:
                return to == ConnectionStatus::CONNECTED ||
                       to == ConnectionStatus::FAILED;

            case ConnectionStatus::FAILED:
                return to == ConnectionStatus::CONNECTING ||
                       to == ConnectionStatus::DISCONNECTED;

            case ConnectionStatus::CONNECTED:
                return to == ConnectionStatus::DISCONNECTED;

            default:
                return false;
        }
    }
};

#endif // WIFI_CONNECTION_STATE_H
