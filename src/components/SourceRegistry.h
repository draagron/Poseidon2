/**
 * @file SourceRegistry.h
 * @brief Central registry for NMEA source statistics tracking and lifecycle management
 *
 * SourceRegistry manages the hierarchical collection of NMEA message sources:
 * - 9 Categories (GPS, Compass, Wind, DST, Rudder, Engine, Saildrive, Battery, ShorePower)
 * - 19 Message Types (PGNs + sentence IDs)
 * - 50 Sources max (individual NMEA devices)
 *
 * Features:
 * - Source discovery: Automatic registration on first message
 * - Frequency tracking: 10-sample rolling average (Hz)
 * - Staleness detection: 5-second threshold
 * - Garbage collection: Remove sources stale >5 minutes
 * - Eviction strategy: Remove oldest when 50-source limit reached
 *
 * Memory: ~5.3KB static allocation
 * Performance: O(1) recordUpdate(), O(n) updateStaleFlags()/garbageCollect()
 *
 * @see specs/012-sources-stats-and/contracts/SourceRegistryContract.md
 * @version 1.0.0
 * @date 2025-10-13
 */

#ifndef SOURCE_REGISTRY_H
#define SOURCE_REGISTRY_H

#include <Arduino.h>
#include "SourceStatistics.h"
#include "utils/FrequencyCalculator.h"
#include "utils/WebSocketLogger.h"
#include "config.h"

class SourceRegistry {
public:
    // === Lifecycle ===

    /**
     * @brief Initialize the registry with static category/message mappings
     *
     * Pre-populates categories with known message types from NMEA2000/0183.
     * No dynamic memory allocation.
     *
     * @param logger Optional WebSocketLogger for diagnostic events
     *
     * @post All 9 categories initialized
     * @post totalSources_ = 0
     * @post hasChanges_ = false
     */
    void init(WebSocketLogger* logger = nullptr);

    // === Source Management ===

    /**
     * @brief Record a message update from a source
     *
     * Creates source if first time seen, updates timestamp and frequency buffer,
     * enforces 50-source limit.
     *
     * @param category BoatData category (GPS, Compass, Wind, etc.)
     * @param messageTypeId PGN number (e.g., "PGN129025") or sentence (e.g., "RSA")
     * @param sourceId Unique source identifier (e.g., "NMEA2000-42", "NMEA0183-AP")
     * @param protocol Protocol type (NMEA2000 or NMEA0183)
     *
     * @return true if update recorded, false if source limit reached and eviction failed
     *
     * @post source.lastUpdateTime = millis()
     * @post source.timestampBuffer updated with new timestamp
     * @post source.frequency recalculated if buffer full
     * @post hasChanges_ = true
     * @post If totalSources_ == MAX_SOURCES, oldest source evicted before adding new one
     */
    bool recordUpdate(CategoryType category, const char* messageTypeId,
                      const char* sourceId, ProtocolType protocol);

    /**
     * @brief Remove a specific source from registry
     *
     * @param sourceId Source identifier to remove
     * @return true if source found and removed, false if not found
     */
    bool removeSource(const char* sourceId);

    /**
     * @brief Remove sources stale >5 minutes
     *
     * Iterates all sources, removes those with timeSinceLast > SOURCE_GC_THRESHOLD_MS.
     *
     * @post Stale sources removed
     * @post totalSources_ updated
     * @post hasChanges_ = true if any removed
     * @post lastGCTime_ = millis()
     *
     * @note Called every 60s via ReactESP timer
     */
    void garbageCollect();

    // === Statistics ===

    /**
     * @brief Update staleness flags for all sources
     *
     * Recalculates timeSinceLast = millis() - lastUpdateTime for all sources.
     * Updates isStale flag based on SOURCE_STALE_THRESHOLD_MS.
     *
     * @post All sources have current timeSinceLast values
     * @post isStale flags updated
     * @post hasChanges_ = true if any flag changed
     *
     * @note Called every WEBSOCKET_UPDATE_INTERVAL_MS in WebSocket batch cycle
     */
    void updateStaleFlags();

    /**
     * @brief Get total number of active sources across all categories
     *
     * @return Total active sources (0-MAX_SOURCES)
     */
    uint8_t getTotalSourceCount() const { return totalSources_; }

    /**
     * @brief Check if registry has changes since last serialization
     *
     * @return true if changes pending
     */
    bool hasChanges() const { return hasChanges_; }

    /**
     * @brief Clear change flag after serialization
     *
     * @post hasChanges_ = false
     */
    void clearChangeFlag() { hasChanges_ = false; }

    // === Access ===

    /**
     * @brief Get category entry (read-only)
     *
     * @param category Category type
     * @return Pointer to CategoryEntry, or nullptr if invalid category
     */
    const CategoryEntry* getCategory(CategoryType category) const;

    /**
     * @brief Find source by ID (read-only)
     *
     * @param sourceId Source identifier to search for
     * @return Pointer to MessageSource, or nullptr if not found
     */
    const MessageSource* findSource(const char* sourceId) const;

private:
    // === Data ===
    CategoryEntry categories_[9];   ///< Fixed array for all 9 categories
    uint8_t totalSources_;          ///< Total active sources (invariant: ≤ MAX_SOURCES)
    bool hasChanges_;               ///< Dirty flag for WebSocket delta updates
    uint32_t lastGCTime_;           ///< millis() timestamp of last garbage collection
    WebSocketLogger* logger_;       ///< Optional logger for diagnostic events

    // === Internal Methods ===

    /**
     * @brief Evict source with longest timeSinceLast
     *
     * Finds source with maximum timeSinceLast value, removes it.
     *
     * @post Source with max timeSinceLast removed
     * @post totalSources_ decremented
     */
    void evictOldestSource();

    /**
     * @brief Find source by ID (mutable)
     *
     * @param sourceId Source identifier
     * @return Pointer to MessageSource, or nullptr if not found
     */
    MessageSource* findSourceMutable(const char* sourceId);

    /**
     * @brief Get or create message type entry in category
     *
     * @param category Category to search
     * @param messageTypeId Message type identifier
     * @param protocol Protocol type
     *
     * @return Pointer to MessageTypeEntry, or nullptr if category full (8 types)
     */
    MessageTypeEntry* getOrCreateMessageType(CategoryType category,
                                              const char* messageTypeId,
                                              ProtocolType protocol);

    /**
     * @brief Parse sourceId to extract SID or talker ID
     *
     * @param sourceId Source identifier (e.g., "NMEA2000-42" or "NMEA0183-AP")
     * @param sid Output: SID value (0-252) or 255 if NMEA0183
     * @param talkerId Output: Talker ID (2 chars) or empty if NMEA2000
     */
    void parseSourceId(const char* sourceId, uint8_t& sid, char* talkerId);

    /**
     * @brief Get category name as string
     *
     * @param category Category type
     * @return Category name (e.g., "GPS", "COMPASS")
     */
    const char* getCategoryName(CategoryType category) const;
};

#endif // SOURCE_REGISTRY_H
