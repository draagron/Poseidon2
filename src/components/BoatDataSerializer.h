#ifndef BOAT_DATA_SERIALIZER_H
#define BOAT_DATA_SERIALIZER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "types/BoatDataTypes.h"
#include "components/BoatData.h"

/**
 * @file BoatDataSerializer.h
 * @brief JSON serialization component for BoatData structures
 *
 * Converts the complete BoatData repository to JSON format for WebSocket streaming.
 * Uses ArduinoJson v6.x with static allocation for memory safety.
 *
 * @note Part of Feature 011-simple-webui-as (Simple WebUI for BoatData Streaming)
 * @see specs/011-simple-webui-as/contracts/BoatDataSerializerContract.md
 */

class BoatDataSerializer {
public:
    /**
     * @brief Serialize complete BoatData structure to JSON string
     *
     * Creates a JSON representation of all 9 sensor groups (GPS, Compass, Wind, DST,
     * Rudder, Engine, Saildrive, Battery, ShorePower) with their respective fields,
     * availability flags, and timestamps.
     *
     * @param boatData Pointer to BoatData repository (must not be nullptr)
     * @return String JSON-formatted string (~1500-1800 bytes), empty string on error
     *
     * @note Performance: <50ms on ESP32 @ 240 MHz
     * @note Memory: Uses static buffer (2048 bytes), no heap allocations
     * @note Error Handling: Returns empty string on null pointer or buffer overflow
     *
     * @example
     * String json = BoatDataSerializer::toJSON(boatData);
     * wsBoatData.textAll(json);  // Broadcast to WebSocket clients
     */
    static String toJSON(BoatData* boatData);

private:
    // Static JSON buffer size (sufficient for all BoatData fields + 27% margin)
    static constexpr size_t JSON_BUFFER_SIZE = 2048;

    /**
     * @brief Serialize GPS data group
     * @param doc ArduinoJson document
     * @param data GPS data structure
     */
    static void serializeGPS(JsonDocument& doc, const GPSData& data);

    /**
     * @brief Serialize Compass data group
     * @param doc ArduinoJson document
     * @param data Compass data structure
     */
    static void serializeCompass(JsonDocument& doc, const CompassData& data);

    /**
     * @brief Serialize Wind data group
     * @param doc ArduinoJson document
     * @param data Wind data structure
     */
    static void serializeWind(JsonDocument& doc, const WindData& data);

    /**
     * @brief Serialize DST (Depth/Speed/Temperature) data group
     * @param doc ArduinoJson document
     * @param data DST data structure
     */
    static void serializeDST(JsonDocument& doc, const DSTData& data);

    /**
     * @brief Serialize Rudder data group
     * @param doc ArduinoJson document
     * @param data Rudder data structure
     */
    static void serializeRudder(JsonDocument& doc, const RudderData& data);

    /**
     * @brief Serialize Engine data group
     * @param doc ArduinoJson document
     * @param data Engine data structure
     */
    static void serializeEngine(JsonDocument& doc, const EngineData& data);

    /**
     * @brief Serialize Saildrive data group
     * @param doc ArduinoJson document
     * @param data Saildrive data structure
     */
    static void serializeSaildrive(JsonDocument& doc, const SaildriveData& data);

    /**
     * @brief Serialize Battery data group (dual banks A and B)
     * @param doc ArduinoJson document
     * @param data Battery data structure
     */
    static void serializeBattery(JsonDocument& doc, const BatteryData& data);

    /**
     * @brief Serialize Shore Power data group
     * @param doc ArduinoJson document
     * @param data Shore Power data structure
     */
    static void serializeShorePower(JsonDocument& doc, const ShorePowerData& data);

    /**
     * @brief Serialize Derived data group (calculated sailing parameters)
     * @param doc ArduinoJson document
     * @param data Derived data structure
     */
    static void serializeDerived(JsonDocument& doc, const DerivedData& data);
};

#endif // BOAT_DATA_SERIALIZER_H
