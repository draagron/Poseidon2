/**
 * @file SourceStatsHandler.cpp
 * @brief Implementation of WebSocket handler for source statistics
 *
 * @see SourceStatsHandler.h for interface documentation
 */

#include "SourceStatsHandler.h"

SourceStatsHandler::SourceStatsHandler(AsyncWebSocket* ws, SourceRegistry* registry, WebSocketLogger* logger)
    : ws_(ws), registry_(registry), logger_(logger) {
}

void SourceStatsHandler::begin() {
    if (ws_ == nullptr) {
        return;
    }

    // Register event callback with this instance as user argument
    ws_->onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client,
                        AwsEventType type, void* arg, uint8_t* data, size_t len) {
        this->handleEvent(client, type, arg, data, len);
    });

    if (logger_) {
        logger_->broadcastLog(LogLevel::INFO, "SourceStatsHandler", "INITIALIZED",
                             "{\"endpoint\":\"/source-stats\"}");
    }
}

void SourceStatsHandler::handleEvent(AsyncWebSocketClient* client, AwsEventType type,
                                      void* arg, uint8_t* data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            if (logger_) {
                logger_->broadcastLog(LogLevel::INFO, "SourceStatsHandler", "CLIENT_CONNECTED",
                                     "{\"clientId\":" + String(client->id()) + "}");
            }
            // Send full snapshot to new client
            sendFullSnapshot(client);
            break;

        case WS_EVT_DISCONNECT:
            if (logger_) {
                logger_->broadcastLog(LogLevel::INFO, "SourceStatsHandler", "CLIENT_DISCONNECTED",
                                     "{\"clientId\":" + String(client->id()) + "}");
            }
            break;

        case WS_EVT_ERROR:
            if (logger_) {
                logger_->broadcastLog(LogLevel::ERROR, "SourceStatsHandler", "CLIENT_ERROR",
                                     "{\"clientId\":" + String(client->id()) + "}");
            }
            break;

        case WS_EVT_DATA:
            // No client-to-server messages expected for source stats endpoint
            // Silently ignore
            break;

        default:
            break;
    }
}

void SourceStatsHandler::sendFullSnapshot(AsyncWebSocketClient* client) {
    if (client == nullptr || registry_ == nullptr) {
        return;
    }

    String json = SourceStatsSerializer::toFullSnapshotJSON(registry_);

    if (json.length() == 0) {
        if (logger_) {
            logger_->broadcastLog(LogLevel::ERROR, "SourceStatsHandler", "SNAPSHOT_FAILED",
                                 "{\"reason\":\"serialization error\"}");
        }
        return;
    }

    client->text(json);

    if (logger_) {
        logger_->broadcastLog(LogLevel::DEBUG, "SourceStatsHandler", "SNAPSHOT_SENT",
                             "{\"clientId\":" + String(client->id()) +
                             ",\"size\":" + String(json.length()) + "}");
    }
}

void SourceStatsHandler::sendDeltaUpdate() {
    if (ws_ == nullptr || registry_ == nullptr) {
        return;
    }

    String json = SourceStatsSerializer::toDeltaJSON(registry_);

    if (json.length() == 0) {
        if (logger_) {
            logger_->broadcastLog(LogLevel::ERROR, "SourceStatsHandler", "DELTA_FAILED",
                                 "{\"reason\":\"serialization error\"}");
        }
        return;
    }

    ws_->textAll(json);

    if (logger_) {
        logger_->broadcastLog(LogLevel::DEBUG, "SourceStatsHandler", "DELTA_SENT",
                             "{\"size\":" + String(json.length()) +
                             ",\"clients\":" + String(ws_->count()) + "}");
    }
}

void SourceStatsHandler::sendRemovalEvent(const char* sourceId, const char* reason) {
    if (ws_ == nullptr || sourceId == nullptr || reason == nullptr) {
        return;
    }

    String json = SourceStatsSerializer::toRemovalJSON(sourceId, reason);

    if (json.length() == 0) {
        return;
    }

    ws_->textAll(json);

    if (logger_) {
        logger_->broadcastLog(LogLevel::INFO, "SourceStatsHandler", "REMOVAL_SENT",
                             "{\"sourceId\":\"" + String(sourceId) +
                             "\",\"reason\":\"" + String(reason) + "\"}");
    }
}
