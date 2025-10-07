/**
 * @file MockWiFiAdapter.h
 * @brief Mock WiFi adapter for unit testing
 *
 * This mock implementation simulates WiFi connection states without
 * requiring actual WiFi hardware. Allows testing of WiFi manager
 * logic in native test environment.
 */

#ifndef MOCK_WIFI_ADAPTER_H
#define MOCK_WIFI_ADAPTER_H

#include "../hal/interfaces/IWiFiAdapter.h"

/**
 * @brief Mock WiFi adapter for testing
 *
 * Simulates WiFi connection behavior with configurable outcomes.
 * Can simulate successful connections, timeouts, and authentication failures.
 */
class MockWiFiAdapter : public IWiFiAdapter {
private:
    WiFiStatus currentStatus;
    WiFiEventCallback eventCallback;
    String currentSSID;
    String currentPassword;
    String ipAddress;
    int rssi;
    bool shouldSucceed;
    unsigned long connectionDelay; // Simulated connection time in ms

public:
    /**
     * @brief Constructor
     * @param shouldSucceed If true, simulates successful connections
     * @param delay Simulated connection delay in milliseconds
     */
    MockWiFiAdapter(bool shouldSucceed = true, unsigned long delay = 0);

    // IWiFiAdapter interface implementation
    bool begin(const char* ssid, const char* password) override;
    WiFiStatus status() override;
    bool disconnect() override;
    void onEvent(WiFiEventCallback callback) override;
    String getIPAddress() override;
    int getRSSI() override;
    String getSSID() override;

    // Test helper methods
    /**
     * @brief Simulate WiFi connection success
     * @param ip IP address to return after connection
     * @param signalStrength RSSI value in dBm
     */
    void simulateConnectionSuccess(const char* ip = "192.168.1.100", int signalStrength = -45);

    /**
     * @brief Simulate WiFi connection failure
     */
    void simulateConnectionFailure();

    /**
     * @brief Simulate WiFi disconnection event
     */
    void simulateDisconnect();

    /**
     * @brief Set whether future connections should succeed
     * @param succeed If true, connections will succeed
     */
    void setConnectionBehavior(bool succeed);

    /**
     * @brief Reset mock to initial state
     */
    void reset();
};

#endif // MOCK_WIFI_ADAPTER_H
