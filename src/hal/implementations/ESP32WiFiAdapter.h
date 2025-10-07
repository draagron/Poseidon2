/**
 * @file ESP32WiFiAdapter.h
 * @brief ESP32-specific WiFi adapter implementation
 *
 * Wraps the Arduino WiFi.h library to implement IWiFiAdapter interface.
 * Provides actual WiFi hardware functionality for ESP32.
 */

#ifndef ESP32_WIFI_ADAPTER_H
#define ESP32_WIFI_ADAPTER_H

#include <WiFi.h>
#include "../../hal/interfaces/IWiFiAdapter.h"

/**
 * @brief ESP32 WiFi adapter implementation
 *
 * Real hardware implementation wrapping ESP32 WiFi library.
 * Used in production (vs MockWiFiAdapter for testing).
 */
class ESP32WiFiAdapter : public IWiFiAdapter {
private:
    WiFiEventCallback eventCallback;

public:
    /**
     * @brief Constructor
     */
    ESP32WiFiAdapter();

    // IWiFiAdapter interface implementation
    bool begin(const char* ssid, const char* password) override;
    WiFiStatus status() override;
    bool disconnect() override;
    void onEvent(WiFiEventCallback callback) override;
    String getIPAddress() override;
    int getRSSI() override;
    String getSSID() override;

private:
    /**
     * @brief Convert Arduino WiFi status to our WiFiStatus enum
     * @param wifiStatus Arduino wl_status_t
     * @return Our WiFiStatus enum value
     */
    WiFiStatus convertStatus(wl_status_t wifiStatus);

    /**
     * @brief Static WiFi event handler for ESP32 WiFi events
     * @param event WiFi event type
     * @param info Event information
     */
    static void WiFiEventHandler(WiFiEvent_t event, WiFiEventInfo_t info);

    /**
     * @brief Instance pointer for static event handler
     */
    static ESP32WiFiAdapter* instance;
};

#endif // ESP32_WIFI_ADAPTER_H
