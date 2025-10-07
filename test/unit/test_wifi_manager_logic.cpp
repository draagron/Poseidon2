/**
 * @file test_wifi_manager_logic.cpp
 * @brief Unit tests for WiFiManager core logic
 *
 * These tests validate WiFiManager methods using mock interfaces:
 * - loadConfig(): Reads /wifi.conf via IFileSystem, parses, populates config
 * - saveConfig(): Writes config to /wifi.conf, validates before write
 * - connect(): Calls IWiFiAdapter.begin(), sets state, registers timeout
 * - handleDisconnect(): WiFi disconnect event, state update, retry logic
 *
 * NOTE: These tests MUST FAIL INITIALLY (TDD approach) until
 * WiFiManager is implemented in Phase 3.11.
 */

#include <gtest/gtest.h>
#include "../../src/components/WiFiManager.h"
#include "../../src/mocks/MockWiFiAdapter.h"
#include "../../src/mocks/MockFileSystem.h"
#include "../../src/components/WiFiConfigFile.h"
#include "../../src/components/WiFiConnectionState.h"
#include "../../src/config.h"

/**
 * Test fixture for WiFiManager tests
 */
class WiFiManagerTest : public ::testing::Test {
protected:
    WiFiManager* manager;
    MockWiFiAdapter* mockWiFi;
    MockFileSystem* mockFS;
    WiFiConfigFile config;
    WiFiConnectionState state;

    void SetUp() override {
        mockWiFi = new MockWiFiAdapter();
        mockFS = new MockFileSystem();

        // Mount filesystem
        mockFS->mount();

        // Create WiFiManager with mocked dependencies
        manager = new WiFiManager(mockWiFi, mockFS);

        // Reset state
        state = WiFiConnectionState();
        config = WiFiConfigFile();
    }

    void TearDown() override {
        delete manager;
        delete mockWiFi;
        delete mockFS;
    }

    /**
     * Helper: Create test config file in mock filesystem
     */
    void createTestConfigFile(const String& content) {
        mockFS->writeFile(CONFIG_FILE_PATH, content.c_str());
    }
};

// ============================================================================
// T025: loadConfig() Tests
// ============================================================================

/**
 * Test: Load valid config file with 3 networks
 */
TEST_F(WiFiManagerTest, LoadConfigValidFile) {
    createTestConfigFile("Network1,password1\nNetwork2,password2\nNetwork3,password3\n");

    bool result = manager->loadConfig(config);

    EXPECT_TRUE(result);
    EXPECT_EQ(config.count, 3);
    EXPECT_EQ(config.networks[0].ssid, "Network1");
    EXPECT_EQ(config.networks[1].ssid, "Network2");
    EXPECT_EQ(config.networks[2].ssid, "Network3");
}

/**
 * Test: Load config file with 1 network
 */
TEST_F(WiFiManagerTest, LoadConfigSingleNetwork) {
    createTestConfigFile("HomeNetwork,mypassword\n");

    bool result = manager->loadConfig(config);

    EXPECT_TRUE(result);
    EXPECT_EQ(config.count, 1);
    EXPECT_EQ(config.networks[0].ssid, "HomeNetwork");
    EXPECT_EQ(config.networks[0].password, "mypassword");
}

/**
 * Test: Load config file with open network
 */
TEST_F(WiFiManagerTest, LoadConfigOpenNetwork) {
    createTestConfigFile("GuestNetwork,\n");

    bool result = manager->loadConfig(config);

    EXPECT_TRUE(result);
    EXPECT_EQ(config.count, 1);
    EXPECT_EQ(config.networks[0].ssid, "GuestNetwork");
    EXPECT_EQ(config.networks[0].password, "");
}

/**
 * Test: Load config - file not found
 */
TEST_F(WiFiManagerTest, LoadConfigFileNotFound) {
    // Don't create config file

    bool result = manager->loadConfig(config);

    EXPECT_FALSE(result);
    EXPECT_EQ(config.count, 0);
}

/**
 * Test: Load config - file exists but empty
 */
TEST_F(WiFiManagerTest, LoadConfigEmptyFile) {
    createTestConfigFile("");

    bool result = manager->loadConfig(config);

    EXPECT_FALSE(result);
    EXPECT_EQ(config.count, 0);
}

/**
 * Test: Load config - mixed valid and invalid lines
 */
TEST_F(WiFiManagerTest, LoadConfigMixedValidInvalid) {
    createTestConfigFile("ValidNetwork,password123\nInvalidSSIDTooLong12345678901234567890,pass\nAnotherValid,password456\n");

    bool result = manager->loadConfig(config);

    EXPECT_TRUE(result); // Partial success
    EXPECT_EQ(config.count, 2); // Only valid networks
    EXPECT_EQ(config.networks[0].ssid, "ValidNetwork");
    EXPECT_EQ(config.networks[1].ssid, "AnotherValid");
}

/**
 * Test: Load config - filesystem not mounted
 */
