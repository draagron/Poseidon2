/**
 * @file test_bugfix_001_wifi_status.cpp
 * @brief Regression tests for bugfix-001 - WiFi status synchronization
 *
 * Tests: DisplayManager::updateWiFiStatus() method
 * Bug: WiFi status shows "Disconnected" when actually connected
 * Fix: Add updateWiFiStatus() method to sync state from WiFi callbacks
 *
 * These tests MUST FAIL before the fix is applied (TDD approach)
 *
 * @version 1.0.0
 * @date 2025-10-10
 */

#include <unity.h>
#include "mocks/MockDisplayAdapter.h"
#include "mocks/MockSystemMetrics.h"
#include "components/DisplayManager.h"
#include "types/DisplayTypes.h"

extern MockDisplayAdapter* g_mockDisplay;
extern MockSystemMetrics* g_mockMetrics;

/**
 * @brief Test: updateWiFiStatus() with CONN_CONNECTED updates display
 *
 * Regression test for bugfix-001 part 1:
 * - WiFi connects
 * - updateWiFiStatus() called with SSID and IP
 * - renderStatusPage() shows correct SSID and IP (not "Disconnected")
 */
void test_wifi_status_shows_ssid_when_connected() {
    // Arrange
    DisplayManager dm(g_mockDisplay, g_mockMetrics);
    dm.init();

    // Act: Update WiFi status to connected
    dm.updateWiFiStatus(CONN_CONNECTED, "HomeNetwork", "192.168.1.100");
    g_mockDisplay->reset();  // Clear init calls
    dm.renderStatusPage();

    // Assert: Display shows SSID and IP
    TEST_ASSERT_TRUE_MESSAGE(
        g_mockDisplay->wasTextRendered("HomeNetwork"),
        "Display should show SSID 'HomeNetwork' when WiFi connected"
    );
    TEST_ASSERT_TRUE_MESSAGE(
        g_mockDisplay->wasTextRendered("192.168.1.100"),
        "Display should show IP address when WiFi connected"
    );
    TEST_ASSERT_FALSE_MESSAGE(
        g_mockDisplay->wasTextRendered("Disconnected"),
        "Display should NOT show 'Disconnected' when WiFi is connected"
    );
}

/**
 * @brief Test: updateWiFiStatus() with CONN_DISCONNECTED clears SSID
 *
 * Regression test for bugfix-001 part 2:
 * - WiFi was connected, then disconnects
 * - updateWiFiStatus(CONN_DISCONNECTED) called
 * - renderStatusPage() shows "Disconnected" (not stale SSID)
 */
void test_wifi_status_shows_disconnected_when_not_connected() {
    // Arrange
    DisplayManager dm(g_mockDisplay, g_mockMetrics);
    dm.init();

    // Act: Update WiFi status to disconnected
    dm.updateWiFiStatus(CONN_DISCONNECTED);
    g_mockDisplay->reset();
    dm.renderStatusPage();

    // Assert: Display shows "Disconnected"
    TEST_ASSERT_TRUE_MESSAGE(
        g_mockDisplay->wasTextRendered("Disconnected"),
        "Display should show 'Disconnected' when WiFi is not connected"
    );
}

/**
 * @brief Test: updateWiFiStatus() clears SSID on disconnect
 *
 * Regression test for bugfix-001 part 3:
 * - WiFi connects (SSID stored)
 * - WiFi disconnects
 * - Internal state should clear SSID and IP
 */
void test_wifi_status_clears_ssid_on_disconnect() {
    // Arrange
    DisplayManager dm(g_mockDisplay, g_mockMetrics);
    dm.init();

    // Connect first
    dm.updateWiFiStatus(CONN_CONNECTED, "TestNet", "192.168.1.50");

    // Act: Disconnect
    dm.updateWiFiStatus(CONN_DISCONNECTED);

    // Assert: SSID and IP cleared in internal state
    SubsystemStatus status = dm.getCurrentStatus();
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "",
        status.wifiSSID,
        "SSID should be cleared when WiFi disconnects"
    );
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "",
        status.wifiIPAddress,
        "IP address should be cleared when WiFi disconnects"
    );
}

/**
 * @brief Test: updateWiFiStatus() updates internal state immediately
 *
 * Verifies state synchronization happens in updateWiFiStatus(),
 * not just on next render cycle.
 */
void test_wifi_status_updates_internal_state_immediately() {
    // Arrange
    DisplayManager dm(g_mockDisplay, g_mockMetrics);
    dm.init();

    // Act: Update WiFi status
    dm.updateWiFiStatus(CONN_CONNECTED, "QuickTest", "10.0.0.1");

    // Assert: Internal state updated immediately (before renderStatusPage)
    SubsystemStatus status = dm.getCurrentStatus();
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        CONN_CONNECTED,
        status.wifiStatus,
        "WiFi status should be CONN_CONNECTED immediately after update"
    );
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "QuickTest",
        status.wifiSSID,
        "SSID should be updated immediately"
    );
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "10.0.0.1",
        status.wifiIPAddress,
        "IP should be updated immediately"
    );
}
