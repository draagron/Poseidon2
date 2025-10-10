/**
 * @file NMEA2000Handlers.cpp
 * @brief NMEA2000 PGN message handlers implementation for Enhanced BoatData v2.0.0
 *
 * Implements PGN handlers that parse NMEA2000 messages, validate data,
 * and update the global BoatData structure with WebSocket logging.
 *
 * @see NMEA2000Handlers.h for handler function documentation
 * @see specs/008-enhanced-boatdata-following/research.md lines 260-313
 * @version 2.0.0
 * @date 2025-10-10
 */

#include "NMEA2000Handlers.h"
#include "../utils/DataValidation.h"

// ============================================================================
// PGN 127251 - Rate of Turn
// ============================================================================

void HandleN2kPGN127251(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger) {
    if (boatData == nullptr || logger == nullptr) return;

    unsigned char SID;
    double rateOfTurn;

    if (ParseN2kPGN127251(N2kMsg, SID, rateOfTurn)) {
        // Check if data is valid (not N2kDoubleNA)
        if (N2kIsNA(rateOfTurn)) {
            logger->broadcastLog(LogLevel::DEBUG, "NMEA2000", "PGN127251_NA",
                F("{\"reason\":\"Rate of turn not available\"}"));
            return;
        }

        // Validate and clamp rate of turn
        bool valid = DataValidation::isValidRateOfTurn(rateOfTurn);
        if (!valid) {
            logger->broadcastLog(LogLevel::WARN, "NMEA2000", "PGN127251_OUT_OF_RANGE",
                String(F("{\"rateOfTurn\":")) + rateOfTurn + F(",\"clamped\":") +
                DataValidation::clampRateOfTurn(rateOfTurn) + F("}"));
            rateOfTurn = DataValidation::clampRateOfTurn(rateOfTurn);
        }

        // Get current compass data
        CompassData compass = boatData->getCompassData();

        // Update rate of turn
        compass.rateOfTurn = rateOfTurn;
        compass.available = true;
        compass.lastUpdate = millis();

        // Store updated data
        boatData->setCompassData(compass);

        // Log update (DEBUG level)
        logger->broadcastLog(LogLevel::DEBUG, "NMEA2000", "PGN127251_UPDATE",
            String(F("{\"rateOfTurn\":")) + rateOfTurn + F(",\"rad_per_sec\":true}"));

        // Increment message counter
        boatData->incrementNMEA2000Count();
    } else {
        logger->broadcastLog(LogLevel::ERROR, "NMEA2000", "PGN127251_PARSE_FAILED",
            F("{\"reason\":\"Failed to parse PGN 127251\"}"));
    }
}

// ============================================================================
// PGN 127252 - Heave
// ============================================================================

void HandleN2kPGN127252(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger) {
    if (boatData == nullptr || logger == nullptr) return;

    unsigned char SID;
    double heave, delay;
    tN2kDelaySource delaySource;

    if (ParseN2kPGN127252(N2kMsg, SID, heave, delay, delaySource)) {
        // Check if heave data is valid (not N2kDoubleNA)
        if (N2kIsNA(heave)) {
            logger->broadcastLog(LogLevel::DEBUG, "NMEA2000", "PGN127252_NA",
                F("{\"reason\":\"Heave not available\"}"));
            return;
        }

        // Validate and clamp heave
        bool valid = DataValidation::isValidHeave(heave);
        if (!valid) {
            logger->broadcastLog(LogLevel::WARN, "NMEA2000", "PGN127252_OUT_OF_RANGE",
                String(F("{\"heave\":")) + heave + F(",\"clamped\":") +
                DataValidation::clampHeave(heave) + F("}"));
            heave = DataValidation::clampHeave(heave);
        }

        // Get current compass data
        CompassData compass = boatData->getCompassData();

        // Update heave
        compass.heave = heave;
        compass.available = true;
        compass.lastUpdate = millis();

        // Store updated data
        boatData->setCompassData(compass);

        // Log update (DEBUG level)
        logger->broadcastLog(LogLevel::DEBUG, "NMEA2000", "PGN127252_UPDATE",
            String(F("{\"heave\":")) + heave + F(",\"meters\":true}"));

        // Increment message counter
        boatData->incrementNMEA2000Count();
    } else {
        logger->broadcastLog(LogLevel::ERROR, "NMEA2000", "PGN127252_PARSE_FAILED",
            F("{\"reason\":\"Failed to parse PGN 127252\"}"));
    }
}

