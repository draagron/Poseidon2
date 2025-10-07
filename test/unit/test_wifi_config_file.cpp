/**
 * @file test_wifi_config_file.cpp
 * @brief Unit tests for WiFiConfigFile struct validation
 *
 * These tests validate the WiFiConfigFile struct:
 * - Maximum 3 networks allowed
 * - Priority ordering (index 0 = highest priority)
 * - Duplicate SSIDs allowed
 * - File persistence validation
 *
 * NOTE: These tests MUST FAIL INITIALLY (TDD approach) until
 * WiFiConfigFile struct is implemented in Phase 3.4.
 */

#include <gtest/gtest.h>
#include "../../src/components/WiFiConfigFile.h"
#include "../../src/components/WiFiCredentials.h"

/**
 * Test fixture for WiFiConfigFile tests
 */
class WiFiConfigFileTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};

/**
 * Test: Create empty config file
 */
TEST_F(WiFiConfigFileTest, CreateEmptyConfigFile) {
    WiFiConfigFile config;

    EXPECT_EQ(config.count, 0);
    EXPECT_TRUE(config.isEmpty());
}

/**
 * Test: Add single network to config
 */
TEST_F(WiFiConfigFileTest, AddSingleNetwork) {
    WiFiConfigFile config;
    WiFiCredentials creds("HomeNetwork", "password123");

    bool result = config.addNetwork(creds);

    EXPECT_TRUE(result);
    EXPECT_EQ(config.count, 1);
    EXPECT_FALSE(config.isEmpty());
    EXPECT_EQ(config.networks[0].ssid, "HomeNetwork");
}

/**
 * Test: Add maximum networks (3)
 */
TEST_F(WiFiConfigFileTest, AddMaximumNetworks) {
    WiFiConfigFile config;

    bool result1 = config.addNetwork(WiFiCredentials("Network1", "pass1"));
    bool result2 = config.addNetwork(WiFiCredentials("Network2", "pass2"));
    bool result3 = config.addNetwork(WiFiCredentials("Network3", "pass3"));

    EXPECT_TRUE(result1);
    EXPECT_TRUE(result2);
    EXPECT_TRUE(result3);
    EXPECT_EQ(config.count, 3);
    EXPECT_TRUE(config.isFull());
}

/**
 * Test: Reject adding more than 3 networks
 */
TEST_F(WiFiConfigFileTest, RejectExceedingMaximumNetworks) {
    WiFiConfigFile config;

    config.addNetwork(WiFiCredentials("Network1", "pass1"));
    config.addNetwork(WiFiCredentials("Network2", "pass2"));
    config.addNetwork(WiFiCredentials("Network3", "pass3"));

    bool result = config.addNetwork(WiFiCredentials("Network4", "pass4"));

    EXPECT_FALSE(result);
    EXPECT_EQ(config.count, 3); // Should still be 3
}

/**
 * Test: Priority ordering - first added is highest priority
 */
TEST_F(WiFiConfigFileTest, PriorityOrdering) {
    WiFiConfigFile config;

    config.addNetwork(WiFiCredentials("Primary", "pass1"));
    config.addNetwork(WiFiCredentials("Secondary", "pass2"));
    config.addNetwork(WiFiCredentials("Tertiary", "pass3"));

    EXPECT_EQ(config.networks[0].ssid, "Primary");   // Highest priority
    EXPECT_EQ(config.networks[1].ssid, "Secondary"); // Medium priority
    EXPECT_EQ(config.networks[2].ssid, "Tertiary");  // Lowest priority
}

/**
 * Test: Allow duplicate SSIDs (same network name, different locations)
 */
