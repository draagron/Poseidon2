#include "NMEA0183Parsers.h"
#include <cstring>
#include <cstdlib>

bool NMEA0183ParseRSA(const tNMEA0183Msg& msg, double& rudderAngle) {
    // Validate talker ID is "AP" (autopilot)
    if (strcmp(msg.Sender(), "AP") != 0) {
        return false;
    }

    // Validate message code is "RSA"
    if (strcmp(msg.MessageCode(), "RSA") != 0) {
        return false;
    }

    // Validate status field (Field 1) is 'A' (valid)
    if (strcmp(msg.Field(1), "A") != 0) {
        return false;
    }

    // Extract rudder angle from Field 0
    const char* angleStr = msg.Field(0);
    if (angleStr == nullptr || angleStr[0] == '\0') {
        return false;
    }

    rudderAngle = atof(angleStr);
    return true;
}
