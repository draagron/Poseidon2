/**
 * @file WebSocketLogger.cpp
 * @brief Implementation of WebSocket logger
 */

#include "WebSocketLogger.h"
#include "LogEnums.h"

WebSocketLogger::WebSocketLogger()
    : ws(nullptr), isInitialized(false), messageCount(0) {
}

bool WebSocketLogger::begin(AsyncWebServer* server, const char* path) {
    if (server == nullptr) {
        return false;
    }

    // Create WebSocket handler
    ws = new AsyncWebSocket(path);

    // Enable WebSocket cleanup (clean up disconnected clients)
    ws->enable(true);

    // Register event handler
    ws->onEvent(onWebSocketEvent);

    // Add WebSocket handler to server
    server->addHandler(ws);

    isInitialized = true;
    return true;
}

void WebSocketLogger::broadcastLog(LogLevel level, const char* component, const char* event, const String& data) {
    if (!isInitialized || ws == nullptr) {
        return;
    }

    // Build JSON message
    String message = buildLogMessage(level, component, event, data);

    // Broadcast to all connected clients
    if (ws->count() > 0) {
        ws->textAll(message);
        messageCount++;
    }
}

uint32_t WebSocketLogger::getClientCount() const {
    if (ws == nullptr) {
        return 0;
    }
    return ws->count();
}

bool WebSocketLogger::hasClients() const {
    return getClientCount() > 0;
}

String WebSocketLogger::buildLogMessage(LogLevel level, const char* component, const char* event, const String& data) const {
    String message = "{";

    // Timestamp
    message += "\"timestamp\":";
    message += String(millis());
    message += ",";

    // Level
    message += "\"level\":\"";
    message += ::logLevelToString(level);
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

void WebSocketLogger::onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                                        AwsEventType type, void* arg, uint8_t* data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            // Client connected
            Serial.printf("WebSocket client #%u connected from %s\n",
                         client->id(), client->remoteIP().toString().c_str());

            // Send welcome message
            {
                String welcome = "{\"status\":\"connected\",\"timestamp\":\"" + String(millis()) + "\"}";
                client->text(welcome);
            }
            break;

        case WS_EVT_DISCONNECT:
            // Client disconnected
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;

        case WS_EVT_DATA:
            // Data received from client (we don't expect any, but handle it)
            break;

        case WS_EVT_PONG:
            // Pong received (keep-alive)
            break;

        case WS_EVT_ERROR:
            // Error occurred
            Serial.printf("WebSocket client #%u error\n", client->id());
            break;
    }
}

void WebSocketLogger::logConnectionEvent(ConnectionEvent event, const String& ssid, int attempt, int timeout) {
    // Send to WebSocket if has clients
    if (hasClients()) {
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

        // Build data JSON
        String data = "{\"ssid\":\"" + ssid + "\"";

        if (attempt > 0) {
            data += ",\"attempt\":" + String(attempt);
        }

        if (timeout > 0) {
            data += ",\"timeout_seconds\":" + String(timeout);
        }

        data += "}";

        // Get event string
        const char* eventStr = ::connectionEventToString(event);

        broadcastLog(level, "WiFiManager", eventStr, data);
    }
}

void WebSocketLogger::logConfigEvent(ConnectionEvent event, bool success, int networkCount, const String& error) {
    // Send to WebSocket if has clients
    if (hasClients()) {
        // Build data JSON
        String data = "{\"success\":";
        data += String(success ? "true" : "false");

        if (networkCount > 0) {
            data += ",\"networks_count\":" + String(networkCount);
        }

        if (error.length() > 0) {
            data += ",\"error\":\"" + error + "\"";
        }

        data += "}";

        // Determine log level
        LogLevel level = success ? LogLevel::INFO : LogLevel::ERROR;

        // Get event string
        const char* eventStr = ::connectionEventToString(event);

        broadcastLog(level, "WiFiManager", eventStr, data);
    }
}

void WebSocketLogger::logRebootEvent(int delaySeconds, const String& reason) {
    // Send to WebSocket if has clients
    if (hasClients()) {
        // Build data JSON
        String data = "{\"delay_seconds\":" + String(delaySeconds);

        if (reason.length() > 0) {
            data += ",\"reason\":\"" + reason + "\"";
        }

        data += "}";

        // Map to reboot scheduled event
        const char* eventStr = ::connectionEventToString(ConnectionEvent::REBOOT_SCHEDULED);

        broadcastLog(LogLevel::WARN, "WiFiManager", eventStr, data);
    }
}

uint32_t WebSocketLogger::getWebSocketClients() const {
    return getClientCount();
}

bool WebSocketLogger::hasWebSocketClients() const {
    return hasClients();
}