// ============================================================================
// PGN 127257 - Attitude (Heel, Pitch, Heave)
// ============================================================================

void HandleN2kPGN127257(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger) {
    if (boatData == nullptr || logger == nullptr) return;

    unsigned char SID;
    double yaw, pitch, roll;

    if (ParseN2kPGN127257(N2kMsg, SID, yaw, pitch, roll)) {
        // Get current compass data
        CompassData compass = boatData->getCompassData();
        bool dataValid = true;

        // Process pitch angle (bow up/down)
        if (!N2kIsNA(pitch)) {
            bool validPitch = DataValidation::isValidPitchAngle(pitch);
            if (!validPitch) {
                logger->broadcastLog(LogLevel::WARN, "NMEA2000", "PGN127257_PITCH_OUT_OF_RANGE",
                    String(F("{\"pitch\":")) + pitch + F(",\"max\":") + (M_PI/6) +
                    F(",\"clamped\":") + DataValidation::clampPitchAngle(pitch) + F("}"));
                pitch = DataValidation::clampPitchAngle(pitch);
                dataValid = false;
            }
            compass.pitchAngle = pitch;
        }

        // Process heel angle (roll = heel in marine context)
        if (!N2kIsNA(roll)) {
            bool validHeel = DataValidation::isValidHeelAngle(roll);
            if (!validHeel) {
                // Warning if exceeds ±45° but still within ±90° range
                logger->broadcastLog(LogLevel::WARN, "NMEA2000", "PGN127257_HEEL_EXCESSIVE",
                    String(F("{\"heel\":")) + roll + F(",\"degrees\":") + (roll * 180.0 / M_PI) + F("}"));
            }
            compass.heelAngle = DataValidation::clampHeelAngle(roll);
        }

        // Note: PGN 127257 doesn't include heave in standard parsing
        // Heave may come from a different PGN or require extended parsing
        // For now, we'll leave heave unchanged

        // Update availability and timestamp
        compass.available = dataValid;
        compass.lastUpdate = millis();

        // Store updated data
        boatData->setCompassData(compass);

        // Log update (DEBUG level)
        logger->broadcastLog(LogLevel::DEBUG, "NMEA2000", "PGN127257_UPDATE",
            String(F("{\"heel\":")) + compass.heelAngle + F(",\"pitch\":") +
            compass.pitchAngle + F(",\"valid\":") + (dataValid ? F("true") : F("false")) + F("}"));

        // Increment message counter
        boatData->incrementNMEA2000Count();
    } else {
        logger->broadcastLog(LogLevel::ERROR, "NMEA2000", "PGN127257_PARSE_FAILED",
            F("{\"reason\":\"Failed to parse PGN 127257\"}"));
    }
}

// ============================================================================
// PGN 129029 - GNSS Position Data (Enhanced for Variation)
// ============================================================================

