/**
 * @file DisplayManager.cpp
 * @brief Implementation of DisplayManager component
 *
 * @version 1.0.0
 * @date 2025-10-09
 */

#include "DisplayManager.h"
#include "utils/DisplayLayout.h"
#include <string.h>

DisplayManager::DisplayManager(IDisplayAdapter* displayAdapter, ISystemMetrics* systemMetrics, WebSocketLogger* logger)
    : _displayAdapter(displayAdapter),
      _systemMetrics(systemMetrics),
      _metricsCollector(nullptr),
      _progressTracker(nullptr),
      _logger(logger) {

    // Initialize current metrics to safe defaults
    _currentMetrics.freeRamBytes = 0;
    _currentMetrics.sketchSizeBytes = 0;
    _currentMetrics.freeFlashBytes = 0;
    _currentMetrics.loopFrequency = 0;
    _currentMetrics.animationState = 0;
    _currentMetrics.lastUpdate = 0;

    // Initialize current status to safe defaults
    _currentStatus.wifiStatus = CONN_DISCONNECTED;
    _currentStatus.wifiSSID[0] = '\0';
    _currentStatus.wifiIPAddress[0] = '\0';
    _currentStatus.fsStatus = FS_MOUNTING;
    _currentStatus.webServerStatus = WS_STARTING;
    _currentStatus.wifiTimestamp = 0;
    _currentStatus.fsTimestamp = 0;
    _currentStatus.wsTimestamp = 0;

    // Create sub-components (static allocation)
    if (_systemMetrics != nullptr) {
        _metricsCollector = new MetricsCollector(_systemMetrics);
    }
    _progressTracker = new StartupProgressTracker();
}

DisplayManager::~DisplayManager() {
    // Clean up owned components
    if (_metricsCollector != nullptr) {
        delete _metricsCollector;
    }
    if (_progressTracker != nullptr) {
        delete _progressTracker;
    }
}

bool DisplayManager::init() {
    if (_displayAdapter == nullptr) {
        // Log ERROR: DisplayAdapter is null
        if (_logger != nullptr) {
            _logger->broadcastLog(LogLevel::ERROR, "DisplayManager", "INIT_FAILED",
                                  F("{\"reason\":\"DisplayAdapter is null\"}"));
        }
        return false;
    }

    bool success = _displayAdapter->init();

    if (success) {
        // Log INFO: Display initialized successfully
        if (_logger != nullptr) {
            _logger->broadcastLog(LogLevel::INFO, "DisplayManager", "INIT_SUCCESS",
                                  F("{\"display\":\"SSD1306\",\"resolution\":\"128x64\"}"));
        }
    } else {
        // Log ERROR: Display initialization failed (I2C error)
        if (_logger != nullptr) {
            _logger->broadcastLog(LogLevel::ERROR, "DisplayManager", "INIT_FAILED",
                                  F("{\"reason\":\"I2C communication error\"}"));
        }
    }

    return success;
}

void DisplayManager::renderStartupProgress(const SubsystemStatus& status) {
    // Graceful degradation: skip if display not ready (FR-027)
    if (_displayAdapter == nullptr || !_displayAdapter->isReady()) {
        return;
    }

    _displayAdapter->clear();
    _displayAdapter->setTextSize(1);

    // Line 0-1: Header
    _displayAdapter->setCursor(0, getLineY(0));
    _displayAdapter->print("Poseidon2 Gateway");

    _displayAdapter->setCursor(0, getLineY(1));
    _displayAdapter->print("Booting...");

    // Line 2: WiFi status
    _displayAdapter->setCursor(0, getLineY(2));
    switch (status.wifiStatus) {
        case CONN_CONNECTING:
            _displayAdapter->print("[~] WiFi");
            break;
        case CONN_CONNECTED:
            _displayAdapter->print("[+] WiFi: ");
            _displayAdapter->print(status.wifiSSID);
            break;
        case CONN_DISCONNECTED:
            _displayAdapter->print("[ ] WiFi");
            break;
        case CONN_FAILED:
            _displayAdapter->print("[X] WiFi: FAILED");
            break;
    }

    // Line 3: Filesystem status
    _displayAdapter->setCursor(0, getLineY(3));
    switch (status.fsStatus) {
        case FS_MOUNTING:
            _displayAdapter->print("[~] Filesystem");
            break;
        case FS_MOUNTED:
            _displayAdapter->print("[+] Filesystem");
            break;
        case FS_FAILED:
            _displayAdapter->print("[X] Filesystem");
            break;
    }

    // Line 4: Web server status
    _displayAdapter->setCursor(0, getLineY(4));
    switch (status.webServerStatus) {
        case WS_STARTING:
            _displayAdapter->print("[~] WebServer");
            break;
        case WS_RUNNING:
            _displayAdapter->print("[+] WebServer");
            break;
        case WS_FAILED:
            _displayAdapter->print("[X] WebServer");
            break;
    }

    _displayAdapter->display();
}

