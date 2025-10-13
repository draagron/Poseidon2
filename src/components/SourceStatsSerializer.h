/**
 * @file SourceStatsSerializer.h
 * @brief JSON serialization for source statistics WebSocket streaming
 *
 * Provides three JSON message types:
 * - Full Snapshot: Complete registry state (sent on client connect)
 * - Delta Update: Changed sources only (sent every 500ms)
 * - Source Removed: Garbage collection event
 *
 * Memory: Static buffers (4096 bytes full snapshot, 2048 bytes delta)
 * Performance: <50ms serialization for 30 sources
 *
 * @see specs/012-sources-stats-and/contracts/SourceStatsSerializerContract.md
 * @version 1.0.0
 * @date 2025-10-13
 */

#ifndef SOURCE_STATS_SERIALIZER_H
#define SOURCE_STATS_SERIALIZER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "SourceRegistry.h"

class SourceStatsSerializer {
public:
    /**
     * @brief Serialize complete registry to JSON (full snapshot)
     *
     * Generates FR-022 compliant JSON with all active sources organized by
     * category and message type.
     *
     * @param registry SourceRegistry instance (must not be nullptr)
     *
     * @return JSON string (~4.5KB for 30 sources), empty string on error
     *
     * @note Called once per WebSocket client connection
     * @note Memory: Uses 4096-byte static buffer (no heap)
     * @note Performance: <50ms on ESP32 @ 240 MHz for 30 sources
     */
    static String toFullSnapshotJSON(const SourceRegistry* registry);

    /**
     * @brief Serialize changed sources to JSON (delta update)
     *
     * Generates FR-023 compliant JSON with only sources that have changed
     * since last serialization.
     *
     * @param registry SourceRegistry instance (must not be nullptr)
     *
     * @return JSON string (~600 bytes for 5 sources), empty string on error
     *
     * @note Called every 500ms if registry->hasChanges() == true
     * @note Memory: Uses 2048-byte static buffer
     * @note Performance: <20ms for typical 5 changed sources
     */
    static String toDeltaJSON(const SourceRegistry* registry);

    /**
     * @brief Serialize source removal event (garbage collection)
     *
     * Generates FR-021 compliant JSON for removed sources.
     *
     * @param sourceId Source identifier that was removed
     * @param reason Removal reason ("stale" or "evicted")
     *
     * @return JSON string (~100 bytes)
     *
     * @note Called by SourceRegistry::garbageCollect() and ::evictOldestSource()
     */
    static String toRemovalJSON(const char* sourceId, const char* reason);

private:
    // Static JSON buffer sizes
    static constexpr size_t FULL_SNAPSHOT_BUFFER_SIZE = 4096;
    static constexpr size_t DELTA_UPDATE_BUFFER_SIZE = 2048;

    /**
     * @brief Get protocol name string
     */
    static const char* getProtocolName(ProtocolType protocol);

    /**
     * @brief Get category name string
     */
    static const char* getCategoryName(CategoryType category);
};

#endif // SOURCE_STATS_SERIALIZER_H
