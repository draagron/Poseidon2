#include <unity.h>
#include <NMEA0183.h>
#include <NMEA0183Msg.h>
#include "utils/NMEA0183Parsers.h"
#include "helpers/nmea0183_test_fixtures.h"

// T014: Unit Test - NMEA0183ParseRSA Custom Parser
void test_nmea0183_parse_rsa() {
    tNMEA0183 nmea0183;
    double rudderAngle;

    // Test valid RSA sentence: $APRSA,15.0,A*3C
    // Parse the fixture sentence
    const char* validRSA = VALID_APRSA;
    for (size_t i = 0; validRSA[i] != '\0'; i++) {
        if (nmea0183.ParseMessages()) {
            // Message complete
            break;
        }
        nmea0183.GetMessage().AddByteToMessage(validRSA[i]);
    }

    // Test: Parser returns true and extracts correct angle
    bool result = NMEA0183ParseRSA(nmea0183.GetMessage(), rudderAngle);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FLOAT_WITHIN(0.1, 15.0, rudderAngle);

    // Test invalid talker ID "VH" (should return false)
    tNMEA0183Msg invalidTalkerMsg;
    invalidTalkerMsg.Init("VH", "RSA");
    invalidTalkerMsg.AddDoubleField(15.0);
    invalidTalkerMsg.AddStrField("A");

    result = NMEA0183ParseRSA(invalidTalkerMsg, rudderAngle);
    TEST_ASSERT_FALSE(result);

    // Test invalid message code "HDM" (should return false)
    tNMEA0183Msg invalidMsgCodeMsg;
    invalidMsgCodeMsg.Init("AP", "HDM");
    invalidMsgCodeMsg.AddDoubleField(15.0);
    invalidMsgCodeMsg.AddStrField("A");

    result = NMEA0183ParseRSA(invalidMsgCodeMsg, rudderAngle);
    TEST_ASSERT_FALSE(result);

    // Test invalid status "V" (should return false)
    tNMEA0183Msg invalidStatusMsg;
    invalidStatusMsg.Init("AP", "RSA");
    invalidStatusMsg.AddDoubleField(15.0);
    invalidStatusMsg.AddStrField("V");

    result = NMEA0183ParseRSA(invalidStatusMsg, rudderAngle);
    TEST_ASSERT_FALSE(result);

    // Test out-of-range angle (parser extracts, handler validates later)
    tNMEA0183Msg outOfRangeMsg;
    outOfRangeMsg.Init("AP", "RSA");
    outOfRangeMsg.AddDoubleField(120.0);
    outOfRangeMsg.AddStrField("A");

    result = NMEA0183ParseRSA(outOfRangeMsg, rudderAngle);
    TEST_ASSERT_TRUE(result);  // Parser extracts value
    TEST_ASSERT_FLOAT_WITHIN(0.1, 120.0, rudderAngle);  // Handler will reject later
}
