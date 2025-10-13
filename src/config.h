/**
 * @file config.h
 * @brief Compile-time configuration for WiFi network management
 *
 * This file contains constants for WiFi connection management, including
 * timeouts, limits, and file paths used by the WiFiManager component.
 */

#ifndef CONFIG_H
#define CONFIG_H

// WiFi Connection Configuration
#define WIFI_TIMEOUT_MS 30000        // 30 seconds timeout per network attempt
#define MAX_NETWORKS 3               // Maximum number of WiFi networks in config
#define CONFIG_FILE_PATH "/wifi.conf" // LittleFS path for WiFi configuration

// Network Debugging Configuration
#define UDP_DEBUG_PORT 4444          // LEGACY: Unused - WebSocket logging now used (ws://<device-ip>/logs)

// Reboot Configuration
#define REBOOT_DELAY_MS 5000         // 5 seconds delay before reboot

// OLED Display Configuration
#define OLED_I2C_ADDRESS 0x3C        // I2C address for SSD1306 OLED (128x64)
#define OLED_SCREEN_WIDTH 128        // Display width in pixels
#define OLED_SCREEN_HEIGHT 64        // Display height in pixels
#define OLED_SDA_PIN 21              // GPIO21 for I2C Bus 2 SDA
#define OLED_SCL_PIN 22              // GPIO22 for I2C Bus 2 SCL
#define OLED_I2C_CLOCK 400000        // 400kHz I2C fast mode
#define DISPLAY_ANIMATION_INTERVAL_MS 1000  // 1 second animation icon update
#define DISPLAY_STATUS_INTERVAL_MS 5000     // 5 seconds status refresh

// NMEA2000 CAN Bus Configuration (SH-ESP32 Board)
#define CAN_TX_PIN 32                // GPIO32 for CAN TX
#define CAN_RX_PIN 34                // GPIO34 for CAN RX

// Source Statistics Configuration
#define MAX_SOURCES 50               // Maximum number of concurrent NMEA sources
#define SOURCE_STALE_THRESHOLD_MS 5000       // 5 seconds without update = stale
#define SOURCE_GC_THRESHOLD_MS 300000        // 5 minutes = eligible for garbage collection
#define WEBSOCKET_UPDATE_INTERVAL_MS 500     // 500ms batch interval for delta updates

#endif // CONFIG_H
