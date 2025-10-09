# Research: OLED Basic Info Display

**Feature**: OLED Basic Info Display
**Date**: 2025-10-08
**Status**: Complete

## Overview
This document consolidates research findings for implementing an OLED display system on the Poseidon2 ESP32 marine gateway. The display shows startup progress and runtime system status using a 128x64 SSD1306 OLED on I2C Bus 2.

## Research Areas

### 1. Adafruit_SSD1306 Library API

**Decision**: Use Adafruit_SSD1306 with Adafruit_GFX for text rendering

**Rationale**:
- Mature, widely-used library with proven ESP32 compatibility
- Excellent documentation and community support
- Part of approved library stack (Adafruit_SSD1306 already approved in constitution)
- Minimal flash footprint (~15KB including GFX library)
- Simple API matches our requirements (text-only display, no complex graphics)

**API Pattern**:
```cpp
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1  // No reset pin on SH-ESP32
#define SCREEN_ADDRESS 0x3C  // Common I2C address for 128x64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Initialization
bool success = display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);

// Rendering pattern
display.clearDisplay();              // Clear buffer
display.setTextSize(1);              // Font size 1 = 5x7 pixels
display.setTextColor(SSD1306_WHITE); // White text on black background
display.setCursor(0, 0);             // Position: column 0, row 0
display.println(F("WiFi: Connected"));  // F() macro for PROGMEM
display.display();                    // Push buffer to hardware via I2C
```

**Key Methods**:
- `begin()`: Initialize I2C, allocate framebuffer (1KB for 128x64 monochrome)
- `clearDisplay()`: Clear framebuffer (does not update screen)
- `setCursor(x, y)`: Set text position (x=0-127 pixels, y=0-63 pixels)
- `setTextSize(n)`: Set font scale (1=5x7, 2=10x14, etc.)
- `print()` / `println()`: Render text to buffer
- `display()`: Push framebuffer to OLED via I2C transaction

**Alternatives Considered**:
- **U8g2**: More features (multiple fonts, graphics primitives), but larger flash footprint (~30KB+). Overkill for text-only display.
- **Direct I2C**: Too low-level, would require manual SSD1306 command sequences. Error-prone and time-consuming.
- **Custom minimal driver**: Reduces flash but increases development/testing time. Not justified given Adafruit library's modest footprint.

**Sources**:
- https://github.com/adafruit/Adafruit_SSD1306
- https://learn.adafruit.com/monochrome-oled-breakouts/arduino-library-and-examples

---

### 2. FreeRTOS CPU Idle Time Statistics

**Decision**: Use FreeRTOS `vTaskGetRunTimeStats()` to calculate CPU idle percentage

**Rationale**:
- ESP32 FreeRTOS provides built-in runtime statistics tracking
- More accurate than loop iteration counting (accounts for all tasks, not just main loop)
- Requires `CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS=y` in sdkconfig (enabled by default in ESP32 Arduino core)
- Low overhead (~1-2% CPU for stats collection)

**Implementation Pattern**:
```cpp
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

uint8_t getCpuIdlePercent() {
    TaskStatus_t* taskStatusArray;
    volatile UBaseType_t taskCount;
    unsigned long totalRuntime;
    unsigned long idleRuntime = 0;

    // Get task count
    taskCount = uxTaskGetNumberOfTasks();

    // Allocate array for task status
    taskStatusArray = (TaskStatus_t*)pvPortMalloc(taskCount * sizeof(TaskStatus_t));
    if (taskStatusArray == NULL) {
        return 0;  // Out of memory, return safe value
    }

    // Get runtime stats for all tasks
    taskCount = uxTaskGetSystemState(taskStatusArray, taskCount, &totalRuntime);

    // Find IDLE task runtime (name = "IDLE" or "IDLE0"/"IDLE1" for dual-core)
    for (UBaseType_t i = 0; i < taskCount; i++) {
        if (strstr(taskStatusArray[i].pcTaskName, "IDLE") != NULL) {
            idleRuntime += taskStatusArray[i].ulRunTimeCounter;
        }
    }

    vPortFree(taskStatusArray);

    // Calculate percentage (avoid division by zero)
    if (totalRuntime == 0) {
        return 100;  // No runtime yet, assume idle
    }

    return (idleRuntime * 100) / totalRuntime;
}
```

