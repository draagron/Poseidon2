/**
 * @file StartupProgressTracker.h
 * @brief Component to track and display subsystem initialization progress
 *
 * Tracks the initialization status of WiFi, filesystem, and web server
 * subsystems during boot. Used by DisplayManager to render startup progress.
 *
 * Requirements: FR-001 to FR-006 (startup progress display)
 *
 * @version 1.0.0
 * @date 2025-10-09
 */

#ifndef STARTUP_PROGRESS_TRACKER_H
#define STARTUP_PROGRESS_TRACKER_H

#include "types/DisplayTypes.h"

/**
 * @brief Tracks subsystem initialization progress
 *
 * Maintains current status and timestamps for WiFi, filesystem,
 * and web server subsystems during boot sequence.
 */
class StartupProgressTracker {
private:
    SubsystemStatus _status;  ///< Current subsystem status

public:
    /**
     * @brief Constructor - initializes with default startup states
     *
     * Initial states:
     * - WiFi: CONN_DISCONNECTED (not yet attempted)
     * - Filesystem: FS_MOUNTING (mount in progress)
     * - WebServer: WS_STARTING (initialization in progress)
     */
    StartupProgressTracker();

    /**
     * @brief Update WiFi connection status
     *
     * Updates WiFi status and timestamp. Optionally updates SSID and IP.
     *
     * @param status New WiFi connection status
     * @param ssid SSID of connected network (optional, nullptr to skip)
     * @param ip IP address as string (optional, nullptr to skip)
     */
    void updateWiFiStatus(DisplayConnectionStatus status, const char* ssid = nullptr, const char* ip = nullptr);

    /**
     * @brief Update filesystem mount status
     *
     * Updates filesystem status and timestamp.
     *
     * @param status New filesystem status
     */
    void updateFilesystemStatus(FilesystemStatus status);

    /**
     * @brief Update web server initialization status
     *
     * Updates web server status and timestamp.
     *
     * @param status New web server status
     */
    void updateWebServerStatus(WebServerStatus status);

    /**
     * @brief Get current subsystem status
     *
     * Returns const reference to internal SubsystemStatus struct.
     * Used by DisplayManager to render startup progress.
     *
     * @return Const reference to current status
     */
    const SubsystemStatus& getStatus() const;
};

#endif // STARTUP_PROGRESS_TRACKER_H
