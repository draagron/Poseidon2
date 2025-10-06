/**
 * @file main.cpp
 * @brief Main application entry point for Poseidon2 WiFi Gateway
 *
 * Implements WiFi network management foundation with:
 * - Automatic WiFi connection with priority-ordered failover
 * - LittleFS persistent configuration storage
 * - HTTP API for configuration management
 * - ReactESP event-driven architecture
 * - WebSocket logging for debugging
 *
 * Hardware: ESP32 (SH-ESP32 board)
 */



#include <Arduino.h>
#include <ReactESP.h>
#include <WiFi.h>

// HAL implementations
#include "hal/implementations/ESP32WiFiAdapter.h"
#include "hal/implementations/LittleFSAdapter.h"

// Components
#include "components/WiFiManager.h"
#include "components/WiFiConfigFile.h"
#include "components/WiFiConnectionState.h"
#include "components/ConfigWebServer.h"

// Utilities
#include "utils/WebSocketLogger.h"
#include "utils/TimeoutManager.h"

// Configuration
#include "config.h"

using namespace reactesp;

// Prevent main.cpp globals and functions from being compiled during unit tests
#ifndef UNIT_TEST

// Global ReactESP application
ReactESP app;

// HAL instances (hardware adapters) - initialized in setup() to avoid global constructor issues
ESP32WiFiAdapter* wifiAdapter = nullptr;
LittleFSAdapter* fileSystem = nullptr;

// Core components
WebSocketLogger logger;
TimeoutManager timeoutManager;
WiFiManager* wifiManager = nullptr;
ConfigWebServer* webServer = nullptr;

// WiFi state
WiFiConfigFile wifiConfig;
WiFiConnectionState connectionState;

// Reboot management
bool rebootScheduled = false;
unsigned long rebootTime = 0;

/**
 * @brief Handle WiFi connection success event
 *
 * Called when ESP32 WiFi connects successfully.
 * Updates connection state and starts web server.
 */
void onWiFiConnected() {
    String ssid = wifiAdapter->getSSID();
    String ip = wifiAdapter->getIPAddress();

    // Update connection state
    wifiManager->handleConnectionSuccess(connectionState, ssid);

    // Log connection success
    logger.logConnectionEvent(ConnectionEvent::CONNECTION_SUCCESS, ssid);

    // Start web server if not already running
    if (webServer == nullptr) {
        webServer = new ConfigWebServer(wifiManager, &wifiConfig, &connectionState);
        webServer->setupRoutes();
        webServer->begin();

        // Attach WebSocket logger to web server for reliable logging
        logger.begin(webServer->getServer(), "/logs");

        logger.broadcastLog(LogLevel::INFO, "WebServer", "STARTED",
            String("{\"ip\":\"") + ip + "\",\"port\":80}");
    }
}

/**
 * @brief Handle WiFi disconnection event
 *
 * Called when ESP32 WiFi disconnects.
 * Triggers retry logic for same network (no failover).
 */
void onWiFiDisconnected() {
    wifiManager->handleDisconnect(connectionState);

    // Attempt to reconnect to same network
    app.onDelay(1000, [&]() {
        if (connectionState.status == ConnectionStatus::DISCONNECTED) {
            wifiManager->connect(connectionState, wifiConfig);
        }
    });
}

/**
 * @brief Broadcast keep-alive message
 *
 * Sends periodic heartbeat message via UDP broadcast every 5 seconds.
 * Only broadcasts when WiFi is connected.
 */
void broadcastKeepAlive() {
    if (connectionState.status == ConnectionStatus::CONNECTED) {
        unsigned long uptime = millis() / 1000; // seconds
        logger.broadcastLog(LogLevel::INFO, "KeepAlive", "HEARTBEAT",
            String("{\"uptime\":") + uptime + ",\"ssid\":\"" + connectionState.connectedSSID + "\"}");
    }
}

/**
 * @brief Check for scheduled reboot
 *
 * Called periodically from main loop to execute scheduled reboots.
 */
void checkScheduledReboot() {
    if (rebootScheduled && millis() >= rebootTime) {
        logger.logRebootEvent(0, F("All networks exhausted - rebooting"));
        delay(100); // Allow UDP packet to send
        ESP.restart();
    }

    // Also check web server scheduled reboots
    if (webServer != nullptr && webServer->shouldReboot()) {
        logger.logRebootEvent(0, F("Configuration updated - rebooting"));
        delay(100); // Allow UDP packet to send
        ESP.restart();
    }
}

