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
    /**
     * @brief Log filter configuration
     *
     * Single shared filter applied to all connected clients.
     * Empty component/event strings match all messages.
     */
    struct LogFilter {
        LogLevel minLevel = LogLevel::INFO;  ///< Minimum log level (default: INFO)
        char components[128] = "";           ///< Comma-separated component filter (empty = all)
        char eventPrefixes[128] = "";        ///< Comma-separated event prefix filter (empty = all)

        /**
         * @brief Check if a log message matches this filter
         * @param level Message log level
         * @param component Message component
         * @param event Message event name
         * @return true if message should be logged
         */
        bool matches(LogLevel level, const String& component, const String& event) const;
    };

    AsyncWebSocket* ws;
    bool isInitialized;
    uint32_t messageCount;
    LogFilter filter;  ///< Shared filter for all clients

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

    /**
     * @brief Set minimum log level filter
     * @param level Minimum log level to broadcast
     */
    void setFilterLevel(LogLevel level);

    /**
     * @brief Set component filter (comma-separated list)
     * @param components Component names to include (empty = all)
     * Example: "NMEA2000,GPS,OneWire"
     */
    void setFilterComponents(const String& components);

    /**
     * @brief Set event prefix filter (comma-separated list)
     * @param events Event name prefixes to include (empty = all)
     * Example: "PGN130306_,ERROR,UPDATE"
     */
    void setFilterEvents(const String& events);

    /**
     * @brief Clear all filters (reset to defaults: INFO level, all components/events)
     */
    void clearFilter();

    /**
     * @brief Get current filter configuration as JSON
     * @return JSON string with current filter settings
     */
    String getFilterConfig() const;

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

    /**
     * @brief Save current filter to LittleFS (/log-filter.json)
     * @return true if save successful
     */
    bool saveFilter();

    /**
     * @brief Load filter from LittleFS (/log-filter.json)
     * @return true if load successful (false = use defaults)
     */
    bool loadFilter();

    static constexpr const char* FILTER_FILE = "/log-filter.json";
};

#endif // WEBSOCKET_LOGGER_H