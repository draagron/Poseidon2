/**
 * @file UDPLogger.cpp
 * @brief Implementation of UDP broadcast logger
 */

#include "UDPLogger.h"
#include <WiFi.h>

UDPLogger::UDPLogger() : isInitialized(false) {
    // Default broadcast address (will be updated when WiFi connects)
    broadcastIP = IPAddress(255, 255, 255, 255);
}

bool UDPLogger::begin() {
    // If WiFi is connected, get the actual broadcast address
    if (WiFi.status() == WL_CONNECTED) {
        IPAddress localIP = WiFi.localIP();
        IPAddress subnet = WiFi.subnetMask();

        // Calculate broadcast address
        for (int i = 0; i < 4; i++) {
            broadcastIP[i] = localIP[i] | (~subnet[i]);
        }
    } else {
        // Use fallback broadcast if WiFi not connected yet
        broadcastIP = IPAddress(255, 255, 255, 255);
    }

    // Initialize WiFiUDP - required for sending packets
    udp.begin(UDP_DEBUG_PORT);

    isInitialized = true;
    return true;
}

void UDPLogger::broadcastLog(LogLevel level, const char* component, const char* event, const String& data) {
    if (!isInitialized) {
        return; // Can't log if not initialized
    }

    // Build JSON message
    String message = buildLogMessage(level, component, event, data);

    // Broadcast via UDP
    udp.beginPacket(broadcastIP, UDP_DEBUG_PORT);
    udp.print(message);
    udp.endPacket();

    Serial.println(message);
}

const char* UDPLogger::logLevelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default:              return "UNKNOWN";
    }
}

const char* UDPLogger::connectionEventToString(ConnectionEvent event) const {
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

void UDPLogger::logConnectionEvent(ConnectionEvent event, const String& ssid, int attempt, int timeout) {
    // Build data JSON
    String data = "{";
    data += "\"ssid\":\"";
    data += ssid;
    data += "\"";

    if (attempt > 0) {
        data += ",\"attempt\":";
        data += String(attempt);
    }

    if (timeout > 0) {
        data += ",\"timeout_seconds\":";
        data += String(timeout);
    }

    data += "}";

    // Determine log level based on event
    LogLevel level = LogLevel::INFO;
    switch (event) {
        case ConnectionEvent::CONNECTION_FAILED:
        case ConnectionEvent::CONNECTION_LOST:
            level = LogLevel::WARN;
            break;
        case ConnectionEvent::CONNECTION_SUCCESS:
            level = LogLevel::INFO;
            break;
        default:
            level = LogLevel::INFO;
            break;
    }

    // Broadcast log
    broadcastLog(level, "WiFiManager", connectionEventToString(event), data);
}

void UDPLogger::logConfigEvent(ConnectionEvent event, bool success, int networkCount, const String& error) {
    // Build data JSON
    String data = "{";
    data += "\"success\":";
    data += success ? "true" : "false";

    if (networkCount > 0) {
        data += ",\"networks_count\":";
        data += String(networkCount);
    }

    if (error.length() > 0) {
        data += ",\"error\":\"";
        data += error;
        data += "\"";
    }

    data += "}";

    // Determine log level
    LogLevel level = success ? LogLevel::INFO : LogLevel::ERROR;

    // Broadcast log
    broadcastLog(level, "WiFiManager", connectionEventToString(event), data);
}

void UDPLogger::logRebootEvent(int delaySeconds, const String& reason) {
    // Build data JSON
    String data = "{";
    data += "\"delay_seconds\":";
    data += String(delaySeconds);

    if (reason.length() > 0) {
        data += ",\"reason\":\"";
        data += reason;
        data += "\"";
    }

    data += "}";

    // Broadcast log
    broadcastLog(LogLevel::WARN, "WiFiManager", connectionEventToString(ConnectionEvent::REBOOT_SCHEDULED), data);
}

String UDPLogger::buildLogMessage(LogLevel level, const char* component, const char* event, const String& data) const {
    String message = "{";

    // Timestamp
    message += "\"timestamp\":";
    message += String(millis());
    message += ",";

    // Level
    message += "\"level\":\"";
    message += logLevelToString(level);
    message += "\",";

    // Component
    message += "\"component\":\"";
    message += component;
    message += "\",";

    // Event
    message += "\"event\":\"";
    message += event;
    message += "\"";

    // Data (if provided)
    if (data.length() > 0) {
        message += ",\"data\":";
        message += data;
    }

    message += "}\n";

    return message;
}