void HandleN2kPGN129029(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger) {
    if (boatData == nullptr || logger == nullptr) return;

    unsigned char SID;
    uint16_t DaysSince1970;
    double SecondsSinceMidnight;
    double Latitude, Longitude, Altitude;
    tN2kGNSStype GNSStype;
    tN2kGNSSmethod GNSSmethod;
    unsigned char nSatellites;
    double HDOP;
    double PDOP;
    double GeoidalSeparation;
    unsigned char nReferenceStations;
    tN2kGNSStype ReferenceStationType;
    uint16_t ReferenceStationID;
    double AgeOfCorrection;

    if (ParseN2kPGN129029(N2kMsg, SID, DaysSince1970, SecondsSinceMidnight,
                          Latitude, Longitude, Altitude,
                          GNSStype, GNSSmethod,
                          nSatellites, HDOP, PDOP, GeoidalSeparation,
                          nReferenceStations, ReferenceStationType,
                          ReferenceStationID, AgeOfCorrection)) {

        // Check if position data is valid
        if (N2kIsNA(Latitude) || N2kIsNA(Longitude)) {
            logger->broadcastLog(LogLevel::DEBUG, "NMEA2000", "PGN129029_NA",
                F("{\"reason\":\"Position not available\"}"));
            return;
        }

        // Get current GPS data
        GPSData gps = boatData->getGPSData();

        // Update position data
        gps.latitude = Latitude;
        gps.longitude = Longitude;

        // Note: PGN 129029 doesn't include COG/SOG in standard parsing
        // Those come from PGN 129026 (COG & SOG, Rapid Update)
        // For now, we'll preserve existing COG/SOG values

        // Note: PGN 129029 doesn't include magnetic variation in standard parsing
        // Variation typically comes from PGN 127258 (Magnetic Variation) or
        // can be calculated from position using WMM model
        // For now, we'll preserve existing variation value

        // Update availability and timestamp
        gps.available = true;
        gps.lastUpdate = millis();

        // Store updated data
        boatData->setGPSData(gps);

        // Log update (DEBUG level)
        logger->broadcastLog(LogLevel::DEBUG, "NMEA2000", "PGN129029_UPDATE",
            String(F("{\"lat\":")) + Latitude + F(",\"lon\":") + Longitude +
            F(",\"sats\":") + nSatellites + F("}"));

        // Increment message counter
        boatData->incrementNMEA2000Count();
    } else {
        logger->broadcastLog(LogLevel::ERROR, "NMEA2000", "PGN129029_PARSE_FAILED",
            F("{\"reason\":\"Failed to parse PGN 129029\"}"));
    }
}

// ============================================================================
// PGN 128267 - Water Depth
// ============================================================================

void HandleN2kPGN128267(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger) {
    if (boatData == nullptr || logger == nullptr) return;

    unsigned char SID;
    double DepthBelowTransducer;
    double Offset;
    double Range;

    if (ParseN2kPGN128267(N2kMsg, SID, DepthBelowTransducer, Offset, Range)) {
        // Check if depth is valid
        if (N2kIsNA(DepthBelowTransducer)) {
            logger->broadcastLog(LogLevel::DEBUG, "NMEA2000", "PGN128267_NA",
                F("{\"reason\":\"Depth not available\"}"));
            return;
        }

        // Calculate depth below waterline (transducer depth + offset)
        double depth = DepthBelowTransducer;
        if (!N2kIsNA(Offset)) {
            depth += Offset;
        }

        // Validate depth
        bool valid = DataValidation::isValidDepth(depth);
        if (!valid) {
            logger->broadcastLog(LogLevel::WARN, "NMEA2000", "PGN128267_INVALID_DEPTH",
                String(F("{\"depth\":")) + depth + F(",\"reason\":\"negative or excessive\"}"));
            depth = DataValidation::clampDepth(depth);
        }

        // Get current DST data
        DSTData dst = boatData->getSpeedData(); // Using compatibility method

        // Update depth
        dst.depth = depth;
        dst.available = valid;
        dst.lastUpdate = millis();

        // Store updated data
        boatData->setSpeedData(dst);

        // Log update (DEBUG level)
        logger->broadcastLog(LogLevel::DEBUG, "NMEA2000", "PGN128267_UPDATE",
            String(F("{\"depth\":")) + depth + F(",\"offset\":") + Offset +
            F(",\"valid\":") + (valid ? F("true") : F("false")) + F("}"));

        // Increment message counter
        boatData->incrementNMEA2000Count();
    } else {
        logger->broadcastLog(LogLevel::ERROR, "NMEA2000", "PGN128267_PARSE_FAILED",
            F("{\"reason\":\"Failed to parse PGN 128267\"}"));
    }
}

// ============================================================================
// PGN 128259 - Speed (Water Referenced)
// ============================================================================