**Alternative Metrics Considered**:
- **Loop iterations/second**: Simple counter, but doesn't account for time spent in ReactESP callbacks, WiFi stack, or other tasks. Misleading metric.
- **`micros()` delta between loop iterations**: Shows responsiveness but not overall CPU load. High overhead to track continuously.
- **Heap fragmentation**: Interesting diagnostic but not a CPU utilization metric.

**Memory APIs** (no controversy, straightforward):
```cpp
uint32_t freeRam = ESP.getFreeHeap();          // Free heap memory in bytes
uint32_t sketchSize = ESP.getSketchSize();     // Uploaded code size in bytes
uint32_t freeFlash = ESP.getFreeSketchSpace(); // Free flash for OTA updates
```

**Sources**:
- https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html
- ESP32 Arduino Core documentation (ESP.h API reference)

---

### 3. I2C Bus 2 Configuration (SH-ESP32 Board)

**Decision**: Use Wire library with `Wire.begin(21, 22)` for I2C Bus 2

**Rationale**:
- SH-ESP32 board pinout dedicates I2C Bus 2 (SDA=GPIO21, SCL=GPIO22) to OLED display
- I2C Bus 1 (SDA=GPIO16, SCL=GPIO17) may be used for other sensors (future expansion)
- Wire library (Arduino core) provides reliable I2C abstraction
- 400kHz clock speed (fast mode) provides responsive updates without excessive CPU overhead

**Configuration**:
```cpp
#include <Wire.h>

#define OLED_SDA 21
#define OLED_SCL 22
#define I2C_CLOCK_SPEED 400000  // 400kHz fast mode

void initI2C() {
    Wire.begin(OLED_SDA, OLED_SCL);
    Wire.setClock(I2C_CLOCK_SPEED);
}
```

**I2C Address**:
- **0x3C** (60 decimal): Most common address for 128x64 SSD1306 displays
- **0x3D** (61 decimal): Alternative address (selectable via hardware jumper on some modules)
- Our implementation will use 0x3C (verify with hardware if init fails)

**Bus Sharing Considerations**:
- I2C Bus 2 may be shared with other devices in future (e.g., additional sensors)
- SSD1306 supports multi-master I2C (safe to share bus)
- Display updates are quick (<10ms typical), minimal bus contention

**Alternatives Considered**:
- **Software I2C**: Slower (~100kHz max), higher CPU overhead. No benefit over hardware I2C.
- **SPI interface**: SSD1306 supports SPI, but requires 5+ GPIO pins (vs 2 for I2C). Not justified given available I2C bus.

**Sources**:
- SH-ESP32 board schematic (GPIO pinout)
- Arduino Wire library documentation

---

### 4. ReactESP Timer Integration

**Decision**: Use `app.onRepeat()` with separate callbacks for animation (1s) and status (5s)

**Rationale**:
- Aligns with existing Poseidon2 architecture (WiFiManager, BoatData already use ReactESP)
- Non-blocking: Callbacks execute in main loop via `app.tick()`
- Separate timers allow independent timing control (animation faster than status refresh)
- No additional FreeRTOS tasks required (simpler, less memory overhead)

**Implementation Pattern**:
```cpp
#include <ReactESP.h>

using namespace reactesp;
extern ReactESP app;  // Global ReactESP instance from main.cpp

// Callback for rotating icon animation (1 second)
void updateAnimation() {
    // Increment animation state (0 -> 1 -> 2 -> 3 -> 0)
    displayMetrics.animationState = (displayMetrics.animationState + 1) % 4;

    // Render icon only (partial update)
    displayManager->updateAnimationIcon();
}

// Callback for status information (5 seconds)
void updateStatus() {
    // Collect fresh metrics
    metricsCollector->updateMetrics(&displayMetrics);

    // Render full status screen
    displayManager->renderStatusPage();
}

// Register timers in setup()
void setup() {
    // ... other init ...

    app.onRepeat(1000, updateAnimation);  // 1000ms = 1 second
    app.onRepeat(5000, updateStatus);     // 5000ms = 5 seconds
}
```

