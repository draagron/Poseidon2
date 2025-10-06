/**
 * @file WebSocketLogger.cpp
 * @brief Implementation of WebSocket logger
 */

#include "WebSocketLogger.h"

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

const char* WebSocketLogger::logLevelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default:              return "UNKNOWN";
    }
}

void WebSocketLogger::onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                                        AwsEventType type, void* arg, uint8_t* data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            // Client connected
            Serial.printf("WebSocket client #%u connected from %s\n",
                         client->id(), client->remoteIP().toString().c_str());

            // Send welcome message
            client->text("{\"status\":\"connected\",\"timestamp\":" + String(millis()) + "}");
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