void HandleN2kPGN128259(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger) {
    if (boatData == nullptr || logger == nullptr) return;

    unsigned char SID;
    double WaterReferenced;
    double GroundReferenced;
    tN2kSpeedWaterReferenceType SWRT;

    if (ParseN2kPGN128259(N2kMsg, SID, WaterReferenced, GroundReferenced, SWRT)) {
        // Check if water-referenced speed is valid
        if (N2kIsNA(WaterReferenced)) {
            logger->broadcastLog(LogLevel::DEBUG, "NMEA2000", "PGN128259_NA",
                F("{\"reason\":\"Water speed not available\"}"));
            return;
        }

        // Validate boat speed (NMEA2000 reports in m/s)
        bool valid = DataValidation::isValidBoatSpeed(WaterReferenced);
        if (!valid) {
            logger->broadcastLog(LogLevel::WARN, "NMEA2000", "PGN128259_OUT_OF_RANGE",
                String(F("{\"speed\":")) + WaterReferenced + F(",\"clamped\":") +
                DataValidation::clampBoatSpeed(WaterReferenced) + F("}"));
            WaterReferenced = DataValidation::clampBoatSpeed(WaterReferenced);
        }

        // Get current DST data
        DSTData dst = boatData->getSpeedData();

        // Update measured boat speed (in m/s, matching NMEA2000 unit)
        dst.measuredBoatSpeed = WaterReferenced;
        dst.available = valid;
        dst.lastUpdate = millis();

        // Store updated data
        boatData->setSpeedData(dst);

        // Log update (DEBUG level)
        logger->broadcastLog(LogLevel::DEBUG, "NMEA2000", "PGN128259_UPDATE",
            String(F("{\"speed_m_s\":")) + WaterReferenced + F(",\"valid\":") +
            (valid ? F("true") : F("false")) + F("}"));

        // Increment message counter
        boatData->incrementNMEA2000Count();
    } else {
        logger->broadcastLog(LogLevel::ERROR, "NMEA2000", "PGN128259_PARSE_FAILED",
            F("{\"reason\":\"Failed to parse PGN 128259\"}"));
    }
}

// ============================================================================
// PGN 130316 - Temperature Extended Range
// ============================================================================

void HandleN2kPGN130316(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger) {
    if (boatData == nullptr || logger == nullptr) return;

    unsigned char SID;
    unsigned char TempInstance;
    tN2kTempSource TempSource;
    double ActualTemperature;
    double SetTemperature;

    if (ParseN2kPGN130316(N2kMsg, SID, TempInstance, TempSource,
                          ActualTemperature, SetTemperature)) {

        // We're only interested in sea temperature
        if (TempSource != N2kts_SeaTemperature) {
            // Ignore other temperature sources (exhaust, engine room, etc.)
            return;
        }

        // Check if temperature is valid
        if (N2kIsNA(ActualTemperature)) {
            logger->broadcastLog(LogLevel::DEBUG, "NMEA2000", "PGN130316_NA",
                F("{\"reason\":\"Sea temperature not available\"}"));
            return;
        }

        // Convert from Kelvin to Celsius
        double tempCelsius = DataValidation::kelvinToCelsius(ActualTemperature);

        // Validate water temperature
        bool valid = DataValidation::isValidWaterTemperature(tempCelsius);
        if (!valid) {
            logger->broadcastLog(LogLevel::WARN, "NMEA2000", "PGN130316_OUT_OF_RANGE",
                String(F("{\"temp_celsius\":")) + tempCelsius + F(",\"clamped\":") +
                DataValidation::clampWaterTemperature(tempCelsius) + F("}"));
            tempCelsius = DataValidation::clampWaterTemperature(tempCelsius);
        }

        // Get current DST data
        DSTData dst = boatData->getSpeedData();

        // Update sea temperature
        dst.seaTemperature = tempCelsius;
        dst.available = valid;
        dst.lastUpdate = millis();

        // Store updated data
        boatData->setSpeedData(dst);

        // Log update (DEBUG level)
        logger->broadcastLog(LogLevel::DEBUG, "NMEA2000", "PGN130316_UPDATE",
            String(F("{\"sea_temp_c\":")) + tempCelsius + F(",\"valid\":") +
            (valid ? F("true") : F("false")) + F("}"));

        // Increment message counter
        boatData->incrementNMEA2000Count();
    } else {
        logger->broadcastLog(LogLevel::ERROR, "NMEA2000", "PGN130316_PARSE_FAILED",
            F("{\"reason\":\"Failed to parse PGN 130316\"}"));
    }
}

// ============================================================================
// PGN 127488 - Engine Parameters, Rapid Update
// ============================================================================

