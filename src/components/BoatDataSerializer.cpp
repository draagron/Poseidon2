#include "BoatDataSerializer.h"
#include "utils/WebSocketLogger.h"

// External logger reference (defined in main.cpp)
extern WebSocketLogger logger;

String BoatDataSerializer::toJSON(BoatData* boatData) {
    // Validate input
    if (boatData == nullptr) {
        logger.broadcastLog(LogLevel::ERROR, "BoatDataSerializer", "NULL_POINTER",
            F("{\"reason\":\"boatData pointer is null\"}"));
        return String("");
    }

    // Performance tracking
    unsigned long startTime = micros();

    // Create static JSON document (no heap allocation)
    StaticJsonDocument<JSON_BUFFER_SIZE> doc;

    // Add root timestamp
    doc["timestamp"] = millis();

    // Serialize all sensor groups
    serializeGPS(doc, boatData->getGPSData());
    serializeCompass(doc, boatData->getCompassData());
    serializeWind(doc, boatData->getWindData());
    serializeDST(doc, boatData->getSpeedData());  // SpeedData is typedef for DSTData
    serializeRudder(doc, boatData->getRudderData());
    serializeEngine(doc, boatData->getEngineData());
    serializeSaildrive(doc, boatData->getSaildriveData());
    serializeBattery(doc, boatData->getBatteryData());
    serializeShorePower(doc, boatData->getShorePowerData());
    serializeDerived(doc, boatData->getDerivedData());

    // Check for buffer overflow
    if (doc.overflowed()) {
        logger.broadcastLog(LogLevel::WARN, "BoatDataSerializer", "BUFFER_OVERFLOW",
            String(F("{\"buffer_size\":")) + String(JSON_BUFFER_SIZE) +
            F(",\"action\":\"increase buffer size\"}"));
        return String("");
    }

    // Serialize to string
    String output;
    size_t jsonSize = serializeJson(doc, output);

    // Performance check (<50ms requirement)
    unsigned long elapsedTime = micros() - startTime;
    if (elapsedTime > 50000) {  // 50ms = 50000 microseconds
        logger.broadcastLog(LogLevel::WARN, "BoatDataSerializer", "PERFORMANCE_EXCEEDED",
            String(F("{\"elapsed_us\":")) + String(elapsedTime) +
            F(",\"threshold_us\":50000}"));
    }

    // Success log (DEBUG level, optional in production)
    logger.broadcastLog(LogLevel::DEBUG, "BoatDataSerializer", "SERIALIZATION_SUCCESS",
        String(F("{\"size_bytes\":")) + String(jsonSize) +
        F(",\"elapsed_us\":") + String(elapsedTime) + F("}"));

    return output;
}

void BoatDataSerializer::serializeGPS(JsonDocument& doc, const GPSData& data) {
    JsonObject gps = doc.createNestedObject("gps");
    gps["latitude"] = data.latitude;
    gps["longitude"] = data.longitude;
    gps["cog"] = data.cog;
    gps["sog"] = data.sog;
    gps["variation"] = data.variation;
    gps["available"] = data.available;
    gps["lastUpdate"] = data.lastUpdate;
}

void BoatDataSerializer::serializeCompass(JsonDocument& doc, const CompassData& data) {
    JsonObject compass = doc.createNestedObject("compass");
    compass["trueHeading"] = data.trueHeading;
    compass["magneticHeading"] = data.magneticHeading;
    compass["rateOfTurn"] = data.rateOfTurn;
    compass["heelAngle"] = data.heelAngle;
    compass["pitchAngle"] = data.pitchAngle;
    compass["heave"] = data.heave;
    compass["available"] = data.available;
    compass["lastUpdate"] = data.lastUpdate;
}

