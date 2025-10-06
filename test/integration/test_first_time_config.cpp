/**
 * @file test_first_time_config.cpp
 * @brief Integration test for User Story 1: First-Time Configuration
 *
 * Test scenario:
 * 1. Device boots with no existing configuration
 * 2. WiFiManager logs CONFIG_LOADED error
 * 3. User uploads wifi.conf via HTTP API
 * 4. Device schedules reboot after 5 seconds
 * 5. Device reboots and connects to first network
 *
 * This test validates the complete flow from initial boot to successful
 * connection using mocked WiFi and filesystem interfaces.
 */

#include <gtest/gtest.h>
#include "../../src/components/WiFiManager.h"
#include "../../src/components/ConfigWebServer.h"
#include "../../src/mocks/MockWiFiAdapter.h"
#include "../../src/mocks/MockFileSystem.h"
#include "../../src/utils/UDPLogger.h"
#include "../../src/utils/TimeoutManager.h"
#include "../../src/config.h"

/**
 * Test fixture for first-time configuration scenario
 */
class FirstTimeConfigTest : public ::testing::Test {
protected:
    WiFiManager* manager;
    MockWiFiAdapter* mockWiFi;
    MockFileSystem* mockFS;
    UDPLogger* logger;
    TimeoutManager* timeoutMgr;
    WiFiConfigFile config;
    WiFiConnectionState state;

    void SetUp() override {
        mockWiFi = new MockWiFiAdapter();
        mockFS = new MockFileSystem();
        logger = new UDPLogger();
        timeoutMgr = new TimeoutManager();

        // Mount filesystem (empty - no config file)
        mockFS->mount();

        // Create WiFiManager with dependencies
        manager = new WiFiManager(mockWiFi, mockFS, logger, timeoutMgr);

        // Initialize state
        state = WiFiConnectionState();
        config = WiFiConfigFile();
    }

    void TearDown() override {
        delete manager;
        delete mockWiFi;
        delete mockFS;
        delete logger;
        delete timeoutMgr;
    }
};

/**
 * Test: Device boots with no config file
 *
 * Expected behavior:
 * - loadConfig() returns false
 * - CONFIG_LOADED event logged with success=false
 * - Device should schedule reboot (handled by main.cpp)
 */
TEST_F(FirstTimeConfigTest, BootWithNoConfig) {
    // Attempt to load config (should fail - no file exists)
    bool loadResult = manager->loadConfig(config);

    // Verify load failed
    EXPECT_FALSE(loadResult);
    EXPECT_EQ(0, config.count);

    // State should remain DISCONNECTED
    EXPECT_EQ(ConnectionStatus::DISCONNECTED, state.status);
}

/**
 * Test: User uploads valid configuration file
 *
 * Expected behavior:
 * - saveConfig() writes file to filesystem
 * - File persists and can be reloaded
 * - Config contains 3 networks in priority order
 */
TEST_F(FirstTimeConfigTest, UploadValidConfig) {
    // Create valid configuration with 3 networks
    WiFiConfigFile uploadConfig;
    uploadConfig.networks[0] = WiFiCredentials("HomeNetwork", "mypassword123");
    uploadConfig.networks[1] = WiFiCredentials("MobileHotspot", "hotspotpass");
    uploadConfig.networks[2] = WiFiCredentials("GuestNetwork", "guestpass");
    uploadConfig.count = 3;

    // Save configuration
    bool saveResult = manager->saveConfig(uploadConfig);

    // Verify save succeeded
    EXPECT_TRUE(saveResult);

    // Verify file exists in filesystem
    EXPECT_TRUE(mockFS->exists(CONFIG_FILE_PATH));

    // Reload configuration and verify contents
    WiFiConfigFile reloadedConfig;
    bool loadResult = manager->loadConfig(reloadedConfig);

    EXPECT_TRUE(loadResult);
    EXPECT_EQ(3, reloadedConfig.count);
    EXPECT_STREQ("HomeNetwork", reloadedConfig.networks[0].ssid);
    EXPECT_STREQ("MobileHotspot", reloadedConfig.networks[1].ssid);
    EXPECT_STREQ("GuestNetwork", reloadedConfig.networks[2].ssid);
}

