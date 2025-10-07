/**
 * @file test_config_parser.cpp
 * @brief Unit tests for ConfigParser component
 *
 * These tests validate the plain text configuration file parsing logic:
 * - Comma-separated format (SSID,password)
 * - Maximum 3 lines enforced
 * - SSID and password validation
 * - Malformed input handling (defensive parsing)
 *
 * NOTE: These tests MUST FAIL INITIALLY (TDD approach) until
 * ConfigParser is implemented in Phase 3.6.
 */

#include <gtest/gtest.h>
#include "../../src/components/ConfigParser.h"
#include "../../src/components/WiFiCredentials.h"
#include "../../src/components/WiFiConfigFile.h"

/**
 * Test fixture for ConfigParser tests
 */
class ConfigParserTest : public ::testing::Test {
protected:
    ConfigParser parser;

    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};

/**
 * Test: Parse single valid line
 */
TEST_F(ConfigParserTest, ParseSingleValidLine) {
    String line = "HomeNetwork,mypassword123";
    WiFiCredentials creds;

    bool result = parser.parseLine(line, creds);

    EXPECT_TRUE(result);
    EXPECT_EQ(creds.ssid, "HomeNetwork");
    EXPECT_EQ(creds.password, "mypassword123");
}

/**
 * Test: Parse line with open network (no password)
 */
TEST_F(ConfigParserTest, ParseLineOpenNetwork) {
    String line = "GuestNetwork,";
    WiFiCredentials creds;

    bool result = parser.parseLine(line, creds);

    EXPECT_TRUE(result);
    EXPECT_EQ(creds.ssid, "GuestNetwork");
    EXPECT_EQ(creds.password, "");
}

/**
 * Test: Parse line with trimming whitespace
 */
TEST_F(ConfigParserTest, ParseLineWithWhitespace) {
    String line = "  HomeNetwork  ,  password123  ";
    WiFiCredentials creds;

    bool result = parser.parseLine(line, creds);

    EXPECT_TRUE(result);
    EXPECT_EQ(creds.ssid, "HomeNetwork");
    EXPECT_EQ(creds.password, "password123");
}

/**
 * Test: Reject line without comma separator
 */
TEST_F(ConfigParserTest, RejectLineWithoutComma) {
    String line = "HomeNetworkPassword";
    WiFiCredentials creds;

    bool result = parser.parseLine(line, creds);

    EXPECT_FALSE(result);
}

/**
 * Test: Reject line with empty SSID
 */
TEST_F(ConfigParserTest, RejectLineWithEmptySSID) {
    String line = ",password123";
    WiFiCredentials creds;

    bool result = parser.parseLine(line, creds);

    EXPECT_FALSE(result);
}

/**
 * Test: Reject line with SSID exceeding 32 characters
 */
TEST_F(ConfigParserTest, RejectLineWithSSIDTooLong) {
    String line = "ThisSSIDIsWayTooLongAndExceedsTheThirtyTwoCharacterLimit,password";
    WiFiCredentials creds;

    bool result = parser.parseLine(line, creds);

    EXPECT_FALSE(result);
}

/**
 * Test: Reject line with password too short (1-7 characters)
 */
TEST_F(ConfigParserTest, RejectLineWithPasswordTooShort) {
    String line = "HomeNetwork,short";
    WiFiCredentials creds;

    bool result = parser.parseLine(line, creds);

    EXPECT_FALSE(result);
}

/**
 * Test: Reject line with password exceeding 63 characters
 */
TEST_F(ConfigParserTest, RejectLineWithPasswordTooLong) {
    String line = "HomeNetwork,ThisPasswordIsWayTooLongAndExceedsTheSixtyThreeCharacterLimitForWPA2Networks";
    WiFiCredentials creds;

    bool result = parser.parseLine(line, creds);

    EXPECT_FALSE(result);
}

/**
 * Test: Parse line with special characters in SSID
 */
TEST_F(ConfigParserTest, ParseLineWithSpecialCharsInSSID) {
    String line = "Network-2.4GHz_Guest,password123";
    WiFiCredentials creds;

    bool result = parser.parseLine(line, creds);

    EXPECT_TRUE(result);
    EXPECT_EQ(creds.ssid, "Network-2.4GHz_Guest");
}

/**
 * Test: Parse line with special characters in password
 */
TEST_F(ConfigParserTest, ParseLineWithSpecialCharsInPassword) {
    String line = "HomeNetwork,P@ssw0rd!#$%";
    WiFiCredentials creds;

    bool result = parser.parseLine(line, creds);

    EXPECT_TRUE(result);
    EXPECT_EQ(creds.password, "P@ssw0rd!#$%");
}

/**
 * Test: Reject empty line
 */
TEST_F(ConfigParserTest, RejectEmptyLine) {
    String line = "";
    WiFiCredentials creds;

    bool result = parser.parseLine(line, creds);

    EXPECT_FALSE(result);
}

