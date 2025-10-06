/**
 * @file IWiFiAdapter.h
 * @brief Hardware abstraction interface for WiFi connectivity
 *
 * This interface abstracts WiFi hardware operations to enable testing
 * without physical WiFi hardware. Implementations must provide methods
 * for connection management and event handling.
 *
 * Usage:
 * @code
 * IWiFiAdapter* wifi = new ESP32WiFiAdapter();
 * wifi->begin("MyNetwork", "password123");
 * wifi->onEvent([](WiFiEvent_t event) {
 *   // Handle WiFi events
 * });
 * @endcode
 */

#ifndef I_WIFI_ADAPTER_H
#define I_WIFI_ADAPTER_H

#include <Arduino.h>

/**
 * @brief WiFi connection status enumeration
 */
enum class WiFiStatus {
    IDLE = 0,           ///< WiFi is idle (not initialized)
    NO_SSID_AVAIL = 1,  ///< Network SSID cannot be reached
    CONNECTED = 3,      ///< Successfully connected to network
    CONNECT_FAILED = 4, ///< Connection failed (wrong password/auth)
    DISCONNECTED = 6    ///< Disconnected from network
};

/**
 * @brief WiFi event types (renamed to avoid conflict with ESP32 WiFi library)
 */
enum class WiFiEventType {
    WIFI_EVENT_STA_START = 0,
    WIFI_EVENT_STA_CONNECTED = 4,
    WIFI_EVENT_STA_DISCONNECTED = 5,
    WIFI_EVENT_STA_GOT_IP = 7
};

/**
 * @brief WiFi event callback function type
 */
typedef void (*WiFiEventCallback)(WiFiEventType event);

/**
 * @brief Abstract interface for WiFi hardware operations
 *
 * All WiFi hardware interactions must go through this interface to
 * maintain testability and hardware abstraction (HAL Principle I).
 */
class IWiFiAdapter {
public:
    virtual ~IWiFiAdapter() {}

    /**
     * @brief Initialize WiFi connection attempt
     * @param ssid Network SSID (1-32 characters)
     * @param password WPA2 password (0 or 8-63 characters)
     * @return true if connection attempt started successfully
     */
    virtual bool begin(const char* ssid, const char* password) = 0;

    /**
     * @brief Get current WiFi connection status
     * @return Current WiFi status
     */
    virtual WiFiStatus status() = 0;

    /**
     * @brief Disconnect from current network
     * @return true if disconnect successful
     */
    virtual bool disconnect() = 0;

    /**
     * @brief Register callback for WiFi events
     * @param callback Function to call on WiFi events
     */
    virtual void onEvent(WiFiEventCallback callback) = 0;

    /**
     * @brief Get IP address of connected network
     * @return IP address as string (e.g., "192.168.1.100")
     */
    virtual String getIPAddress() = 0;

    /**
     * @brief Get RSSI (signal strength) of connected network
     * @return Signal strength in dBm (e.g., -45)
     */
    virtual int getRSSI() = 0;

    /**
     * @brief Get SSID of currently connected network
     * @return SSID string (empty if not connected)
     */
    virtual String getSSID() = 0;
};

#endif // I_WIFI_ADAPTER_H
