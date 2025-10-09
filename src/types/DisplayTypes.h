/**
 * @file DisplayTypes.h
 * @brief Core data structures for OLED display system
 *
 * Defines data structures for system metrics, subsystem status, and display pages.
 * All structures use efficient data types (uint8_t, uint16_t, uint32_t) and static
 * allocation to minimize RAM usage per constitutional requirements.
 *
 * Memory footprint: ~97 bytes total static allocation
 * - DisplayMetrics: 21 bytes
 * - SubsystemStatus: 66 bytes
 * - DisplayPage: 10 bytes
 *
 * @version 1.0.0
 * @date 2025-10-08
 */

#ifndef DISPLAY_TYPES_H
#define DISPLAY_TYPES_H

#include <stdint.h>

// Forward declaration for function pointer
class IDisplayAdapter;

/**
 * @brief System resource metrics for display
 *
 * All metrics updated every 5 seconds (FR-016).
 * Animation state updated every 1 second (FR-016a).
 */
struct DisplayMetrics {
    uint32_t freeRamBytes;          ///< Free heap memory in bytes (ESP.getFreeHeap())
    uint32_t sketchSizeBytes;       ///< Uploaded code size in bytes (ESP.getSketchSize())
    uint32_t freeFlashBytes;        ///< Free flash space in bytes (ESP.getFreeSketchSpace())
    uint8_t  cpuIdlePercent;        ///< CPU idle time 0-100% (FreeRTOS runtime stats)
    uint8_t  animationState;        ///< Rotating icon state: 0=/, 1=-, 2=\, 3=|
    unsigned long lastUpdate;       ///< millis() timestamp of last metrics update
};

/**
 * @brief Display connection status enumeration
 *
 * Note: This is separate from WiFiConnectionState's ConnectionStatus enum class
 * to avoid naming conflicts. Used for OLED display rendering only.
 */
enum DisplayConnectionStatus {
    CONN_CONNECTING,    ///< WiFi connection in progress
    CONN_CONNECTED,     ///< WiFi connected successfully
    CONN_DISCONNECTED,  ///< WiFi disconnected (idle or connection lost)
    CONN_FAILED         ///< WiFi connection failed (timeout or error)
};

/**
 * @brief Filesystem status enumeration
 */
enum FilesystemStatus {
    FS_MOUNTING,  ///< LittleFS mount in progress
    FS_MOUNTED,   ///< LittleFS mounted successfully
    FS_FAILED     ///< LittleFS mount failed
};

/**
 * @brief Web server status enumeration
 */
enum WebServerStatus {
    WS_STARTING,  ///< Web server initialization in progress
    WS_RUNNING,   ///< Web server running and accepting connections
    WS_FAILED     ///< Web server failed to start
};

/**
 * @brief Subsystem status tracking structure
 *
 * Tracks WiFi, filesystem, and web server status for display during
 * startup (FR-001 to FR-006) and runtime (FR-007 to FR-010).
 */
struct SubsystemStatus {
    DisplayConnectionStatus wifiStatus;      ///< Current WiFi connection state
    char wifiSSID[33];                ///< Connected SSID (max 32 chars + null terminator)
    char wifiIPAddress[16];           ///< IP address as string ("255.255.255.255\0")
    FilesystemStatus fsStatus;        ///< Current filesystem state
    WebServerStatus webServerStatus;  ///< Current web server state
    unsigned long wifiTimestamp;      ///< millis() when WiFi state last changed
    unsigned long fsTimestamp;        ///< millis() when filesystem state last changed
    unsigned long wsTimestamp;        ///< millis() when web server state last changed
};

/**
 * @brief Page render function signature
 *
 * @param display Pointer to display adapter for rendering operations
 * @param metrics Current system metrics to display
 * @param status Current subsystem status to display
 */
typedef void (*PageRenderFunction)(
    IDisplayAdapter* display,
    const DisplayMetrics& metrics,
    const SubsystemStatus& status
);

/**
 * @brief Display page definition
 *
 * Represents a logical page of information on the OLED.
 * This feature implements only Page 1 (system status), but architecture
 * supports future multi-page expansion (button navigation, FR-021).
 */
struct DisplayPage {
    uint8_t pageNumber;               ///< Page identifier (1 = system status, 2+ reserved)
    PageRenderFunction renderFunc;    ///< Function pointer to render this page
    const char* pageName;             ///< Short name (e.g., "Status", "NMEA", "Sensors")
};

#endif // DISPLAY_TYPES_H