/**
 * Test: Reject whitespace-only line
 */
TEST_F(ConfigParserTest, RejectWhitespaceOnlyLine) {
    String line = "   \t   ";
    WiFiCredentials creds;

    bool result = parser.parseLine(line, creds);

    EXPECT_FALSE(result);
}

/**
 * Test: Parse file with 3 valid networks
 */
TEST_F(ConfigParserTest, ParseFileWithThreeNetworks) {
    String fileContent = "Network1,password1\nNetwork2,password2\nNetwork3,password3\n";
    WiFiConfigFile config;

    bool result = parser.parseFile(fileContent, config);

    EXPECT_TRUE(result);
    EXPECT_EQ(config.count, 3);
    EXPECT_EQ(config.networks[0].ssid, "Network1");
    EXPECT_EQ(config.networks[1].ssid, "Network2");
    EXPECT_EQ(config.networks[2].ssid, "Network3");
}

/**
 * Test: Parse file with mixed valid and invalid lines
 */
TEST_F(ConfigParserTest, ParseFileMixedValidInvalid) {
    String fileContent = "ValidNetwork1,password123\n"
                        "InvalidSSIDThatIsTooLongAndExceedsThirtyTwoCharacters,pass\n"
                        "ValidNetwork2,password456\n";
    WiFiConfigFile config;

    bool result = parser.parseFile(fileContent, config);

    EXPECT_TRUE(result); // Partial success
    EXPECT_EQ(config.count, 2); // Only 2 valid networks
    EXPECT_EQ(config.networks[0].ssid, "ValidNetwork1");
    EXPECT_EQ(config.networks[1].ssid, "ValidNetwork2");
}

/**
 * Test: Parse file truncates to max 3 networks
 */
TEST_F(ConfigParserTest, ParseFileTruncateToMax) {
    String fileContent = "Net1,pass12345678\n"
                        "Net2,pass12345678\n"
                        "Net3,pass12345678\n"
                        "Net4,pass12345678\n"
                        "Net5,pass12345678\n";
    WiFiConfigFile config;

    bool result = parser.parseFile(fileContent, config);

    EXPECT_TRUE(result);
    EXPECT_EQ(config.count, 3); // Only first 3
    EXPECT_EQ(config.networks[0].ssid, "Net1");
    EXPECT_EQ(config.networks[1].ssid, "Net2");
    EXPECT_EQ(config.networks[2].ssid, "Net3");
}

/**
 * Test: Parse file with empty lines (skip them)
 */
TEST_F(ConfigParserTest, ParseFileSkipEmptyLines) {
    String fileContent = "Network1,password1\n"
                        "\n"
                        "Network2,password2\n"
                        "   \n"
                        "Network3,password3\n";
    WiFiConfigFile config;

    bool result = parser.parseFile(fileContent, config);

    EXPECT_TRUE(result);
    EXPECT_EQ(config.count, 3);
}

/**
 * Test: Parse file with Windows line endings (CRLF)
 */
TEST_F(ConfigParserTest, ParseFileWithCRLF) {
    String fileContent = "Network1,password1\r\nNetwork2,password2\r\n";
    WiFiConfigFile config;

    bool result = parser.parseFile(fileContent, config);

    EXPECT_TRUE(result);
    EXPECT_EQ(config.count, 2);
    EXPECT_EQ(config.networks[0].ssid, "Network1");
    EXPECT_EQ(config.networks[1].ssid, "Network2");
}

/**
 * Test: Parse empty file
 */
TEST_F(ConfigParserTest, ParseEmptyFile) {
    String fileContent = "";
    WiFiConfigFile config;

    bool result = parser.parseFile(fileContent, config);

    EXPECT_FALSE(result); // No valid networks
    EXPECT_EQ(config.count, 0);
}

/**
 * Test: Parse file with only invalid lines
 */
TEST_F(ConfigParserTest, ParseFileOnlyInvalidLines) {
    String fileContent = "InvalidSSIDThatIsTooLongAndExceedsThirtyTwoCharacters,pass\n"
                        "Network,short\n"
                        ",password123\n";
    WiFiConfigFile config;

    bool result = parser.parseFile(fileContent, config);

    EXPECT_FALSE(result); // No valid networks parsed
    EXPECT_EQ(config.count, 0);
}

/**
 * Test: Parse file with single network
 */
TEST_F(ConfigParserTest, ParseFileSingleNetwork) {
    String fileContent = "HomeNetwork,password123\n";
    WiFiConfigFile config;

    bool result = parser.parseFile(fileContent, config);

    EXPECT_TRUE(result);
    EXPECT_EQ(config.count, 1);
    EXPECT_EQ(config.networks[0].ssid, "HomeNetwork");
    EXPECT_EQ(config.networks[0].password, "password123");
}

/**
 * Test: Parse file with open network
 */
