/**
 * @file LogEnums.h
 * @brief Shared logging enums for the Poseidon2 project
 *
 * Contains shared enums used across different logging components.
 */

#ifndef LOG_ENUMS_H
#define LOG_ENUMS_H

#include <Arduino.h>

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
 * @brief Convert log level to string
 * @param level Log level
 * @return Level as string
 */
inline const char* logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default:              return "UNKNOWN";
    }
}

/**
 * @brief Parse log level from string
 * @param levelStr Log level as string (case-insensitive)
 * @return Parsed log level (defaults to INFO if unknown)
 */
inline LogLevel parseLogLevel(const String& levelStr) {
    String upper = levelStr;
    upper.toUpperCase();

    if (upper == "DEBUG") return LogLevel::DEBUG;
    if (upper == "INFO")  return LogLevel::INFO;
    if (upper == "WARN")  return LogLevel::WARN;
    if (upper == "ERROR") return LogLevel::ERROR;
    if (upper == "FATAL") return LogLevel::FATAL;

    return LogLevel::INFO;  // Default to INFO if unknown
}

/**
 * @brief Convert connection event to string
 * @param event Connection event
 * @return Event as string
 */
inline const char* connectionEventToString(ConnectionEvent event) {
    switch (event) {
        case ConnectionEvent::CONNECTION_ATTEMPT:  return "CONNECTION_ATTEMPT";
        case ConnectionEvent::CONNECTION_SUCCESS:  return "CONNECTION_SUCCESS";
        case ConnectionEvent::CONNECTION_FAILED:   return "CONNECTION_FAILED";
        case ConnectionEvent::CONNECTION_LOST:     return "CONNECTION_LOST";
        case ConnectionEvent::CONFIG_LOADED:       return "CONFIG_LOADED";
        case ConnectionEvent::CONFIG_SAVED:        return "CONFIG_SAVED";
        case ConnectionEvent::CONFIG_INVALID:      return "CONFIG_INVALID";
        case ConnectionEvent::REBOOT_SCHEDULED:    return "REBOOT_SCHEDULED";
        default:                                   return "UNKNOWN_EVENT";
    }
}

#endif // LOG_ENUMS_H