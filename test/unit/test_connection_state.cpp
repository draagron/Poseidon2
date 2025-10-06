/**
 * @file test_connection_state.cpp
 * @brief Unit tests for WiFiConnectionState enum and state transitions
 *
 * These tests validate the WiFiConnectionState state machine:
 * - State transitions: DISCONNECTED→CONNECTING→CONNECTED→FAILED
 * - State validation and allowed transitions
 * - Timestamp and retry tracking
 *
 * NOTE: These tests MUST FAIL INITIALLY (TDD approach) until
 * WiFiConnectionState struct is implemented in Phase 3.4.
 */

#include <gtest/gtest.h>
#include "../../src/components/WiFiConnectionState.h"

/**
 * Test fixture for WiFiConnectionState tests
 */
class WiFiConnectionStateTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};

/**
 * Test: Default constructor initializes to DISCONNECTED
 */
TEST_F(WiFiConnectionStateTest, DefaultConstructorInitializesDisconnected) {
    WiFiConnectionState state;

    EXPECT_EQ(state.status, ConnectionStatus::DISCONNECTED);
    EXPECT_EQ(state.currentNetworkIndex, 0);
    EXPECT_EQ(state.attemptStartTime, 0);
    EXPECT_EQ(state.connectedSSID, "");
    EXPECT_EQ(state.retryCount, 0);
}

/**
 * Test: Transition from DISCONNECTED to CONNECTING
 */
TEST_F(WiFiConnectionStateTest, TransitionDisconnectedToConnecting) {
    WiFiConnectionState state;
    state.status = ConnectionStatus::DISCONNECTED;

    bool result = state.transitionTo(ConnectionStatus::CONNECTING);

    EXPECT_TRUE(result);
    EXPECT_EQ(state.status, ConnectionStatus::CONNECTING);
}

/**
 * Test: Transition from CONNECTING to CONNECTED
 */
TEST_F(WiFiConnectionStateTest, TransitionConnectingToConnected) {
    WiFiConnectionState state;
    state.status = ConnectionStatus::CONNECTING;

    bool result = state.transitionTo(ConnectionStatus::CONNECTED);

    EXPECT_TRUE(result);
    EXPECT_EQ(state.status, ConnectionStatus::CONNECTED);
}

/**
 * Test: Transition from CONNECTING to FAILED (timeout)
 */
TEST_F(WiFiConnectionStateTest, TransitionConnectingToFailed) {
    WiFiConnectionState state;
    state.status = ConnectionStatus::CONNECTING;

    bool result = state.transitionTo(ConnectionStatus::FAILED);

    EXPECT_TRUE(result);
    EXPECT_EQ(state.status, ConnectionStatus::FAILED);
}

/**
 * Test: Transition from FAILED to CONNECTING (retry)
 */
TEST_F(WiFiConnectionStateTest, TransitionFailedToConnecting) {
    WiFiConnectionState state;
    state.status = ConnectionStatus::FAILED;

    bool result = state.transitionTo(ConnectionStatus::CONNECTING);

    EXPECT_TRUE(result);
    EXPECT_EQ(state.status, ConnectionStatus::CONNECTING);
}

/**
 * Test: Transition from CONNECTED to DISCONNECTED (connection lost)
 */
TEST_F(WiFiConnectionStateTest, TransitionConnectedToDisconnected) {
    WiFiConnectionState state;
    state.status = ConnectionStatus::CONNECTED;
    state.connectedSSID = "TestNetwork";

    bool result = state.transitionTo(ConnectionStatus::DISCONNECTED);

    EXPECT_TRUE(result);
    EXPECT_EQ(state.status, ConnectionStatus::DISCONNECTED);
}

/**
 * Test: Reject invalid transition (DISCONNECTED to CONNECTED)
 */
TEST_F(WiFiConnectionStateTest, RejectInvalidTransition) {
    WiFiConnectionState state;
    state.status = ConnectionStatus::DISCONNECTED;

    bool result = state.transitionTo(ConnectionStatus::CONNECTED);

    EXPECT_FALSE(result);
    EXPECT_EQ(state.status, ConnectionStatus::DISCONNECTED); // Unchanged
}

/**
 * Test: Record attempt start time when transitioning to CONNECTING
 */
TEST_F(WiFiConnectionStateTest, RecordAttemptStartTime) {
    WiFiConnectionState state;
    state.status = ConnectionStatus::DISCONNECTED;

    unsigned long beforeTime = millis();
    state.transitionTo(ConnectionStatus::CONNECTING);
    unsigned long afterTime = millis();

    EXPECT_GE(state.attemptStartTime, beforeTime);
    EXPECT_LE(state.attemptStartTime, afterTime);
}

/**
 * Test: Set connected SSID when transitioning to CONNECTED
 */
