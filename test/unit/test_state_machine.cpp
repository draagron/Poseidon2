/**
 * @file test_state_machine.cpp
 * @brief Unit tests for ConnectionStateMachine component
 *
 * These tests validate the connection state machine logic:
 * - State transitions (DISCONNECTED→CONNECTING→CONNECTED→FAILED)
 * - Timeout detection (30 seconds)
 * - Network failover (move to next network on timeout)
 * - Reboot scheduling (all networks failed)
 *
 * NOTE: These tests MUST FAIL INITIALLY (TDD approach) until
 * ConnectionStateMachine is implemented in Phase 3.8.
 */

#include <gtest/gtest.h>
#include "../../src/components/ConnectionStateMachine.h"
#include "../../src/components/WiFiConnectionState.h"
#include "../../src/components/WiFiConfigFile.h"
#include "../../src/config.h"

/**
 * Test fixture for ConnectionStateMachine tests
 */
class ConnectionStateMachineTest : public ::testing::Test {
protected:
    ConnectionStateMachine* stateMachine;
    WiFiConnectionState state;
    WiFiConfigFile config;

    void SetUp() override {
        stateMachine = new ConnectionStateMachine();
        state = WiFiConnectionState();
        config = WiFiConfigFile();

        // Add test networks
        config.addNetwork(WiFiCredentials("Network1", "password1"));
        config.addNetwork(WiFiCredentials("Network2", "password2"));
        config.addNetwork(WiFiCredentials("Network3", "password3"));
    }

    void TearDown() override {
        delete stateMachine;
    }
};

/**
 * Test: Initial state is DISCONNECTED
 */
TEST_F(ConnectionStateMachineTest, InitialStateIsDisconnected) {
    EXPECT_EQ(state.status, ConnectionStatus::DISCONNECTED);
    EXPECT_EQ(state.currentNetworkIndex, 0);
    EXPECT_EQ(state.retryCount, 0);
}

/**
 * Test: Transition from DISCONNECTED to CONNECTING on begin()
 */
TEST_F(ConnectionStateMachineTest, TransitionToConnectingOnBegin) {
    stateMachine->transition(state, ConnectionStatus::CONNECTING);

    EXPECT_EQ(state.status, ConnectionStatus::CONNECTING);
    EXPECT_GT(state.attemptStartTime, 0); // Timestamp should be set
}

/**
 * Test: Transition from CONNECTING to CONNECTED on success
 */
TEST_F(ConnectionStateMachineTest, TransitionToConnectedOnSuccess) {
    state.status = ConnectionStatus::CONNECTING;

    stateMachine->transition(state, ConnectionStatus::CONNECTED, "Network1");

    EXPECT_EQ(state.status, ConnectionStatus::CONNECTED);
    EXPECT_EQ(state.connectedSSID, "Network1");
    EXPECT_EQ(state.retryCount, 0); // Retry count reset on success
}

/**
 * Test: Transition from CONNECTING to FAILED on timeout
 */
TEST_F(ConnectionStateMachineTest, TransitionToFailedOnTimeout) {
    state.status = ConnectionStatus::CONNECTING;
    state.attemptStartTime = millis() - 31000; // 31 seconds ago

    bool timedOut = stateMachine->shouldRetry(state, WIFI_TIMEOUT_MS);

    EXPECT_TRUE(timedOut);

    stateMachine->transition(state, ConnectionStatus::FAILED);

    EXPECT_EQ(state.status, ConnectionStatus::FAILED);
    EXPECT_EQ(state.retryCount, 1);
}

/**
 * Test: shouldRetry() returns false when within timeout window
 */
TEST_F(ConnectionStateMachineTest, ShouldRetryWithinTimeout) {
    state.status = ConnectionStatus::CONNECTING;
    state.attemptStartTime = millis(); // Just started

    bool timedOut = stateMachine->shouldRetry(state, WIFI_TIMEOUT_MS);

    EXPECT_FALSE(timedOut);
}

/**
 * Test: shouldRetry() returns true when timeout exceeded
 */
TEST_F(ConnectionStateMachineTest, ShouldRetryAfterTimeout) {
    state.status = ConnectionStatus::CONNECTING;
    state.attemptStartTime = millis() - (WIFI_TIMEOUT_MS + 1000); // Exceeded

    bool timedOut = stateMachine->shouldRetry(state, WIFI_TIMEOUT_MS);

    EXPECT_TRUE(timedOut);
}

