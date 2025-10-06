/**
 * @file MockWiFiAdapter.cpp
 * @brief Implementation of mock WiFi adapter for unit testing
 */

#include "MockWiFiAdapter.h"

MockWiFiAdapter::MockWiFiAdapter(bool shouldSucceed, unsigned long delay)
    : currentStatus(WiFiStatus::IDLE),
      eventCallback(nullptr),
      currentSSID(""),
      currentPassword(""),
      ipAddress(""),
      rssi(0),
      shouldSucceed(shouldSucceed),
      connectionDelay(delay) {
}

bool MockWiFiAdapter::begin(const char* ssid, const char* password) {
    if (ssid == nullptr || strlen(ssid) == 0) {
        return false;
    }

    currentSSID = String(ssid);
    currentPassword = String(password);
    currentStatus = WiFiStatus::IDLE;

    // Simulate connection attempt
    if (shouldSucceed) {
        simulateConnectionSuccess();
    } else {
        simulateConnectionFailure();
    }

    return true;
}

WiFiStatus MockWiFiAdapter::status() {
    return currentStatus;
}

bool MockWiFiAdapter::disconnect() {
    if (currentStatus == WiFiStatus::CONNECTED) {
        simulateDisconnect();
        return true;
    }
    return false;
}

void MockWiFiAdapter::onEvent(WiFiEventCallback callback) {
    eventCallback = callback;
}

String MockWiFiAdapter::getIPAddress() {
    return ipAddress;
}

int MockWiFiAdapter::getRSSI() {
    return rssi;
}

String MockWiFiAdapter::getSSID() {
    return currentSSID;
}

void MockWiFiAdapter::simulateConnectionSuccess(const char* ip, int signalStrength) {
    currentStatus = WiFiStatus::CONNECTED;
    ipAddress = String(ip);
    rssi = signalStrength;

    // Trigger connected event if callback registered
    if (eventCallback != nullptr) {
        eventCallback(WiFiEventType::WIFI_EVENT_STA_CONNECTED);
        eventCallback(WiFiEventType::WIFI_EVENT_STA_GOT_IP);
    }
}

void MockWiFiAdapter::simulateConnectionFailure() {
    currentStatus = WiFiStatus::CONNECT_FAILED;
    ipAddress = "";
    rssi = 0;

    // No event triggered for failure - timeout detected by manager
}

void MockWiFiAdapter::simulateDisconnect() {
    currentStatus = WiFiStatus::DISCONNECTED;
    ipAddress = "";
    rssi = 0;

    // Trigger disconnect event if callback registered
    if (eventCallback != nullptr) {
        eventCallback(WiFiEventType::WIFI_EVENT_STA_DISCONNECTED);
    }
}

void MockWiFiAdapter::setConnectionBehavior(bool succeed) {
    shouldSucceed = succeed;
}

void MockWiFiAdapter::reset() {
    currentStatus = WiFiStatus::IDLE;
    currentSSID = "";
    currentPassword = "";
    ipAddress = "";
    rssi = 0;
    shouldSucceed = true;
    connectionDelay = 0;
}