TEST_F(WiFiConnectionStateTest, SetConnectedSSIDOnSuccess) {
    WiFiConnectionState state;
    state.status = ConnectionStatus::CONNECTING;

    state.transitionTo(ConnectionStatus::CONNECTED, "HomeNetwork");

    EXPECT_EQ(state.status, ConnectionStatus::CONNECTED);
    EXPECT_EQ(state.connectedSSID, "HomeNetwork");
}

/**
 * Test: Clear connected SSID when disconnecting
 */
TEST_F(WiFiConnectionStateTest, ClearConnectedSSIDOnDisconnect) {
    WiFiConnectionState state;
    state.status = ConnectionStatus::CONNECTED;
    state.connectedSSID = "HomeNetwork";

    state.transitionTo(ConnectionStatus::DISCONNECTED);

    EXPECT_EQ(state.connectedSSID, "");
}

/**
 * Test: Increment retry count on failed attempts
 */
TEST_F(WiFiConnectionStateTest, IncrementRetryCountOnFailure) {
    WiFiConnectionState state;
    state.status = ConnectionStatus::CONNECTING;
    state.retryCount = 0;

    state.transitionTo(ConnectionStatus::FAILED);

    EXPECT_EQ(state.retryCount, 1);

    // Retry and fail again
    state.transitionTo(ConnectionStatus::CONNECTING);
    state.transitionTo(ConnectionStatus::FAILED);

    EXPECT_EQ(state.retryCount, 2);
}

/**
 * Test: Reset retry count on successful connection
 */
TEST_F(WiFiConnectionStateTest, ResetRetryCountOnSuccess) {
    WiFiConnectionState state;
    state.status = ConnectionStatus::CONNECTING;
    state.retryCount = 5;

    state.transitionTo(ConnectionStatus::CONNECTED, "TestNetwork");

    EXPECT_EQ(state.retryCount, 0);
}

/**
 * Test: Increment network index when moving to next network
 */
TEST_F(WiFiConnectionStateTest, IncrementNetworkIndex) {
    WiFiConnectionState state;
    state.currentNetworkIndex = 0;

    state.moveToNextNetwork();

    EXPECT_EQ(state.currentNetworkIndex, 1);

    state.moveToNextNetwork();

    EXPECT_EQ(state.currentNetworkIndex, 2);
}

/**
 * Test: Check if all networks exhausted (wrap to reboot)
 */
TEST_F(WiFiConnectionStateTest, CheckAllNetworksExhausted) {
    WiFiConnectionState state;
    state.currentNetworkIndex = 0;

    EXPECT_FALSE(state.allNetworksExhausted(3));

    state.moveToNextNetwork();
    EXPECT_FALSE(state.allNetworksExhausted(3));

    state.moveToNextNetwork();
    EXPECT_FALSE(state.allNetworksExhausted(3));

    state.moveToNextNetwork();
    EXPECT_TRUE(state.allNetworksExhausted(3)); // Index 3 >= count 3
}

/**
 * Test: Reset network index after reboot
 */
TEST_F(WiFiConnectionStateTest, ResetNetworkIndex) {
    WiFiConnectionState state;
    state.currentNetworkIndex = 2;

    state.resetNetworkIndex();

    EXPECT_EQ(state.currentNetworkIndex, 0);
}

/**
 * Test: Check if connection status is CONNECTED
 */
TEST_F(WiFiConnectionStateTest, CheckIsConnected) {
    WiFiConnectionState state;
    state.status = ConnectionStatus::DISCONNECTED;

    EXPECT_FALSE(state.isConnected());

    state.transitionTo(ConnectionStatus::CONNECTING);
    EXPECT_FALSE(state.isConnected());

    state.transitionTo(ConnectionStatus::CONNECTED, "TestNetwork");
    EXPECT_TRUE(state.isConnected());

    state.transitionTo(ConnectionStatus::DISCONNECTED);
    EXPECT_FALSE(state.isConnected());
}

/**
 * Test: Check if connection attempt is in progress
 */
TEST_F(WiFiConnectionStateTest, CheckIsConnecting) {
    WiFiConnectionState state;
    state.status = ConnectionStatus::DISCONNECTED;

    EXPECT_FALSE(state.isConnecting());

    state.transitionTo(ConnectionStatus::CONNECTING);
    EXPECT_TRUE(state.isConnecting());

    state.transitionTo(ConnectionStatus::CONNECTED, "TestNetwork");
    EXPECT_FALSE(state.isConnecting());
}

/**
 * Test: Calculate elapsed time since attempt start
 */
TEST_F(WiFiConnectionStateTest, CalculateElapsedTime) {
    WiFiConnectionState state;
    state.status = ConnectionStatus::DISCONNECTED;

    state.transitionTo(ConnectionStatus::CONNECTING);
    unsigned long startTime = state.attemptStartTime;

    // Simulate some time passing (in real test, would use mock millis)
    delay(100); // 100ms delay

    unsigned long elapsed = state.getElapsedTime();

    EXPECT_GE(elapsed, 100);
    EXPECT_LT(elapsed, 200); // Should be around 100ms
}

/**
 * Test: Check if timeout exceeded (30 seconds)
 */
