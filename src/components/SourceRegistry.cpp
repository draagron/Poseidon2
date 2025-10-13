/**
 * @file SourceRegistry.cpp
 * @brief Implementation of NMEA source statistics registry
 *
 * @see SourceRegistry.h for interface documentation
 */

#include "SourceRegistry.h"
#include <string.h>

// Category-to-MessageType mappings (from data-model.md)
struct MessageMapping {
    CategoryType category;
    const char* messageTypeId;
    ProtocolType protocol;
};

static const MessageMapping MESSAGE_MAPPINGS[] = {
    // GPS (7 types)
    {CategoryType::GPS, "PGN129025", ProtocolType::NMEA2000},
    {CategoryType::GPS, "PGN129026", ProtocolType::NMEA2000},
    {CategoryType::GPS, "PGN129029", ProtocolType::NMEA2000},
    {CategoryType::GPS, "PGN127258", ProtocolType::NMEA2000},
    {CategoryType::GPS, "GGA", ProtocolType::NMEA0183},
    {CategoryType::GPS, "RMC", ProtocolType::NMEA0183},
    {CategoryType::GPS, "VTG", ProtocolType::NMEA0183},

    // Compass (5 types)
    {CategoryType::COMPASS, "PGN127250", ProtocolType::NMEA2000},
    {CategoryType::COMPASS, "PGN127251", ProtocolType::NMEA2000},
    {CategoryType::COMPASS, "PGN127252", ProtocolType::NMEA2000},
    {CategoryType::COMPASS, "PGN127257", ProtocolType::NMEA2000},
    {CategoryType::COMPASS, "HDM", ProtocolType::NMEA0183},

    // Wind (1 type)
    {CategoryType::WIND, "PGN130306", ProtocolType::NMEA2000},

    // DST (3 types)
    {CategoryType::DST, "PGN128267", ProtocolType::NMEA2000},
    {CategoryType::DST, "PGN128259", ProtocolType::NMEA2000},
    {CategoryType::DST, "PGN130316", ProtocolType::NMEA2000},

    // Rudder (1 type)
    {CategoryType::RUDDER, "RSA", ProtocolType::NMEA0183},

    // Engine (2 types)
    {CategoryType::ENGINE, "PGN127488", ProtocolType::NMEA2000},
    {CategoryType::ENGINE, "PGN127489", ProtocolType::NMEA2000},
};

static const int MESSAGE_MAPPING_COUNT = sizeof(MESSAGE_MAPPINGS) / sizeof(MessageMapping);

void SourceRegistry::init(WebSocketLogger* logger) {
    // Store logger reference
    logger_ = logger;

    // Initialize all categories
    for (int i = 0; i < 9; i++) {
        categories_[i].init((CategoryType)i);
    }

    totalSources_ = 0;
    hasChanges_ = false;
    lastGCTime_ = millis();

    // Pre-populate message type entries for known mappings
    for (int i = 0; i < MESSAGE_MAPPING_COUNT; i++) {
        const MessageMapping& mapping = MESSAGE_MAPPINGS[i];
        getOrCreateMessageType(mapping.category, mapping.messageTypeId, mapping.protocol);
    }

    // Log initialization
    if (logger_ != nullptr) {
        logger_->broadcastLog(LogLevel::INFO, "SourceRegistry", "INITIALIZED",
            String(F("{\"messageTypes\":")) + MESSAGE_MAPPING_COUNT + F("}"));
    }
}

