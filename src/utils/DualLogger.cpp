/**
 * @file DualLogger.cpp
 * @brief Implementation of dual-mode logger
 */

#include "DualLogger.h"

DualLogger::DualLogger()
    : wsLogger(nullptr), wsEnabled(false) {
}

DualLogger::~DualLogger() {
    if (wsLogger != nullptr) {
        delete wsLogger;
    }
}

bool DualLogger::begin() {
    // Initialize UDP logger (always available)
    return udpLogger.begin();
}

bool DualLogger::attachWebSocket(AsyncWebServer* server, const char* path) {
    if (server == nullptr) {
        return false;
    }

    // Create WebSocket logger if not already created
    if (wsLogger == nullptr) {
        wsLogger = new WebSocketLogger();
    }

    // Initialize WebSocket
    if (wsLogger->begin(server, path)) {
        wsEnabled = true;
        return true;
    }

    return false;
}

void DualLogger::broadcastLog(LogLevel level, const char* component, const char* event, const String& data) {
    // Always send to UDP (unreliable but always available)
    udpLogger.broadcastLog(level, component, event, data);

    // Send to WebSocket if enabled and has clients
    if (wsEnabled && wsLogger != nullptr && wsLogger->hasClients()) {
        wsLogger->broadcastLog(level, component, event, data);
    }
}

void DualLogger::logConnectionEvent(ConnectionEvent event, const String& ssid, int attempt, int timeout) {
    udpLogger.logConnectionEvent(event, ssid, attempt, timeout);

    // WebSocket clients will receive via broadcastLog called by udpLogger
    // (if we refactor UDPLogger to use DualLogger, otherwise duplicate here)
}

void DualLogger::logConfigEvent(ConnectionEvent event, bool success, int networkCount, const String& error) {
    udpLogger.logConfigEvent(event, success, networkCount, error);
}

void DualLogger::logRebootEvent(int delaySeconds, const String& reason) {
    udpLogger.logRebootEvent(delaySeconds, reason);
}

uint32_t DualLogger::getWebSocketClients() const {
    if (wsLogger == nullptr) {
        return 0;
    }
    return wsLogger->getClientCount();
}

bool DualLogger::hasWebSocketClients() const {
    return getWebSocketClients() > 0;
}