void DisplayManager::renderStatusPage() {
    // Collect fresh metrics
    if (_metricsCollector != nullptr) {
        _metricsCollector->collectMetrics(&_currentMetrics);
    }

    // Log rendering event (DEBUG level to avoid flooding logs every 5 seconds)
    if (_logger != nullptr) {
        _logger->broadcastLog(LogLevel::DEBUG, "DisplayManager", "RENDER_STATUS_PAGE",
                              F("{\"page\":\"status\"}"));
    }

    // Render using internal status (updated externally via progressTracker)
    renderStatusPage(_currentStatus);
}

void DisplayManager::renderStatusPage(const SubsystemStatus& status) {
    // Graceful degradation: skip if display not ready (FR-027)
    if (_displayAdapter == nullptr || !_displayAdapter->isReady()) {
        return;
    }

    _displayAdapter->clear();
    _displayAdapter->setTextSize(1);

    char buffer[22];  // Buffer for formatted strings

    // Line 0: WiFi SSID or status
    _displayAdapter->setCursor(0, getLineY(0));
    if (status.wifiStatus == CONN_CONNECTED && status.wifiSSID[0] != '\0') {
        _displayAdapter->print("WiFi: ");
        _displayAdapter->print(status.wifiSSID);
    } else if (status.wifiStatus == CONN_CONNECTING) {
        _displayAdapter->print("WiFi: Connecting");
    } else {
        _displayAdapter->print("WiFi: Disconnected");
    }

    // Line 1: IP address
    _displayAdapter->setCursor(0, getLineY(1));
    DisplayFormatter::formatIPAddress(status.wifiIPAddress, buffer);
    _displayAdapter->print(buffer);

    // Line 2: Free RAM
    _displayAdapter->setCursor(0, getLineY(2));
    _displayAdapter->print("RAM: ");
    DisplayFormatter::formatBytes(_currentMetrics.freeRamBytes, buffer);
    _displayAdapter->print(buffer);

    // Line 3: Flash usage
    _displayAdapter->setCursor(0, getLineY(3));
    _displayAdapter->print("Flash: ");
    uint32_t totalFlash = _currentMetrics.sketchSizeBytes + _currentMetrics.freeFlashBytes;
    DisplayFormatter::formatFlashUsage(_currentMetrics.sketchSizeBytes, totalFlash, buffer);
    _displayAdapter->print(buffer);

    // Line 4: Loop frequency
    _displayAdapter->setCursor(0, getLineY(4));
    _displayAdapter->print("Loop: ");
    DisplayFormatter::formatFrequency(_currentMetrics.loopFrequency, buffer);
    _displayAdapter->print(buffer);
    _displayAdapter->print(" Hz");

    // Line 5: Animation icon (right corner)
    _displayAdapter->setCursor(118, getLineY(5));  // 128 - 10 pixels for " X " (bugfix-001)
    char icon = DisplayFormatter::getAnimationIcon(_currentMetrics.animationState);
    char iconStr[2] = {icon, '\0'};
    _displayAdapter->print(iconStr);

    _displayAdapter->display();
}

void DisplayManager::updateAnimationIcon(const DisplayMetrics& metrics) {
    // Graceful degradation: skip if display not ready (FR-027)
    if (_displayAdapter == nullptr || !_displayAdapter->isReady()) {
        return;
    }

    // Update internal state for next call
    _currentMetrics.animationState = metrics.animationState;

    // Render only the animation icon (partial update)
    // Note: Full clear and redraw is simpler for SSD1306
    // Optimization: Could just update the corner, but full refresh is <10ms
    _displayAdapter->setCursor(118, getLineY(5));  // Bugfix-001: adjusted from 108 to 118
    char icon = DisplayFormatter::getAnimationIcon(metrics.animationState);
    char iconStr[2] = {icon, '\0'};
    _displayAdapter->print(iconStr);
    _displayAdapter->display();
}

void DisplayManager::updateAnimationIcon() {
    // Increment animation state (0 → 1 → 2 → 3 → 0)
    _currentMetrics.animationState = (_currentMetrics.animationState + 1) % 4;

    // Render using internal state
    updateAnimationIcon(_currentMetrics);
}

void DisplayManager::updateWiFiStatus(DisplayConnectionStatus status,
                                     const char* ssid,
                                     const char* ip) {
    // Graceful degradation: skip if progressTracker not initialized
    if (_progressTracker != nullptr) {
        // Delegate to internal StartupProgressTracker
        _progressTracker->updateWiFiStatus(status, ssid, ip);

        // CRITICAL: Synchronize _currentStatus with progressTracker state
        // This ensures renderStatusPage() uses updated WiFi info
        _currentStatus = _progressTracker->getStatus();
    }
}
