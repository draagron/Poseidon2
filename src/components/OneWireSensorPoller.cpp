/**
 * @file OneWireSensorPoller.cpp
 * @brief Implementation of 1-Wire sensor polling logic
 */

#include "OneWireSensorPoller.h"
#include <Arduino.h>

OneWireSensorPoller::OneWireSensorPoller(ESP32OneWireSensors* sensors, BoatData* data, WebSocketLogger* log)
    : oneWireSensors(sensors), boatData(data), logger(log) {
}

void OneWireSensorPoller::pollSaildriveData() {
    if (oneWireSensors == nullptr || boatData == nullptr) {
        return;
    }

    SaildriveData saildriveData;
    if (oneWireSensors->readSaildriveStatus(saildriveData)) {
        boatData->setSaildriveData(saildriveData);
        logger->broadcastLog(LogLevel::DEBUG, "OneWire", "SAILDRIVE_UPDATE",
            String(F("{\"engaged\":")) + (saildriveData.saildriveEngaged ? F("true") : F("false")) + F("}"));
    } else {
        logger->broadcastLog(LogLevel::WARN, "OneWire", "SAILDRIVE_READ_FAILED",
            F("{\"reason\":\"Sensor read error or CRC failure\"}"));
    }
}

void OneWireSensorPoller::pollBatteryData() {
    if (oneWireSensors == nullptr || boatData == nullptr) {
        return;
    }

    BatteryMonitorData batteryA, batteryB;
    bool successA = oneWireSensors->readBatteryA(batteryA);
    bool successB = oneWireSensors->readBatteryB(batteryB);

    if (successA && successB) {
        // Combine into BatteryData structure
        BatteryData batteryData;
        batteryData.voltageA = batteryA.voltage;
        batteryData.amperageA = batteryA.amperage;
        batteryData.stateOfChargeA = batteryA.stateOfCharge;
        batteryData.shoreChargerOnA = batteryA.shoreChargerOn;
        batteryData.engineChargerOnA = batteryA.engineChargerOn;
        batteryData.voltageB = batteryB.voltage;
        batteryData.amperageB = batteryB.amperage;
        batteryData.stateOfChargeB = batteryB.stateOfCharge;
        batteryData.shoreChargerOnB = batteryB.shoreChargerOn;
        batteryData.engineChargerOnB = batteryB.engineChargerOn;
        batteryData.available = true;
        batteryData.lastUpdate = millis();

        boatData->setBatteryData(batteryData);

        logger->broadcastLog(LogLevel::DEBUG, "OneWire", "BATTERY_UPDATE",
            String(F("{\"battA_V\":")) + batteryA.voltage +
            F(",\"battA_A\":") + batteryA.amperage +
            F(",\"battA_SOC\":") + batteryA.stateOfCharge +
            F(",\"battB_V\":") + batteryB.voltage +
            F(",\"battB_A\":") + batteryB.amperage +
            F(",\"battB_SOC\":") + batteryB.stateOfCharge + F("}"));
    } else {
        logger->broadcastLog(LogLevel::WARN, "OneWire", "BATTERY_READ_FAILED",
            String(F("{\"battA_success\":")) + (successA ? F("true") : F("false")) +
            F(",\"battB_success\":") + (successB ? F("true") : F("false")) + F("}"));
    }
}

void OneWireSensorPoller::pollShorePowerData() {
    if (oneWireSensors == nullptr || boatData == nullptr) {
        return;
    }

    ShorePowerData shorePower;
    if (oneWireSensors->readShorePower(shorePower)) {
        boatData->setShorePowerData(shorePower);
        logger->broadcastLog(LogLevel::DEBUG, "OneWire", "SHORE_POWER_UPDATE",
            String(F("{\"connected\":")) + (shorePower.shorePowerOn ? F("true") : F("false")) +
            F(",\"power_W\":") + shorePower.power + F("}"));
    } else {
        logger->broadcastLog(LogLevel::WARN, "OneWire", "SHORE_POWER_READ_FAILED",
            F("{\"reason\":\"Sensor read error or CRC failure\"}"));
    }
}
