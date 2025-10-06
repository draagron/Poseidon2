/**
 * @file WebSocketLogger.h
 * @brief WebSocket-based logger for reliable network debugging
 *
 * Provides WebSocket logging as an alternative to UDP broadcast.
 * WebSocket uses TCP, providing reliable, ordered delivery of log messages.
 *
 * Usage:
 * @code
 * WebSocketLogger wsLogger;
 * wsLogger.begin(webServer);
 * wsLogger.broadcastLog(LogLevel::INFO, "Component", "EVENT", "{\"data\":1}");
 * @endcode
 */

#ifndef WEBSOCKET_LOGGER_H
#define WEBSOCKET_LOGGER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "UDPLogger.h"  // Reuse LogLevel enum

/**
 * @brief WebSocket logger class
 *
 * Broadcasts log messages to all connected WebSocket clients.
 * Provides reliable TCP-based delivery vs unreliable UDP.
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
     * @brief Get number of connected WebSocket clients
     * @return Number of active connections
     */
    uint32_t getClientCount() const;

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
     * @brief Convert log level to string
     * @param level Log level
     * @return Level as string
     */
    const char* logLevelToString(LogLevel level) const;

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
