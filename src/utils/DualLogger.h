/**
 * @file DualLogger.h
 * @brief Unified logger supporting both UDP and WebSocket output
 *
 * Provides simultaneous logging to UDP (fast, unreliable) and WebSocket (reliable).
 * Automatically falls back to UDP-only if no WebSocket clients connected.
 *
 * Usage:
 * @code
 * DualLogger logger;
 * logger.begin();  // Initialize UDP
 * logger.attachWebSocket(webServer);  // Add WebSocket when server ready
 * logger.log(LogLevel::INFO, "Component", "EVENT", "{\"data\":1}");
 * @endcode
 */

#ifndef DUAL_LOGGER_H
#define DUAL_LOGGER_H

#include "UDPLogger.h"
#include "WebSocketLogger.h"

/**
 * @brief Dual-mode logger (UDP + WebSocket)
 *
 * Combines UDP broadcast (unreliable but always available) with
 * WebSocket (reliable but requires client connection).
 */
class DualLogger {
private:
    UDPLogger udpLogger;
    WebSocketLogger* wsLogger;
    bool wsEnabled;

public:
    /**
     * @brief Constructor
     */
    DualLogger();

    /**
     * @brief Destructor
     */
    ~DualLogger();

    /**
     * @brief Initialize UDP logger
     * @return true if successful
     *
     * Call this after WiFi connects. WebSocket can be attached later.
     */
    bool begin();

    /**
     * @brief Attach WebSocket logger
     * @param server AsyncWebServer instance
     * @param path WebSocket endpoint path (default: "/logs")
     * @return true if successful
     *
     * Call this after web server is created (after WiFi connects).
     * Can be called before or after begin().
     */
    bool attachWebSocket(AsyncWebServer* server, const char* path = "/logs");

    /**
     * @brief Broadcast log message to all available transports
     * @param level Log level
     * @param component Component name
     * @param event Event name
     * @param data JSON data string (optional)
     *
     * Sends to both UDP and WebSocket (if enabled and clients connected).
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
     * @brief Get number of WebSocket clients connected
     * @return Client count (0 if WebSocket not enabled)
     */
    uint32_t getWebSocketClients() const;

    /**
     * @brief Check if WebSocket is enabled and has clients
     */
    bool hasWebSocketClients() const;
};

#endif // DUAL_LOGGER_H