TEST_F(WiFiConnectionStateTest, CheckTimeoutExceeded) {
    WiFiConnectionState state;
    state.status = ConnectionStatus::CONNECTING;
    state.attemptStartTime = millis();

    EXPECT_FALSE(state.isTimeoutExceeded(30000)); // Not exceeded yet

    // Simulate timeout by setting old start time
    state.attemptStartTime = millis() - 31000; // 31 seconds ago

    EXPECT_TRUE(state.isTimeoutExceeded(30000));
}

/**
 * Test: Get status as string for logging
 */
TEST_F(WiFiConnectionStateTest, GetStatusAsString) {
    WiFiConnectionState state;

    state.status = ConnectionStatus::DISCONNECTED;
    EXPECT_EQ(state.getStatusString(), "DISCONNECTED");

    state.status = ConnectionStatus::CONNECTING;
    EXPECT_EQ(state.getStatusString(), "CONNECTING");

    state.status = ConnectionStatus::CONNECTED;
    EXPECT_EQ(state.getStatusString(), "CONNECTED");

    state.status = ConnectionStatus::FAILED;
    EXPECT_EQ(state.getStatusString(), "FAILED");
}

/**
 * Test: Complete state transition sequence (boot to connected)
 */
TEST_F(WiFiConnectionStateTest, CompleteStateSequenceSuccess) {
    WiFiConnectionState state;

    // Initial state
    EXPECT_EQ(state.status, ConnectionStatus::DISCONNECTED);

    // Start connection attempt
    EXPECT_TRUE(state.transitionTo(ConnectionStatus::CONNECTING));
    EXPECT_EQ(state.status, ConnectionStatus::CONNECTING);
    EXPECT_GT(state.attemptStartTime, 0);

    // Connection successful
    EXPECT_TRUE(state.transitionTo(ConnectionStatus::CONNECTED, "HomeNetwork"));
    EXPECT_EQ(state.status, ConnectionStatus::CONNECTED);
    EXPECT_EQ(state.connectedSSID, "HomeNetwork");
    EXPECT_EQ(state.retryCount, 0);
}

/**
 * Test: Complete state transition sequence (timeout and retry)
 */
TEST_F(WiFiConnectionStateTest, CompleteStateSequenceTimeout) {
    WiFiConnectionState state;

    // Start connection attempt
    state.transitionTo(ConnectionStatus::CONNECTING);

    // Connection times out
    EXPECT_TRUE(state.transitionTo(ConnectionStatus::FAILED));
    EXPECT_EQ(state.status, ConnectionStatus::FAILED);
    EXPECT_EQ(state.retryCount, 1);

    // Move to next network
    state.moveToNextNetwork();
    EXPECT_EQ(state.currentNetworkIndex, 1);

    // Retry connection
    EXPECT_TRUE(state.transitionTo(ConnectionStatus::CONNECTING));
    EXPECT_EQ(state.status, ConnectionStatus::CONNECTING);
}

/**
 * Test: Connection loss and retry same network
 */
TEST_F(WiFiConnectionStateTest, ConnectionLossRetry) {
    WiFiConnectionState state;
    state.transitionTo(ConnectionStatus::CONNECTING);
    state.transitionTo(ConnectionStatus::CONNECTED, "HomeNetwork");
    state.currentNetworkIndex = 1;

    // Connection lost
    EXPECT_TRUE(state.transitionTo(ConnectionStatus::DISCONNECTED));
    EXPECT_EQ(state.status, ConnectionStatus::DISCONNECTED);
    EXPECT_EQ(state.currentNetworkIndex, 1); // Should NOT change

    // Retry (should retry same network, not move to next)
    EXPECT_TRUE(state.transitionTo(ConnectionStatus::CONNECTING));
    EXPECT_EQ(state.currentNetworkIndex, 1); // Still same network
}

/**
 * Test: Copy constructor
 */
TEST_F(WiFiConnectionStateTest, CopyConstructor) {
    WiFiConnectionState original;
    original.status = ConnectionStatus::CONNECTED;
    original.currentNetworkIndex = 2;
    original.connectedSSID = "TestSSID";
    original.retryCount = 5;

    WiFiConnectionState copy(original);

    EXPECT_EQ(copy.status, ConnectionStatus::CONNECTED);
    EXPECT_EQ(copy.currentNetworkIndex, 2);
    EXPECT_EQ(copy.connectedSSID, "TestSSID");
    EXPECT_EQ(copy.retryCount, 5);
}

/**
 * Test: Assignment operator
 */
TEST_F(WiFiConnectionStateTest, AssignmentOperator) {
    WiFiConnectionState state1;
    state1.status = ConnectionStatus::CONNECTED;
    state1.connectedSSID = "Network1";

    WiFiConnectionState state2;
    state2 = state1;

    EXPECT_EQ(state2.status, ConnectionStatus::CONNECTED);
    EXPECT_EQ(state2.connectedSSID, "Network1");
}
