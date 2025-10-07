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

#endif // CONFIG_H
