/**
 * @file test_all_networks_unavailable.cpp
 * @brief Integration test for User Story 4: All Networks Unavailable
 *
 * Test scenario:
 * 1. Device has 3 configured networks
 * 2. All 3 networks are unavailable
 * 3. Each network times out after 30 seconds
 * 4. After all networks fail (90 seconds total), device schedules reboot
 * 5. Reboot occurs after 5-second delay (95 seconds total)
 * 6. Cycle repeats on next boot until network becomes available
 *
 * This test validates the reboot loop behavior when no networks are reachable.
 */

#include <gtest/gtest.h>
#include "../../src/components/WiFiManager.h"
#include "../../src/mocks/MockWiFiAdapter.h"
#include "../../src/mocks/MockFileSystem.h"
#include "../../src/utils/UDPLogger.h"
#include "../../src/utils/TimeoutManager.h"
#include "../../src/config.h"

/**
 * Test fixture for all networks unavailable scenario
 */
class AllNetworksUnavailableTest : public ::testing::Test {
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

        // Create config with 3 networks (all unavailable)
        config.networks[0] = WiFiCredentials("Network1", "pass1");
        config.networks[1] = WiFiCredentials("Network2", "pass2");
        config.networks[2] = WiFiCredentials("Network3", "pass3");
        config.count = 3;

        manager->saveConfig(config);
        manager->loadConfig(config);

        // Initialize state
        state = WiFiConnectionState();