TEST_F(WiFiManagerTest, LoadConfigFileSystemNotMounted) {
    MockFileSystem unmountedFS;
    WiFiManager mgr(mockWiFi, &unmountedFS);

    bool result = mgr.loadConfig(config);

    EXPECT_FALSE(result);
}

// ============================================================================
// T026: saveConfig() Tests
// ============================================================================

/**
 * Test: Save valid config file
 */
TEST_F(WiFiManagerTest, SaveConfigValid) {
    config.addNetwork(WiFiCredentials("Network1", "password1"));
    config.addNetwork(WiFiCredentials("Network2", "password2"));

    bool result = manager->saveConfig(config);

    EXPECT_TRUE(result);

    // Verify file was written
    EXPECT_TRUE(mockFS->exists(CONFIG_FILE_PATH));

    // Read back and verify content
    String content = mockFS->readFile(CONFIG_FILE_PATH);
    EXPECT_TRUE(content.indexOf("Network1,password1") >= 0);
    EXPECT_TRUE(content.indexOf("Network2,password2") >= 0);
}

/**
 * Test: Save config with single network
 */
TEST_F(WiFiManagerTest, SaveConfigSingleNetwork) {
    config.addNetwork(WiFiCredentials("HomeNetwork", "mypassword"));

    bool result = manager->saveConfig(config);

    EXPECT_TRUE(result);

    String content = mockFS->readFile(CONFIG_FILE_PATH);
    EXPECT_TRUE(content.indexOf("HomeNetwork,mypassword") >= 0);
}

/**
 * Test: Save config with open network
 */
TEST_F(WiFiManagerTest, SaveConfigOpenNetwork) {
    config.addNetwork(WiFiCredentials("GuestNetwork", ""));

    bool result = manager->saveConfig(config);

    EXPECT_TRUE(result);

    String content = mockFS->readFile(CONFIG_FILE_PATH);
    EXPECT_TRUE(content.indexOf("GuestNetwork,") >= 0);
}

/**
 * Test: Save config - reject invalid credentials
 */
TEST_F(WiFiManagerTest, SaveConfigRejectInvalid) {
    WiFiConfigFile invalidConfig;
    // Manually add invalid credentials (bypassing validation)
    invalidConfig.count = 1;
    invalidConfig.networks[0].ssid = ""; // Invalid: empty SSID
    invalidConfig.networks[0].password = "password";

    bool result = manager->saveConfig(invalidConfig);

    EXPECT_FALSE(result);
    EXPECT_FALSE(mockFS->exists(CONFIG_FILE_PATH)); // File should not be created
}

/**
 * Test: Save config - empty config
 */
TEST_F(WiFiManagerTest, SaveConfigEmpty) {
    WiFiConfigFile emptyConfig;

    bool result = manager->saveConfig(emptyConfig);

    EXPECT_FALSE(result); // Empty config should be rejected
}

/**
 * Test: Save config - overwrite existing file
 */
TEST_F(WiFiManagerTest, SaveConfigOverwrite) {
    // Create initial config
    createTestConfigFile("OldNetwork,oldpass\n");

    // Save new config
    config.addNetwork(WiFiCredentials("NewNetwork", "newpass"));
    bool result = manager->saveConfig(config);

    EXPECT_TRUE(result);

    // Verify old content replaced
    String content = mockFS->readFile(CONFIG_FILE_PATH);
    EXPECT_FALSE(content.indexOf("OldNetwork") >= 0);
    EXPECT_TRUE(content.indexOf("NewNetwork") >= 0);
}

// ============================================================================
// T027: connect() Tests
// ============================================================================

/**
 * Test: Connect to network - successful
 */
TEST_F(WiFiManagerTest, ConnectSuccess) {
    config.addNetwork(WiFiCredentials("TestNetwork", "testpass"));
    mockWiFi->setConnectionBehavior(true); // Simulate success

    bool result = manager->connect(state, config);

    EXPECT_TRUE(result);
    EXPECT_EQ(state.status, ConnectionStatus::CONNECTING);
    EXPECT_GT(state.attemptStartTime, 0); // Timestamp should be set
    EXPECT_EQ(mockWiFi->getSSID(), "TestNetwork");
}

/**
 * Test: Connect to second network after first fails
 */
TEST_F(WiFiManagerTest, ConnectSecondNetwork) {
    config.addNetwork(WiFiCredentials("Network1", "pass1"));
    config.addNetwork(WiFiCredentials("Network2", "pass2"));

    state.currentNetworkIndex = 1; // Try second network

    bool result = manager->connect(state, config);

    EXPECT_TRUE(result);
    EXPECT_EQ(mockWiFi->getSSID(), "Network2");
}

/**
 * Test: Connect - no networks in config
 */
TEST_F(WiFiManagerTest, ConnectNoNetworks) {
    WiFiConfigFile emptyConfig;

    bool result = manager->connect(state, emptyConfig);

    EXPECT_FALSE(result);
}

/**
 * Test: Connect - invalid network index
 */
TEST_F(WiFiManagerTest, ConnectInvalidIndex) {
    config.addNetwork(WiFiCredentials("Network1", "pass1"));
    state.currentNetworkIndex = 5; // Out of bounds

    bool result = manager->connect(state, config);

    EXPECT_FALSE(result);
}

