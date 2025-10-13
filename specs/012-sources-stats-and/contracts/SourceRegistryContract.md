# Contract: SourceRegistry

**Component**: Source Statistics Tracking
**Feature**: 012-sources-stats-and
**Version**: 1.0.0

## Purpose

SourceRegistry manages the hierarchical collection of NMEA message sources, tracking their update frequencies, staleness, and lifecycle. It serves as the central authority for source discovery, statistics calculation, and garbage collection.

## Interface

```cpp
class SourceRegistry {
public:
    // === Lifecycle ===

    /**
     * @brief Initialize the registry with static category/message mappings
     *
     * Pre-populates categories and message type entries based on supported
     * PGNs (FR-001) and sentences (FR-002). No dynamic memory allocation.
     *
     * @post All 9 categories initialized
     * @post 19 message types registered across categories
     * @post totalSources_ = 0
     * @post hasChanges_ = false
     */
    void init();

    // === Source Management ===

    /**
     * @brief Record a message update from a source
     *
     * Creates source if first time seen (FR-013), updates timestamp and
     * frequency buffer, enforces 50-source limit (FR-019).
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
     * @post If totalSources_ == 50, oldest source evicted before adding new one
     *
     * @note Called from NMEA2000Handlers.cpp and NMEA0183Handler.cpp
     * @note Performance: O(1) average case (direct lookup), O(n) worst case (eviction)
     */
    bool recordUpdate(CategoryType category, const char* messageTypeId,
                      const char* sourceId, ProtocolType protocol);

    /**
     * @brief Remove a specific source from registry
     *
     * @param sourceId Source identifier to remove
     *
     * @return true if source found and removed, false if not found
     *
     * @post Source marked inactive
     * @post totalSources_ decremented
     * @post hasChanges_ = true
     *
     * @note Used internally by garbage collection and eviction
     */
    bool removeSource(const char* sourceId);

    /**
     * @brief Remove sources stale >5 minutes (FR-018)
     *
     * Iterates all sources, removes those with timeSinceLast > 300000ms.
     * Emits WebSocket removal events (FR-021).
     *
     * @post Stale sources removed
     * @post totalSources_ updated
     * @post hasChanges_ = true if any removed
     * @post lastGCTime_ = millis()
     *
     * @note Called every 60s via ReactESP timer
     * @note Performance: O(n) where n = total active sources
     */
    void garbageCollect();

    // === Statistics ===

    /**
     * @brief Update staleness flags for all sources
     *
     * Recalculates timeSinceLast = millis() - lastUpdateTime for all sources.
     * Updates isStale flag based on 5-second threshold (FR-007).
     *
     * @post All sources have current timeSinceLast values
     * @post isStale flags updated (true if timeSinceLast > 5000)
     * @post hasChanges_ = true if any flag changed
     *
     * @note Called every 500ms in WebSocket batch cycle
     * @note Performance: O(n) where n = total active sources
     */
    void updateStaleFlags();

    /**
     * @brief Get total number of active sources across all categories
     *
     * @return Total active sources (0-50)
     */
    uint8_t getTotalSourceCount() const { return totalSources_; }

    /**
     * @brief Check if registry has changes since last serialization
     *
     * @return true if changes pending (new updates, stale flag changes, removals)
     */
    bool hasChanges() const { return hasChanges_; }

    /**
     * @brief Clear change flag after serialization
     *
     * @post hasChanges_ = false
     *
     * @note Called by SourceStatsSerializer after generating delta update
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
     *
     * @note Performance: O(n) linear search across all categories/messages/sources
     */
    const MessageSource* findSource(const char* sourceId) const;

private:
    // === Data ===
    CategoryEntry categories_[9];   ///< Fixed array for all 9 categories
    uint8_t totalSources_;          ///< Total active sources (invariant: ≤ 50)
    bool hasChanges_;               ///< Dirty flag for WebSocket delta updates
    uint32_t lastGCTime_;           ///< millis() timestamp of last garbage collection

    // === Internal Methods ===

    /**
     * @brief Evict source with longest timeSinceLast (FR-019)
     *
     * Finds source with maximum timeSinceLast value, removes it, emits
     * WebSocket removal event with reason="evicted".
     *
     * @post Source with max timeSinceLast removed
     * @post totalSources_ decremented
     *
     * @note Called when source limit (50) reached and new source discovered
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
};
```

