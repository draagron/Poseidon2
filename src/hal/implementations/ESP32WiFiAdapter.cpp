/**
 * @file ESP32WiFiAdapter.cpp
 * @brief Implementation of ESP32 WiFi adapter
 */

#include "ESP32WiFiAdapter.h"

// Static instance for event handler
ESP32WiFiAdapter* ESP32WiFiAdapter::instance = nullptr;

ESP32WiFiAdapter::ESP32WiFiAdapter() : eventCallback(nullptr) {
    // Set static instance for event handler
    instance = this;

    // Set WiFi mode to station (client)
    WiFi.mode(WIFI_STA);

    // Register ESP32 WiFi event handler
    WiFi.onEvent(WiFiEventHandler);
}

bool ESP32WiFiAdapter::begin(const char* ssid, const char* password) {
    if (ssid == nullptr || strlen(ssid) == 0) {
        return false;
    }

    // Disconnect if already connected
    if (WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect();
        delay(100); // Small delay for clean disconnect
    }

    // Begin connection attempt
    if (password == nullptr || strlen(password) == 0) {
        // Open network
        WiFi.begin(ssid);
    } else {
        // WPA2 network
        WiFi.begin(ssid, password);
    }

    return true;
}

WiFiStatus ESP32WiFiAdapter::status() {
    return convertStatus(WiFi.status());
}

bool ESP32WiFiAdapter::disconnect() {
    if (WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect();
        return true;
    }
    return false;
}

void ESP32WiFiAdapter::onEvent(WiFiEventCallback callback) {
    eventCallback = callback;
}

String ESP32WiFiAdapter::getIPAddress() {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.localIP().toString();
    }
    return "";
}

int ESP32WiFiAdapter::getRSSI() {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.RSSI();
    }
    return 0;
}

String ESP32WiFiAdapter::getSSID() {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.SSID();
    }
    return "";
}

WiFiStatus ESP32WiFiAdapter::convertStatus(wl_status_t wifiStatus) {
    switch (wifiStatus) {
        case WL_IDLE_STATUS:
            return WiFiStatus::IDLE;
        case WL_NO_SSID_AVAIL:
            return WiFiStatus::NO_SSID_AVAIL;
        case WL_CONNECTED:
            return WiFiStatus::CONNECTED;
        case WL_CONNECT_FAILED:
            return WiFiStatus::CONNECT_FAILED;
        case WL_DISCONNECTED:
            return WiFiStatus::DISCONNECTED;
        default:
            return WiFiStatus::DISCONNECTED;
    }
}

void ESP32WiFiAdapter::WiFiEventHandler(WiFiEvent_t event, WiFiEventInfo_t info) {
    if (instance == nullptr || instance->eventCallback == nullptr) {
        return;
    }

    // Convert ESP32 WiFi events to our event types and invoke callback
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_START:
            instance->eventCallback(WiFiEventType::WIFI_EVENT_STA_START);
            break;

        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            instance->eventCallback(WiFiEventType::WIFI_EVENT_STA_CONNECTED);
            break;

        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            instance->eventCallback(WiFiEventType::WIFI_EVENT_STA_DISCONNECTED);
            break;

        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            instance->eventCallback(WiFiEventType::WIFI_EVENT_STA_GOT_IP);
            break;

        default:
            // Ignore other events
            break;
    }
}
