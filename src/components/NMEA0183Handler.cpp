#include "NMEA0183Handler.h"
#include "utils/UnitConverter.h"
#include "utils/NMEA0183Parsers.h"
#include <NMEA0183Messages.h>
#include <cmath>
#include <cstring>

// ***** TEMPORARY DEBUG FLAG - REMOVE AFTER DEBUGGING *****
#define DEBUG_RAW_SERIAL2 0  // Set to 1 to enable raw serial dump
// **********************************************************

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
                                 BoatData* boatData, WebSocketLogger* logger, SourceRegistry* registry)
    : nmea0183_(nmea0183), serialPort_(serialPort), boatData_(boatData), logger_(logger), registry_(registry) {
    s_instance = this;  // Register this instance for callbacks
}

void NMEA0183Handler::init() {
    // Initialize Serial2 at 38400 baud (project configuration)
    serialPort_->begin(38400);

    // Get underlying Stream for NMEA0183 library (required for ParseMessages)
    Stream* stream = serialPort_->getStream();
    if (stream == nullptr) {
        logger_->broadcastLog(LogLevel::ERROR, "NMEA0183", "INIT_FAILED",
                             "{\"reason\":\"Stream pointer is null\"}");
        return;
    }

    // Set message stream for NMEA0183 library (critical - library reads from this stream)
    nmea0183_->SetMessageStream(stream);

    // Register message handler callback
    nmea0183_->SetMsgHandler(NMEA0183MsgHandler);

    // Open/initialize NMEA0183 library (critical - prepares parser)
    nmea0183_->Open();

    logger_->broadcastLog(LogLevel::INFO, "NMEA0183", "INIT",
                         "{\"port\":\"Serial2\",\"baud\":38400,\"stream\":\"configured\"}");
}

void NMEA0183Handler::processSentences() {
    // Check if any data is available on Serial2 (basic connectivity check)
    int bytesAvailable = serialPort_->available();

    // Log data availability periodically (every ~5 seconds at 10ms polling = 500 calls)
    static uint16_t callCount = 0;
    static unsigned long lastAvailableLog = 0;
    callCount++;

    if (bytesAvailable > 0 && (millis() - lastAvailableLog > 5000)) {
        String logData = String("{\"bytes_available\":") + String(bytesAvailable) + String("}");
        logger_->broadcastLog(LogLevel::DEBUG, "NMEA0183", "SERIAL_DATA_AVAILABLE", logData);
        lastAvailableLog = millis();
    }

    // Log if no data for extended period (every 30 seconds)
    static unsigned long lastNoDataLog = 0;
    if (bytesAvailable == 0 && (millis() - lastNoDataLog > 30000)) {
        logger_->broadcastLog(LogLevel::WARN, "NMEA0183", "NO_SERIAL_DATA",
                             "{\"warning\":\"No data on Serial2 for 30+ seconds\"}");
        lastNoDataLog = millis();
    }

#if DEBUG_RAW_SERIAL2
    // ***** TEMPORARY DEBUG CODE - Print raw Serial2 bytes to Serial (USB) *****
    // Print all available bytes to help diagnose sentence parsing issues
    if (bytesAvailable > 0) {
        Serial.print("RAW[");
        Serial.print(bytesAvailable);
        Serial.print("]: ");

        // Read and echo all available bytes
        while (serialPort_->available() > 0) {
            int byte = serialPort_->read();
            if (byte >= 32 && byte <= 126) {
                // Printable ASCII
                Serial.write(byte);
            } else if (byte == '\r') {
                Serial.print("<CR>");
            } else if (byte == '\n') {
                Serial.print("<LF>");
            } else {
                // Non-printable - show hex
                Serial.print("<0x");
                if (byte < 16) Serial.print("0");
                Serial.print(byte, HEX);
                Serial.print(">");
            }
        }
        Serial.println();  // End of raw dump

        // WARNING: We've consumed the bytes! Parser won't see them.
        // This is intentional for debugging - we want to see RAW data before parsing
    }
#else
    // Process all available NMEA 0183 messages
    // The NMEA0183 library's ParseMessages() reads from the stream and calls
    // the message handler callback for each complete sentence
    nmea0183_->ParseMessages();
#endif
}