/**
 * @brief Schedule device reboot after specified delay
 * @param delayMs Delay in milliseconds before reboot
 * @param reason Reason for reboot (for logging)
 */
void scheduleReboot(unsigned long delayMs, const String& reason) {
    rebootScheduled = true;
    rebootTime = millis() + delayMs;

    logger.logRebootEvent(delayMs / 1000, reason);
}

/**
 * @brief Check WiFi connection timeout
 *
 * Called periodically to detect connection timeouts.
 * Moves to next network on timeout, schedules reboot if all exhausted.
 */
void checkConnectionTimeout() {
    if (timeoutManager.checkTimeout()) {
        // Timeout occurred, handled by timeout callback
        if (wifiManager->checkTimeout(connectionState, wifiConfig)) {
            // checkTimeout returns true if should try next network
            wifiManager->connect(connectionState, wifiConfig);
        } else {
            // All networks exhausted, schedule reboot
            scheduleReboot(REBOOT_DELAY_MS, F("All networks failed"));
        }
    }
}

/**
 * @brief Setup function - runs once at boot
 *
 * Initialization sequence (constitutional requirement):
 * 1. Mount LittleFS filesystem
 * 2. Create HAL adapter instances
 * 3. Create WiFiManager with dependencies
 * 4. Register ReactESP event loops
 * 5. Load WiFi configuration
 * 6. Attempt first network connection
 * 7. Register WiFi event handlers
 */
void setup() {
    // Initialize serial for initial debugging (minimal use per constitution)
    Serial.begin(115200);
    delay(100);
    Serial.println(F("Poseidon2 WiFi Gateway - Initializing..."));

    // T044: Create HAL instances (must be done in setup, not as globals, to avoid watchdog)
    wifiAdapter = new ESP32WiFiAdapter();
    fileSystem = new LittleFSAdapter();

    // Mount LittleFS
    if (!fileSystem->mount()) {
        Serial.println(F("ERROR: Failed to mount LittleFS"));
        // Try to format and remount
        Serial.println(F("Attempting to format LittleFS..."));
        // Note: LittleFS.format() would be called inside LittleFSAdapter if mount fails
        delay(5000);
        ESP.restart();
    }
    Serial.println(F("LittleFS mounted successfully"));

    // Create WiFiManager with HAL dependencies
    wifiManager = new WiFiManager(wifiAdapter, fileSystem, &logger, &timeoutManager);

    // T045: WiFi initialization sequence
    Serial.println(F("Loading WiFi configuration..."));
    if (!wifiManager->loadConfig(wifiConfig)) {
        Serial.println(F("ERROR: No valid WiFi configuration found"));
        logger.logConfigEvent(ConnectionEvent::CONFIG_LOADED, false, 0, F("File not found or invalid"));

        // No config available - enter fail-safe mode (reboot loop until config uploaded)
        scheduleReboot(REBOOT_DELAY_MS, F("No WiFi configuration"));
    } else {
        Serial.printf("WiFi config loaded: %d networks\n", wifiConfig.count);
        logger.logConfigEvent(ConnectionEvent::CONFIG_LOADED, true, wifiConfig.count);

        // Initialize connection state
        connectionState.status = ConnectionStatus::DISCONNECTED;
        connectionState.currentNetworkIndex = 0;
        connectionState.retryCount = 0;

        // Register WiFi event handlers using Arduino's WiFi library directly
        WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
            if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
                onWiFiConnected();
            } else if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
                onWiFiDisconnected();
            }
        });

        // Attempt first network connection
        Serial.println(F("Attempting WiFi connection..."));
        wifiManager->connect(connectionState, wifiConfig);
    }

    // T046: ReactESP loop integration
    // Periodic timeout check every 1 second
    app.onRepeat(1000, checkConnectionTimeout);

    // Periodic reboot check every 500ms
    app.onRepeat(500, checkScheduledReboot);

    // Periodic keep-alive broadcast every 5 seconds
    app.onRepeat(5000, broadcastKeepAlive);

    // Log initialization complete
    Serial.println(F("Setup complete - entering main loop"));
}

/**
 * @brief Main loop - runs repeatedly
 *
 * ReactESP event loop processes all async events:
 * - WiFi connection timeouts
 * - Scheduled reboots
 * - Web server requests (handled by ESPAsyncWebServer)
 */
void loop() {
    // Process ReactESP events
    app.tick();

    // Small delay to prevent watchdog issues
    delay(10);
}

#endif // UNIT_TEST