bool SourceRegistry::recordUpdate(CategoryType category, const char* messageTypeId,
                                   const char* sourceId, ProtocolType protocol) {
    // Validate inputs
    if (category >= CategoryType::COUNT || messageTypeId == nullptr || sourceId == nullptr) {
        return false;
    }

    // Find or create message type entry
    MessageTypeEntry* msgType = getOrCreateMessageType(category, messageTypeId, protocol);
    if (msgType == nullptr) {
        // Category full (8 message types)
        return false;
    }

    // Find existing source in this message type
    MessageSource* source = nullptr;
    for (uint8_t i = 0; i < msgType->sourceCount; i++) {
        if (strcmp(msgType->sources[i].sourceId, sourceId) == 0) {
            source = &msgType->sources[i];
            break;
        }
    }

    // If source not found, create new one
    if (source == nullptr) {
        // Check global source limit
        if (totalSources_ >= MAX_SOURCES) {
            // Evict oldest source to make room
            evictOldestSource();
        }

        // Check message type limit (5 sources per type)
        if (msgType->sourceCount >= 5) {
            // Message type full - cannot add source
            return false;
        }

        // Add new source
        source = &msgType->sources[msgType->sourceCount];
        source->init();
        strncpy(source->sourceId, sourceId, sizeof(source->sourceId) - 1);
        source->sourceId[sizeof(source->sourceId) - 1] = '\0';
        source->protocol = protocol;
        source->active = true;

        // Parse sourceId to extract SID/talker ID
        parseSourceId(sourceId, source->sid, source->talkerId);

        msgType->sourceCount++;
        totalSources_++;

        // Log source discovery
        if (logger_ != nullptr) {
            // Get category name
            const char* catName = getCategoryName(category);
            String data = String(F("{\"sourceId\":\"")) + sourceId + F("\",\"category\":\"") +
                         catName + F("\",\"messageType\":\"") + messageTypeId +
                         F("\",\"totalSources\":") + totalSources_ + F("}");
            logger_->broadcastLog(LogLevel::INFO, "SourceRegistry", "SOURCE_DISCOVERED", data);
        }
    }

    // Update timestamp
    uint32_t now = millis();
    source->lastUpdateTime = now;
    source->updateCount++;

    // Add timestamp to circular buffer
    FrequencyCalculator::addTimestamp(source->timestampBuffer, source->bufferIndex,
                                      source->bufferFull, now);

    // Calculate frequency if buffer is full
    if (source->bufferFull) {
        source->frequency = FrequencyCalculator::calculate(source->timestampBuffer, 10, source->bufferIndex);
    }

    // Mark as fresh (not stale)
    source->timeSinceLast = 0;
    source->isStale = false;

    hasChanges_ = true;
    return true;
}

void SourceRegistry::updateStaleFlags() {
    uint32_t now = millis();
    bool anyChanged = false;

    // Iterate all categories
    for (uint8_t catIdx = 0; catIdx < 9; catIdx++) {
        CategoryEntry& category = categories_[catIdx];

        // Iterate all message types in this category
        for (uint8_t msgIdx = 0; msgIdx < category.messageCount; msgIdx++) {
            MessageTypeEntry& msgType = category.messages[msgIdx];

            // Iterate all sources in this message type
            for (uint8_t srcIdx = 0; srcIdx < msgType.sourceCount; srcIdx++) {
                MessageSource& source = msgType.sources[srcIdx];

                if (!source.active) continue;

                // Calculate time since last update
                uint32_t timeSinceLast = now - source.lastUpdateTime;
                bool wasStale = source.isStale;

                source.timeSinceLast = timeSinceLast;
                source.isStale = (timeSinceLast > SOURCE_STALE_THRESHOLD_MS);

                // Track if any staleness flag changed
                if (source.isStale != wasStale) {
                    anyChanged = true;
                }
            }
        }
    }

    if (anyChanged) {
        hasChanges_ = true;
    }
}