/**
 * Test: Move to next network after failure
 */
TEST_F(ConnectionStateMachineTest, MoveToNextNetworkAfterFailure) {
    state.currentNetworkIndex = 0;
    state.status = ConnectionStatus::FAILED;

    WiFiCredentials* nextNetwork = stateMachine->getNextNetwork(state, config);

    EXPECT_NE(nextNetwork, nullptr);
    EXPECT_EQ(state.currentNetworkIndex, 1);
    EXPECT_EQ(nextNetwork->ssid, "Network2");
}

/**
 * Test: Return nullptr when all networks exhausted
 */
TEST_F(ConnectionStateMachineTest, ReturnNullWhenAllNetworksExhausted) {
    state.currentNetworkIndex = 2; // On last network (index 2)
    state.status = ConnectionStatus::FAILED;

    WiFiCredentials* nextNetwork = stateMachine->getNextNetwork(state, config);

    EXPECT_NE(nextNetwork, nullptr); // Still return Network3
    EXPECT_EQ(nextNetwork->ssid, "Network3");

    // Try to move beyond last network
    state.status = ConnectionStatus::FAILED;
    nextNetwork = stateMachine->getNextNetwork(state, config);

    EXPECT_EQ(nextNetwork, nullptr); // All networks exhausted
    EXPECT_EQ(state.currentNetworkIndex, 3); // Index >= count
}

/**
 * Test: Check if all networks exhausted
 */
TEST_F(ConnectionStateMachineTest, CheckAllNetworksExhausted) {
    state.currentNetworkIndex = 0;
    EXPECT_FALSE(state.allNetworksExhausted(config.count));

    state.currentNetworkIndex = 1;
    EXPECT_FALSE(state.allNetworksExhausted(config.count));

    state.currentNetworkIndex = 2;
    EXPECT_FALSE(state.allNetworksExhausted(config.count));

    state.currentNetworkIndex = 3;
    EXPECT_TRUE(state.allNetworksExhausted(config.count));
}

/**
 * Test: Reset network index after reboot
 */
TEST_F(ConnectionStateMachineTest, ResetNetworkIndexAfterReboot) {
    state.currentNetworkIndex = 3;

    stateMachine->resetForReboot(state);

    EXPECT_EQ(state.currentNetworkIndex, 0);
    EXPECT_EQ(state.status, ConnectionStatus::DISCONNECTED);
}

/**
 * Test: Complete sequence - first network success
 */
TEST_F(ConnectionStateMachineTest, CompleteSequenceFirstNetworkSuccess) {
    // Start connection attempt
    stateMachine->transition(state, ConnectionStatus::CONNECTING);
    EXPECT_EQ(state.status, ConnectionStatus::CONNECTING);
    EXPECT_EQ(state.currentNetworkIndex, 0);

    // Connection succeeds
    stateMachine->transition(state, ConnectionStatus::CONNECTED, "Network1");
    EXPECT_EQ(state.status, ConnectionStatus::CONNECTED);
    EXPECT_EQ(state.connectedSSID, "Network1");
}

/**
 * Test: Complete sequence - first fails, second succeeds
 */
TEST_F(ConnectionStateMachineTest, CompleteSequenceFirstFailsSecondSucceeds) {
    // Try first network
    stateMachine->transition(state, ConnectionStatus::CONNECTING);
    state.attemptStartTime = millis() - 31000; // Simulate timeout

    // Check timeout
    EXPECT_TRUE(stateMachine->shouldRetry(state, WIFI_TIMEOUT_MS));

    // Mark as failed
    stateMachine->transition(state, ConnectionStatus::FAILED);
    EXPECT_EQ(state.retryCount, 1);

    // Move to next network
    WiFiCredentials* nextNetwork = stateMachine->getNextNetwork(state, config);
    EXPECT_EQ(state.currentNetworkIndex, 1);
    EXPECT_EQ(nextNetwork->ssid, "Network2");

    // Try second network
    stateMachine->transition(state, ConnectionStatus::CONNECTING);

    // Connection succeeds
    stateMachine->transition(state, ConnectionStatus::CONNECTED, "Network2");
    EXPECT_EQ(state.status, ConnectionStatus::CONNECTED);
    EXPECT_EQ(state.connectedSSID, "Network2");
    EXPECT_EQ(state.retryCount, 0); // Reset on success
}

