/**
 * @file DisplayManager.h
 * @brief Main display orchestration component
 *
 * Coordinates OLED display operations by orchestrating MetricsCollector,
 * DisplayFormatter, and StartupProgressTracker components. Renders both
 * startup progress screens and runtime status displays.
 *
 * Constitutional Principles:
 * - Principle I (Hardware Abstraction): Uses IDisplayAdapter interface
 * - Principle II (Resource Management): Static allocation, minimal heap usage
 * - Principle VII (Fail-Safe): Graceful degradation if display init fails
 *
 * @version 1.0.0
 * @date 2025-10-09
 */

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include "hal/interfaces/IDisplayAdapter.h"
#include "hal/interfaces/ISystemMetrics.h"
#include "types/DisplayTypes.h"
#include "MetricsCollector.h"
#include "DisplayFormatter.h"
#include "StartupProgressTracker.h"
#include "utils/WebSocketLogger.h"

/**
 * @brief Orchestrates OLED display operations
 *
 * Main component for managing OLED display rendering. Coordinates
 * metrics collection, string formatting, and display adapter operations.
 */
class DisplayManager {
private:
    IDisplayAdapter* _displayAdapter;      ///< HAL interface for display hardware
    ISystemMetrics* _systemMetrics;        ///< HAL interface for system metrics
    MetricsCollector* _metricsCollector;   ///< Metrics gathering component
    StartupProgressTracker* _progressTracker;  ///< Startup progress tracking
    WebSocketLogger* _logger;              ///< WebSocket logger for display events

    DisplayMetrics _currentMetrics;        ///< Current system metrics (static allocation)
    SubsystemStatus _currentStatus;        ///< Current subsystem status (static allocation)

public:
    /**
     * @brief Constructor with dependency injection
     *
     * @param displayAdapter Pointer to IDisplayAdapter implementation
     * @param systemMetrics Pointer to ISystemMetrics implementation
     * @param logger Pointer to WebSocketLogger for logging (optional, can be nullptr for testing)
     */
    DisplayManager(IDisplayAdapter* displayAdapter, ISystemMetrics* systemMetrics, WebSocketLogger* logger = nullptr);

    /**
     * @brief Destructor - clean up owned components
     */
    ~DisplayManager();

    /**
     * @brief Initialize display hardware
     *
     * Calls displayAdapter->init() to initialize I2C and OLED hardware.
     * Logs result via WebSocket (T026).
     *
     * Graceful degradation (FR-027): If init fails, returns false but
     * system continues operating. Subsequent render calls will no-op.
     *
     * @return true if initialization succeeded, false on I2C error
     */
    bool init();

    /**
     * @brief Render startup progress screen
     *
     * Displays boot sequence with subsystem initialization status:
     * - "Poseidon2 Gateway"
     * - "Booting..."
     * - WiFi status (Connecting/Connected/Failed)
     * - Filesystem status (Mounting/Mounted/Failed)
     * - Web Server status (Starting/Running/Failed)
     *
     * Requirements: FR-001 to FR-006
     *
     * @param status Current subsystem status to display
     */
    void renderStartupProgress(const SubsystemStatus& status);

    /**
     * @brief Render runtime status page
     *
     * Displays system status with current metrics:
     * - Line 0: WiFi SSID or "Disconnected"
     * - Line 1: IP address or "---"
     * - Line 2: Free RAM (KB)
     * - Line 3: Flash usage (used/total KB)
     * - Line 4: CPU idle percentage
     * - Line 5: Animation icon (rotating)
     *
     * Requirements: FR-007 to FR-017
     *
     * Note: This method queries fresh metrics each time it's called.
     * Typically called every 5 seconds via ReactESP (FR-016).
     */
    void renderStatusPage();

    /**
     * @brief Render runtime status page with provided status
     *
     * Overload that accepts SubsystemStatus for testing.
     *
     * @param status Subsystem status to display (for testing)
     */
    void renderStatusPage(const SubsystemStatus& status);

    /**
     * @brief Update animation icon only (partial render)
     *
     * Increments animation state (0 → 1 → 2 → 3 → 0) and re-renders
     * only the animation icon in the corner. More efficient than full
     * page refresh for 1-second animation updates (FR-016a).
     *
     * @param metrics Metrics with current animation state (for testing)
     */
    void updateAnimationIcon(const DisplayMetrics& metrics);

    /**
     * @brief Update animation icon using internal state
     *
     * Increments internal animation state and renders icon.
     * Typically called every 1 second via ReactESP (FR-016a).
     */
    void updateAnimationIcon();

    /**
     * @brief Update WiFi connection status
     *
     * Updates internal status for display rendering. Should be called
     * when WiFi state changes (connected, disconnected, etc.).
     *
     * Bugfix-001: Added to sync WiFi state from event callbacks to DisplayManager.
     *
     * @param status New WiFi connection status
     * @param ssid SSID of connected network (optional, nullptr to skip)
     * @param ip IP address string (optional, nullptr to skip)
     */
    void updateWiFiStatus(DisplayConnectionStatus status,
                         const char* ssid = nullptr,
                         const char* ip = nullptr);

    /**
     * @brief Get current metrics (for testing)
     *
     * @return Const reference to current DisplayMetrics
     */
    const DisplayMetrics& getCurrentMetrics() const { return _currentMetrics; }

    /**
     * @brief Get current status (for testing)
     *
     * @return Const reference to current SubsystemStatus
     */
    const SubsystemStatus& getCurrentStatus() const { return _currentStatus; }
};

#endif // DISPLAY_MANAGER_H