void SourceRegistry::garbageCollect() {
    uint32_t now = millis();
    uint32_t startMicros = micros();
    lastGCTime_ = now;
    uint8_t removedCount = 0;
    uint8_t sourcesBeforeGC = totalSources_;

    // Iterate all categories
    for (uint8_t catIdx = 0; catIdx < 9; catIdx++) {
        CategoryEntry& category = categories_[catIdx];

        // Iterate all message types
        for (uint8_t msgIdx = 0; msgIdx < category.messageCount; msgIdx++) {
            MessageTypeEntry& msgType = category.messages[msgIdx];

            // Iterate sources in reverse (to handle removal safely)
            for (int srcIdx = msgType.sourceCount - 1; srcIdx >= 0; srcIdx--) {
                MessageSource& source = msgType.sources[srcIdx];

                if (!source.active) continue;

                // Check if source is stale beyond GC threshold
                uint32_t timeSinceLast = now - source.lastUpdateTime;
                if (timeSinceLast > SOURCE_GC_THRESHOLD_MS) {
                    // Log removal (individual event)
                    if (logger_ != nullptr) {
                        const char* catName = getCategoryName((CategoryType)catIdx);
                        String data = String(F("{\"sourceId\":\"")) + source.sourceId +
                                     F("\",\"category\":\"") + catName +
                                     F("\",\"timeSinceLast\":") + timeSinceLast +
                                     F(",\"reason\":\"GC\"}");
                        logger_->broadcastLog(LogLevel::DEBUG, "SourceRegistry", "SOURCE_REMOVED", data);
                    }

                    // Remove this source
                    source.active = false;

                    // Shift remaining sources down
                    for (uint8_t i = srcIdx; i < msgType.sourceCount - 1; i++) {
                        msgType.sources[i] = msgType.sources[i + 1];
                    }

                    msgType.sourceCount--;
                    totalSources_--;
                    removedCount++;
                    hasChanges_ = true;
                }
            }
        }
    }

    // Log GC completion
    if (logger_ != nullptr) {
        uint32_t durationMicros = micros() - startMicros;
        String data = String(F("{\"removed\":")) + removedCount +
                     F(",\"remaining\":") + totalSources_ +
                     F(",\"duration_us\":") + durationMicros + F("}");
        LogLevel level = (removedCount > 0) ? LogLevel::INFO : LogLevel::DEBUG;
        logger_->broadcastLog(level, "SourceRegistry", "GC_COMPLETE", data);
    }
}

void SourceRegistry::evictOldestSource() {
    // Find source with maximum timeSinceLast
    MessageSource* oldestSource = nullptr;
    uint32_t maxTimeSinceLast = 0;
    uint32_t now = millis();

    for (uint8_t catIdx = 0; catIdx < 9; catIdx++) {
        CategoryEntry& category = categories_[catIdx];

        for (uint8_t msgIdx = 0; msgIdx < category.messageCount; msgIdx++) {
            MessageTypeEntry& msgType = category.messages[msgIdx];

            for (uint8_t srcIdx = 0; srcIdx < msgType.sourceCount; srcIdx++) {
                MessageSource& source = msgType.sources[srcIdx];

                if (!source.active) continue;

                uint32_t timeSinceLast = now - source.lastUpdateTime;
                if (timeSinceLast > maxTimeSinceLast) {
                    maxTimeSinceLast = timeSinceLast;
                    oldestSource = &source;
                }
            }
        }
    }

    // Remove oldest source
    if (oldestSource != nullptr) {
        removeSource(oldestSource->sourceId);
    }
}

bool SourceRegistry::removeSource(const char* sourceId) {
    // Find and remove source
    for (uint8_t catIdx = 0; catIdx < 9; catIdx++) {
        CategoryEntry& category = categories_[catIdx];

        for (uint8_t msgIdx = 0; msgIdx < category.messageCount; msgIdx++) {
            MessageTypeEntry& msgType = category.messages[msgIdx];

            for (uint8_t srcIdx = 0; srcIdx < msgType.sourceCount; srcIdx++) {
                MessageSource& source = msgType.sources[srcIdx];

                if (strcmp(source.sourceId, sourceId) == 0) {
                    // Log removal before removing
                    if (logger_ != nullptr) {
                        const char* catName = getCategoryName((CategoryType)catIdx);
                        uint32_t timeSinceLast = millis() - source.lastUpdateTime;
                        String data = String(F("{\"sourceId\":\"")) + sourceId +
                                     F("\",\"category\":\"") + catName +
                                     F("\",\"timeSinceLast\":") + timeSinceLast +
                                     F(",\"totalSources\":") + (totalSources_ - 1) + F("}");
                        logger_->broadcastLog(LogLevel::INFO, "SourceRegistry", "SOURCE_REMOVED", data);
                    }

                    // Mark inactive
                    source.active = false;

                    // Shift remaining sources down
                    for (uint8_t i = srcIdx; i < msgType.sourceCount - 1; i++) {
                        msgType.sources[i] = msgType.sources[i + 1];
                    }

                    msgType.sourceCount--;
                    totalSources_--;
                    hasChanges_ = true;
                    return true;
                }
            }
        }
    }

    return false;  // Source not found
}

