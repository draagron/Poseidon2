/**
 * @file WebSocketLogger.h
 * @brief WebSocket-based logger for reliable network debugging
 *
 * Provides WebSocket logging as the primary logging mechanism.
 * WebSocket uses TCP, providing reliable, ordered delivery of log messages.
 *
 * Usage:
 * @code
 * WebSocketLogger logger;
 * logger.begin(webServer);
 * logger.broadcastLog(LogLevel::INFO, "Component", "EVENT", "{\"data\":1}");
 * @endcode
 */

#ifndef WEBSOCKET_LOGGER_H
#define WEBSOCKET_LOGGER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "LogEnums.h"

/**
 * @brief WebSocket logger class
 *
 * Broadcasts log messages to all connected WebSocket clients.
 * Provides reliable TCP-based delivery.
 */
class WebSocketLogger {
private:
    AsyncWebSocket* ws;
    bool isInitialized;
    uint32_t messageCount;

public:
    /**
     * @brief Constructor
     */
    WebSocketLogger();

    /**
     * @brief Initialize WebSocket logger
     * @param server AsyncWebServer instance to attach WebSocket to
     * @param path WebSocket endpoint path (default: "/logs")
     * @return true if initialization successful
     */
    bool begin(AsyncWebServer* server, const char* path = "/logs");

    /**
     * @brief Broadcast a log message to all connected clients
     * @param level Log level
     * @param component Component name (e.g., "WiFiManager")
     * @param event Event name (e.g., "CONNECTION_ATTEMPT")
     * @param data JSON data string (optional)
     */
    void broadcastLog(LogLevel level, const char* component, const char* event, const String& data = "");

    /**
     * @brief Log WiFi connection event
     */
    void logConnectionEvent(ConnectionEvent event, const String& ssid, int attempt = 0, int timeout = 0);

    /**
     * @brief Log config file event
     */
    void logConfigEvent(ConnectionEvent event, bool success, int networkCount = 0, const String& error = "");

    /**
     * @brief Log reboot event
     */
    void logRebootEvent(int delaySeconds, const String& reason);

    /**
     * @brief Get number of connected WebSocket clients
     * @return Number of active connections
     */
    uint32_t getClientCount() const;

    /**
     * @brief Get number of WebSocket clients connected
     * @return Client count
     */
    uint32_t getWebSocketClients() const;

    /**
     * @brief Get total messages sent
     * @return Message count since startup
     */
    uint32_t getMessageCount() const { return messageCount; }

    /**
     * @brief Check if any clients are connected
     * @return true if at least one client connected
     */
    bool hasClients() const;

    /**
     * @brief Check if WebSocket has clients
     */
    bool hasWebSocketClients() const;

private:
    /**
     * @brief Build JSON log message
     * @param level Log level
     * @param component Component name
     * @param event Event name
     * @param data Data string
     * @return JSON formatted log message
     */
    String buildLogMessage(LogLevel level, const char* component, const char* event, const String& data) const;

    /**
     * @brief Handle WebSocket events (connect/disconnect/data)
     * @param server WebSocket server
     * @param client Client connection
     * @param type Event type
     * @param arg Event argument
     * @param data Event data
     * @param len Data length
     */
    static void onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                                  AwsEventType type, void* arg, uint8_t* data, size_t len);
};

#endif // WEBSOCKET_LOGGER_H