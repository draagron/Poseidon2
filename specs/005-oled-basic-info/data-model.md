# Data Model: OLED Basic Info Display

**Feature**: OLED Basic Info Display
**Date**: 2025-10-08
**Status**: Complete

## Overview
This document defines the data structures for the OLED display system. All structures use static allocation with efficient data types (uint8_t, uint16_t, uint32_t) to minimize RAM usage per constitutional requirements.

---

## Core Data Structures

### DisplayMetrics

Represents system resource metrics displayed on the OLED.

```cpp
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
```

**Size**: 21 bytes (5×4 bytes + 2×1 byte + 4 bytes + 3 bytes padding)

**Validation Rules**:
- `freeRamBytes`: Must be > 0 and ≤ 320KB (ESP32 total RAM)
- `sketchSizeBytes`: Must be > 0 and ≤ 1.9MB (partition size)
- `freeFlashBytes`: Must be ≥ 0 and ≤ 1.9MB
- `cpuIdlePercent`: Must be 0-100
- `animationState`: Must be 0-3
- `lastUpdate`: Monotonically increasing (millis() wraps after ~49 days, acceptable)

**Lifecycle**:
1. **Initialization**: Set to safe defaults (0 for counts, 100 for CPU idle, 0 for animation state)
2. **Update**: MetricsCollector updates metrics every 5 seconds
3. **Animation**: AnimationState increments every 1 second (cyclic 0→1→2→3→0)

---

### SubsystemStatus

Represents initialization and runtime status of monitored subsystems.

```cpp
/**
 * @brief Connection status enumeration
 */
enum ConnectionStatus {
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
    ConnectionStatus wifiStatus;      ///< Current WiFi connection state
    char wifiSSID[33];                ///< Connected SSID (max 32 chars + null terminator)
    char wifiIPAddress[16];           ///< IP address as string ("255.255.255.255\0")
    FilesystemStatus fsStatus;        ///< Current filesystem state
    WebServerStatus webServerStatus;  ///< Current web server state
    unsigned long wifiTimestamp;      ///< millis() when WiFi state last changed
    unsigned long fsTimestamp;        ///< millis() when filesystem state last changed
    unsigned long wsTimestamp;        ///< millis() when web server state last changed
};
```

**Size**: ~66 bytes (4 bytes enums + 33 bytes SSID + 16 bytes IP + 12 bytes timestamps + 1 byte padding)

**Validation Rules**:
- `wifiSSID`: Must be null-terminated, max 32 characters (WiFi spec limit)
- `wifiIPAddress`: Must be valid IPv4 format ("xxx.xxx.xxx.xxx") or empty string ("")
- All timestamps: Monotonically increasing (millis() based)

**State Transitions**:

**WiFi States**:
```
DISCONNECTED → CONNECTING → CONNECTED
             ↓ (timeout)
            FAILED → DISCONNECTED (reset on retry)

CONNECTED → DISCONNECTED (connection lost) → CONNECTING (auto-reconnect)
```

**Filesystem States**:
```
MOUNTING → MOUNTED (success)
         → FAILED (error)
```

**Web Server States**:
```
STARTING → RUNNING (success)
         → FAILED (error, e.g., port already in use)
```

**Lifecycle**:
1. **Initialization**: All states set to initial values (DISCONNECTED, MOUNTING, STARTING)
2. **Startup**: States transition as subsystems initialize (tracked by StartupProgressTracker)
3. **Runtime**: WiFi state may change (connect/disconnect), filesystem and web server states typically stable

---

### DisplayPage

Foundation for multi-page display architecture (FR-019, FR-020, FR-021).

```cpp
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
```

**Size**: ~10 bytes (1 byte page number + 4 bytes function pointer + 4 bytes string pointer + 1 byte padding)

**Validation Rules**:
- `pageNumber`: Must be > 0 (page 0 reserved)
- `renderFunc`: Must not be NULL
- `pageName`: Must be null-terminated, max 16 characters recommended

**Usage Pattern**:
```cpp
// Page 1: System Status (implemented in this feature)
void renderStatusPage(IDisplayAdapter* display,
                      const DisplayMetrics& metrics,
                      const SubsystemStatus& status) {
    display->clear();
    display->setCursor(0, 0);
    // ... render WiFi, RAM, flash, CPU, animation ...
    display->display();
}

DisplayPage statusPage = {
    .pageNumber = 1,
    .renderFunc = renderStatusPage,
    .pageName = "Status"
};

// Future pages (placeholders for future features)
DisplayPage nmeaPage = {
    .pageNumber = 2,
    .renderFunc = renderNMEAPage,  // To be implemented
    .pageName = "NMEA"
};
```