TEST_F(WiFiConfigFileTest, AllowDuplicateSSIDs) {
    WiFiConfigFile config;

    bool result1 = config.addNetwork(WiFiCredentials("Starbucks", "location1pass"));
    bool result2 = config.addNetwork(WiFiCredentials("Starbucks", "location2pass"));

    EXPECT_TRUE(result1);
    EXPECT_TRUE(result2);
    EXPECT_EQ(config.count, 2);
    EXPECT_EQ(config.networks[0].ssid, "Starbucks");
    EXPECT_EQ(config.networks[1].ssid, "Starbucks");
    // Different passwords for same SSID
    EXPECT_NE(config.networks[0].password, config.networks[1].password);
}

/**
 * Test: Get network by index
 */
TEST_F(WiFiConfigFileTest, GetNetworkByIndex) {
    WiFiConfigFile config;
    config.addNetwork(WiFiCredentials("Network1", "pass1"));
    config.addNetwork(WiFiCredentials("Network2", "pass2"));

    WiFiCredentials* network = config.getNetwork(0);

    ASSERT_NE(network, nullptr);
    EXPECT_EQ(network->ssid, "Network1");
}

/**
 * Test: Get network with invalid index returns nullptr
 */
TEST_F(WiFiConfigFileTest, GetNetworkInvalidIndex) {
    WiFiConfigFile config;
    config.addNetwork(WiFiCredentials("Network1", "pass1"));

    WiFiCredentials* network1 = config.getNetwork(5); // Out of bounds
    WiFiCredentials* network2 = config.getNetwork(-1); // Negative

    EXPECT_EQ(network1, nullptr);
    EXPECT_EQ(network2, nullptr);
}

/**
 * Test: Clear all networks
 */
TEST_F(WiFiConfigFileTest, ClearAllNetworks) {
    WiFiConfigFile config;
    config.addNetwork(WiFiCredentials("Network1", "pass1"));
    config.addNetwork(WiFiCredentials("Network2", "pass2"));

    config.clear();

    EXPECT_EQ(config.count, 0);
    EXPECT_TRUE(config.isEmpty());
}

/**
 * Test: Reject adding invalid credentials
 */
TEST_F(WiFiConfigFileTest, RejectInvalidCredentials) {
    WiFiConfigFile config;
    WiFiCredentials invalidCreds("", "password"); // Empty SSID

    bool result = config.addNetwork(invalidCreds);

    EXPECT_FALSE(result);
    EXPECT_EQ(config.count, 0);
}

/**
 * Test: Remove network by index
 */
TEST_F(WiFiConfigFileTest, RemoveNetworkByIndex) {
    WiFiConfigFile config;
    config.addNetwork(WiFiCredentials("Network1", "pass1"));
    config.addNetwork(WiFiCredentials("Network2", "pass2"));
    config.addNetwork(WiFiCredentials("Network3", "pass3"));

    bool result = config.removeNetwork(1); // Remove Network2

    EXPECT_TRUE(result);
    EXPECT_EQ(config.count, 2);
    EXPECT_EQ(config.networks[0].ssid, "Network1");
    EXPECT_EQ(config.networks[1].ssid, "Network3"); // Network3 moved up
}

/**
 * Test: Remove network with invalid index
 */
TEST_F(WiFiConfigFileTest, RemoveNetworkInvalidIndex) {
    WiFiConfigFile config;
    config.addNetwork(WiFiCredentials("Network1", "pass1"));

    bool result = config.removeNetwork(5);

    EXPECT_FALSE(result);
    EXPECT_EQ(config.count, 1); // Unchanged
}

/**
 * Test: Check if config is full
 */
TEST_F(WiFiConfigFileTest, CheckIfFull) {
    WiFiConfigFile config;

    EXPECT_FALSE(config.isFull());

    config.addNetwork(WiFiCredentials("Network1", "pass1"));
    EXPECT_FALSE(config.isFull());

    config.addNetwork(WiFiCredentials("Network2", "pass2"));
    EXPECT_FALSE(config.isFull());

    config.addNetwork(WiFiCredentials("Network3", "pass3"));
    EXPECT_TRUE(config.isFull());
}

/**
 * Test: Serialize to plain text format
 */
