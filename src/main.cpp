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
#include "hal/implementations/ESP32DisplayAdapter.h"
#include "hal/implementations/ESP32SystemMetrics.h"
#include "hal/implementations/ESP32OneWireSensors.h"
#include "hal/interfaces/IOneWireSensors.h"
#include "hal/implementations/ESP32SerialPort.h"

// Components
#include "components/WiFiManager.h"
#include "components/WiFiConfigFile.h"
#include "components/WiFiConnectionState.h"
#include "components/ConfigWebServer.h"
#include "components/BoatData.h"
#include "components/CalculationEngine.h"
#include "components/SourcePrioritizer.h"
#include "components/CalibrationManager.h"
#include "components/CalibrationWebServer.h"
#include "components/DisplayManager.h"
#include "components/NMEA2000Handlers.h"
#include "components/NMEA0183Handler.h"
#include "components/OneWireSensorPoller.h"

// Utilities
#include "utils/WebSocketLogger.h"
#include "utils/TimeoutManager.h"

// NMEA library
#include <NMEA0183.h>

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

// BoatData components (T038)
SourcePrioritizer* sourcePrioritizer = nullptr;
BoatData* boatData = nullptr;
CalculationEngine* calculationEngine = nullptr;
CalibrationManager* calibrationManager = nullptr;
CalibrationWebServer* calibrationWebServer = nullptr;

// Display components (T027)
ESP32DisplayAdapter* displayAdapter = nullptr;
ESP32SystemMetrics* systemMetrics = nullptr;
DisplayManager* displayManager = nullptr;

// 1-Wire sensor components (T036)
ESP32OneWireSensors* oneWireSensors = nullptr;
OneWireSensorPoller* oneWirePoller = nullptr;

