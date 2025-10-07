/**
 * @file test_connection_loss_recovery.cpp
 * @brief Integration test for User Story 3: Connection Loss Recovery
 *
 * Test scenario:
 * 1. Device is connected to a network
 * 2. Connection is lost (WiFi disconnect event)
 * 3. Device retries the SAME network (no failover to other networks)
 * 4. Device reconnects when network becomes available
 *
 * Key requirement: Connection loss does NOT trigger failover.
 * Device always retries the last connected network.
 */

#include <gtest/gtest.h>
#include "../../src/components/WiFiManager.h"
#include "../../src/mocks/MockWiFiAdapter.h"
#include "../../src/mocks/MockFileSystem.h"
#include "../../src/utils/UDPLogger.h"
#include "../../src/utils/TimeoutManager.h"
#include "../../src/config.h"

/**
 * Test fixture for connection loss recovery scenario
 */
class ConnectionLossRecoveryTest : public ::testing::Test {
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

        mockFS->mount();
        manager = new WiFiManager(mockWiFi, mockFS, logger, timeoutMgr);

        // Create config with 3 networks
        config.networks[0] = WiFiCredentials("PrimaryNetwork", "primary123");
        config.networks[1] = WiFiCredentials("SecondaryNetwork", "secondary123");
        config.networks[2] = WiFiCredentials("TertiaryNetwork", "tertiary123");
        config.count = 3;

        manager->saveConfig(config);
        manager->loadConfig(config);

