/**
 * @file test_wifi_credentials.cpp
 * @brief Unit tests for WiFiCredentials struct validation
 *
 * These tests validate the WiFiCredentials struct according to WiFi spec:
 * - SSID: 1-32 characters, non-empty
 * - Password: 0 (open network) or 8-63 characters (WPA2)
 *
 * NOTE: These tests MUST FAIL INITIALLY (TDD approach) until
 * WiFiCredentials struct is implemented in Phase 3.4.
 */

#include <gtest/gtest.h>
#include "../../src/components/WiFiCredentials.h"

/**
 * Test fixture for WiFiCredentials tests
 */
class WiFiCredentialsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};

/**
 * Test: Valid WiFi credentials with SSID and password
 */
TEST_F(WiFiCredentialsTest, ValidCredentialsWithPassword) {
    WiFiCredentials creds;
    creds.ssid = "MyHomeNetwork";
    creds.password = "mypassword123";

    EXPECT_TRUE(creds.isValid());
    EXPECT_EQ(creds.ssid, "MyHomeNetwork");
    EXPECT_EQ(creds.password, "mypassword123");
}

/**
 * Test: Valid WiFi credentials for open network (no password)
 */
TEST_F(WiFiCredentialsTest, ValidCredentialsOpenNetwork) {
    WiFiCredentials creds;
    creds.ssid = "GuestNetwork";
    creds.password = "";

    EXPECT_TRUE(creds.isValid());
    EXPECT_EQ(creds.password.length(), 0);
}

/**
 * Test: SSID validation - minimum length (1 character)
 */
TEST_F(WiFiCredentialsTest, SSIDMinimumLength) {
    WiFiCredentials creds;
    creds.ssid = "A";
    creds.password = "validpass123";

    EXPECT_TRUE(creds.isValid());
}

/**
 * Test: SSID validation - maximum length (32 characters)
 */
TEST_F(WiFiCredentialsTest, SSIDMaximumLength) {
    WiFiCredentials creds;
    creds.ssid = "12345678901234567890123456789012"; // Exactly 32 chars
    creds.password = "validpass123";

    EXPECT_TRUE(creds.isValid());
}

/**
 * Test: SSID validation - reject empty SSID
 */
TEST_F(WiFiCredentialsTest, RejectEmptySSID) {
    WiFiCredentials creds;
    creds.ssid = "";
    creds.password = "validpass123";

    EXPECT_FALSE(creds.isValid());
}

/**
 * Test: SSID validation - reject SSID exceeding 32 characters
 */
TEST_F(WiFiCredentialsTest, RejectSSIDTooLong) {
    WiFiCredentials creds;
    creds.ssid = "123456789012345678901234567890123"; // 33 chars
    creds.password = "validpass123";

    EXPECT_FALSE(creds.isValid());
}

/**
 * Test: Password validation - minimum length for WPA2 (8 characters)
 */
TEST_F(WiFiCredentialsTest, PasswordMinimumLength) {
    WiFiCredentials creds;
    creds.ssid = "TestNetwork";
    creds.password = "12345678"; // Exactly 8 chars

    EXPECT_TRUE(creds.isValid());
}

/**
 * Test: Password validation - maximum length (63 characters)
 */
TEST_F(WiFiCredentialsTest, PasswordMaximumLength) {
    WiFiCredentials creds;
    creds.ssid = "TestNetwork";
    creds.password = "123456789012345678901234567890123456789012345678901234567890123"; // 63 chars

    EXPECT_TRUE(creds.isValid());
}

/**
 * Test: Password validation - reject password with 1-7 characters
 */
TEST_F(WiFiCredentialsTest, RejectPasswordTooShort) {
    WiFiCredentials creds;
    creds.ssid = "TestNetwork";
    creds.password = "1234567"; // 7 chars - invalid for WPA2

    EXPECT_FALSE(creds.isValid());
}

/**
 * Test: Password validation - reject password exceeding 63 characters
 */
TEST_F(WiFiCredentialsTest, RejectPasswordTooLong) {
    WiFiCredentials creds;
    creds.ssid = "TestNetwork";
    creds.password = "1234567890123456789012345678901234567890123456789012345678901234"; // 64 chars

    EXPECT_FALSE(creds.isValid());
}

/**
 * Test: Reject credentials with newline in SSID
 */
TEST_F(WiFiCredentialsTest, RejectSSIDWithNewline) {
    WiFiCredentials creds;
    creds.ssid = "Network\nName";
    creds.password = "validpass123";

    EXPECT_FALSE(creds.isValid());
}

/**
 * Test: Reject credentials with newline in password
 */
TEST_F(WiFiCredentialsTest, RejectPasswordWithNewline) {
    WiFiCredentials creds;
    creds.ssid = "NetworkName";
    creds.password = "pass\nword123";

    EXPECT_FALSE(creds.isValid());
}

/**
 * Test: Constructor with parameters
 */
TEST_F(WiFiCredentialsTest, ConstructorWithParameters) {
    WiFiCredentials creds("TestSSID", "testpass123");

    EXPECT_EQ(creds.ssid, "TestSSID");
    EXPECT_EQ(creds.password, "testpass123");
    EXPECT_TRUE(creds.isValid());
}

/**
 * Test: Default constructor initializes empty credentials
 */
TEST_F(WiFiCredentialsTest, DefaultConstructor) {
    WiFiCredentials creds;

    EXPECT_EQ(creds.ssid, "");
    EXPECT_EQ(creds.password, "");
    EXPECT_FALSE(creds.isValid()); // Empty SSID is invalid
}

/**
 * Test: Copy constructor
 */
TEST_F(WiFiCredentialsTest, CopyConstructor) {
    WiFiCredentials original("OriginalSSID", "originalpass");
    WiFiCredentials copy(original);

    EXPECT_EQ(copy.ssid, original.ssid);
    EXPECT_EQ(copy.password, original.password);
    EXPECT_TRUE(copy.isValid());
}

/**
 * Test: Assignment operator
 */
TEST_F(WiFiCredentialsTest, AssignmentOperator) {
    WiFiCredentials creds1("SSID1", "password1");
    WiFiCredentials creds2;

    creds2 = creds1;

    EXPECT_EQ(creds2.ssid, "SSID1");
    EXPECT_EQ(creds2.password, "password1");
    EXPECT_TRUE(creds2.isValid());
}

/**
 * Test: Special characters in SSID
 */
TEST_F(WiFiCredentialsTest, SpecialCharactersInSSID) {
    WiFiCredentials creds;
    creds.ssid = "Network-2.4GHz_Guest";
    creds.password = "validpass123";

    EXPECT_TRUE(creds.isValid());
}

/**
 * Test: Special characters in password
 */
TEST_F(WiFiCredentialsTest, SpecialCharactersInPassword) {
    WiFiCredentials creds;
    creds.ssid = "TestNetwork";
    creds.password = "P@ssw0rd!#$%";

    EXPECT_TRUE(creds.isValid());
}