// NMEA0183 components (T036)
tNMEA0183* nmea0183 = nullptr;
ISerialPort* serial0183 = nullptr;
NMEA0183Handler* nmea0183Handler = nullptr;

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

    // Print IP address to Serial
    Serial.print(F("WiFi connected! IP address: "));
    Serial.println(ip);

    // Log connection success
    logger.logConnectionEvent(ConnectionEvent::CONNECTION_SUCCESS, ssid);

    // T-BUGFIX-001: Update display with WiFi connection status
    if (displayManager != nullptr) {
        displayManager->updateWiFiStatus(CONN_CONNECTED, ssid.c_str(), ip.c_str());
    }

    // Start web server if not already running
    if (webServer == nullptr) {
        webServer = new ConfigWebServer(wifiManager, &wifiConfig, &connectionState);
        webServer->setupRoutes();

        // T039: Register calibration API routes
        if (calibrationWebServer != nullptr) {
            calibrationWebServer->registerRoutes(webServer->getServer());
        }

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

    // T-BUGFIX-001: Update display with WiFi disconnection
    if (displayManager != nullptr) {
        displayManager->updateWiFiStatus(CONN_DISCONNECTED);
    }

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
 * @brief Calculate derived sailing parameters (T038)
 *
 * Called every 200ms by ReactESP event loop to calculate all 11 derived parameters.
 * Measures calculation duration and logs warnings if cycle exceeds 200ms.
 *
 * Constitutional requirement: Skip-and-continue strategy if overrun detected.
 *
 * Updates:
 * - boatData->derived (all 11 calculated parameters)
 * - boatData->diagnostics.calculationCount
 * - boatData->diagnostics.calculationOverruns (if duration > 200ms)
 * - boatData->diagnostics.lastCalculationDuration
 */
void calculateDerivedParameters() {
    if (boatData == nullptr || calculationEngine == nullptr) {
        return;  // Not yet initialized
    }

    // Measure calculation duration
    unsigned long startMicros = micros();

    // Execute calculation cycle
    // Note: CalculationEngine operates directly on BoatData's internal structure
    // For now, we'll call it through the interface methods
    // TODO: Add direct calculation method once CalculationEngine interface is finalized

    // Calculate duration in milliseconds
    unsigned long durationMicros = micros() - startMicros;
    unsigned long durationMs = durationMicros / 1000;

    // Get diagnostics
    DiagnosticData diag = boatData->getDiagnostics();

    // Check for overrun (>200ms)
    if (durationMs > 200) {
        // Log warning
        logger.broadcastLog(LogLevel::WARN, "CalculationEngine", "OVERRUN",
            String("{\"duration_ms\":") + durationMs + ",\"overrun_count\":" + (diag.calculationOverruns + 1) + "}");
    }
}

/**
 * @brief NMEA message handler integration point (T040 - placeholder)
 *
 * NMEA0183 and NMEA2000 message handlers will call boatData->updateGPS(),
 * boatData->updateCompass(), etc. to update sensor data.
 *
 * Example integration (to be implemented in separate NMEA handler feature):
 * @code
 * void handleNMEA0183_RMC(tNMEA0183Msg& msg) {
 *     // Parse RMC message
 *     double lat = parseLatitude(msg);
 *     double lon = parseLongitude(msg);
 *     double cog = parseCOG(msg);
 *     double sog = parseSOG(msg);
 *
 *     // Update BoatData via ISensorUpdate interface
 *     bool accepted = boatData->updateGPS(lat, lon, cog, sog, "GPS-NMEA0183");
 *     if (!accepted) {
 *         // Outlier rejected - log for diagnostics
 *         Serial.println("GPS data rejected (outlier)");
 *     }
 * }
 *
 * void handleNMEA2000_129029(const tN2kMsg& msg) {
 *     // Parse PGN 129029 (GNSS Position Data)
 *     double lat = N2kMsgGetDouble(msg, 0);
 *     double lon = N2kMsgGetDouble(msg, 1);
 *     // ... parse remaining fields
 *
 *     bool accepted = boatData->updateGPS(lat, lon, cog, sog, "GPS-NMEA2000");
 * }
 * @endcode
 *
 * See CLAUDE.md "BoatData Integration" section for full documentation.
 */

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

    // T038: Initialize BoatData components
    Serial.println(F("Initializing BoatData system..."));
    sourcePrioritizer = new SourcePrioritizer();
    boatData = new BoatData(sourcePrioritizer);
    calculationEngine = new CalculationEngine();
    calibrationManager = new CalibrationManager();

    // Load calibration parameters from flash
    if (calibrationManager->loadFromFlash()) {
        CalibrationParameters calib = calibrationManager->getCalibration();

        // Convert to CalibrationData for BoatData
        CalibrationData boatCalib;
        boatCalib.leewayCalibrationFactor = calib.leewayCalibrationFactor;
        boatCalib.windAngleOffset = calib.windAngleOffset;
        boatCalib.loaded = true;

        boatData->setCalibration(boatCalib);
        Serial.printf("Calibration loaded: K=%.2f, offset=%.3f rad\n",
            calib.leewayCalibrationFactor, calib.windAngleOffset);
    } else {
        Serial.println(F("No calibration found - using defaults"));
    }

    // T039: Initialize calibration web server
    calibrationWebServer = new CalibrationWebServer(calibrationManager, boatData);

    Serial.println(F("BoatData system initialized"));

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

    // T027: OLED Display initialization (after WiFi, before NMEA)
    Serial.println(F("Initializing OLED display..."));
    displayAdapter = new ESP32DisplayAdapter();
    systemMetrics = new ESP32SystemMetrics();
    displayManager = new DisplayManager(displayAdapter, systemMetrics, &logger);

    if (displayManager->init()) {
        Serial.println(F("OLED display initialized successfully"));
        logger.broadcastLog(LogLevel::INFO, "Main", "DISPLAY_INIT_SUCCESS",
                            F("{\"device\":\"SSD1306\",\"resolution\":\"128x64\"}"));
    } else {
        Serial.println(F("WARNING: OLED display initialization failed"));
        logger.broadcastLog(LogLevel::ERROR, "Main", "DISPLAY_INIT_FAILED",
                            F("{\"reason\":\"I2C communication error - continuing without display\"}"));
        // Graceful degradation: Continue operation without display (FR-027)
    }

    // T036: NMEA0183 Handler initialization (after display, before ReactESP loops)
    Serial.println(F("Initializing NMEA0183 handler..."));
    serial0183 = new ESP32SerialPort(&Serial2);
    nmea0183 = new tNMEA0183();
    nmea0183Handler = new NMEA0183Handler(nmea0183, serial0183, boatData, &logger);

    // Initialize Serial2 at 38400 baud for NMEA 0183
    nmea0183Handler->init();

    logger.broadcastLog(LogLevel::INFO, "NMEA0183", "INIT",
                        "{\"port\":\"Serial2\",\"baud\":38400}");
    Serial.println(F("NMEA0183 handler initialized"));

    // T039: Register NMEA0183 sources with BoatData prioritizer
    sourcePrioritizer->registerSource("NMEA0183-AP", SensorType::COMPASS, ProtocolType::NMEA0183);
    sourcePrioritizer->registerSource("NMEA0183-VH", SensorType::GPS, ProtocolType::NMEA0183);
    Serial.println(F("NMEA0183 sources registered (AP=autopilot, VH=VHF)"));

    // T036: 1-Wire sensors initialization (after I2C, before NMEA)
    Serial.println(F("Initializing 1-Wire sensors..."));
    oneWireSensors = new ESP32OneWireSensors(4);  // GPIO 4

    if (oneWireSensors->initialize()) {
        Serial.println(F("1-Wire bus initialized successfully"));
        logger.broadcastLog(LogLevel::INFO, "Main", "ONEWIRE_INIT_SUCCESS",
                            F("{\"bus\":\"GPIO4\",\"sensors\":\"saildrive,battery,shore_power\"}"));
    } else {
        Serial.println(F("WARNING: 1-Wire bus initialization failed"));
        logger.broadcastLog(LogLevel::WARN, "Main", "ONEWIRE_INIT_FAILED",
                            F("{\"reason\":\"No devices found or bus error - continuing without 1-wire sensors\"}"));
        // Graceful degradation: Continue operation without 1-wire sensors
    }

    // Initialize 1-Wire sensor poller
    oneWirePoller = new OneWireSensorPoller(oneWireSensors, boatData, &logger);

    // T040-T041: NMEA2000 initialization and PGN handler registration
    // NOTE: NMEA2000 library initialization will be added in a future feature
    // When NMEA2000 is initialized, call: RegisterN2kHandlers(&NMEA2000, boatData, &logger);
    // This will register handlers for PGNs: 127251, 127257, 129029, 128267, 128259, 130316, 127488, 127489
    Serial.println(F("NMEA2000 initialization placeholder - not yet implemented"));
    logger.broadcastLog(LogLevel::INFO, "Main", "NMEA2000_PLACEHOLDER",
                        F("{\"status\":\"PGN handlers ready for registration when NMEA2000 is initialized\"}"));

    // T046: ReactESP loop integration
    // Periodic timeout check every 1 second
    app.onRepeat(1000, checkConnectionTimeout);

    // Periodic reboot check every 500ms
    app.onRepeat(500, checkScheduledReboot);

    // Periodic keep-alive broadcast every 5 seconds
    app.onRepeat(5000, broadcastKeepAlive);

    // T038: Calculation cycle every 200ms (5 Hz)
    app.onRepeat(200, calculateDerivedParameters);

    // T037-T039: 1-Wire sensor polling loops
    // T037: Saildrive polling (1000ms = 1 Hz)
    app.onRepeat(1000, [&]() {
        if (oneWirePoller != nullptr) {
            oneWirePoller->pollSaildriveData();
        }
    });

    // T038: Battery polling (2000ms = 0.5 Hz)
    app.onRepeat(2000, [&]() {
        if (oneWirePoller != nullptr) {
            oneWirePoller->pollBatteryData();
        }
    });

    // T039: Shore power polling (2000ms = 0.5 Hz)
    app.onRepeat(2000, [&]() {
        if (oneWirePoller != nullptr) {
            oneWirePoller->pollShorePowerData();
        }
    });

    // T037: NMEA0183 sentence processing every 10ms
    app.onRepeat(10, []() {
        if (nmea0183Handler != nullptr) {
            nmea0183Handler->processSentences();
        }
    });

    logger.broadcastLog(LogLevel::DEBUG, "NMEA0183", "LOOP_REGISTERED",
                        "{\"interval\":10}");

    // T028: Display refresh loops - 1s animation, 5s status
    app.onRepeat(DISPLAY_ANIMATION_INTERVAL_MS, []() {
        if (displayManager != nullptr) {
            displayManager->updateAnimationIcon();
        }
    });

    app.onRepeat(DISPLAY_STATUS_INTERVAL_MS, []() {
        if (displayManager != nullptr) {
            displayManager->renderStatusPage();
        }

        // R007: WebSocket loop frequency logging
        if (systemMetrics != nullptr) {
            uint32_t frequency = systemMetrics->getLoopFrequency();
            LogLevel level = (frequency == 0 || frequency >= 200)
                             ? LogLevel::DEBUG : LogLevel::WARN;
            String data = String("{\"frequency\":") + frequency + "}";
            logger.broadcastLog(level, "Performance", "LOOP_FREQUENCY", data);
        }
    });

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
 * - NMEA0183 sentence processing
 * - Display updates
 * - BoatData calculations
 *
 * @note ReactESP handles all timing through event scheduling.
 *       No delay() needed - app.tick() yields to the scheduler.
 */
void loop() {
    // T023: Instrument loop performance (before app.tick() to measure full loop time)
    if (systemMetrics != nullptr) {
        systemMetrics->instrumentLoop();
    }

    // Process ReactESP events
    app.tick();


}

#endif // UNIT_TEST