**Timer Accuracy**:
- ReactESP uses `millis()` internally for timing
- Accuracy: ±10ms typical (sufficient for display updates)
- Jitter: Minimal if `app.tick()` called frequently in main loop (existing architecture already does this)

**Alternatives Considered**:
- **FreeRTOS tasks**: Creates dedicated task for display updates. Overkill (requires stack allocation, task scheduling overhead). ReactESP callbacks are simpler and already proven in codebase.
- **`millis()` polling in main loop**: Manual tracking of last update time. More error-prone, harder to maintain than ReactESP's declarative API.
- **Single timer with conditional logic**: `if (millis() % 1000 == 0) updateAnimation()`. Brittle timing, misses cycles if `app.tick()` not called exactly on 1s boundary.

**Sources**:
- ReactESP library documentation: https://github.com/mairas/ReactESP
- Existing Poseidon2 code: `src/main.cpp` (WiFi timeout checks, BoatData calculation cycle)

---

### 5. Display Layout for 128x64 Pixels

**Decision**: 6-line text layout with font size 1 (5x7 pixel characters)

**Rationale**:
- Font size 1 provides maximum information density (21 characters/line, 8 lines max)
- Using 6 lines allows room for status indicators and keeps text readable
- All required metrics fit on single page (no scrolling needed)
- Aligns with user requirement: "avoid over-abstraction, efficient use of resources"

**Layout Specification**:
```
+----------------------------+ (128 pixels wide)
| Line 0: WiFi SSID          | (0, 0)   - 21 chars max
| Line 1: IP Address         | (0, 10)  - e.g., "192.168.1.100"
| Line 2: RAM Free           | (0, 20)  - e.g., "RAM: 245KB"
| Line 3: Flash Usage        | (0, 30)  - e.g., "Flash: 850/1920KB"
| Line 4: CPU Idle           | (0, 40)  - e.g., "CPU Idle: 87%"
| Line 5: Animation Icon     | (0, 50)  - Right corner: [ / ]
+----------------------------+ (64 pixels tall)
```

**Character Dimensions** (font size 1):
- Width: 5 pixels + 1 pixel spacing = 6 pixels/char
- Height: 7 pixels + 3 pixel line spacing = 10 pixels/line
- Max chars/line: 128 / 6 = 21 characters
- Max lines: 64 / 10 = 6.4 lines (use 6 for clean layout)

**Text Formatting Conventions**:
```cpp
// Concise metric display to fit 21-char line limit
"WiFi: MyHomeSSID"      // 16 chars (fits comfortably)
"IP: 192.168.1.100"     // 18 chars
"RAM: 245KB"            // 11 chars
"Flash: 850/1920KB"     // 18 chars
"CPU: 87%"              // 9 chars
"[ | ]"                 // 5 chars (right-aligned in corner)
```

**Startup Progress Layout** (alternative layout during boot):
```
+----------------------------+
| Poseidon2 Gateway          | Header
| Booting...                 |
| [X] WiFi                   | ✓ or ✗
| [X] Filesystem             | ✓ or ✗
| [X] WebServer              | ✓ or ✗
| Connecting... 12s          | Timeout counter
+----------------------------+
```

**Alternatives Considered**:
- **Font size 2 (10x14 pixels)**: Only 3 lines fit. Not enough for all metrics (would require multi-page navigation, out of scope for this feature).
- **Mixed font sizes**: Header in size 2, metrics in size 1. Adds complexity, minimal UX benefit.
- **Graphical bars** (for RAM, CPU): More visually appealing but requires ~2KB additional flash for drawing primitives. User requirement prioritizes "avoid library bloat."
- **Scrolling text**: For long SSIDs (>21 chars). Adds complexity, uncommon use case (most SSIDs <20 chars).

**Sources**:
- Adafruit_GFX font rendering documentation
- SSD1306 display datasheet (pixel resolution)

---

### 6. WiFi Connection Timeout Display (Deferred Clarification)

**Decision**: Show elapsed time during connection attempt ("Connecting... 10s")

