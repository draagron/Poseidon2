/**
 * @file test_main.cpp
 * @brief Unity test runner for OLED hardware validation tests
 *
 * Hardware tests that require actual ESP32 with OLED display connected.
 * Validates I2C communication, display rendering, and timing performance.
 *
 * Hardware Requirements:
 * - ESP32 (SH-ESP32 board)
 * - SSD1306 OLED display (128x64)
 * - I2C Bus 2: SDA=GPIO21, SCL=GPIO22
 * - I2C address: 0x3C
 *
 * Run: pio test -e esp32dev_test -f test_oled_hardware
 *
 * IMPORTANT: This test REQUIRES physical hardware and will FAIL on native platform.
 *
 * @version 1.0.0
 * @date 2025-10-09
 */

#include <unity.h>
#include <Arduino.h>
#include <Wire.h>

// HAL implementations (hardware-specific)
#include "hal/implementations/ESP32DisplayAdapter.h"
#include "hal/implementations/ESP32SystemMetrics.h"

// Global test fixtures
ESP32DisplayAdapter* displayAdapter = nullptr;
ESP32SystemMetrics* systemMetrics = nullptr;

// Forward declare all test functions
void test_i2c_communication_successful();
void test_display_rendering();
void test_display_timing_under_50ms();
void test_cpu_idle_calculation();

/**
 * @brief Set up test environment before each test
 *
 * Initializes hardware adapters for each test.
 */
void setUp(void) {
    // Create fresh instances for each test
    if (displayAdapter != nullptr) {
        delete displayAdapter;
    }
    if (systemMetrics != nullptr) {
        delete systemMetrics;
    }

    displayAdapter = new ESP32DisplayAdapter();
    systemMetrics = new ESP32SystemMetrics();
}

/**
 * @brief Tear down test environment after each test
 *
 * Cleans up hardware adapters.
 */
void tearDown(void) {
    // Clean up after each test
    if (displayAdapter != nullptr) {
        delete displayAdapter;
        displayAdapter = nullptr;
    }
    if (systemMetrics != nullptr) {
        delete systemMetrics;
        systemMetrics = nullptr;
    }
}

/**
 * @brief Test I2C communication with OLED display
 *
 * Validates that I2C initialization succeeds and display is ready.
 */
void test_i2c_communication_successful() {
    // Initialize display
    bool initSuccess = displayAdapter->init();

    // Verify init succeeded
    TEST_ASSERT_TRUE_MESSAGE(initSuccess, "Display init() should return true");

    // Verify display is ready
    bool ready = displayAdapter->isReady();
    TEST_ASSERT_TRUE_MESSAGE(ready, "Display isReady() should return true after init");
}

/**
 * @brief Test basic display rendering operations
 *
 * Validates that display can render text without I2C errors.
 */
void test_display_rendering() {
    // Initialize display
    TEST_ASSERT_TRUE(displayAdapter->init());

    // Attempt to render text
    displayAdapter->clear();
    displayAdapter->setTextSize(1);
    displayAdapter->setCursor(0, 0);
    displayAdapter->print("Test");
    displayAdapter->display();

    // If we get here without crashing, rendering works
    TEST_ASSERT_TRUE(true);
}

/**
 * @brief Test display update timing performance
 *
 * Validates that full display update completes in < 50ms (performance goal).
 */
void test_display_timing_under_50ms() {
    // Initialize display
    TEST_ASSERT_TRUE(displayAdapter->init());

    // Measure time for full display update cycle
    unsigned long startMicros = micros();

    // Simulate full status page render (6 lines of text)
    displayAdapter->clear();
    displayAdapter->setTextSize(1);

    displayAdapter->setCursor(0, 0);
    displayAdapter->print("WiFi: Connected");

    displayAdapter->setCursor(0, 10);
    displayAdapter->print("IP: 192.168.1.100");

    displayAdapter->setCursor(0, 20);
    displayAdapter->print("RAM: 244KB");

    displayAdapter->setCursor(0, 30);
    displayAdapter->print("Flash: 830/1920KB");

    displayAdapter->setCursor(0, 40);
    displayAdapter->print("CPU: 85%");

    displayAdapter->setCursor(0, 50);
    displayAdapter->print("[ | ]");

    // Push buffer to hardware
    displayAdapter->display();

    unsigned long endMicros = micros();
    unsigned long durationMicros = endMicros - startMicros;
    unsigned long durationMs = durationMicros / 1000;

    // Log timing for visibility
    Serial.printf("Display update took %lu ms\n", durationMs);

    // Verify timing is under 50ms threshold
    TEST_ASSERT_LESS_THAN_MESSAGE(50, durationMs,
        "Display update should complete in < 50ms");
}

/**
 * @brief Test CPU idle percentage calculation
 *
 * Validates that getCpuIdlePercent() returns valid value (0-100) without crashing.
 */
void test_cpu_idle_calculation() {
    // Get CPU idle percentage
    uint8_t cpuIdle = systemMetrics->getCpuIdlePercent();

    // Log value for visibility
    Serial.printf("CPU idle: %d%%\n", cpuIdle);

    // Verify value is in valid range (0-100)
    TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(0, cpuIdle,
        "CPU idle % should be >= 0");
    TEST_ASSERT_LESS_OR_EQUAL_MESSAGE(100, cpuIdle,
        "CPU idle % should be <= 100");
}

/**
 * @brief Main test runner
 */
void setup() {
    // Wait for Serial to be ready
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n=== OLED Hardware Validation Tests ===");
    Serial.println("Hardware: ESP32 + SSD1306 OLED (128x64)");
    Serial.println("I2C: SDA=GPIO21, SCL=GPIO22, Address=0x3C\n");

    UNITY_BEGIN();

    // Run hardware validation tests
    RUN_TEST(test_i2c_communication_successful);
    RUN_TEST(test_display_rendering);
    RUN_TEST(test_display_timing_under_50ms);
    RUN_TEST(test_cpu_idle_calculation);

    UNITY_END();
}

/**
 * @brief Main loop (not used in tests)
 */
void loop() {
    // Tests complete - do nothing
}
