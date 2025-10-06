/**
 * @file WiFiManager.h
 * @brief WiFi connection manager
 *
 * Orchestrates WiFi connection management using HAL interfaces.
 * Handles config loading/saving, connection attempts, timeout detection,
 * and network failover.
 *
 * Dependencies are injected via constructor for testability.
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include "../hal/interfaces/IWiFiAdapter.h"
#include "../hal/interfaces/IFileSystem.h"
#include "WiFiConfigFile.h"
#include "WiFiConnectionState.h"
#include "ConfigParser.h"
#include "ConnectionStateMachine.h"
#include "../utils/DualLogger.h"
#include "../utils/TimeoutManager.h"
#include "../config.h"

/**
 * @brief WiFi connection manager class
 *
 * Main orchestrator for WiFi connectivity. Uses dependency injection
 * for hardware abstraction (testability via mocks).
 */
class WiFiManager {
private:
    IWiFiAdapter* wifiAdapter;
    IFileSystem* fileSystem;
    ConfigParser parser;
    ConnectionStateMachine stateMachine;
    DualLogger* logger;
    TimeoutManager* timeoutManager;

public:
    /**
     * @brief Constructor with dependency injection
     * @param wifi WiFi adapter interface
     * @param fs Filesystem interface
     * @param log Dual logger (optional, can be null for testing)
     * @param timeout Timeout manager (optional, can be null for testing)
     */
    WiFiManager(IWiFiAdapter* wifi, IFileSystem* fs, DualLogger* log = nullptr, TimeoutManager* timeout = nullptr);

    /**
     * @brief Load WiFi configuration from persistent storage
     * @param config Output configuration structure
     * @return true if config loaded successfully
     *
     * Reads CONFIG_FILE_PATH from filesystem and parses plain text format.
     * Logs errors via UDP if parsing fails.
     */
    bool loadConfig(WiFiConfigFile& config);

    /**
     * @brief Save WiFi configuration to persistent storage
     * @param config Configuration to save
     * @return true if config saved successfully
     *
     * Validates config before writing. Writes to CONFIG_FILE_PATH.
     * Logs success/failure via UDP.
     */
    bool saveConfig(const WiFiConfigFile& config);

    /**
     * @brief Attempt to connect to current network
     * @param state Connection state to update
     * @param config WiFi configuration
     * @return true if connection attempt initiated
     *
     * Uses state.currentNetworkIndex to select network.
     * Calls WiFi adapter begin() and transitions state to CONNECTING.
     * Registers timeout callback via TimeoutManager.
     */
    bool connect(WiFiConnectionState& state, const WiFiConfigFile& config);

    /**
     * @brief Check if connection attempt has timed out
     * @param state Current connection state
     * @param config WiFi configuration
     * @return true if timeout handling performed
     *
     * Called periodically from ReactESP event loop.
     * Moves to next network on timeout, schedules reboot if all exhausted.
     */
    bool checkTimeout(WiFiConnectionState& state, WiFiConfigFile& config);

    /**
     * @brief Handle WiFi disconnect event
     * @param state Connection state to update
     *
     * Called when WiFi disconnect event received.
     * Sets state to DISCONNECTED, keeps currentIndex unchanged (retry logic).
     * Logs disconnect event via UDP.
     */
    void handleDisconnect(WiFiConnectionState& state);

    /**
     * @brief Handle successful WiFi connection event
     * @param state Connection state to update
     * @param ssid SSID of connected network
     *
     * Called when WiFi connection succeeds.
     * Transitions state to CONNECTED, cancels timeout.
     * Logs success via UDP.
     */
    void handleConnectionSuccess(WiFiConnectionState& state, const String& ssid);
};

#endif // WIFI_MANAGER_H
