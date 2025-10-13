/**
 * @file SourceStatsHandler.h
 * @brief WebSocket endpoint handler for source statistics streaming
 *
 * Manages WebSocket clients for /source-stats endpoint:
 * - Sends full snapshot on client connect
 * - Broadcasts delta updates every 500ms
 * - Sends removal events on garbage collection
 *
 * @see specs/012-sources-stats-and/plan.md
 * @version 1.0.0
 * @date 2025-10-13
 */

#ifndef SOURCE_STATS_HANDLER_H
#define SOURCE_STATS_HANDLER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "SourceRegistry.h"
#include "SourceStatsSerializer.h"
#include "../utils/WebSocketLogger.h"

/**
 * @brief WebSocket handler for source statistics
 *
 * Handles /source-stats WebSocket endpoint lifecycle:
 * - Client connection: Send full snapshot
 * - Periodic updates: Broadcast deltas every 500ms (called from ReactESP)
 * - Garbage collection: Send removal events
 */
class SourceStatsHandler {
public:
    /**
     * @brief Constructor
     *
     * @param ws AsyncWebSocket instance for /source-stats endpoint
     * @param registry SourceRegistry instance
     * @param logger WebSocketLogger for diagnostics
     */
    SourceStatsHandler(AsyncWebSocket* ws, SourceRegistry* registry, WebSocketLogger* logger);

    /**
     * @brief Initialize handler (register WebSocket event callback)
     */
    void begin();

    /**
     * @brief Send delta update to all clients
     *
     * Called every 500ms from ReactESP timer if registry->hasChanges() == true
     */
    void sendDeltaUpdate();

    /**
     * @brief Send source removal event to all clients
     *
     * Called by SourceRegistry during garbage collection
     *
     * @param sourceId Source identifier that was removed
     * @param reason "stale" or "evicted"
     */
    void sendRemovalEvent(const char* sourceId, const char* reason);

private:
    AsyncWebSocket* ws_;
    SourceRegistry* registry_;
    WebSocketLogger* logger_;

    /**
     * @brief Send full snapshot to a specific client
     *
     * @param client WebSocket client to send to
     */
    void sendFullSnapshot(AsyncWebSocketClient* client);

    /**
     * @brief WebSocket event callback (static method)
     *
     * @param server WebSocket server instance
     * @param client Client connection
     * @param type Event type (connect/disconnect/data/error)
     * @param arg Event argument
     * @param data Event data
     * @param len Data length
     */
    static void onWebSocketEvent(void* arg, AsyncWebSocket* server, AsyncWebSocketClient* client,
                                  AwsEventType type, void* eventArg, uint8_t* data, size_t len);

    /**
     * @brief Handle WebSocket event (instance method)
     */
    void handleEvent(AsyncWebSocketClient* client, AwsEventType type,
                    void* arg, uint8_t* data, size_t len);
};

#endif // SOURCE_STATS_HANDLER_H