void HandleN2kPGN127488(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger) {
    if (boatData == nullptr || logger == nullptr) return;

    unsigned char EngineInstance;
    double EngineSpeed;
    double EngineBoostPressure;
    int8_t EngineTiltTrim;

    if (ParseN2kPGN127488(N2kMsg, EngineInstance, EngineSpeed,
                          EngineBoostPressure, EngineTiltTrim)) {

        // Check if engine speed is valid
        if (N2kIsNA(EngineSpeed)) {
            logger->broadcastLog(LogLevel::DEBUG, "NMEA2000", "PGN127488_NA",
                F("{\"reason\":\"Engine speed not available\"}"));
            return;
        }

        // Validate engine RPM
        bool valid = DataValidation::isValidEngineRPM(EngineSpeed);
        if (!valid) {
            logger->broadcastLog(LogLevel::WARN, "NMEA2000", "PGN127488_RPM_OUT_OF_RANGE",
                String(F("{\"rpm\":")) + EngineSpeed + F(",\"clamped\":") +
                DataValidation::clampEngineRPM(EngineSpeed) + F("}"));
            EngineSpeed = DataValidation::clampEngineRPM(EngineSpeed);
        }

        // Get current engine data
        EngineData engine = boatData->getEngineData();
        engine.engineRev = EngineSpeed;
        engine.available = valid;
        engine.lastUpdate = millis();

        // Store updated data
        boatData->setEngineData(engine);

        // Log update (DEBUG level)
        logger->broadcastLog(LogLevel::DEBUG, "NMEA2000", "PGN127488_UPDATE",
            String(F("{\"rpm\":")) + EngineSpeed + F(",\"instance\":") +
            EngineInstance + F(",\"valid\":") + (valid ? F("true") : F("false")) + F("}"));

        // Increment message counter
        boatData->incrementNMEA2000Count();
    } else {
        logger->broadcastLog(LogLevel::ERROR, "NMEA2000", "PGN127488_PARSE_FAILED",
            F("{\"reason\":\"Failed to parse PGN 127488\"}"));
    }
}

// ============================================================================
// PGN 127489 - Engine Parameters, Dynamic
// ============================================================================

void HandleN2kPGN127489(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger) {
    if (boatData == nullptr || logger == nullptr) return;

    unsigned char EngineInstance;
    double EngineOilPress;
    double EngineOilTemp;
    double EngineCoolantTemp;
    double AltenatorVoltage;
    double FuelRate;
    double EngineHours;
    double EngineCoolantPress;
    double EngineFuelPress;
    int8_t EngineLoad;
    int8_t EngineTorque;
    tN2kEngineDiscreteStatus1 Status1;
    tN2kEngineDiscreteStatus2 Status2;

    if (ParseN2kPGN127489(N2kMsg, EngineInstance, EngineOilPress, EngineOilTemp,
                          EngineCoolantTemp, AltenatorVoltage, FuelRate, EngineHours,
                          EngineCoolantPress, EngineFuelPress, EngineLoad, EngineTorque,
                          Status1, Status2)) {

        EngineData engine;
        bool hasValidData = false;

        // Process oil temperature if available
        if (!N2kIsNA(EngineOilTemp)) {
            // Convert from Kelvin to Celsius
            double oilTempCelsius = DataValidation::kelvinToCelsius(EngineOilTemp);

            bool valid = DataValidation::isValidOilTemperature(oilTempCelsius);
            if (!valid) {
                logger->broadcastLog(LogLevel::WARN, "NMEA2000", "PGN127489_OIL_TEMP_OUT_OF_RANGE",
                    String(F("{\"temp_celsius\":")) + oilTempCelsius + F(",\"clamped\":") +
                    DataValidation::clampOilTemperature(oilTempCelsius) + F("}"));
                oilTempCelsius = DataValidation::clampOilTemperature(oilTempCelsius);
            }

            engine.oilTemperature = oilTempCelsius;
            hasValidData = true;

            // Warn if oil temperature is excessively high (>120°C)
            if (oilTempCelsius > 120.0) {
                logger->broadcastLog(LogLevel::WARN, "NMEA2000", "PGN127489_OIL_TEMP_HIGH",
                    String(F("{\"temp_celsius\":")) + oilTempCelsius + F(",\"threshold\":120}"));
            }
        }

        // Process alternator voltage if available
        if (!N2kIsNA(AltenatorVoltage)) {
            bool valid = DataValidation::isWithinVoltageRange(AltenatorVoltage);
            if (!valid) {
                logger->broadcastLog(LogLevel::WARN, "NMEA2000", "PGN127489_VOLTAGE_OUT_OF_RANGE",
                    String(F("{\"voltage\":")) + AltenatorVoltage + F(",\"clamped\":") +
                    DataValidation::clampBatteryVoltage(AltenatorVoltage) + F("}"));
                AltenatorVoltage = DataValidation::clampBatteryVoltage(AltenatorVoltage);
            }

            // Warn if voltage is outside normal 12V system range [12-15V]
            if (!DataValidation::isValidBatteryVoltage(AltenatorVoltage)) {
                logger->broadcastLog(LogLevel::WARN, "NMEA2000", "PGN127489_VOLTAGE_ABNORMAL",
                    String(F("{\"voltage\":")) + AltenatorVoltage +
                    F(",\"expected_range\":\"12-15V\"}"));
            }

            engine.alternatorVoltage = AltenatorVoltage;
            hasValidData = true;
        }

        // Update availability and timestamp
        engine.available = hasValidData;
        engine.lastUpdate = millis();

        // Store updated data
        boatData->setEngineData(engine);

        // Log update (DEBUG level)
        logger->broadcastLog(LogLevel::DEBUG, "NMEA2000", "PGN127489_UPDATE",
            String(F("{\"oil_temp_c\":")) + engine.oilTemperature +
            F(",\"alt_voltage\":") + engine.alternatorVoltage +
            F(",\"instance\":") + EngineInstance + F("}"));

        // Increment message counter
        boatData->incrementNMEA2000Count();
    } else {
        logger->broadcastLog(LogLevel::ERROR, "NMEA2000", "PGN127489_PARSE_FAILED",
            F("{\"reason\":\"Failed to parse PGN 127489\"}"));
    }
}

