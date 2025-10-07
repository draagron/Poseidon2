/**
 * @file ConfigWebServer.h
 * @brief Web server for WiFi configuration management
 *
 * Provides HTTP API endpoints for WiFi configuration and status:
 * - POST /upload-wifi-config: Upload new WiFi configuration
 * - GET /wifi-config: Retrieve current configuration (passwords redacted)
 * - GET /wifi-status: Get current connection status
 *
 * Uses ESPAsyncWebServer for non-blocking HTTP handling.
 */

#ifndef CONFIG_WEB_SERVER_H
#define CONFIG_WEB_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "WiFiManager.h"
#include "WiFiConfigFile.h"
#include "WiFiConnectionState.h"
#include "../config.h"

/**
 * @brief Web server for WiFi configuration API
 *
 * Provides HTTP endpoints for configuration management.
 * Integrates with WiFiManager for config operations.
 */
class ConfigWebServer {
private:
    AsyncWebServer* server;
    WiFiManager* wifiManager;
    WiFiConfigFile* config;
    WiFiConnectionState* state;
    bool rebootScheduled;
    unsigned long rebootTime;

public:
    /**
     * @brief Constructor
     * @param mgr WiFi manager instance
     * @param cfg WiFi configuration reference
     * @param st Connection state reference
     * @param port HTTP server port (default 80)
     */
    ConfigWebServer(WiFiManager* mgr, WiFiConfigFile* cfg, WiFiConnectionState* st, int port = 80);

    /**
     * @brief Destructor
     */
    ~ConfigWebServer();

    /**
     * @brief Setup and register all HTTP routes
     *
     * Registers handlers for:
     * - POST /upload-wifi-config
     * - GET /wifi-config
     * - GET /wifi-status
     */
    void setupRoutes();

    /**
     * @brief Start the web server
     */
    void begin();

    /**
     * @brief Check if reboot is scheduled and should execute
     * @return true if reboot should happen now
     *
     * Call this periodically from main loop to handle scheduled reboots.
     */
    bool shouldReboot();

    /**
     * @brief Schedule a reboot after specified delay
     * @param delayMs Delay in milliseconds
     */
    void scheduleReboot(unsigned long delayMs);

    /**
     * @brief Get the underlying AsyncWebServer instance
     * @return Pointer to AsyncWebServer
     *
     * Used for attaching additional handlers (e.g., WebSocket for logging).
     */
    AsyncWebServer* getServer() { return server; }

private:
    /**
     * @brief Handle POST /upload-wifi-config
     * @param request HTTP request
     * @param filename Uploaded filename
     * @param index Current position in file
     * @param data File data chunk
     * @param len Length of data chunk
     * @param final True if this is the last chunk
     *
     * Parses multipart form data, validates config, saves to filesystem,
     * schedules reboot after 5 seconds.
     */
    void handleUpload(AsyncWebServerRequest* request, const String& filename,
                     size_t index, uint8_t* data, size_t len, bool final);

    /**
     * @brief Handle GET /wifi-config
     * @param request HTTP request
     *
     * Returns JSON with SSIDs (passwords redacted) and priority order.
     */
    void handleGetConfig(AsyncWebServerRequest* request);

    /**
     * @brief Handle GET /wifi-status
     * @param request HTTP request
     *
     * Returns JSON with current connection status, SSID, IP, signal strength.
     * Response format varies based on connection state.
     */
    void handleGetStatus(AsyncWebServerRequest* request);

    /**
     * @brief Build JSON error response
     * @param message Error message
     * @param errors Array of error details (optional)
     * @return JSON string
     */
    String buildErrorResponse(const String& message, const String& errors = "");

    /**
     * @brief Redact password from credentials (security)
     * @param password Original password
     * @return Redacted string (e.g., "********")
     */
    String redactPassword(const String& password);
};

#endif // CONFIG_WEB_SERVER_H