void BoatDataSerializer::serializeWind(JsonDocument& doc, const WindData& data) {
    JsonObject wind = doc.createNestedObject("wind");
    wind["apparentWindAngle"] = data.apparentWindAngle;
    wind["apparentWindSpeed"] = data.apparentWindSpeed;
    wind["available"] = data.available;
    wind["lastUpdate"] = data.lastUpdate;
}

void BoatDataSerializer::serializeDST(JsonDocument& doc, const DSTData& data) {
    JsonObject dst = doc.createNestedObject("dst");
    dst["depth"] = data.depth;
    dst["measuredBoatSpeed"] = data.measuredBoatSpeed;
    dst["seaTemperature"] = data.seaTemperature;
    dst["available"] = data.available;
    dst["lastUpdate"] = data.lastUpdate;
}

void BoatDataSerializer::serializeRudder(JsonDocument& doc, const RudderData& data) {
    JsonObject rudder = doc.createNestedObject("rudder");
    rudder["steeringAngle"] = data.steeringAngle;
    rudder["available"] = data.available;
    rudder["lastUpdate"] = data.lastUpdate;
}

void BoatDataSerializer::serializeEngine(JsonDocument& doc, const EngineData& data) {
    JsonObject engine = doc.createNestedObject("engine");
    engine["engineRev"] = data.engineRev;
    engine["oilTemperature"] = data.oilTemperature;
    engine["alternatorVoltage"] = data.alternatorVoltage;
    engine["available"] = data.available;
    engine["lastUpdate"] = data.lastUpdate;
}

void BoatDataSerializer::serializeSaildrive(JsonDocument& doc, const SaildriveData& data) {
    JsonObject saildrive = doc.createNestedObject("saildrive");
    saildrive["saildriveEngaged"] = data.saildriveEngaged;
    saildrive["available"] = data.available;
    saildrive["lastUpdate"] = data.lastUpdate;
}

void BoatDataSerializer::serializeBattery(JsonDocument& doc, const BatteryData& data) {
    JsonObject battery = doc.createNestedObject("battery");

    // Battery A (House Bank)
    battery["voltageA"] = data.voltageA;
    battery["amperageA"] = data.amperageA;
    battery["stateOfChargeA"] = data.stateOfChargeA;
    battery["shoreChargerOnA"] = data.shoreChargerOnA;
    battery["engineChargerOnA"] = data.engineChargerOnA;

    // Battery B (Starter Bank)
    battery["voltageB"] = data.voltageB;
    battery["amperageB"] = data.amperageB;
    battery["stateOfChargeB"] = data.stateOfChargeB;
    battery["shoreChargerOnB"] = data.shoreChargerOnB;
    battery["engineChargerOnB"] = data.engineChargerOnB;

    // Common metadata
    battery["available"] = data.available;
    battery["lastUpdate"] = data.lastUpdate;
}

void BoatDataSerializer::serializeShorePower(JsonDocument& doc, const ShorePowerData& data) {
    JsonObject shorePower = doc.createNestedObject("shorePower");
    shorePower["shorePowerOn"] = data.shorePowerOn;
    shorePower["power"] = data.power;
    shorePower["available"] = data.available;
    shorePower["lastUpdate"] = data.lastUpdate;
}

void BoatDataSerializer::serializeDerived(JsonDocument& doc, const DerivedData& data) {
    JsonObject derived = doc.createNestedObject("derived");

    // Corrected apparent wind angles
    derived["awaOffset"] = data.awaOffset;
    derived["awaHeel"] = data.awaHeel;

    // Leeway and speed
    derived["leeway"] = data.leeway;
    derived["stw"] = data.stw;

    // True wind
    derived["tws"] = data.tws;
    derived["twa"] = data.twa;
    derived["wdir"] = data.wdir;

    // Performance
    derived["vmg"] = data.vmg;

    // Current
    derived["soc"] = data.soc;
    derived["doc"] = data.doc;

    // Common metadata
    derived["available"] = data.available;
    derived["lastUpdate"] = data.lastUpdate;
}