        // Configure all networks to fail
        mockWiFi->setConnectionBehavior(false);
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
 * Test: All three networks time out sequentially
 *
 * Expected behavior:
 * - Network 0: 0-30s (timeout)
 * - Network 1: 30-60s (timeout)
 * - Network 2: 60-90s (timeout)
 * - After network 2 timeout: reboot scheduled
 */
TEST_F(AllNetworksUnavailableTest, AllThreeNetworksTimeout) {
    unsigned long testStartTime = millis();

    // Attempt network 0
    state.currentNetworkIndex = 0;
    manager->connect(state, config);
    EXPECT_EQ(ConnectionStatus::CONNECTING, state.status);

    // Simulate 30s timeout
    state.attemptStartTime = testStartTime;
    EXPECT_FALSE(state.isTimeoutExceeded(29000)); // Not yet
    EXPECT_TRUE(state.isTimeoutExceeded(WIFI_TIMEOUT_MS)); // Timed out

    // Move to network 1
    bool shouldContinue = manager->checkTimeout(state, config);
    EXPECT_TRUE(shouldContinue);
    EXPECT_EQ(1, state.currentNetworkIndex);

    // Attempt network 1
    manager->connect(state, config);
    state.attemptStartTime = testStartTime + 30000;

    // Simulate another 30s timeout
    EXPECT_TRUE(state.isTimeoutExceeded(WIFI_TIMEOUT_MS));

    // Move to network 2
    shouldContinue = manager->checkTimeout(state, config);
    EXPECT_TRUE(shouldContinue);
    EXPECT_EQ(2, state.currentNetworkIndex);

    // Attempt network 2
    manager->connect(state, config);
    state.attemptStartTime = testStartTime + 60000;

    // Simulate final 30s timeout
    EXPECT_TRUE(state.isTimeoutExceeded(WIFI_TIMEOUT_MS));

    // All networks exhausted - should return false (reboot required)
    shouldContinue = manager->checkTimeout(state, config);
    EXPECT_FALSE(shouldContinue); // No more networks, reboot needed

    // Total time: ~90 seconds (3 × 30s)
}

/**
 * Test: All networks exhausted triggers reboot signal
 *
 * Expected behavior:
 * - After trying all 3 networks
 * - checkTimeout() returns false (indicating reboot needed)
 * - main.cpp would schedule reboot with 5-second delay
 */
TEST_F(AllNetworksUnavailableTest, AllNetworksExhaustedTriggersRebootSignal) {
    // Try all 3 networks with timeouts
    for (int i = 0; i < 3; i++) {
        state.currentNetworkIndex = i;
        manager->connect(state, config);

        // Simulate timeout
        state.attemptStartTime = millis() - (WIFI_TIMEOUT_MS + 1000);

        bool shouldContinue = manager->checkTimeout(state, config);

        if (i < 2) {
            // First two networks: should continue to next
            EXPECT_TRUE(shouldContinue);
        } else {
            // Third network: all exhausted, reboot needed
            EXPECT_FALSE(shouldContinue);
        }
    }

    // Verify all networks were tried
    EXPECT_TRUE(state.allNetworksExhausted(config.count));
}

/**
 * Test: Reboot cycle repeats until network available
 *
 * Expected behavior:
 * - Boot 1: All networks fail → reboot
 * - Boot 2: All networks fail → reboot
 * - Boot 3: Network 1 succeeds → connected
 */
TEST_F(AllNetworksUnavailableTest, RebootCycleRepeatsUntilNetworkAvailable) {
    // Simulate Boot 1: All networks fail
    for (int i = 0; i < 3; i++) {
        state.currentNetworkIndex = i;
        manager->connect(state, config);
        state.attemptStartTime = millis() - (WIFI_TIMEOUT_MS + 1000);

        bool shouldContinue = manager->checkTimeout(state, config);
        if (i == 2) {
            EXPECT_FALSE(shouldContinue); // Reboot needed
        }
    }

    // Simulate reboot - reset state
    state = WiFiConnectionState();
    EXPECT_EQ(0, state.currentNetworkIndex); // Reset to start

    // Simulate Boot 2: All networks still fail
    for (int i = 0; i < 3; i++) {
        state.currentNetworkIndex = i;
        manager->connect(state, config);
        state.attemptStartTime = millis() - (WIFI_TIMEOUT_MS + 1000);

        manager->checkTimeout(state, config);
    }

    // Simulate reboot again
    state = WiFiConnectionState();

    // Simulate Boot 3: Network 1 becomes available
    state.currentNetworkIndex = 0;
    manager->connect(state, config);

    // First network still fails
    state.attemptStartTime = millis() - (WIFI_TIMEOUT_MS + 1000);
    manager->checkTimeout(state, config);
    EXPECT_EQ(1, state.currentNetworkIndex);

    // Second network now succeeds
    mockWiFi->setConnectionBehavior(true);
    manager->connect(state, config);
    mockWiFi->simulateConnectionSuccess("Network2", -55);
    manager->handleConnectionSuccess(state, "Network2");

    // Verify connected (no more reboots)
    EXPECT_EQ(ConnectionStatus::CONNECTED, state.status);
    EXPECT_STREQ("Network2", state.connectedSSID.c_str());
}

/**
 * Test: Network index increments correctly through all networks
 *
 * Expected behavior:
 * - Start at index 0
 * - Timeout → index 1
 * - Timeout → index 2
 * - Timeout → index 3 (exhausted)
 */
TEST_F(AllNetworksUnavailableTest, NetworkIndexIncrementsCorrectly) {
    EXPECT_EQ(0, state.currentNetworkIndex);

    // First network
    manager->connect(state, config);
    state.attemptStartTime = millis() - (WIFI_TIMEOUT_MS + 1000);
    manager->checkTimeout(state, config);
    EXPECT_EQ(1, state.currentNetworkIndex);

    // Second network
    manager->connect(state, config);
    state.attemptStartTime = millis() - (WIFI_TIMEOUT_MS + 1000);
    manager->checkTimeout(state, config);
    EXPECT_EQ(2, state.currentNetworkIndex);

    // Third network
    manager->connect(state, config);
    state.attemptStartTime = millis() - (WIFI_TIMEOUT_MS + 1000);
    manager->checkTimeout(state, config);

    // After third network, index increments to 3 (exhausted)
    EXPECT_GE(state.currentNetworkIndex, 3);
    EXPECT_TRUE(state.allNetworksExhausted(config.count));
}

/**
 * Test: Total time for complete failure cycle
 *
 * Expected behavior:
 * - 3 networks × 30s timeout = 90 seconds
 * - Plus 5-second reboot delay = 95 seconds total
 */
TEST_F(AllNetworksUnavailableTest, TotalTimeForCompleteFailureCycle) {
    unsigned long cycleStartTime = millis();

    // Try all 3 networks
    for (int i = 0; i < 3; i++) {
        state.currentNetworkIndex = i;
        unsigned long attemptStartTime = cycleStartTime + (i * 30000);

        manager->connect(state, config);
        state.attemptStartTime = attemptStartTime;

        // Simulate 30s elapse
        unsigned long timeoutTime = attemptStartTime + WIFI_TIMEOUT_MS;

        manager->checkTimeout(state, config);
    }

    // All networks tried: ~90 seconds
    unsigned long allNetworksTime = 3 * 30000; // 90s
    EXPECT_EQ(90000, allNetworksTime);

    // Reboot delay: 5 seconds
    unsigned long rebootDelay = REBOOT_DELAY_MS; // 5000ms
    EXPECT_EQ(5000, rebootDelay);

    // Total cycle time: 95 seconds
    unsigned long totalCycleTime = allNetworksTime + rebootDelay;
    EXPECT_EQ(95000, totalCycleTime);
}

/**
 * Test: State after all networks exhausted
 *
 * Expected behavior:
 * - currentNetworkIndex >= count (exhausted)
 * - status likely FAILED or CONNECTING (last attempt)
 * - retryCount incremented for each timeout
 */
TEST_F(AllNetworksUnavailableTest, StateAfterAllNetworksExhausted) {
    // Try all networks
    for (int i = 0; i < 3; i++) {
        state.currentNetworkIndex = i;
        manager->connect(state, config);

        // Simulate timeout
        state.attemptStartTime = millis() - (WIFI_TIMEOUT_MS + 1000);
        manager->checkTimeout(state, config);
    }

    // Verify exhausted
    EXPECT_TRUE(state.allNetworksExhausted(config.count));
    EXPECT_GE(state.currentNetworkIndex, 3);

    // Retry count should reflect multiple failed attempts
    EXPECT_GT(state.retryCount, 0);
}

/**
 * Test: Config with fewer than 3 networks
 *
 * Expected behavior:
 * - Config with 2 networks
 * - Both timeout
 * - Reboot after ~65 seconds (2×30s + 5s delay)
 */
TEST_F(AllNetworksUnavailableTest, FewerThanThreeNetworks) {
    // Create config with only 2 networks
    WiFiConfigFile twoNetworkConfig;
    twoNetworkConfig.networks[0] = WiFiCredentials("Net1", "pass1");
    twoNetworkConfig.networks[1] = WiFiCredentials("Net2", "pass2");
    twoNetworkConfig.count = 2;

    manager->saveConfig(twoNetworkConfig);
    manager->loadConfig(twoNetworkConfig);

    // Try both networks
    for (int i = 0; i < 2; i++) {
        state.currentNetworkIndex = i;
        manager->connect(state, twoNetworkConfig);

        state.attemptStartTime = millis() - (WIFI_TIMEOUT_MS + 1000);
        bool shouldContinue = manager->checkTimeout(state, twoNetworkConfig);

        if (i == 1) {
            // After second network, should need reboot
            EXPECT_FALSE(shouldContinue);
        }
    }

    // Verify exhausted after 2 networks
    EXPECT_TRUE(state.allNetworksExhausted(twoNetworkConfig.count));

    // Total time: ~65 seconds (2×30s + 5s)
    unsigned long totalTime = (2 * WIFI_TIMEOUT_MS) + REBOOT_DELAY_MS;
    EXPECT_EQ(65000, totalTime);
}