**Rationale**:
- Provides user feedback during 30-second timeout window
- Indicates system is responsive (not frozen)
- Simple to implement: Increment counter every second during CONNECTING state
- Aligns with FR-002: "System MUST display WiFi connection status during startup, including which network is being attempted"

**Implementation**:
```cpp
// During WiFi connection attempt
if (subsystemStatus.wifiStatus == CONN_CONNECTING) {
    unsigned long elapsedSec = (millis() - subsystemStatus.wifiTimestamp) / 1000;
    snprintf(buffer, sizeof(buffer), "Connecting... %lus", elapsedSec);
    display.println(buffer);
}
```

**Display Pattern**:
```
Frame 1 (t=0s):   "WiFi: HomeNetwork"
                  "Connecting... 0s"
Frame 2 (t=1s):   "Connecting... 1s"
Frame 3 (t=2s):   "Connecting... 2s"
...
Frame 30 (t=30s): "Connecting... 30s"
                  "WiFi: FAILED"
```

**Alternative**: Static "Connecting..." (no counter)
- **Rejected**: Less informative, user may think system is frozen after 20+ seconds

**Sources**:
- User requirements: R004 - OLED basic info.md
- Deferred clarification from spec.md review

---

### 7. Filesystem Mount Failure Display (Deferred Clarification)

**Decision**: Display "FS: FAILED" on startup screen, continue operation

**Rationale**:
- Consistent with graceful degradation principle (FR-027)
- User informed of issue via both display and WebSocket logging
- Filesystem failure is independent of OLED hardware (no reason to block display init)
- Allows user to diagnose issue without losing visibility into other subsystems (WiFi still functional, logs still available)

**Implementation**:
```cpp
// During filesystem mount attempt
if (subsystemStatus.fsStatus == FS_FAILED) {
    display.println(F("FS: FAILED"));
} else {
    display.println(F("FS: OK"));
}
```

**Startup Screen with Filesystem Failure**:
```
+----------------------------+
| Poseidon2 Gateway          |
| Booting...                 |
| [✓] WiFi                   |
| [✗] Filesystem             | <-- Failure indicated
| [✓] WebServer              |
|                            |
+----------------------------+
```

**Runtime Behavior**:
- Filesystem failure persists as status indicator on main screen
- WebSocket log includes detailed error: `{"level":"ERROR","component":"FileSystem","event":"MOUNT_FAILED"}`
- User can reboot device, reflash firmware, or investigate via WebSocket logs

**Alternative**: Reboot on filesystem failure
- **Rejected**: Too aggressive. Loses visibility into other subsystems. User cannot diagnose issue if device continuously reboots.

**Sources**:
- Constitution v1.2.0, Principle VII: Fail-Safe Operation (graceful degradation preferred)
- Deferred clarification from spec.md review

---

## Summary of Decisions

| Area | Decision | Key Justification |
|------|----------|-------------------|
| Display Library | Adafruit_SSD1306 + GFX | Mature, proven, modest flash footprint (~15KB) |
| CPU Idle Metric | FreeRTOS `vTaskGetRunTimeStats()` | Accurate, low overhead, accounts for all tasks |
| I2C Configuration | Wire.begin(21, 22), 400kHz | SH-ESP32 pinout, fast mode, shared bus safe |
| Timer Integration | ReactESP `app.onRepeat()` | Aligns with existing architecture, non-blocking |
| Display Layout | 6-line text, font size 1 | Maximum info density, fits all metrics |
| WiFi Timeout UX | Show elapsed time counter | User feedback, indicates system responsive |
| FS Failure Handling | Display "FAILED", continue | Graceful degradation, maintain visibility |

All decisions prioritize:
- **Resource efficiency**: Static allocation, PROGMEM strings, minimal heap usage
- **Constitutional compliance**: HAL abstraction, graceful degradation, WebSocket logging
- **Simplicity**: Avoid over-abstraction, use proven libraries, leverage existing patterns

## Next Steps

**Phase 1**: Generate data model (data-model.md), HAL contracts (/contracts/), quickstart tests (quickstart.md), and update agent context (CLAUDE.md)

---
*Research complete: 2025-10-08*