**Lifecycle**:
1. **Initialization**: Page array defined statically in DisplayManager
2. **Runtime**: Current page tracked by DisplayManager (initially page 1)
3. **Future**: Button press will cycle through pages (not implemented in this feature)

---

## Relationships

```
DisplayManager
    ├── has: DisplayMetrics (1)
    ├── has: SubsystemStatus (1)
    ├── has: DisplayPage[] (array, currently 1 page)
    ├── uses: IDisplayAdapter* (HAL interface)
    └── uses: ISystemMetrics* (HAL interface)

MetricsCollector
    ├── uses: ISystemMetrics* (HAL interface)
    └── produces: DisplayMetrics (updates struct)

StartupProgressTracker
    ├── monitors: WiFiManager, LittleFSAdapter, ConfigWebServer
    └── produces: SubsystemStatus (updates struct)

DisplayFormatter
    ├── consumes: DisplayMetrics
    ├── consumes: SubsystemStatus
    └── produces: formatted strings (char buffers)
```

---

## Memory Footprint Analysis

**Total Static Allocation** (per display instance):
- DisplayMetrics: 21 bytes
- SubsystemStatus: 66 bytes
- DisplayPage (array of 1): 10 bytes
- **Total**: ~97 bytes

**Additional Heap Allocation** (justified):
- Adafruit_SSD1306 framebuffer: 1024 bytes (128×64 pixels / 8 bits)
  - Required for double-buffering (prevents flicker)
  - Allocated once during init, not freed until reboot

**Total Memory Impact**: ~1121 bytes (~0.35% of 320KB ESP32 RAM)

**Constitutional Compliance**: ✓ Well within resource constraints (Principle II)

---

## Example Usage

```cpp
// Global instances (static allocation)
DisplayMetrics displayMetrics = {
    .freeRamBytes = 0,
    .sketchSizeBytes = 0,
    .freeFlashBytes = 0,
    .cpuIdlePercent = 100,
    .animationState = 0,
    .lastUpdate = 0
};

SubsystemStatus subsystemStatus = {
    .wifiStatus = CONN_DISCONNECTED,
    .wifiSSID = {0},
    .wifiIPAddress = {0},
    .fsStatus = FS_MOUNTING,
    .webServerStatus = WS_STARTING,
    .wifiTimestamp = 0,
    .fsTimestamp = 0,
    .wsTimestamp = 0
};

// Update metrics (called every 5 seconds by ReactESP)
void updateMetrics() {
    displayMetrics.freeRamBytes = metricsCollector->getFreeRamBytes();
    displayMetrics.sketchSizeBytes = metricsCollector->getSketchSizeBytes();
    displayMetrics.freeFlashBytes = metricsCollector->getFreeFlashBytes();
    displayMetrics.cpuIdlePercent = metricsCollector->getCpuIdlePercent();
    displayMetrics.lastUpdate = millis();
}

// Update animation (called every 1 second by ReactESP)
void updateAnimation() {
    displayMetrics.animationState = (displayMetrics.animationState + 1) % 4;
}

// Update WiFi status (called by WiFiManager event handler)
void onWiFiConnected(const char* ssid, const char* ip) {
    subsystemStatus.wifiStatus = CONN_CONNECTED;
    strncpy(subsystemStatus.wifiSSID, ssid, sizeof(subsystemStatus.wifiSSID) - 1);
    strncpy(subsystemStatus.wifiIPAddress, ip, sizeof(subsystemStatus.wifiIPAddress) - 1);
    subsystemStatus.wifiTimestamp = millis();
}
```

---

## Notes

1. **PROGMEM Strings**: All display text constants (labels, error messages) stored in PROGMEM to save RAM. Use `F()` macro or explicit PROGMEM declarations.

2. **Timestamp Wrapping**: `millis()` wraps after ~49 days. Acceptable for marine gateway (unlikely to run 49+ days without reboot). If needed, delta calculations handle wrapping: `(unsigned long)(currentMillis - lastMillis)`.

3. **Thread Safety**: All structures updated from single thread (main loop via ReactESP callbacks). No mutex/semaphore required.

4. **Future Expansion**: DisplayPage array can grow to support multiple pages (NMEA data, sensor graphs, configuration). Button press on GPIO13 will cycle pages (future feature).

---

*Data model complete: 2025-10-08*
