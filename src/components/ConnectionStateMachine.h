/**
 * @file ConnectionStateMachine.h
 * @brief Connection state machine for WiFi management
 *
 * Manages WiFi connection state transitions and network failover logic.
 * Enforces valid state transitions and handles timeout detection.
 *
 * State Flow:
 *   DISCONNECTED → CONNECTING → CONNECTED
 *                ↓ (timeout)
 *              FAILED → CONNECTING (next network)
 *                     ↓ (all failed)
 *                  DISCONNECTED (reboot)
 */

#ifndef CONNECTION_STATE_MACHINE_H
#define CONNECTION_STATE_MACHINE_H

#include <Arduino.h>
#include "WiFiConnectionState.h"
#include "WiFiConfigFile.h"
#include "WiFiCredentials.h"

/**
 * @brief Connection state machine class
 *
 * Stateless class that operates on WiFiConnectionState structures.
 * All methods are const - state is passed by reference and modified.
 */
class ConnectionStateMachine {
public:
    /**
     * @brief Constructor
     */
    ConnectionStateMachine();

    /**
     * @brief Transition to a new connection state
     * @param state Connection state to update
     * @param newStatus Target status
     * @param ssid SSID to set when transitioning to CONNECTED (optional)
     * @return true if transition is valid and completed
     *
     * Automatically updates timestamps and retry counts based on transition.
     * Validates state transitions before applying.
     */
    bool transition(WiFiConnectionState& state, ConnectionStatus newStatus, const String& ssid = "");

    /**
     * @brief Check if connection attempt should be retried (timeout check)
     * @param state Current connection state
     * @param timeoutMs Timeout threshold in milliseconds
     * @return true if timeout exceeded
     *
     * Uses millis() - attemptStartTime to calculate elapsed time.
     */
    bool shouldRetry(const WiFiConnectionState& state, unsigned long timeoutMs) const;

    /**
     * @brief Get next network to try after failure
     * @param state Connection state to update (increments currentNetworkIndex)
     * @param config WiFi configuration with network list
     * @return Pointer to next network credentials, or nullptr if all exhausted
     *
     * Side effect: Increments state.currentNetworkIndex
     */
    WiFiCredentials* getNextNetwork(WiFiConnectionState& state, WiFiConfigFile& config);

    /**
     * @brief Get current network credentials
     * @param state Current connection state
     * @param config WiFi configuration with network list
     * @return Pointer to current network credentials, or nullptr if invalid index
     */
    WiFiCredentials* getCurrentNetwork(const WiFiConnectionState& state, WiFiConfigFile& config) const;

    /**
     * @brief Check if device should reboot (all networks exhausted)
     * @param state Current connection state
     * @param config WiFi configuration with network list
     * @return true if all networks have been tried and failed
     */
    bool shouldReboot(const WiFiConnectionState& state, const WiFiConfigFile& config) const;

    /**
     * @brief Reset state machine for reboot
     * @param state Connection state to reset
     *
     * Resets to initial state (DISCONNECTED, index 0, retry count 0)
     */
    void resetForReboot(WiFiConnectionState& state);
};

#endif // CONNECTION_STATE_MACHINE_H
