/**
 * @file StartupProgressTracker.cpp
 * @brief Implementation of StartupProgressTracker component
 *
 * @version 1.0.0
 * @date 2025-10-09
 */

#include "StartupProgressTracker.h"
#include <string.h>

// Note: In production, we'd include Arduino.h for millis()
// For native tests, millis() is provided by mock
#ifdef ARDUINO
#include <Arduino.h>
#else
// Mock millis() for native tests
extern "C" unsigned long millis() {
    return 0;
}
#endif

StartupProgressTracker::StartupProgressTracker() {
    // Initialize with default startup states
    _status.wifiStatus = CONN_DISCONNECTED;
    _status.wifiSSID[0] = '\0';
    _status.wifiIPAddress[0] = '\0';
    _status.fsStatus = FS_MOUNTING;
    _status.webServerStatus = WS_STARTING;
    _status.wifiTimestamp = millis();
    _status.fsTimestamp = millis();
    _status.wsTimestamp = millis();
}

void StartupProgressTracker::updateWiFiStatus(DisplayConnectionStatus status, const char* ssid, const char* ip) {
    _status.wifiStatus = status;
    _status.wifiTimestamp = millis();

    // Update SSID if provided
    if (ssid != nullptr) {
        strncpy(_status.wifiSSID, ssid, sizeof(_status.wifiSSID) - 1);
        _status.wifiSSID[sizeof(_status.wifiSSID) - 1] = '\0';  // Ensure null termination
    }

    // Update IP if provided
    if (ip != nullptr) {
        strncpy(_status.wifiIPAddress, ip, sizeof(_status.wifiIPAddress) - 1);
        _status.wifiIPAddress[sizeof(_status.wifiIPAddress) - 1] = '\0';  // Ensure null termination
    }

    // Clear SSID and IP on disconnect
    if (status == CONN_DISCONNECTED || status == CONN_FAILED) {
        _status.wifiSSID[0] = '\0';
        _status.wifiIPAddress[0] = '\0';
    }
}

void StartupProgressTracker::updateFilesystemStatus(FilesystemStatus status) {
    _status.fsStatus = status;
    _status.fsTimestamp = millis();
}

void StartupProgressTracker::updateWebServerStatus(WebServerStatus status) {
    _status.webServerStatus = status;
    _status.wsTimestamp = millis();
}

const SubsystemStatus& StartupProgressTracker::getStatus() const {
    return _status;
}
