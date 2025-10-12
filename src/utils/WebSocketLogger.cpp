/**
 * @file WebSocketLogger.cpp
 * @brief Implementation of WebSocket logger
 */

#include "WebSocketLogger.h"
#include "LogEnums.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

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

    // Load filter from LittleFS (if exists, otherwise use defaults)
    loadFilter();

    isInitialized = true;
    return true;
}

void WebSocketLogger::broadcastLog(LogLevel level, const char* component, const char* event, const String& data) {
    if (!isInitialized || ws == nullptr) {
        return;
    }

    // Check if any clients are connected (early exit)
    if (ws->count() == 0) {
        return;
    }

    // Apply filter check (early exit if message doesn't match)
    if (!filter.matches(level, String(component), String(event))) {
        return;
    }

    // Build JSON message
    String message = buildLogMessage(level, component, event, data);

    // Broadcast to all connected clients
    ws->textAll(message);
    messageCount++;
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

void WebSocketLogger::setFilterLevel(LogLevel level) {
    filter.minLevel = level;
    saveFilter();  // Automatically persist to flash
}

void WebSocketLogger::setFilterComponents(const String& components) {
    strncpy(filter.components, components.c_str(), sizeof(filter.components) - 1);
    filter.components[sizeof(filter.components) - 1] = '\0';  // Ensure null termination
    saveFilter();  // Automatically persist to flash
}

void WebSocketLogger::setFilterEvents(const String& events) {
    strncpy(filter.eventPrefixes, events.c_str(), sizeof(filter.eventPrefixes) - 1);
    filter.eventPrefixes[sizeof(filter.eventPrefixes) - 1] = '\0';  // Ensure null termination
    saveFilter();  // Automatically persist to flash
}

void WebSocketLogger::clearFilter() {
    filter.minLevel = LogLevel::INFO;
    filter.components[0] = '\0';
    filter.eventPrefixes[0] = '\0';
    saveFilter();  // Automatically persist to flash
}

String WebSocketLogger::getFilterConfig() const {
    String config = "{";

    config += "\"level\":\"";
    config += logLevelToString(filter.minLevel);
    config += "\",";

    config += "\"components\":\"";
    config += filter.components;
    config += "\",";

    config += "\"events\":\"";
    config += filter.eventPrefixes;
    config += "\"";

    config += "}";

    return config;
}

bool WebSocketLogger::LogFilter::matches(LogLevel level, const String& component, const String& event) const {
    // Level check - message level must be >= minimum level
    if (level < minLevel) {
        return false;
    }

    // Component check (empty = match all)
    if (components[0] != '\0') {
        String componentsStr(components);
        // Check if component is in comma-separated list
        if (componentsStr.indexOf(component) < 0) {
            return false;
        }
    }

    // Event prefix check (empty = match all)
    if (eventPrefixes[0] != '\0') {
        String eventsStr(eventPrefixes);
        bool found = false;

        // Check if event starts with any prefix in comma-separated list
        int startPos = 0;
        while (startPos < eventsStr.length()) {
            int commaPos = eventsStr.indexOf(',', startPos);
            String prefix;

            if (commaPos >= 0) {
                prefix = eventsStr.substring(startPos, commaPos);
                startPos = commaPos + 1;
            } else {
                prefix = eventsStr.substring(startPos);
                startPos = eventsStr.length();
            }

            prefix.trim();  // Remove whitespace

            if (prefix.length() > 0 && event.startsWith(prefix)) {
                found = true;
                break;
            }
        }

        if (!found) {
            return false;
        }
    }

    return true;
}

bool WebSocketLogger::saveFilter() {
    // Create JSON document
    StaticJsonDocument<256> doc;

    doc["level"] = logLevelToString(filter.minLevel);
    doc["components"] = String(filter.components);
    doc["events"] = String(filter.eventPrefixes);

    // Open file for writing
    File file = LittleFS.open(FILTER_FILE, "w");
    if (!file) {
        return false;
    }

    // Serialize JSON to file
    size_t bytesWritten = serializeJson(doc, file);
    file.close();

    return (bytesWritten > 0);
}

bool WebSocketLogger::loadFilter() {
    // Check if file exists
    if (!LittleFS.exists(FILTER_FILE)) {
        // File not found - use defaults
        return false;
    }

    // Open file for reading
    File file = LittleFS.open(FILTER_FILE, "r");
    if (!file) {
        return false;
    }

    // Parse JSON
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        // JSON parse error - use defaults
        return false;
    }

    // Extract values
    const char* levelStr = doc["level"] | "INFO";
    const char* components = doc["components"] | "";
    const char* events = doc["events"] | "";

    // Apply loaded values
    filter.minLevel = parseLogLevel(String(levelStr));
    strncpy(filter.components, components, sizeof(filter.components) - 1);
    filter.components[sizeof(filter.components) - 1] = '\0';
    strncpy(filter.eventPrefixes, events, sizeof(filter.eventPrefixes) - 1);
    filter.eventPrefixes[sizeof(filter.eventPrefixes) - 1] = '\0';

    return true;
}