/**
 * Test: Complete sequence - all networks fail, reboot
 */
TEST_F(ConnectionStateMachineTest, CompleteSequenceAllNetworksFailReboot) {
    // Try network 1
    stateMachine->transition(state, ConnectionStatus::CONNECTING);
    state.attemptStartTime = millis() - 31000;
    stateMachine->transition(state, ConnectionStatus::FAILED);

    // Move to network 2
    stateMachine->getNextNetwork(state, config);
    stateMachine->transition(state, ConnectionStatus::CONNECTING);
    state.attemptStartTime = millis() - 31000;
    stateMachine->transition(state, ConnectionStatus::FAILED);

    // Move to network 3
    stateMachine->getNextNetwork(state, config);
    stateMachine->transition(state, ConnectionStatus::CONNECTING);
    state.attemptStartTime = millis() - 31000;
    stateMachine->transition(state, ConnectionStatus::FAILED);

    // Try to get next network - should return nullptr
    WiFiCredentials* nextNetwork = stateMachine->getNextNetwork(state, config);
    EXPECT_EQ(nextNetwork, nullptr);

    // All networks exhausted
    EXPECT_TRUE(state.allNetworksExhausted(config.count));

    // Should trigger reboot
    bool shouldReboot = stateMachine->shouldReboot(state, config);
    EXPECT_TRUE(shouldReboot);
}

/**
 * Test: Connection loss - retry same network
 */
TEST_F(ConnectionStateMachineTest, ConnectionLossRetrySameNetwork) {
    // Connect to network 2
    state.currentNetworkIndex = 1;
    stateMachine->transition(state, ConnectionStatus::CONNECTING);
    stateMachine->transition(state, ConnectionStatus::CONNECTED, "Network2");

    // Connection lost
    stateMachine->transition(state, ConnectionStatus::DISCONNECTED);
    EXPECT_EQ(state.status, ConnectionStatus::DISCONNECTED);
    EXPECT_EQ(state.currentNetworkIndex, 1); // Should NOT change

    // Retry - should stay on same network
    stateMachine->transition(state, ConnectionStatus::CONNECTING);
    EXPECT_EQ(state.currentNetworkIndex, 1); // Still on Network2
}

/**
 * Test: shouldReboot() returns false when networks available
 */
TEST_F(ConnectionStateMachineTest, ShouldRebootFalseWhenNetworksAvailable) {
    state.currentNetworkIndex = 0;

    bool shouldReboot = stateMachine->shouldReboot(state, config);

    EXPECT_FALSE(shouldReboot);
}

/**
 * Test: shouldReboot() returns true when all networks exhausted
 */
TEST_F(ConnectionStateMachineTest, ShouldRebootTrueWhenAllExhausted) {
    state.currentNetworkIndex = 3; // Beyond last network

    bool shouldReboot = stateMachine->shouldReboot(state, config);

    EXPECT_TRUE(shouldReboot);
}

/**
 * Test: Validate state transition logic
 */
TEST_F(ConnectionStateMachineTest, ValidateStateTransitions) {
    // DISCONNECTED → CONNECTING
    EXPECT_TRUE(stateMachine->transition(state, ConnectionStatus::CONNECTING));
    EXPECT_EQ(state.status, ConnectionStatus::CONNECTING);

    // CONNECTING → CONNECTED
    EXPECT_TRUE(stateMachine->transition(state, ConnectionStatus::CONNECTED, "Test"));
    EXPECT_EQ(state.status, ConnectionStatus::CONNECTED);

    // CONNECTED → DISCONNECTED
    EXPECT_TRUE(stateMachine->transition(state, ConnectionStatus::DISCONNECTED));
    EXPECT_EQ(state.status, ConnectionStatus::DISCONNECTED);

    // DISCONNECTED → CONNECTING
    EXPECT_TRUE(stateMachine->transition(state, ConnectionStatus::CONNECTING));

    // CONNECTING → FAILED
    EXPECT_TRUE(stateMachine->transition(state, ConnectionStatus::FAILED));
    EXPECT_EQ(state.status, ConnectionStatus::FAILED);

    // FAILED → CONNECTING
    EXPECT_TRUE(stateMachine->transition(state, ConnectionStatus::CONNECTING));
    EXPECT_EQ(state.status, ConnectionStatus::CONNECTING);
}