// ============================================================================
// Handler Registration
// ============================================================================

// Message handler class for NMEA2000 library
class N2kBoatDataHandler : public tNMEA2000::tMsgHandler {
private:
    BoatData* boatData;
    WebSocketLogger* logger;

public:
    N2kBoatDataHandler(BoatData* bd, WebSocketLogger* log)
        : boatData(bd), logger(log) {}

    void HandleMsg(const tN2kMsg &N2kMsg) override {
        switch (N2kMsg.PGN) {
            case 127251L:
                HandleN2kPGN127251(N2kMsg, boatData, logger);
                break;
            case 127252L:
                HandleN2kPGN127252(N2kMsg, boatData, logger);
                break;
            case 127257L:
                HandleN2kPGN127257(N2kMsg, boatData, logger);
                break;
            case 129029L:
                HandleN2kPGN129029(N2kMsg, boatData, logger);
                break;
            case 128267L:
                HandleN2kPGN128267(N2kMsg, boatData, logger);
                break;
            case 128259L:
                HandleN2kPGN128259(N2kMsg, boatData, logger);
                break;
            case 130316L:
                HandleN2kPGN130316(N2kMsg, boatData, logger);
                break;
            case 127488L:
                HandleN2kPGN127488(N2kMsg, boatData, logger);
                break;
            case 127489L:
                HandleN2kPGN127489(N2kMsg, boatData, logger);
                break;
            default:
                // Ignore other PGNs
                break;
        }
    }
};

// Global handler instance (will be initialized in RegisterN2kHandlers)
static N2kBoatDataHandler* globalHandler = nullptr;

void RegisterN2kHandlers(tNMEA2000* nmea2000, BoatData* boatData, WebSocketLogger* logger) {
    if (nmea2000 == nullptr || boatData == nullptr || logger == nullptr) {
        return;
    }

    // Extend receive message list to include our PGNs
    const unsigned long ReceiveMessages[] PROGMEM = {
        127251L,  // Rate of Turn
        127257L,  // Attitude
        129029L,  // GNSS Position
        128267L,  // Water Depth
        128259L,  // Speed
        130316L,  // Temperature Extended Range
        127488L,  // Engine Rapid
        127489L,  // Engine Dynamic
        0
    };

    nmea2000->ExtendReceiveMessages(ReceiveMessages);

    // Create and attach handler
    globalHandler = new N2kBoatDataHandler(boatData, logger);
    nmea2000->AttachMsgHandler(globalHandler);

    logger->broadcastLog(LogLevel::INFO, "NMEA2000", "HANDLERS_REGISTERED",
        F("{\"count\":8,\"pgns\":[127251,127257,129029,128267,128259,130316,127488,127489]}"));
}