TEST_F(ConfigParserTest, ParseFileOpenNetwork) {
    String fileContent = "OpenNetwork,\n";
    WiFiConfigFile config;

    bool result = parser.parseFile(fileContent, config);

    EXPECT_TRUE(result);
    EXPECT_EQ(config.count, 1);
    EXPECT_EQ(config.networks[0].ssid, "OpenNetwork");
    EXPECT_EQ(config.networks[0].password, "");
}

/**
 * Test: Parse file with duplicate SSIDs (allowed)
 */
TEST_F(ConfigParserTest, ParseFileDuplicateSSIDs) {
    String fileContent = "Starbucks,location1pass\n"
                        "Starbucks,location2pass\n";
    WiFiConfigFile config;

    bool result = parser.parseFile(fileContent, config);

    EXPECT_TRUE(result);
    EXPECT_EQ(config.count, 2);
    EXPECT_EQ(config.networks[0].ssid, "Starbucks");
    EXPECT_EQ(config.networks[1].ssid, "Starbucks");
    EXPECT_NE(config.networks[0].password, config.networks[1].password);
}

/**
 * Test: Get error message for invalid line
 */
TEST_F(ConfigParserTest, GetErrorMessageForInvalidLine) {
    String line = "InvalidSSIDThatIsTooLongAndExceedsThirtyTwoCharacters,password";
    WiFiCredentials creds;

    parser.parseLine(line, creds);
    String error = parser.getLastError();

    EXPECT_TRUE(error.length() > 0);
    EXPECT_TRUE(error.indexOf("SSID") >= 0 || error.indexOf("32") >= 0);
}

/**
 * Test: Error count tracking
 */
TEST_F(ConfigParserTest, TrackErrorCount) {
    String fileContent = "ValidNetwork,password123\n"
                        "InvalidSSIDTooLong12345678901234567890,pass\n"
                        "Network,short\n"
                        "AnotherValid,password456\n";
    WiFiConfigFile config;

    parser.parseFile(fileContent, config);
    int errorCount = parser.getErrorCount();

    EXPECT_EQ(errorCount, 2); // 2 invalid lines
}

/**
 * Test: Reset parser state
 */
TEST_F(ConfigParserTest, ResetParserState) {
    String line = "InvalidSSIDTooLong12345678901234567890,pass";
    WiFiCredentials creds;

    parser.parseLine(line, creds);
    EXPECT_GT(parser.getErrorCount(), 0);

    parser.reset();

    EXPECT_EQ(parser.getErrorCount(), 0);
    EXPECT_EQ(parser.getLastError(), "");
}

/**
 * Test: Parse line with multiple commas (use first comma as delimiter)
 */
TEST_F(ConfigParserTest, ParseLineMultipleCommas) {
    String line = "Network,pass,word,extra";
    WiFiCredentials creds;

    bool result = parser.parseLine(line, creds);

    // Should split on first comma only
    EXPECT_TRUE(result);
    EXPECT_EQ(creds.ssid, "Network");
    EXPECT_EQ(creds.password, "pass,word,extra");
}

/**
 * Test: Validate SSID length boundary (32 chars exactly)
 */
TEST_F(ConfigParserTest, ValidateSSIDLengthBoundary) {
    String line32 = "12345678901234567890123456789012,password123"; // 32 chars
    String line33 = "123456789012345678901234567890123,password123"; // 33 chars
    WiFiCredentials creds;

    bool result32 = parser.parseLine(line32, creds);
    bool result33 = parser.parseLine(line33, creds);

    EXPECT_TRUE(result32);  // 32 chars should be valid
    EXPECT_FALSE(result33); // 33 chars should be invalid
}

/**
 * Test: Validate password length boundary (8 chars minimum)
 */
TEST_F(ConfigParserTest, ValidatePasswordLengthBoundary) {
    WiFiCredentials creds;

    String line7 = "Network,1234567"; // 7 chars - invalid
    String line8 = "Network,12345678"; // 8 chars - valid

    bool result7 = parser.parseLine(line7, creds);
    bool result8 = parser.parseLine(line8, creds);

    EXPECT_FALSE(result7); // 7 chars should be invalid
    EXPECT_TRUE(result8);  // 8 chars should be valid
}

/**
 * Test: Validate password length boundary (63 chars maximum)
 */
TEST_F(ConfigParserTest, ValidatePasswordLengthMaxBoundary) {
    WiFiCredentials creds;

    String line63 = "Network,123456789012345678901234567890123456789012345678901234567890123"; // 63 chars
    String line64 = "Network,1234567890123456789012345678901234567890123456789012345678901234"; // 64 chars

    bool result63 = parser.parseLine(line63, creds);
    bool result64 = parser.parseLine(line64, creds);

    EXPECT_TRUE(result63);  // 63 chars should be valid
    EXPECT_FALSE(result64); // 64 chars should be invalid
}
