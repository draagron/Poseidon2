/**
 * @file ConnectionStateMachine.cpp
 * @brief Implementation of connection state machine
 */

#include "ConnectionStateMachine.h"

ConnectionStateMachine::ConnectionStateMachine() {
}

bool ConnectionStateMachine::transition(WiFiConnectionState& state, ConnectionStatus newStatus, const String& ssid) {
    // Use the state's own transitionTo method which validates transitions
    return state.transitionTo(newStatus, ssid);
}

bool ConnectionStateMachine::shouldRetry(const WiFiConnectionState& state, unsigned long timeoutMs) const {
    // Check if timeout has been exceeded
    return state.isTimeoutExceeded(timeoutMs);
}

WiFiCredentials* ConnectionStateMachine::getNextNetwork(WiFiConnectionState& state, WiFiConfigFile& config) {
    // Move to next network
    state.moveToNextNetwork();

    // Check if we've exhausted all networks
    if (state.allNetworksExhausted(config.count)) {
        return nullptr;
    }

    // Return the current network credentials
    return config.getNetwork(state.currentNetworkIndex);
}

WiFiCredentials* ConnectionStateMachine::getCurrentNetwork(const WiFiConnectionState& state, WiFiConfigFile& config) const {
    return config.getNetwork(state.currentNetworkIndex);
}

bool ConnectionStateMachine::shouldReboot(const WiFiConnectionState& state, const WiFiConfigFile& config) const {
    return state.allNetworksExhausted(config.count);
}

void ConnectionStateMachine::resetForReboot(WiFiConnectionState& state) {
    state.resetNetworkIndex();
    state.transitionTo(ConnectionStatus::DISCONNECTED);
}
