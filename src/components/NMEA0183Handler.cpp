#include "NMEA0183Handler.h"
#include "utils/UnitConverter.h"
#include "utils/NMEA0183Parsers.h"
#include <NMEA0183Messages.h>
#include <cmath>
#include <cstring>

// Static instance pointer for callback (single instance pattern)
static NMEA0183Handler* s_instance = nullptr;

// Handler dispatch table
const NMEA0183Handler::HandlerEntry NMEA0183Handler::handlers_[] = {
    {"RSA", &NMEA0183Handler::handleRSA},
    {"HDM", &NMEA0183Handler::handleHDM},
    {"GGA", &NMEA0183Handler::handleGGA},
    {"RMC", &NMEA0183Handler::handleRMC},
    {"VTG", &NMEA0183Handler::handleVTG}
};

const size_t NMEA0183Handler::handlersCount_ = sizeof(handlers_) / sizeof(handlers_[0]);

// Static callback for NMEA0183 library
void NMEA0183MsgHandler(const tNMEA0183Msg &msg) {
    if (s_instance != nullptr) {
        s_instance->dispatchMessage(msg);
    }
}

NMEA0183Handler::NMEA0183Handler(tNMEA0183* nmea0183, ISerialPort* serialPort,
                                 BoatData* boatData, WebSocketLogger* logger)
    : nmea0183_(nmea0183), serialPort_(serialPort), boatData_(boatData), logger_(logger) {
    s_instance = this;  // Register this instance for callbacks
}

void NMEA0183Handler::init() {
    // Initialize Serial2 at 38400 baud (project configuration)
    serialPort_->begin(38400);

    // Register message handler callback
    nmea0183_->SetMsgHandler(NMEA0183MsgHandler);

    logger_->broadcastLog(LogLevel::INFO, "NMEA0183", "INIT",
                         "{\"port\":\"Serial2\",\"baud\":38400}");
}

void NMEA0183Handler::processSentences() {
    // Process all available NMEA 0183 messages
    // The NMEA0183 library's ParseMessages() reads from the stream and calls
    // the message handler callback for each complete sentence
    nmea0183_->ParseMessages();
}

void NMEA0183Handler::dispatchMessage(const tNMEA0183Msg& msg) {
    // Look up handler in dispatch table
    const char* msgCode = msg.MessageCode();
    for (size_t i = 0; i < handlersCount_; i++) {
        if (strcmp(msgCode, handlers_[i].messageCode) == 0) {
            // Call handler function
            (this->*(handlers_[i].handler))(msg);
            break;
        }
    }
    // If message code not in table, silently ignore (FR-007)
}

void NMEA0183Handler::handleRSA(const tNMEA0183Msg& msg) {
    double rudderAngle;

    // Parse RSA sentence (validates talker ID and status internally)
    if (!NMEA0183ParseRSA(msg, rudderAngle)) {
        return;  // Silent discard - invalid sentence or wrong talker ID
    }

    // Validate range: ±90° (FR-026)
    if (fabs(rudderAngle) > 90.0) {
        return;  // Silent discard - out of range
    }

    // Convert to radians
    double angleRadians = UnitConverter::degreesToRadians(rudderAngle);

    // Update BoatData
    bool accepted = boatData_->updateRudder(angleRadians, "NMEA0183-AP");

    // Log if accepted (DEBUG level for valid sentences)
    if (accepted) {
        String logData = String("{\"type\":\"RSA\",\"source\":\"NMEA0183-AP\",\"value\":") +
                        String(angleRadians, 4) + String("}");
        logger_->broadcastLog(LogLevel::DEBUG, "NMEA0183", "SENTENCE_PROCESSED", logData);
    }
}

void NMEA0183Handler::handleHDM(const tNMEA0183Msg& msg) {
    // Validate talker ID is "AP" (autopilot)
    if (strcmp(msg.Sender(), "AP") != 0) {
        return;  // Silent discard - wrong talker ID
    }

    double heading;

    // Parse HDM sentence using library parser
    if (!NMEA0183ParseHDM_nc(msg, heading)) {
        return;  // Silent discard - malformed sentence
    }

    // Validate range: [0°, 360°]
    if (heading < 0.0 || heading > 360.0) {
        return;  // Silent discard - out of range
    }

    // Convert to radians
    double headingRadians = UnitConverter::degreesToRadians(heading);

    // Update BoatData (trueHdg=0.0 not updated by HDM, variation=0.0 not updated)
    bool accepted = boatData_->updateCompass(0.0, headingRadians, 0.0, "NMEA0183-AP");

    // Log if accepted
    if (accepted) {
        String logData = String("{\"type\":\"HDM\",\"source\":\"NMEA0183-AP\",\"value\":") +
                        String(headingRadians, 4) + String("}");
        logger_->broadcastLog(LogLevel::DEBUG, "NMEA0183", "SENTENCE_PROCESSED", logData);
    }
}