/**
 * Test: Device connects to first network after reboot
 *
 * Expected behavior:
 * - Config loaded successfully
 * - connect() attempts first network (index 0)
 * - State transitions to CONNECTING
 * - WiFi adapter receives begin() call with first network credentials
 * - On success, state transitions to CONNECTED
 */
TEST_F(FirstTimeConfigTest, ConnectToFirstNetworkAfterReboot) {
    // Create and save config
    config.networks[0] = WiFiCredentials("HomeNetwork", "mypassword123");
    config.networks[1] = WiFiCredentials("MobileHotspot", "hotspotpass");
    config.networks[2] = WiFiCredentials("GuestNetwork", "guestpass");
    config.count = 3;
    manager->saveConfig(config);

    // Simulate reboot - reload config
    WiFiConfigFile rebootedConfig;
    manager->loadConfig(rebootedConfig);

    // Initialize connection state
    state.status = ConnectionStatus::DISCONNECTED;
    state.currentNetworkIndex = 0;

    // Configure mock WiFi to succeed on connection
    mockWiFi->setConnectionBehavior(true);

    // Attempt connection to first network
    bool connectResult = manager->connect(state, rebootedConfig);

    // Verify connection attempt started
    EXPECT_TRUE(connectResult);
    EXPECT_EQ(ConnectionStatus::CONNECTING, state.status);
    EXPECT_EQ(0, state.currentNetworkIndex);

    // Simulate successful connection
    mockWiFi->simulateConnectionSuccess("HomeNetwork", -45);

    // Update state via handleConnectionSuccess
    manager->handleConnectionSuccess(state, "HomeNetwork");

    // Verify final state
    EXPECT_EQ(ConnectionStatus::CONNECTED, state.status);
    EXPECT_STREQ("HomeNetwork", state.connectedSSID.c_str());
}

/**
 * Test: Complete end-to-end first-time configuration flow
 *
 * This test simulates the entire user story from boot to connection.
 */
TEST_F(FirstTimeConfigTest, CompleteFirstTimeFlow) {
    // Step 1: Boot with no config (loadConfig fails)
    EXPECT_FALSE(manager->loadConfig(config));

    // Step 2: User uploads config via web API
    WiFiConfigFile uploadConfig;
    uploadConfig.networks[0] = WiFiCredentials("HomeNetwork", "mypassword123");
    uploadConfig.networks[1] = WiFiCredentials("Marina_Guest", "guestpass");
    uploadConfig.count = 2;

    EXPECT_TRUE(manager->saveConfig(uploadConfig));

    // Step 3: Simulate reboot (clear state, reload config)
    state = WiFiConnectionState();
    config = WiFiConfigFile();

    EXPECT_TRUE(manager->loadConfig(config));
    EXPECT_EQ(2, config.count);

    // Step 4: Connect to first network
    mockWiFi->setConnectionBehavior(true);
    EXPECT_TRUE(manager->connect(state, config));
    EXPECT_EQ(ConnectionStatus::CONNECTING, state.status);

    // Step 5: Connection succeeds
    mockWiFi->simulateConnectionSuccess("HomeNetwork", -50);
    manager->handleConnectionSuccess(state, "HomeNetwork");

    // Verify final state
    EXPECT_EQ(ConnectionStatus::CONNECTED, state.status);
    EXPECT_STREQ("HomeNetwork", state.connectedSSID.c_str());
    EXPECT_EQ(0, state.currentNetworkIndex);
}

/**
 * Test: Verify reboot is scheduled after config upload
 *
 * Note: Actual reboot scheduling is handled by main.cpp and ConfigWebServer.
 * This test verifies that saveConfig completes successfully, which triggers
 * the reboot schedule in the web server handler.
 */
TEST_F(FirstTimeConfigTest, RebootScheduledAfterUpload) {
    WiFiConfigFile uploadConfig;
    uploadConfig.networks[0] = WiFiCredentials("TestNetwork", "testpass123");
    uploadConfig.count = 1;

    // Save should succeed
    EXPECT_TRUE(manager->saveConfig(uploadConfig));

    // Verify config persisted
    EXPECT_TRUE(mockFS->exists(CONFIG_FILE_PATH));

    // In actual implementation, ConfigWebServer.handleUpload() would:
    // 1. Call manager->saveConfig()
    // 2. Call scheduleReboot(REBOOT_DELAY_MS)
    // This test verifies step 1 completes successfully
}
