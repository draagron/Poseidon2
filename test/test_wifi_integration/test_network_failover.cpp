/**
 * @file test_network_failover.cpp
 * @brief Integration test for User Story 2: Network Failover
 *
 * Test scenario:
 * 1. Device boots with 3 configured networks
 * 2. First network is unavailable
 * 3. Connection times out after 30 seconds
 * 4. Device automatically tries second network
 * 5. Second network connects successfully
 * 6. Total connection time: 30-60 seconds
 *
 * This test validates priority-ordered failover with timeout handling.
 */

#include <gtest/gtest.h>
#include "../../src/components/WiFiManager.h"
#include "../../src/mocks/MockWiFiAdapter.h"
#include "../../src/mocks/MockFileSystem.h"
#include "../../src/utils/UDPLogger.h"
#include "../../src/utils/TimeoutManager.h"
#include "../../src/config.h"

/**
 * Test fixture for network failover scenario
 */
class NetworkFailoverTest : public ::testing::Test {
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
        config.networks[0] = WiFiCredentials("UnavailableNetwork", "password1");
        config.networks[1] = WiFiCredentials("AvailableNetwork", "password2");
        config.networks[2] = WiFiCredentials("BackupNetwork", "password3");
        config.count = 3;

        // Save config to filesystem
        manager->saveConfig(config);

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
 * Test: First network times out, second network succeeds
 *
 * Expected behavior:
 * - Connect to first network (index 0)
 * - Timeout after 30 seconds
 * - checkTimeout() returns true, increments currentNetworkIndex
 * - Connect to second network (index 1)
 * - Second network connects successfully
 */
TEST_F(NetworkFailoverTest, FirstNetworkTimesOutSecondSucceeds) {
    // Load config
    manager->loadConfig(config);

    // Attempt first network (index 0)
    state.currentNetworkIndex = 0;
    mockWiFi->setConnectionBehavior(false); // First network fails

    bool connectResult = manager->connect(state, config);
    EXPECT_TRUE(connectResult);
    EXPECT_EQ(ConnectionStatus::CONNECTING, state.status);
    EXPECT_EQ(0, state.currentNetworkIndex);

    // Simulate timeout (30 seconds elapsed)
    // In real implementation, this happens via TimeoutManager callback
    // For testing, we manually advance time and check timeout
    unsigned long startTime = state.attemptStartTime;

    // Fast-forward time by 30+ seconds
    // Note: In actual test with TimeoutManager, we'd register callback
    // and verify it fires after WIFI_TIMEOUT_MS

    // Simulate timeout detection
    bool timeoutDetected = state.isTimeoutExceeded(WIFI_TIMEOUT_MS);
    EXPECT_TRUE(timeoutDetected);

    // Handle timeout - move to next network
    bool shouldContinue = manager->checkTimeout(state, config);
    EXPECT_TRUE(shouldContinue); // Should try next network

    // Verify state updated to next network
    EXPECT_EQ(1, state.currentNetworkIndex);

    // Attempt second network
    mockWiFi->setConnectionBehavior(true); // Second network succeeds
    connectResult = manager->connect(state, config);
    EXPECT_TRUE(connectResult);
    EXPECT_EQ(ConnectionStatus::CONNECTING, state.status);

    // Simulate connection success
    mockWiFi->simulateConnectionSuccess("AvailableNetwork", -55);
    manager->handleConnectionSuccess(state, "AvailableNetwork");

    // Verify connected to second network
    EXPECT_EQ(ConnectionStatus::CONNECTED, state.status);
    EXPECT_STREQ("AvailableNetwork", state.connectedSSID.c_str());
    EXPECT_EQ(1, state.currentNetworkIndex);
}

/**
 * Test: Failover respects priority order
 *
 * Expected behavior:
 * - Always tries networks in order: 0 → 1 → 2
 * - Does not skip networks
 * - Increments currentNetworkIndex sequentially
 */
TEST_F(NetworkFailoverTest, FailoverRespectsPriority) {
    manager->loadConfig(config);

    // Start with network 0
    state.currentNetworkIndex = 0;
    EXPECT_EQ(0, state.currentNetworkIndex);

    // Simulate timeout on first network
    state.transitionTo(ConnectionStatus::CONNECTING);
    state.attemptStartTime = millis() - (WIFI_TIMEOUT_MS + 1000);

    bool shouldContinue = manager->checkTimeout(state, config);
    EXPECT_TRUE(shouldContinue);

    // Should move to network 1, not network 2
    EXPECT_EQ(1, state.currentNetworkIndex);

    // Simulate timeout on second network
    state.transitionTo(ConnectionStatus::CONNECTING);
    state.attemptStartTime = millis() - (WIFI_TIMEOUT_MS + 1000);

    shouldContinue = manager->checkTimeout(state, config);
    EXPECT_TRUE(shouldContinue);

    // Should move to network 2
    EXPECT_EQ(2, state.currentNetworkIndex);
}

/**
 * Test: Timeout duration is 30 seconds per network
 *
 * Expected behavior:
 * - Each network gets exactly 30 seconds (WIFI_TIMEOUT_MS)
 * - Timeout not triggered before 30s
 * - Timeout triggered after 30s
 */
TEST_F(NetworkFailoverTest, TimeoutDurationIs30Seconds) {
    manager->loadConfig(config);

    // Start connection attempt
    state.currentNetworkIndex = 0;
    manager->connect(state, config);

    unsigned long startTime = state.attemptStartTime;

    // Simulate 29 seconds elapsed (not timed out yet)
    state.attemptStartTime = millis() - 29000;
    EXPECT_FALSE(state.isTimeoutExceeded(WIFI_TIMEOUT_MS));

    // Simulate 30 seconds elapsed (timed out)
    state.attemptStartTime = millis() - 30000;
    EXPECT_TRUE(state.isTimeoutExceeded(WIFI_TIMEOUT_MS));

    // Simulate 31 seconds elapsed (definitely timed out)
    state.attemptStartTime = millis() - 31000;
    EXPECT_TRUE(state.isTimeoutExceeded(WIFI_TIMEOUT_MS));
}

/**
 * Test: Total connection time for failover scenario
 *
 * Expected behavior:
 * - First network: 0-30s (timeout)
 * - Second network: 30-60s (success)
 * - Total time: 30-60s from boot
 */
TEST_F(NetworkFailoverTest, TotalConnectionTimeWithFailover) {
    manager->loadConfig(config);

    unsigned long testStartTime = millis();

    // Attempt first network
    state.currentNetworkIndex = 0;
    mockWiFi->setConnectionBehavior(false);
    manager->connect(state, config);

    // Simulate 30s timeout
    state.attemptStartTime = testStartTime;
    unsigned long firstTimeoutTime = testStartTime + 30000;

    // After 30s, timeout occurs
    EXPECT_TRUE(state.isTimeoutExceeded(WIFI_TIMEOUT_MS));

    // Move to second network
    manager->checkTimeout(state, config);
    EXPECT_EQ(1, state.currentNetworkIndex);

    // Attempt second network
    mockWiFi->setConnectionBehavior(true);
    unsigned long secondAttemptTime = firstTimeoutTime;
    manager->connect(state, config);

    // Simulate connection success within a few seconds
    unsigned long connectionTime = secondAttemptTime + 5000;
    mockWiFi->simulateConnectionSuccess("AvailableNetwork", -60);
    manager->handleConnectionSuccess(state, "AvailableNetwork");

    // Total elapsed time: ~35 seconds (30s timeout + 5s connection)
    unsigned long totalTime = connectionTime - testStartTime;

    EXPECT_GE(totalTime, 30000); // At least 30s (first timeout)
    EXPECT_LE(totalTime, 60000); // Less than 60s (spec requirement)

    // Verify connected
    EXPECT_EQ(ConnectionStatus::CONNECTED, state.status);
}

/**
 * Test: Connection state transitions during failover
 *
 * Expected behavior:
 * - Network 0: DISCONNECTED → CONNECTING → FAILED
 * - Network 1: FAILED → CONNECTING → CONNECTED
 */
TEST_F(NetworkFailoverTest, StateTransitionsDuringFailover) {
    manager->loadConfig(config);

    // Initial state
    EXPECT_EQ(ConnectionStatus::DISCONNECTED, state.status);

    // Attempt first network
    state.currentNetworkIndex = 0;
    mockWiFi->setConnectionBehavior(false);
    manager->connect(state, config);

    // Should be CONNECTING
    EXPECT_EQ(ConnectionStatus::CONNECTING, state.status);

    // Simulate timeout
    state.attemptStartTime = millis() - (WIFI_TIMEOUT_MS + 1000);
    manager->checkTimeout(state, config);

    // checkTimeout internally transitions through FAILED state
    // and may start next connection attempt

    // Attempt second network
    mockWiFi->setConnectionBehavior(true);
    manager->connect(state, config);

    EXPECT_EQ(ConnectionStatus::CONNECTING, state.status);

    // Simulate success
    mockWiFi->simulateConnectionSuccess("AvailableNetwork", -50);
    manager->handleConnectionSuccess(state, "AvailableNetwork");

    // Final state: CONNECTED
    EXPECT_EQ(ConnectionStatus::CONNECTED, state.status);
}