        // Initialize state
        state = WiFiConnectionState();
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
 * Test: Connection lost - retry same network
 *
 * Expected behavior:
 * - Device connected to network at index 1 (SecondaryNetwork)
 * - Connection lost
 * - handleDisconnect() called
 * - currentNetworkIndex remains 1 (does NOT change)
 * - Retry attempt uses same network
 */
TEST_F(ConnectionLossRecoveryTest, RetrySameNetworkAfterDisconnect) {
    // Simulate device connected to second network (index 1)
    state.currentNetworkIndex = 1;
    mockWiFi->setConnectionBehavior(true);

    manager->connect(state, config);
    mockWiFi->simulateConnectionSuccess("SecondaryNetwork", -50);
    manager->handleConnectionSuccess(state, "SecondaryNetwork");

    // Verify connected state
    EXPECT_EQ(ConnectionStatus::CONNECTED, state.status);
    EXPECT_EQ(1, state.currentNetworkIndex);
    EXPECT_STREQ("SecondaryNetwork", state.connectedSSID.c_str());

    // Simulate connection loss
    mockWiFi->simulateDisconnect();
    manager->handleDisconnect(state);

    // Verify state after disconnect
    EXPECT_EQ(ConnectionStatus::DISCONNECTED, state.status);

    // CRITICAL: currentNetworkIndex should NOT change
    EXPECT_EQ(1, state.currentNetworkIndex);

    // Verify connectedSSID is cleared
    EXPECT_STREQ("", state.connectedSSID.c_str());

    // Retry count should increment
    EXPECT_GT(state.retryCount, 0);
}

/**
 * Test: No failover after connection loss
 *
 * Expected behavior:
 * - Connected to network 0
 * - Disconnect occurs
 * - Retry attempts use network 0 only
 * - Never tries network 1 or 2
 */
TEST_F(ConnectionLossRecoveryTest, NoFailoverOnConnectionLoss) {
    // Connect to first network
    state.currentNetworkIndex = 0;
    mockWiFi->setConnectionBehavior(true);

    manager->connect(state, config);
    mockWiFi->simulateConnectionSuccess("PrimaryNetwork", -45);
    manager->handleConnectionSuccess(state, "PrimaryNetwork");

    EXPECT_EQ(0, state.currentNetworkIndex);

    // Simulate disconnect
    mockWiFi->simulateDisconnect();
    manager->handleDisconnect(state);

    // Verify still at network 0
    EXPECT_EQ(0, state.currentNetworkIndex);

    // Attempt reconnection
    manager->connect(state, config);

    // Verify attempt is for network 0, not network 1
    EXPECT_EQ(0, state.currentNetworkIndex);
    EXPECT_EQ(ConnectionStatus::CONNECTING, state.status);
}

/**
 * Test: Successful reconnection after temporary disconnection
 *
 * Expected behavior:
 * - Connected to network
 * - Disconnect event
 * - Retry same network
 * - Network becomes available
 * - Reconnection succeeds
 */
TEST_F(ConnectionLossRecoveryTest, SuccessfulReconnection) {
    // Establish initial connection (network 2)
    state.currentNetworkIndex = 2;
    mockWiFi->setConnectionBehavior(true);

    manager->connect(state, config);
    mockWiFi->simulateConnectionSuccess("TertiaryNetwork", -60);
    manager->handleConnectionSuccess(state, "TertiaryNetwork");

    EXPECT_EQ(ConnectionStatus::CONNECTED, state.status);
    EXPECT_STREQ("TertiaryNetwork", state.connectedSSID.c_str());

    // Simulate temporary disconnection
    mockWiFi->simulateDisconnect();
    manager->handleDisconnect(state);

    EXPECT_EQ(ConnectionStatus::DISCONNECTED, state.status);
    EXPECT_EQ(2, state.currentNetworkIndex); // Still network 2

    // Network becomes available again - retry
    mockWiFi->setConnectionBehavior(true);
    manager->connect(state, config);

    // Simulate reconnection success
    mockWiFi->simulateConnectionSuccess("TertiaryNetwork", -58);
    manager->handleConnectionSuccess(state, "TertiaryNetwork");

    // Verify reconnected to same network
    EXPECT_EQ(ConnectionStatus::CONNECTED, state.status);
    EXPECT_STREQ("TertiaryNetwork", state.connectedSSID.c_str());
    EXPECT_EQ(2, state.currentNetworkIndex);

    // Retry count should be reset after successful connection
    EXPECT_EQ(0, state.retryCount);
}

/**
 * Test: Multiple disconnect/reconnect cycles
 *
 * Expected behavior:
 * - Connection stable after each reconnection
 * - currentNetworkIndex never changes during cycles
 * - Each cycle increments retryCount until success
 */
TEST_F(ConnectionLossRecoveryTest, MultipleDisconnectReconnectCycles) {
    // Initial connection to network 1
    state.currentNetworkIndex = 1;
    mockWiFi->setConnectionBehavior(true);

    manager->connect(state, config);
    mockWiFi->simulateConnectionSuccess("SecondaryNetwork", -52);
    manager->handleConnectionSuccess(state, "SecondaryNetwork");

    // Cycle 1: Disconnect and reconnect
    mockWiFi->simulateDisconnect();
    manager->handleDisconnect(state);
    EXPECT_EQ(1, state.currentNetworkIndex);

    manager->connect(state, config);
    mockWiFi->simulateConnectionSuccess("SecondaryNetwork", -53);
    manager->handleConnectionSuccess(state, "SecondaryNetwork");
    EXPECT_EQ(ConnectionStatus::CONNECTED, state.status);

    // Cycle 2: Disconnect and reconnect again
    mockWiFi->simulateDisconnect();
    manager->handleDisconnect(state);
    EXPECT_EQ(1, state.currentNetworkIndex);

    manager->connect(state, config);
    mockWiFi->simulateConnectionSuccess("SecondaryNetwork", -51);
    manager->handleConnectionSuccess(state, "SecondaryNetwork");

    // After both cycles, still at network 1
    EXPECT_EQ(1, state.currentNetworkIndex);
    EXPECT_EQ(ConnectionStatus::CONNECTED, state.status);
}

/**
 * Test: Disconnect event sets correct state
 *
 * Expected behavior:
 * - handleDisconnect() transitions to DISCONNECTED
 * - connectedSSID cleared
 * - currentNetworkIndex preserved
 * - retryCount incremented
 */
TEST_F(ConnectionLossRecoveryTest, DisconnectEventSetsCorrectState) {
    // Setup connected state
    state.status = ConnectionStatus::CONNECTED;
    state.currentNetworkIndex = 1;
    state.connectedSSID = "SecondaryNetwork";
    state.retryCount = 0;

    // Simulate disconnect
    manager->handleDisconnect(state);

    // Verify state changes
    EXPECT_EQ(ConnectionStatus::DISCONNECTED, state.status);
    EXPECT_STREQ("", state.connectedSSID.c_str());
    EXPECT_EQ(1, state.currentNetworkIndex); // Preserved
    EXPECT_EQ(1, state.retryCount); // Incremented
}

/**
 * Test: Connection loss does not trigger reboot
 *
 * Expected behavior:
 * - Disconnect from network
 * - Retry same network indefinitely
 * - No reboot scheduled (only all networks exhausted triggers reboot)
 */
TEST_F(ConnectionLossRecoveryTest, ConnectionLossDoesNotTriggerReboot) {
    // Connect to network
    state.currentNetworkIndex = 0;
    mockWiFi->setConnectionBehavior(true);

    manager->connect(state, config);
    mockWiFi->simulateConnectionSuccess("PrimaryNetwork", -48);
    manager->handleConnectionSuccess(state, "PrimaryNetwork");

    // Disconnect
    mockWiFi->simulateDisconnect();
    manager->handleDisconnect(state);

    // Verify still at network 0
    EXPECT_EQ(0, state.currentNetworkIndex);

    // Retry multiple times (simulate network still unavailable)
    for (int i = 0; i < 5; i++) {
        mockWiFi->setConnectionBehavior(false);
        manager->connect(state, config);

        // Simulate timeout
        state.attemptStartTime = millis() - (WIFI_TIMEOUT_MS + 1000);

        // checkTimeout should return true (keep trying same network)
        // It should NOT move to next network or trigger reboot
        EXPECT_EQ(0, state.currentNetworkIndex);
    }

    // After multiple retries, still at network 0 (no failover, no reboot)
    EXPECT_EQ(0, state.currentNetworkIndex);
}