/**
 * Test: Reject invalid state transitions
 */
TEST_F(ConnectionStateMachineTest, RejectInvalidTransitions) {
    state.status = ConnectionStatus::DISCONNECTED;

    // DISCONNECTED → CONNECTED (invalid - must go through CONNECTING)
    bool result = stateMachine->transition(state, ConnectionStatus::CONNECTED, "Test");

    EXPECT_FALSE(result);
    EXPECT_EQ(state.status, ConnectionStatus::DISCONNECTED); // Unchanged
}

/**
 * Test: Get current network credentials
 */
TEST_F(ConnectionStateMachineTest, GetCurrentNetwork) {
    state.currentNetworkIndex = 1;

    WiFiCredentials* current = stateMachine->getCurrentNetwork(state, config);

    EXPECT_NE(current, nullptr);
    EXPECT_EQ(current->ssid, "Network2");
}

/**
 * Test: Get current network returns nullptr for invalid index
 */
TEST_F(ConnectionStateMachineTest, GetCurrentNetworkInvalidIndex) {
    state.currentNetworkIndex = 10; // Out of bounds

    WiFiCredentials* current = stateMachine->getCurrentNetwork(state, config);

    EXPECT_EQ(current, nullptr);
}

/**
 * Test: Elapsed time calculation
 */
TEST_F(ConnectionStateMachineTest, ElapsedTimeCalculation) {
    state.attemptStartTime = millis() - 5000; // 5 seconds ago

    unsigned long elapsed = state.getElapsedTime();

    EXPECT_GE(elapsed, 5000);
    EXPECT_LT(elapsed, 6000); // Should be around 5000ms
}

/**
 * Test: Timeout detection with custom timeout value
 */
TEST_F(ConnectionStateMachineTest, TimeoutDetectionCustomValue) {
    state.attemptStartTime = millis() - 10000; // 10 seconds ago

    // 5 second timeout
    EXPECT_TRUE(stateMachine->shouldRetry(state, 5000));

    // 15 second timeout
    EXPECT_FALSE(stateMachine->shouldRetry(state, 15000));
}

/**
 * Test: State machine handles empty config
 */
TEST_F(ConnectionStateMachineTest, HandleEmptyConfig) {
    WiFiConfigFile emptyConfig;

    WiFiCredentials* network = stateMachine->getNextNetwork(state, emptyConfig);

    EXPECT_EQ(network, nullptr);
}

/**
 * Test: State machine handles single network config
 */
TEST_F(ConnectionStateMachineTest, HandleSingleNetworkConfig) {
    WiFiConfigFile singleConfig;
    singleConfig.addNetwork(WiFiCredentials("OnlyNetwork", "password"));

    WiFiCredentials* network = stateMachine->getCurrentNetwork(state, singleConfig);

    EXPECT_NE(network, nullptr);
    EXPECT_EQ(network->ssid, "OnlyNetwork");
}

/**
 * Test: Retry count increments correctly
 */
TEST_F(ConnectionStateMachineTest, RetryCountIncrement) {
    EXPECT_EQ(state.retryCount, 0);

    stateMachine->transition(state, ConnectionStatus::CONNECTING);
    stateMachine->transition(state, ConnectionStatus::FAILED);
    EXPECT_EQ(state.retryCount, 1);

    stateMachine->transition(state, ConnectionStatus::CONNECTING);
    stateMachine->transition(state, ConnectionStatus::FAILED);
    EXPECT_EQ(state.retryCount, 2);

    stateMachine->transition(state, ConnectionStatus::CONNECTING);
    stateMachine->transition(state, ConnectionStatus::FAILED);
    EXPECT_EQ(state.retryCount, 3);
}

/**
 * Test: Retry count resets on successful connection
 */
TEST_F(ConnectionStateMachineTest, RetryCountResetOnSuccess) {
    state.retryCount = 5;

    stateMachine->transition(state, ConnectionStatus::CONNECTING);
    stateMachine->transition(state, ConnectionStatus::CONNECTED, "Network1");

    EXPECT_EQ(state.retryCount, 0);
}
