/**
 * @file ConfigWebServer.cpp
 * @brief Implementation of WiFi configuration web server
 */

#include "ConfigWebServer.h"

ConfigWebServer::ConfigWebServer(WiFiManager* mgr, WiFiConfigFile* cfg, WiFiConnectionState* st, int port)
    : wifiManager(mgr),
      config(cfg),
      state(st),
      rebootScheduled(false),
      rebootTime(0) {
    server = new AsyncWebServer(port);
}

ConfigWebServer::~ConfigWebServer() {
    delete server;
}

void ConfigWebServer::begin() {
    server->begin();
}

bool ConfigWebServer::shouldReboot() {
    if (rebootScheduled && millis() >= rebootTime) {
        return true;
    }
    return false;
}

void ConfigWebServer::scheduleReboot(unsigned long delayMs) {
    rebootScheduled = true;
    rebootTime = millis() + delayMs;
}

// ============================================================================
// T040: setupRoutes() Implementation
// ============================================================================

void ConfigWebServer::setupRoutes() {
    // POST /upload-wifi-config - Upload configuration file
    server->on("/upload-wifi-config", HTTP_POST,
        [](AsyncWebServerRequest* request) {
            // This callback is for when upload completes
            // Actual upload handling is in the upload handler
            request->send(200);
        },
        [this](AsyncWebServerRequest* request, const String& filename, size_t index, uint8_t* data, size_t len, bool final) {
            handleUpload(request, filename, index, data, len, final);
        }
    );

    // GET /wifi-config - Retrieve current configuration
    server->on("/wifi-config", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleGetConfig(request);
    });

    // GET /wifi-status - Get connection status
    server->on("/wifi-status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleGetStatus(request);
    });
}

// ============================================================================
// T041: handleUpload() Implementation
// ============================================================================

void ConfigWebServer::handleUpload(AsyncWebServerRequest* request, const String& filename,
                                   size_t index, uint8_t* data, size_t len, bool final) {
    static String uploadBuffer;

    // First chunk - initialize buffer
    if (index == 0) {
        uploadBuffer = "";
    }

    // Append data to buffer
    for (size_t i = 0; i < len; i++) {
        uploadBuffer += (char)data[i];
    }

    // Final chunk - process complete upload
    if (final) {
        // Parse uploaded config
        WiFiConfigFile newConfig;
        ConfigParser parser;

        bool parseResult = parser.parseFile(uploadBuffer, newConfig);

        if (!parseResult || newConfig.isEmpty()) {
            // Parse failed or no valid networks
            String errors = "{\"errors\":[\"" + parser.getLastError() + "\"]}";
            String response = buildErrorResponse("Invalid configuration file", errors);
            request->send(400, "application/json", response);
            uploadBuffer = "";
            return;
        }

        // Validate all networks
        for (int i = 0; i < newConfig.count; i++) {
            if (!newConfig.networks[i].isValid()) {
                String error = "Line " + String(i + 1) + ": " + newConfig.networks[i].getValidationError();
                String errors = "{\"errors\":[\"" + error + "\"]}";
                String response = buildErrorResponse("Invalid configuration file", errors);
                request->send(400, "application/json", response);
                uploadBuffer = "";
                return;
            }
        }

        // Save configuration
        if (wifiManager->saveConfig(newConfig)) {
            // Update current config reference
            *config = newConfig;

            // Schedule reboot
            scheduleReboot(REBOOT_DELAY_MS);

            // Build success response
            String response = "{";
            response += "\"status\":\"success\",";
            response += "\"message\":\"Configuration uploaded successfully. Device will reboot in 5 seconds.\",";
            response += "\"networks_count\":" + String(newConfig.count);
            response += "}";

            request->send(200, "application/json", response);
        } else {
            String response = buildErrorResponse("Failed to save configuration");
            request->send(500, "application/json", response);
        }

        uploadBuffer = "";
    }
}

// ============================================================================
// T042: handleGetConfig() Implementation
// ============================================================================

void ConfigWebServer::handleGetConfig(AsyncWebServerRequest* request) {
    String response = "{";

    // Networks array
    response += "\"networks\":[";
    for (int i = 0; i < config->count; i++) {
        if (i > 0) {
            response += ",";
        }
        response += "{";
        response += "\"ssid\":\"" + config->networks[i].ssid + "\",";
        response += "\"priority\":" + String(i + 1);
        response += "}";
    }
    response += "],";

    // Max networks
    response += "\"max_networks\":" + String(MAX_NETWORKS) + ",";

    // Current connection
    response += "\"current_connection\":\"" + state->connectedSSID + "\"";

    response += "}";

    request->send(200, "application/json", response);
}

// ============================================================================
// T043: handleGetStatus() Implementation
// ============================================================================

void ConfigWebServer::handleGetStatus(AsyncWebServerRequest* request) {
    String response = "{";

    // Status
    response += "\"status\":\"" + state->getStatusString() + "\"";

    if (state->status == ConnectionStatus::CONNECTED) {
        // Connected - include SSID, IP, signal strength
        response += ",\"ssid\":\"" + state->connectedSSID + "\"";

        // Get IP address from WiFi (would need WiFi adapter reference)
        // For now, use placeholder
        response += ",\"ip_address\":\"0.0.0.0\"";

        // Signal strength (would need WiFi adapter reference)
        response += ",\"signal_strength\":0";

        // Uptime
        response += ",\"uptime_seconds\":" + String(millis() / 1000);

    } else if (state->status == ConnectionStatus::CONNECTING) {
        // Connecting - include current attempt and time remaining
        if (state->currentNetworkIndex < config->count) {
            response += ",\"current_attempt\":\"" + config->networks[state->currentNetworkIndex].ssid + "\"";
        }
        response += ",\"attempt_number\":" + String(state->retryCount + 1);

        // Calculate time remaining
        unsigned long elapsed = state->getElapsedTime();
        unsigned long remaining = (elapsed < WIFI_TIMEOUT_MS) ? (WIFI_TIMEOUT_MS - elapsed) / 1000 : 0;
        response += ",\"time_remaining_seconds\":" + String(remaining);

    } else if (state->status == ConnectionStatus::DISCONNECTED) {
        // Disconnected - include retry count and reboot countdown
        response += ",\"retry_count\":" + String(state->retryCount);

        // Reboot countdown (if scheduled)
        if (rebootScheduled) {
            unsigned long countdown = (millis() < rebootTime) ? (rebootTime - millis()) / 1000 : 0;
            response += ",\"next_reboot_in_seconds\":" + String(countdown);
        }
    }

    response += "}";

    request->send(200, "application/json", response);
}

// ============================================================================
// Helper Methods
// ============================================================================

String ConfigWebServer::buildErrorResponse(const String& message, const String& errors) {
    String response = "{";
    response += "\"status\":\"error\",";
    response += "\"message\":\"" + message + "\"";

    if (errors.length() > 0) {
        response += "," + errors;
    }

    response += "}";
    return response;
}

String ConfigWebServer::redactPassword(const String& password) {
    if (password.length() == 0) {
        return "";
    }
    return "********";
}
