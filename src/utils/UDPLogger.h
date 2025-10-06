/**
 * @file UDPLogger.h
 * @brief UDP broadcast logger for network-based debugging
 *
 * Provides UDP broadcast logging as required by constitution (Principle V).
 * Serial ports are reserved for NMEA communication, so all debug output
 * goes to UDP port 4444.
 *
 * Log format: JSON with timestamp, level, component, event, and data.
 *
 * Usage:
 * @code
 * UDPLogger logger;
 * logger.begin();
 * logger.logConnectionEvent(CONNECTION_ATTEMPT, "HomeNetwork", 1);
 * @endcode
 */

#ifndef UDP_LOGGER_H
#define UDP_LOGGER_H

#include <Arduino.h>
#include <WiFiUdp.h>
#include "../config.h"

/**
 * @brief Log level enumeration
 */
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    FATAL = 4
};

/**
 * @brief WiFi connection event types for logging
 */
enum class ConnectionEvent {
    CONNECTION_ATTEMPT,
    CONNECTION_SUCCESS,
    CONNECTION_FAILED,
    CONNECTION_LOST,
    CONFIG_LOADED,
    CONFIG_SAVED,
    CONFIG_INVALID,
    REBOOT_SCHEDULED
};

/**
 * @brief UDP broadcast logger class
 *
 * Broadcasts log messages to UDP port 4444 for network-based debugging.
 * All devices on the same network segment can receive logs.
 */
class UDPLogger {
private:
    WiFiUDP udp;
    IPAddress broadcastIP;
    bool isInitialized;

public:
    /**
     * @brief Constructor
     */
    UDPLogger();

    /**
     * @brief Initialize UDP logger
     * @return true if initialization successful
     *
     * Call this after WiFi is connected to get broadcast address.
     * Can be called before WiFi connected - will use fallback broadcast.
     */
    bool begin();

    /**
     * @brief Broadcast a log message
     * @param level Log level
     * @param component Component name (e.g., "WiFiManager")
     * @param event Event name (e.g., "CONNECTION_ATTEMPT")
     * @param data JSON data string (optional)
     *
     * Formats message as JSON:
     * {"timestamp":1234,"level":"INFO","component":"WiFiManager","event":"CONNECTION_ATTEMPT","data":{...}}
     */
    void broadcastLog(LogLevel level, const char* component, const char* event, const String& data = "");

    /**
     * @brief Log a WiFi connection event with standard format
     * @param event Connection event type
     * @param ssid Network SSID
     * @param attempt Attempt number (optional)
     * @param timeout Timeout in seconds (optional)
     *
     * Helper method for common WiFi connection events.
     */
    void logConnectionEvent(ConnectionEvent event, const String& ssid, int attempt = 0, int timeout = 0);

    /**
     * @brief Log a config file event
     * @param event Config event type
     * @param success Whether operation succeeded
     * @param networkCount Number of networks (optional)
     * @param error Error message (optional)
     */
    void logConfigEvent(ConnectionEvent event, bool success, int networkCount = 0, const String& error = "");

    /**
     * @brief Log a reboot event
     * @param delaySeconds Delay before reboot in seconds
     * @param reason Reason for reboot
     */
    void logRebootEvent(int delaySeconds, const String& reason);

    /**
     * @brief Convert log level to string
     * @param level Log level
     * @return Level as string
     */
    const char* logLevelToString(LogLevel level) const;

    /**
     * @brief Convert connection event to string
     * @param event Connection event
     * @return Event as string
     */
    const char* connectionEventToString(ConnectionEvent event) const;

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
};

#endif // UDP_LOGGER_H