void NMEA0183Handler::handleGGA(const tNMEA0183Msg& msg) {
    // Validate talker ID is "VH" (VHF radio)
    if (strcmp(msg.Sender(), "VH") != 0) {
        return;  // Silent discard - wrong talker ID
    }

    double GPSTime;
    double Latitude, Longitude, Altitude, GeoidalSeparation, DGPSAge;
    int SatelliteCount;
    double HDOP;
    int GPSQualityIndicator;
    int DGPSReferenceStationID;

    // Parse GGA sentence using library parser
    if (!NMEA0183ParseGGA_nc(msg, GPSTime, Latitude, Longitude, GPSQualityIndicator,
                              SatelliteCount, HDOP, Altitude, GeoidalSeparation,
                              DGPSAge, DGPSReferenceStationID)) {
        return;  // Silent discard - malformed sentence
    }

    // Validate fix quality (must have valid fix)
    if (GPSQualityIndicator == 0) {
        return;  // Silent discard - no fix
    }

    // Validate range: latitude [-90, 90], longitude [-180, 180]
    if (fabs(Latitude) > 90.0 || fabs(Longitude) > 180.0) {
        return;  // Silent discard - out of range
    }

    // Update BoatData (COG/SOG not in GGA, set to 0.0)
    bool accepted = boatData_->updateGPS(Latitude, Longitude, 0.0, 0.0, "NMEA0183-VH");

    // Log if accepted
    if (accepted) {
        String logData = String("{\"type\":\"GGA\",\"source\":\"NMEA0183-VH\",\"lat\":") +
                        String(Latitude, 6) + String(",\"lon\":") + String(Longitude, 6) + String("}");
        logger_->broadcastLog(LogLevel::DEBUG, "NMEA0183", "SENTENCE_PROCESSED", logData);
    }
}

void NMEA0183Handler::handleRMC(const tNMEA0183Msg& msg) {
    // Validate talker ID is "VH" (VHF radio)
    if (strcmp(msg.Sender(), "VH") != 0) {
        return;  // Silent discard - wrong talker ID
    }

    double GPSTime;
    double Latitude, Longitude, TrueCourse, SpeedOverGround;
    unsigned long GPSDate;
    double Variation;

    // Parse RMC sentence using library parser
    if (!NMEA0183ParseRMC_nc(msg, GPSTime, Latitude, Longitude, TrueCourse,
                              SpeedOverGround, GPSDate, Variation)) {
        return;  // Silent discard - malformed sentence
    }

    // Validate coordinate ranges
    if (fabs(Latitude) > 90.0 || fabs(Longitude) > 180.0) {
        return;  // Silent discard - out of range
    }

    // Validate SOG range [0, 100 knots]
    if (SpeedOverGround < 0.0 || SpeedOverGround > 100.0) {
        return;  // Silent discard - invalid speed
    }

    // Validate variation range: ±30° (FR-026)
    if (fabs(Variation) > 30.0) {
        return;  // Silent discard - invalid variation
    }

    // Convert units
    double cogRadians = UnitConverter::degreesToRadians(TrueCourse);
    double variationRadians = UnitConverter::degreesToRadians(Variation);

    // Update GPS data
    bool gpsAccepted = boatData_->updateGPS(Latitude, Longitude, cogRadians,
                                            SpeedOverGround, "NMEA0183-VH");

    // Update compass variation
    bool compassAccepted = boatData_->updateCompass(0.0, 0.0, variationRadians, "NMEA0183-VH");

    // Log if accepted
    if (gpsAccepted || compassAccepted) {
        String logData = String("{\"type\":\"RMC\",\"source\":\"NMEA0183-VH\",\"lat\":") +
                        String(Latitude, 6) + String(",\"lon\":") + String(Longitude, 6) +
                        String(",\"cog\":") + String(cogRadians, 4) + String(",\"sog\":") +
                        String(SpeedOverGround, 2) + String(",\"var\":") + String(variationRadians, 4) + String("}");
        logger_->broadcastLog(LogLevel::DEBUG, "NMEA0183", "SENTENCE_PROCESSED", logData);
    }
}

void NMEA0183Handler::handleVTG(const tNMEA0183Msg& msg) {
    // Validate talker ID is "VH" (VHF radio)
    if (strcmp(msg.Sender(), "VH") != 0) {
        return;  // Silent discard - wrong talker ID
    }

    double TrueCourse, MagneticCourse, SpeedKnots;

    // Parse VTG sentence using library parser (only 3 parameters)
    if (!NMEA0183ParseVTG_nc(msg, TrueCourse, MagneticCourse, SpeedKnots)) {
        return;  // Silent discard - malformed sentence
    }

    // Calculate variation from COG difference
    double variation = UnitConverter::calculateVariation(TrueCourse, MagneticCourse);

    // Validate variation range: ±30° (FR-026)
    if (fabs(variation) > 30.0) {
        return;  // Silent discard - calculated variation out of range
    }

    // Validate SOG range [0, 100 knots]
    if (SpeedKnots < 0.0 || SpeedKnots > 100.0) {
        return;  // Silent discard - invalid speed
    }

    // Convert units
    double trueCOGRadians = UnitConverter::degreesToRadians(TrueCourse);
    double variationRadians = UnitConverter::degreesToRadians(variation);

    // Update GPS data (lat/lon not in VTG, set to 0.0)
    bool gpsAccepted = boatData_->updateGPS(0.0, 0.0, trueCOGRadians, SpeedKnots, "NMEA0183-VH");

    // Update compass variation
    bool compassAccepted = boatData_->updateCompass(0.0, 0.0, variationRadians, "NMEA0183-VH");

    // Log if accepted
    if (gpsAccepted || compassAccepted) {
        String logData = String("{\"type\":\"VTG\",\"source\":\"NMEA0183-VH\",\"cog\":") +
                        String(trueCOGRadians, 4) + String(",\"sog\":") + String(SpeedKnots, 2) +
                        String(",\"var\":") + String(variationRadians, 4) + String("}");
        logger_->broadcastLog(LogLevel::DEBUG, "NMEA0183", "SENTENCE_PROCESSED", logData);
    }
}