/**
 * Test: Connect - state transitions correctly
 */
TEST_F(WiFiManagerTest, ConnectStateTransition) {
    config.addNetwork(WiFiCredentials("TestNetwork", "testpass"));
    state.status = ConnectionStatus::DISCONNECTED;

    manager->connect(state, config);

    EXPECT_EQ(state.status, ConnectionStatus::CONNECTING);
}

/**
 * Test: Connect - WiFi adapter begin() called
 */
TEST_F(WiFiManagerTest, ConnectCallsAdapterBegin) {
    config.addNetwork(WiFiCredentials("Network1", "password1"));

    manager->connect(state, config);

    // Verify WiFi adapter was called with correct credentials
    EXPECT_EQ(mockWiFi->getSSID(), "Network1");
}

// ============================================================================
// T028: handleDisconnect() Tests
// ============================================================================

/**
 * Test: Handle disconnect - sets state to DISCONNECTED
 */
TEST_F(WiFiManagerTest, HandleDisconnectSetsState) {
    state.status = ConnectionStatus::CONNECTED;
    state.connectedSSID = "TestNetwork";
    state.currentNetworkIndex = 1;

    manager->handleDisconnect(state);

    EXPECT_EQ(state.status, ConnectionStatus::DISCONNECTED);
}

/**
 * Test: Handle disconnect - keeps current network index unchanged
 */
TEST_F(WiFiManagerTest, HandleDisconnectKeepsIndex) {
    state.status = ConnectionStatus::CONNECTED;
    state.currentNetworkIndex = 2;

    manager->handleDisconnect(state);

    EXPECT_EQ(state.currentNetworkIndex, 2); // Should NOT move to next network
}

/**
 * Test: Handle disconnect - clears connected SSID
 */
TEST_F(WiFiManagerTest, HandleDisconnectClearsSSID) {
    state.status = ConnectionStatus::CONNECTED;
    state.connectedSSID = "HomeNetwork";

    manager->handleDisconnect(state);

    EXPECT_EQ(state.connectedSSID, "");
}

/**
 * Test: Handle disconnect - from CONNECTING state
 */
TEST_F(WiFiManagerTest, HandleDisconnectFromConnecting) {
    state.status = ConnectionStatus::CONNECTING;

    manager->handleDisconnect(state);

    EXPECT_EQ(state.status, ConnectionStatus::DISCONNECTED);
}

/**
 * Test: Handle disconnect - multiple times (idempotent)
 */
TEST_F(WiFiManagerTest, HandleDisconnectIdempotent) {
    state.status = ConnectionStatus::CONNECTED;

    manager->handleDisconnect(state);
    EXPECT_EQ(state.status, ConnectionStatus::DISCONNECTED);

    manager->handleDisconnect(state);
    EXPECT_EQ(state.status, ConnectionStatus::DISCONNECTED); // Still disconnected
}

/**
 * Test: Handle disconnect - retry count unchanged
 */
TEST_F(WiFiManagerTest, HandleDisconnectRetryCountUnchanged) {
    state.status = ConnectionStatus::CONNECTED;
    state.retryCount = 3;

    manager->handleDisconnect(state);

    // Retry count should NOT be incremented on disconnect
    // (only on FAILED transitions)
    EXPECT_EQ(state.retryCount, 3);
}

// ============================================================================
// Integration Tests (Multiple Methods)
// ============================================================================

/**
 * Test: Complete flow - load, connect, disconnect
 */
TEST_F(WiFiManagerTest, CompleteFlowLoadConnectDisconnect) {
    // 1. Load config
    createTestConfigFile("HomeNetwork,password123\n");
    manager->loadConfig(config);
    EXPECT_EQ(config.count, 1);

    // 2. Connect
    mockWiFi->setConnectionBehavior(true);
    manager->connect(state, config);
    EXPECT_EQ(state.status, ConnectionStatus::CONNECTING);

    // 3. Simulate successful connection
    state.transitionTo(ConnectionStatus::CONNECTED, "HomeNetwork");
    EXPECT_EQ(state.status, ConnectionStatus::CONNECTED);

    // 4. Handle disconnect
    manager->handleDisconnect(state);
    EXPECT_EQ(state.status, ConnectionStatus::DISCONNECTED);
    EXPECT_EQ(state.currentNetworkIndex, 0); // Should retry same network
}

/**
 * Test: Save then load config
 */
TEST_F(WiFiManagerTest, SaveThenLoadConfig) {
    // Save config
    config.addNetwork(WiFiCredentials("Network1", "pass1"));
    config.addNetwork(WiFiCredentials("Network2", "pass2"));
    manager->saveConfig(config);

    // Clear and reload
    WiFiConfigFile reloadedConfig;
    manager->loadConfig(reloadedConfig);

    EXPECT_EQ(reloadedConfig.count, 2);
    EXPECT_EQ(reloadedConfig.networks[0].ssid, "Network1");
    EXPECT_EQ(reloadedConfig.networks[1].ssid, "Network2");
}
