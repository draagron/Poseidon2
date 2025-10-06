/**
 * @file WiFiManager.cpp
 * @brief Implementation of WiFi connection manager
 */

#include "WiFiManager.h"

WiFiManager::WiFiManager(IWiFiAdapter* wifi, IFileSystem* fs, DualLogger* log, TimeoutManager* timeout)
    : wifiAdapter(wifi),
      fileSystem(fs),
      logger(log),
      timeoutManager(timeout) {
}

// ============================================================================
// T029: loadConfig() Implementation
// ============================================================================

bool WiFiManager::loadConfig(WiFiConfigFile& config) {
    // Check if filesystem is available
    if (fileSystem == nullptr) {
        if (logger != nullptr) {
            logger->logConfigEvent(ConnectionEvent::CONFIG_LOADED, false, 0, "Filesystem not available");
        }
        return false;
    }

    // Check if config file exists
    if (!fileSystem->exists(CONFIG_FILE_PATH)) {
        if (logger != nullptr) {
            logger->logConfigEvent(ConnectionEvent::CONFIG_LOADED, false, 0, "Config file not found");
        }
        return false;
    }

    // Read file content
    String content = fileSystem->readFile(CONFIG_FILE_PATH);

    if (content.length() == 0) {
        if (logger != nullptr) {
            logger->logConfigEvent(ConnectionEvent::CONFIG_LOADED, false, 0, "Config file empty");
        }
        return false;
    }

    // Parse file content
    bool result = parser.parseFile(content, config);

    if (result) {
        // Log success
        if (logger != nullptr) {
            logger->logConfigEvent(ConnectionEvent::CONFIG_LOADED, true, config.count);
        }
    } else {
        // Log failure
        if (logger != nullptr) {
            String error = "Parse error: ";
            error += parser.getLastError();
            logger->logConfigEvent(ConnectionEvent::CONFIG_LOADED, false, 0, error);
        }
    }

    return result;
}

// ============================================================================
// T030: saveConfig() Implementation
// ============================================================================

bool WiFiManager::saveConfig(const WiFiConfigFile& config) {
    // Validate config before saving
    if (config.isEmpty()) {
        if (logger != nullptr) {
            logger->logConfigEvent(ConnectionEvent::CONFIG_SAVED, false, 0, "Config is empty");
        }
        return false;
    }

    // Validate all networks in config
    for (int i = 0; i < config.count; i++) {
        if (!config.networks[i].isValid()) {
            if (logger != nullptr) {
                String error = "Invalid network at index ";
                error += String(i);
                error += ": ";
                error += config.networks[i].getValidationError();
                logger->logConfigEvent(ConnectionEvent::CONFIG_SAVED, false, config.count, error);
            }
            return false;
        }
    }

    // Convert config to plain text
    String content = config.toPlainText();

    // Write to filesystem
    if (fileSystem == nullptr) {
        if (logger != nullptr) {
            logger->logConfigEvent(ConnectionEvent::CONFIG_SAVED, false, config.count, "Filesystem not available");
        }
        return false;
    }

    bool result = fileSystem->writeFile(CONFIG_FILE_PATH, content.c_str());

    if (result) {
        // Log success
        if (logger != nullptr) {
            logger->logConfigEvent(ConnectionEvent::CONFIG_SAVED, true, config.count);
        }
    } else {
        // Log failure
        if (logger != nullptr) {
            logger->logConfigEvent(ConnectionEvent::CONFIG_SAVED, false, config.count, "Write failed");
        }
    }

    return result;
}

// ============================================================================
// T031: connect() Implementation
// ============================================================================