bool SourceRegistry::updateDeviceInfo(const char* sourceId, const DeviceInfo& info) {
    // Find the source
    MessageSource* source = findSourceMutable(sourceId);
    if (source == nullptr) {
        return false;  // Source not found
    }

    // Update device info
    source->deviceInfo = info;
    hasChanges_ = true;

    // Log device metadata update
    if (logger_ != nullptr) {
        String data = String(F("{\"sourceId\":\"")) + sourceId +
                     F("\",\"hasInfo\":") + (info.hasInfo ? F("true") : F("false"));
        if (info.hasInfo) {
            data += String(F(",\"manufacturer\":\"")) + info.manufacturer +
                   F("\",\"modelId\":\"") + info.modelId +
                   F("\",\"serialNumber\":") + info.serialNumber + F("}");
        } else {
            data += F("}");
        }
        logger_->broadcastLog(LogLevel::DEBUG, "SourceRegistry", "DEVICE_INFO_UPDATED", data);
    }

    return true;
}

const CategoryEntry* SourceRegistry::getCategory(CategoryType category) const {
    if (category >= CategoryType::COUNT) {
        return nullptr;
    }
    return &categories_[(uint8_t)category];
}

const MessageSource* SourceRegistry::findSource(const char* sourceId) const {
    for (uint8_t catIdx = 0; catIdx < 9; catIdx++) {
        const CategoryEntry& category = categories_[catIdx];

        for (uint8_t msgIdx = 0; msgIdx < category.messageCount; msgIdx++) {
            const MessageTypeEntry& msgType = category.messages[msgIdx];

            for (uint8_t srcIdx = 0; srcIdx < msgType.sourceCount; srcIdx++) {
                const MessageSource& source = msgType.sources[srcIdx];

                if (strcmp(source.sourceId, sourceId) == 0 && source.active) {
                    return &source;
                }
            }
        }
    }

    return nullptr;
}

MessageSource* SourceRegistry::findSourceMutable(const char* sourceId) {
    // Cast away const from const version
    return const_cast<MessageSource*>(findSource(sourceId));
}

MessageTypeEntry* SourceRegistry::getOrCreateMessageType(CategoryType category,
                                                          const char* messageTypeId,
                                                          ProtocolType protocol) {
    if (category >= CategoryType::COUNT) {
        return nullptr;
    }

    CategoryEntry& cat = categories_[(uint8_t)category];

    // Search for existing message type
    for (uint8_t i = 0; i < cat.messageCount; i++) {
        if (strcmp(cat.messages[i].messageTypeId, messageTypeId) == 0) {
            return &cat.messages[i];
        }
    }

    // Create new message type if space available
    if (cat.messageCount < 8) {
        MessageTypeEntry& msgType = cat.messages[cat.messageCount];
        msgType.init();
        strncpy(msgType.messageTypeId, messageTypeId, sizeof(msgType.messageTypeId) - 1);
        msgType.messageTypeId[sizeof(msgType.messageTypeId) - 1] = '\0';
        msgType.protocol = protocol;

        cat.messageCount++;
        return &msgType;
    }

    return nullptr;  // Category full
}

void SourceRegistry::parseSourceId(const char* sourceId, uint8_t& sid, char* talkerId) {
    // Format: "NMEA2000-<SID>" or "NMEA0183-<TalkerID>"

    if (strncmp(sourceId, "NMEA2000-", 9) == 0) {
        // NMEA2000 format
        sid = (uint8_t)atoi(sourceId + 9);
        talkerId[0] = '\0';
    } else if (strncmp(sourceId, "NMEA0183-", 9) == 0) {
        // NMEA0183 format
        sid = 255;
        strncpy(talkerId, sourceId + 9, 3);
        talkerId[3] = '\0';
    } else {
        // Unknown format
        sid = 255;
        talkerId[0] = '\0';
    }
}

const char* SourceRegistry::getCategoryName(CategoryType category) const {
    switch (category) {
        case CategoryType::GPS: return "GPS";
        case CategoryType::COMPASS: return "COMPASS";
        case CategoryType::WIND: return "WIND";
        case CategoryType::DST: return "DST";
        case CategoryType::RUDDER: return "RUDDER";
        case CategoryType::ENGINE: return "ENGINE";
        case CategoryType::SAILDRIVE: return "SAILDRIVE";
        case CategoryType::BATTERY: return "BATTERY";
        case CategoryType::SHORE_POWER: return "SHORE_POWER";
        default: return "UNKNOWN";
    }
}