void NMEA0183Handler::dispatchMessage(const tNMEA0183Msg& msg) {
    // Look up handler in dispatch table
    const char* msgCode = msg.MessageCode();
    bool handled = false;

    for (size_t i = 0; i < handlersCount_; i++) {
        if (strcmp(msgCode, handlers_[i].messageCode) == 0) {
            // Call handler function
            (this->*(handlers_[i].handler))(msg);
            handled = true;
            break;
        }
    }

    // Log unhandled message codes (FR-007 - silently ignore, but log for visibility)
    if (!handled) {
        String logData = String("{\"talker\":\"") + String(msg.Sender()) +
                        String("\",\"message_code\":\"") + String(msgCode) + String("\"}");
        logger_->broadcastLog(LogLevel::DEBUG, "NMEA0183", "MESSAGE_NOT_HANDLED", logData);
    }
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

    // Record source update in registry
    if (registry_ != nullptr) {
        registry_->recordUpdate(CategoryType::RUDDER, "RSA", "NMEA0183-AP", ProtocolType::NMEA0183);
    }

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
        // Log rejection due to wrong talker ID
        String logData = String("{\"talker\":\"") + String(msg.Sender()) +
                        String("\",\"expected\":\"AP\",\"message_code\":\"HDM\"}");
        logger_->broadcastLog(LogLevel::DEBUG, "NMEA0183", "WRONG_TALKER_REJECTED", logData);
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

    // Record source update in registry
    if (registry_ != nullptr) {
        registry_->recordUpdate(CategoryType::COMPASS, "HDM", "NMEA0183-AP", ProtocolType::NMEA0183);
    }

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
        // Log rejection due to wrong talker ID
        String logData = String("{\"talker\":\"") + String(msg.Sender()) +
                        String("\",\"expected\":\"VH\",\"message_code\":\"GGA\"}");
        logger_->broadcastLog(LogLevel::DEBUG, "NMEA0183", "WRONG_TALKER_REJECTED", logData);
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

    // Record source update in registry
    if (registry_ != nullptr) {
        registry_->recordUpdate(CategoryType::GPS, "GGA", "NMEA0183-VH", ProtocolType::NMEA0183);
    }

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
        // Log rejection due to wrong talker ID
        String logData = String("{\"talker\":\"") + String(msg.Sender()) +
                        String("\",\"expected\":\"VH\",\"message_code\":\"RMC\"}");
        logger_->broadcastLog(LogLevel::DEBUG, "NMEA0183", "WRONG_TALKER_REJECTED", logData);
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

    // Record source update in registry
    if (registry_ != nullptr) {
        registry_->recordUpdate(CategoryType::GPS, "RMC", "NMEA0183-VH", ProtocolType::NMEA0183);
    }

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
        // Log rejection due to wrong talker ID
        String logData = String("{\"talker\":\"") + String(msg.Sender()) +
                        String("\",\"expected\":\"VH\",\"message_code\":\"VTG\"}");
        logger_->broadcastLog(LogLevel::DEBUG, "NMEA0183", "WRONG_TALKER_REJECTED", logData);
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

    // Record source update in registry
    if (registry_ != nullptr) {
        registry_->recordUpdate(CategoryType::GPS, "VTG", "NMEA0183-VH", ProtocolType::NMEA0183);
    }

    // Log if accepted
    if (gpsAccepted || compassAccepted) {
        String logData = String("{\"type\":\"VTG\",\"source\":\"NMEA0183-VH\",\"cog\":") +
                        String(trueCOGRadians, 4) + String(",\"sog\":") + String(SpeedKnots, 2) +
                        String(",\"var\":") + String(variationRadians, 4) + String("}");
        logger_->broadcastLog(LogLevel::DEBUG, "NMEA0183", "SENTENCE_PROCESSED", logData);
    }
}