TEST_F(WiFiConfigFileTest, SerializeToPlainText) {
    WiFiConfigFile config;
    config.addNetwork(WiFiCredentials("Network1", "password1"));
    config.addNetwork(WiFiCredentials("Network2", "password2"));

    String plainText = config.toPlainText();

    EXPECT_TRUE(plainText.indexOf("Network1,password1") >= 0);
    EXPECT_TRUE(plainText.indexOf("Network2,password2") >= 0);
}

/**
 * Test: Parse from plain text format
 */
TEST_F(WiFiConfigFileTest, ParseFromPlainText) {
    WiFiConfigFile config;
    String plainText = "HomeNetwork,mypass123\nGuestNetwork,guestpass\n";

    bool result = config.fromPlainText(plainText);

    EXPECT_TRUE(result);
    EXPECT_EQ(config.count, 2);
    EXPECT_EQ(config.networks[0].ssid, "HomeNetwork");
    EXPECT_EQ(config.networks[0].password, "mypass123");
    EXPECT_EQ(config.networks[1].ssid, "GuestNetwork");
    EXPECT_EQ(config.networks[1].password, "guestpass");
}

/**
 * Test: Parse plain text with open network (no password)
 */
TEST_F(WiFiConfigFileTest, ParsePlainTextOpenNetwork) {
    WiFiConfigFile config;
    String plainText = "OpenNetwork,\n";

    bool result = config.fromPlainText(plainText);

    EXPECT_TRUE(result);
    EXPECT_EQ(config.count, 1);
    EXPECT_EQ(config.networks[0].ssid, "OpenNetwork");
    EXPECT_EQ(config.networks[0].password, "");
}

/**
 * Test: Parse plain text - skip invalid lines, keep valid ones
 */
TEST_F(WiFiConfigFileTest, ParsePlainTextSkipInvalidLines) {
    WiFiConfigFile config;
    String plainText = "ValidNetwork,validpass123\nInvalidSSIDThatIsTooLongAndExceeds32Characters,pass\nAnotherValid,pass12345678\n";

    bool result = config.fromPlainText(plainText);

    EXPECT_TRUE(result); // Partial success
    EXPECT_EQ(config.count, 2); // Only 2 valid networks
    EXPECT_EQ(config.networks[0].ssid, "ValidNetwork");
    EXPECT_EQ(config.networks[1].ssid, "AnotherValid");
}

/**
 * Test: Parse plain text - truncate to max 3 networks
 */
TEST_F(WiFiConfigFileTest, ParsePlainTextTruncateToMax) {
    WiFiConfigFile config;
    String plainText = "Net1,pass1\nNet2,pass2\nNet3,pass3\nNet4,pass4\nNet5,pass5\n";

    bool result = config.fromPlainText(plainText);

    EXPECT_TRUE(result);
    EXPECT_EQ(config.count, 3); // Only first 3
    EXPECT_EQ(config.networks[0].ssid, "Net1");
    EXPECT_EQ(config.networks[1].ssid, "Net2");
    EXPECT_EQ(config.networks[2].ssid, "Net3");
}

/**
 * Test: Copy constructor
 */
TEST_F(WiFiConfigFileTest, CopyConstructor) {
    WiFiConfigFile original;
    original.addNetwork(WiFiCredentials("Network1", "pass1"));
    original.addNetwork(WiFiCredentials("Network2", "pass2"));

    WiFiConfigFile copy(original);

    EXPECT_EQ(copy.count, 2);
    EXPECT_EQ(copy.networks[0].ssid, "Network1");
    EXPECT_EQ(copy.networks[1].ssid, "Network2");
}

/**
 * Test: Assignment operator
 */
TEST_F(WiFiConfigFileTest, AssignmentOperator) {
    WiFiConfigFile config1;
    config1.addNetwork(WiFiCredentials("Network1", "pass1"));

    WiFiConfigFile config2;
    config2 = config1;

    EXPECT_EQ(config2.count, 1);
    EXPECT_EQ(config2.networks[0].ssid, "Network1");
}