## Usage Example

```cpp
// In main.cpp setup()
SourceRegistry sourceRegistry;
sourceRegistry.init();

// In NMEA2000 handler
void HandleN2kPGN129025(const tN2kMsg &N2kMsg, BoatData* boatData,
                        WebSocketLogger* logger, SourceRegistry* registry) {
    unsigned char sid = N2kMsg.Source;
    char sourceId[20];
    snprintf(sourceId, sizeof(sourceId), "NMEA2000-%u", sid);

    // Parse and update BoatData...

    // Record source statistics
    registry->recordUpdate(CategoryType::GPS, "PGN129025", sourceId,
                          ProtocolType::NMEA2000);
}

// In main.cpp ReactESP loop
app.onRepeat(500, [&]() {
    // Update staleness flags every 500ms
    sourceRegistry.updateStaleFlags();

    // Serialize and send WebSocket update if changes
    if (sourceRegistry.hasChanges()) {
        String json = SourceStatsSerializer::toDeltaJSON(&sourceRegistry);
        wsSourceStats.textAll(json);
        sourceRegistry.clearChangeFlag();
    }
});

app.onRepeat(60000, [&]() {
    // Garbage collect every 60 seconds
    sourceRegistry.garbageCollect();
});
```

## Invariants

1. **Source Limit**: `totalSources_ ≤ 50` at all times
2. **Consistency**: Sum of all `CategoryEntry::sourceCount` == `totalSources_`
3. **Uniqueness**: No duplicate `sourceId` across entire registry
4. **Staleness**: `isStale == true` if and only if `timeSinceLast > 5000`
5. **Frequency**: `frequency == 0` if `bufferFull == false`
6. **Active Flag**: If `totalSources_ < 50`, all active sources have `active == true`

## Error Handling

**Source Limit Reached**:
- Action: Evict oldest source (max `timeSinceLast`)
- Behavior: Emit WebSocket removal event, then add new source
- Return: `recordUpdate()` returns true if successful, false if eviction failed

**Invalid Parameters**:
- Action: Return false from `recordUpdate()`
- Log: WebSocketLogger at ERROR level
- No state change: Registry unmodified

**Garbage Collection**:
- No errors possible (iterates active sources, skips inactive)
- Always succeeds

## Performance Characteristics

**`recordUpdate()`**:
- Best case: O(1) - source exists, direct pointer update
- Average case: O(1) - hash map lookup (if implemented)
- Worst case: O(n) - new source, eviction required

**`updateStaleFlags()`**:
- Always: O(n) - must iterate all active sources

**`garbageCollect()`**:
- Always: O(n) - must check all active sources

**Memory**:
- Static: ~5.3 KB for 50 sources (worst case)
- No heap allocations

## Testing Strategy

**Unit Tests** (`test/test_source_stats_units/`):
- Test `recordUpdate()` creates new sources
- Test `recordUpdate()` enforces 50-source limit
- Test `garbageCollect()` removes stale >5 min
- Test `evictOldestSource()` removes correct source
- Test `updateStaleFlags()` calculates correctly

**Contract Tests** (`test/test_source_stats_contracts/`):
- Verify invariants hold after each operation
- Test thread safety (single-threaded assertions)
- Test memory footprint ≤ 10KB for 50 sources

**Integration Tests** (`test/test_source_stats_integration/`):
- Simulate 10 NMEA sources updating at 1-10 Hz
- Verify frequency calculations accurate ±10%
- Verify staleness detection within 5.5s

## Dependencies

**Required**:
- `CategoryType` enum (from data-model.md)
- `MessageSource` struct (from data-model.md)
- `MessageTypeEntry` struct (from data-model.md)
- `CategoryEntry` struct (from data-model.md)
- `ProtocolType` enum (from BoatDataTypes.h)
- `millis()` (Arduino core)

**Used By**:
- NMEA2000Handlers.cpp (calls `recordUpdate()`)
- NMEA0183Handler.cpp (calls `recordUpdate()`)
- SourceStatsSerializer (calls `getCategory()`, `getTotalSourceCount()`)
- Main event loop (calls `updateStaleFlags()`, `garbageCollect()`)

## Future Extensions

**Considered but Deferred**:
- Priority-based source selection (use existing SourceManager)
- Persistent source preferences (LittleFS storage)
- Per-category source limits (currently global 50)
- Manual source disable/enable (WebUI control)