bool WiFiManager::connect(WiFiConnectionState& state, const WiFiConfigFile& config) {
    // Get current network credentials
    WiFiCredentials* creds = stateMachine.getCurrentNetwork(state, const_cast<WiFiConfigFile&>(config));

    if (creds == nullptr) {
        // No valid network at current index
        if (logger != nullptr) {
            logger->broadcastLog(LogLevel::ERROR, "WiFiManager", "CONNECT_FAILED", "{\"reason\":\"No network at index\"}");
        }
        return false;
    }

    // Transition state to CONNECTING
    if (!stateMachine.transition(state, ConnectionStatus::CONNECTING)) {
        // Invalid state transition
        if (logger != nullptr) {
            logger->broadcastLog(LogLevel::ERROR, "WiFiManager", "CONNECT_FAILED", "{\"reason\":\"Invalid state transition\"}");
        }
        return false;
    }

    // Call WiFi adapter to begin connection
    if (wifiAdapter == nullptr) {
        if (logger != nullptr) {
            logger->broadcastLog(LogLevel::ERROR, "WiFiManager", "CONNECT_FAILED", "{\"reason\":\"WiFi adapter not available\"}");
        }
        return false;
    }

    bool result = wifiAdapter->begin(creds->ssid.c_str(), creds->password.c_str());

    if (result) {
        // Log connection attempt
        if (logger != nullptr) {
            logger->logConnectionEvent(ConnectionEvent::CONNECTION_ATTEMPT, creds->ssid, state.retryCount + 1, WIFI_TIMEOUT_MS / 1000);
        }

        // Register timeout callback
        if (timeoutManager != nullptr) {
            timeoutManager->registerTimeout(WIFI_TIMEOUT_MS, [this, &state, &config]() {
                // Timeout callback - handled by checkTimeout()
            });
        }
    }

    return result;
}

// ============================================================================
// T032: checkTimeout() Implementation
// ============================================================================

bool WiFiManager::checkTimeout(WiFiConnectionState& state, WiFiConfigFile& config) {
    // Check if we should retry (timeout exceeded)
    if (!stateMachine.shouldRetry(state, WIFI_TIMEOUT_MS)) {
        return false; // Timeout not exceeded
    }

    // Timeout exceeded - transition to FAILED
    stateMachine.transition(state, ConnectionStatus::FAILED);

    // Log failure
    if (logger != nullptr) {
        WiFiCredentials* creds = stateMachine.getCurrentNetwork(state, config);
        if (creds != nullptr) {
            logger->logConnectionEvent(ConnectionEvent::CONNECTION_FAILED, creds->ssid, state.retryCount);
        }
    }

    // Try next network
    WiFiCredentials* nextNetwork = stateMachine.getNextNetwork(state, config);

    if (nextNetwork != nullptr) {
        // Connect to next network
        connect(state, config);
    } else {
        // All networks exhausted - schedule reboot
        if (logger != nullptr) {
            logger->logRebootEvent(REBOOT_DELAY_MS / 1000, "All networks failed");
        }

        // Note: Actual reboot will be handled by main loop
        // This just sets the state
        stateMachine.transition(state, ConnectionStatus::DISCONNECTED);
    }

    return true;
}

// ============================================================================
// T033: handleDisconnect() Implementation
// ============================================================================

void WiFiManager::handleDisconnect(WiFiConnectionState& state) {
    // Log disconnect event
    if (logger != nullptr) {
        logger->logConnectionEvent(ConnectionEvent::CONNECTION_LOST, state.connectedSSID);
    }

    // Transition to DISCONNECTED
    // Note: currentNetworkIndex is NOT changed - we retry the same network
    stateMachine.transition(state, ConnectionStatus::DISCONNECTED);
}

// ============================================================================
// T034: handleConnectionSuccess() Implementation
// ============================================================================

void WiFiManager::handleConnectionSuccess(WiFiConnectionState& state, const String& ssid) {
    // Cancel timeout
    if (timeoutManager != nullptr) {
        timeoutManager->cancelTimeout();
    }

    // Transition to CONNECTED
    stateMachine.transition(state, ConnectionStatus::CONNECTED, ssid);

    // Log success
    if (logger != nullptr) {
        logger->logConnectionEvent(ConnectionEvent::CONNECTION_SUCCESS, ssid);
    }
}
