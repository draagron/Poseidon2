/**
 * @file SourceStatsSerializer.cpp
 * @brief Implementation of JSON serialization for source statistics
 *
 * @see SourceStatsSerializer.h for interface documentation
 */

#include "SourceStatsSerializer.h"

String SourceStatsSerializer::toFullSnapshotJSON(const SourceRegistry* registry) {
    if (registry == nullptr) {
        return "";
    }

    StaticJsonDocument<FULL_SNAPSHOT_BUFFER_SIZE> doc;

    // Message envelope
    doc["event"] = "fullSnapshot";
    doc["version"] = 1;
    doc["timestamp"] = millis();

    // Create sources object (hierarchical: category -> message type -> sources array)
    JsonObject sources = doc.createNestedObject("sources");

    // Iterate all categories
    for (uint8_t catIdx = 0; catIdx < 9; catIdx++) {
        const CategoryEntry* category = registry->getCategory((CategoryType)catIdx);
        if (category == nullptr || category->messageCount == 0) {
            continue;
        }

        // Create category object
        JsonObject catObj = sources.createNestedObject(getCategoryName(category->category));

        // Iterate all message types in this category
        for (uint8_t msgIdx = 0; msgIdx < category->messageCount; msgIdx++) {
            const MessageTypeEntry& msgType = category->messages[msgIdx];

            if (msgType.sourceCount == 0) {
                continue;
            }

            // Create message type array
            JsonArray msgArray = catObj.createNestedArray(msgType.messageTypeId);

            // Iterate all sources in this message type
            for (uint8_t srcIdx = 0; srcIdx < msgType.sourceCount; srcIdx++) {
                const MessageSource& source = msgType.sources[srcIdx];

                if (!source.active) {
                    continue;
                }

                // Create source object
                JsonObject srcObj = msgArray.createNestedObject();
                srcObj["sourceId"] = source.sourceId;
                srcObj["protocol"] = getProtocolName(source.protocol);
                srcObj["frequency"] = round(source.frequency * 10.0) / 10.0;  // 1 decimal place
                srcObj["timeSinceLast"] = source.timeSinceLast;
                srcObj["isStale"] = source.isStale;
            }
        }
    }

    // Serialize to string
    String output;
    serializeJson(doc, output);

    return output;
}

String SourceStatsSerializer::toDeltaJSON(const SourceRegistry* registry) {
    if (registry == nullptr) {
        return "";
    }

    StaticJsonDocument<DELTA_UPDATE_BUFFER_SIZE> doc;

    // Message envelope
    doc["event"] = "deltaUpdate";
    doc["timestamp"] = millis();

    // Create changes array
    JsonArray changes = doc.createNestedArray("changes");

    // Iterate all categories to find changed sources
    // For this implementation, we include all active sources with updated timeSinceLast
    // A more sophisticated implementation could track previous state to detect actual changes
    for (uint8_t catIdx = 0; catIdx < 9; catIdx++) {
        const CategoryEntry* category = registry->getCategory((CategoryType)catIdx);
        if (category == nullptr) {
            continue;
        }

        for (uint8_t msgIdx = 0; msgIdx < category->messageCount; msgIdx++) {
            const MessageTypeEntry& msgType = category->messages[msgIdx];

            for (uint8_t srcIdx = 0; srcIdx < msgType.sourceCount; srcIdx++) {
                const MessageSource& source = msgType.sources[srcIdx];

                if (!source.active) {
                    continue;
                }

                // Create source change object
                JsonObject change = changes.createNestedObject();
                change["sourceId"] = source.sourceId;
                change["frequency"] = round(source.frequency * 10.0) / 10.0;
                change["timeSinceLast"] = source.timeSinceLast;
                change["isStale"] = source.isStale;
            }
        }
    }

    // Serialize to string
    String output;
    serializeJson(doc, output);

    return output;
}

String SourceStatsSerializer::toRemovalJSON(const char* sourceId, const char* reason) {
    if (sourceId == nullptr || reason == nullptr) {
        return "";
    }

    StaticJsonDocument<256> doc;

    doc["event"] = "sourceRemoved";
    doc["sourceId"] = sourceId;
    doc["timestamp"] = millis();
    doc["reason"] = reason;

    String output;
    serializeJson(doc, output);

    return output;
}

const char* SourceStatsSerializer::getProtocolName(ProtocolType protocol) {
    switch (protocol) {
        case ProtocolType::NMEA0183:
            return "NMEA0183";
        case ProtocolType::NMEA2000:
            return "NMEA2000";
        default:
            return "Unknown";
    }
}

const char* SourceStatsSerializer::getCategoryName(CategoryType category) {
    switch (category) {
        case CategoryType::GPS:
            return "GPS";
        case CategoryType::COMPASS:
            return "Compass";
        case CategoryType::WIND:
            return "Wind";
        case CategoryType::DST:
            return "DST";
        case CategoryType::RUDDER:
            return "Rudder";
        case CategoryType::ENGINE:
            return "Engine";
        case CategoryType::SAILDRIVE:
            return "Saildrive";
        case CategoryType::BATTERY:
            return "Battery";
        case CategoryType::SHORE_POWER:
            return "ShorePower";
        default:
            return "Unknown";
    }
}
