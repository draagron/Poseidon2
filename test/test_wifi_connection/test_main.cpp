/**
 * @file test_wifi_connection.cpp
 * @brief Hardware test for actual ESP32 WiFi connection
 *
 * IMPORTANT: This test requires actual ESP32 hardware and a real WiFi network.
 * It validates that the real WiFi hardware works correctly with our implementation.
 *
 * Constitutional principle: "Hardware-dependent tests kept minimal"
 * This is the ONLY hardware test required for WiFi functionality.
 * All other tests use mocks (see tests/unit/ and tests/integration/).
 *
 * Setup requirements:
 * 1. ESP32 hardware connected via USB
 * 2. WiFi network credentials configured (see below)
 * 3. Upload firmware to ESP32
 * 4. Monitor serial output for test results
 *
 * Test execution:
 * - platformio test -e esp32dev --filter test_wifi_connection
 * - Or upload and monitor manually via PlatformIO
 */

#include <Arduino.h>
#include <unity.h>
#include <hal/implementations/ESP32WiFiAdapter.h>
#include <hal/implementations/LittleFSAdapter.h>
#include <components/WiFiManager.h>
#include <components/WiFiConfigFile.h>
#include <components/WiFiConnectionState.h>
#include <utils/UDPLogger.h>
#include <utils/TimeoutManager.h>
#include <config.h>

// ========================================================================
// TEST CONFIGURATION - MODIFY THESE FOR YOUR TEST ENVIRONMENT
// ========================================================================

// IMPORTANT: Set these to match your actual WiFi network
// For security, consider using a dedicated test network
#define TEST_WIFI_SSID     "YourTestNetworkSSID"
#define TEST_WIFI_PASSWORD "YourTestNetworkPassword"

// Secondary network for failover testing (optional)
#define TEST_WIFI_SSID_2     "5cwifi"
#define TEST_WIFI_PASSWORD_2 "scankap99"

// Timeout for connection attempts (milliseconds)
#define TEST_TIMEOUT_MS 30000

// ========================================================================
// TEST GLOBALS
// ========================================================================

ESP32WiFiAdapter* wifiAdapter = nullptr;
LittleFSAdapter* fileSystem = nullptr;
WiFiManager* wifiManager = nullptr;
UDPLogger* logger = nullptr;
TimeoutManager* timeoutMgr = nullptr;

WiFiConfigFile config;
WiFiConnectionState state;

bool connectionSucceeded = false;
unsigned long connectionStartTime = 0;

// ========================================================================
// TEST HELPER FUNCTIONS
// ========================================================================

/**
 * @brief Setup hardware test environment
 */
void hardware_test_setup() {
    // Initialize serial for test output
    Serial.begin(115200);
    delay(2000); // Wait for serial to stabilize

    Serial.println(F("\n========================================"));
    Serial.println(F("ESP32 WiFi Hardware Test"));
    Serial.println(F("========================================"));

    // Create HAL instances (real hardware)
    wifiAdapter = new ESP32WiFiAdapter();
    fileSystem = new LittleFSAdapter();
    logger = new UDPLogger();
    timeoutMgr = new TimeoutManager();

    // Mount filesystem
    if (!fileSystem->mount()) {
        Serial.println(F("ERROR: Failed to mount LittleFS"));
        TEST_FAIL_MESSAGE("LittleFS mount failed");
        return;
    }

    // Create WiFiManager
    wifiManager = new WiFiManager(wifiAdapter, fileSystem, logger, timeoutMgr);

    // Initialize state
    state = WiFiConnectionState();
    config = WiFiConfigFile();

    Serial.println(F("Hardware test setup complete"));
}

/**
 * @brief Cleanup hardware test environment
 */
void hardware_test_cleanup() {
    if (wifiAdapter != nullptr) {
        wifiAdapter->disconnect();
        delete wifiAdapter;
        wifiAdapter = nullptr;
    }

    delete wifiManager;
    delete fileSystem;
    delete logger;
    delete timeoutMgr;

    wifiManager = nullptr;
    fileSystem = nullptr;
    logger = nullptr;
    timeoutMgr = nullptr;

    Serial.println(F("Hardware test cleanup complete"));
}

/**
 * @brief Wait for WiFi connection with timeout
 * @param maxWaitMs Maximum time to wait in milliseconds
 * @return true if connected, false if timeout
 */
bool waitForConnection(unsigned long maxWaitMs) {
    unsigned long startTime = millis();

    while (millis() - startTime < maxWaitMs) {
        WiFiStatus status = wifiAdapter->status();

        if (status == WiFiStatus::CONNECTED) {
            return true;
        }

        delay(500); // Check every 500ms
    }

    return false;
}

// ========================================================================
// HARDWARE TESTS
// ========================================================================

/**
 * Test: ESP32WiFiAdapter initialization
 *
 * Validates that the WiFi adapter can be created and is in a valid state.
 */
void test_wifi_adapter_initialization() {
    Serial.println(F("\n--- Test: WiFi Adapter Initialization ---"));

    TEST_ASSERT_NOT_NULL_MESSAGE(wifiAdapter, "WiFi adapter should be initialized");

    // Check initial status (should be IDLE or DISCONNECTED)
    WiFiStatus status = wifiAdapter->status();
    Serial.printf("Initial WiFi status: %d\n", (int)status);

    TEST_ASSERT_TRUE_MESSAGE(
        status == WiFiStatus::IDLE || status == WiFiStatus::DISCONNECTED,
        "WiFi should be IDLE or DISCONNECTED initially"
    );

    Serial.println(F("✓ WiFi adapter initialized correctly"));
}

/**
 * Test: Actual WiFi connection to real network
 *
 * This is the core hardware test - validates that:
 * 1. WiFi.begin() works with real hardware
 * 2. Connection succeeds within timeout period
 * 3. IP address is obtained
 * 4. RSSI (signal strength) can be read
 */
void test_actual_wifi_connection() {
    Serial.println(F("\n--- Test: Actual WiFi Connection ---"));

    // Create config with test credentials
    config.networks[0] = WiFiCredentials(TEST_WIFI_SSID, TEST_WIFI_PASSWORD);
    config.count = 1;

    Serial.printf("Attempting connection to: %s\n", TEST_WIFI_SSID);

    // Start connection
    state.currentNetworkIndex = 0;
    bool beginResult = wifiManager->connect(state, config);

    TEST_ASSERT_TRUE_MESSAGE(beginResult, "connect() should return true");

    Serial.println(F("Waiting for connection (max 30 seconds)..."));

    // Wait for connection
    bool connected = waitForConnection(TEST_TIMEOUT_MS);

    TEST_ASSERT_TRUE_MESSAGE(connected, "WiFi should connect within 30 seconds");

    // Verify connection details
    String ipAddress = wifiAdapter->getIPAddress();
    int rssi = wifiAdapter->getRSSI();
    String ssid = wifiAdapter->getSSID();

    Serial.printf("✓ Connected successfully!\n");
    Serial.printf("  SSID: %s\n", ssid.c_str());
    Serial.printf("  IP Address: %s\n", ipAddress.c_str());
    Serial.printf("  Signal Strength: %d dBm\n", rssi);

    // Validate IP address is not empty
    TEST_ASSERT_TRUE_MESSAGE(ipAddress.length() > 0, "IP address should be assigned");

    // Validate SSID matches
    TEST_ASSERT_EQUAL_STRING_MESSAGE(TEST_WIFI_SSID, ssid.c_str(), "SSID should match");

    // Validate RSSI is in valid range (-100 to 0 dBm)
    TEST_ASSERT_TRUE_MESSAGE(rssi >= -100 && rssi <= 0, "RSSI should be in valid range");
}

/**
 * Test: WiFi disconnect and reconnect
 *
 * Validates that:
 * 1. Disconnect works
 * 2. Reconnection works
 * 3. Status transitions correctly
 */
void test_wifi_disconnect_reconnect() {
    Serial.println(F("\n--- Test: Disconnect and Reconnect ---"));

    // Ensure connected first
    if (wifiAdapter->status() != WiFiStatus::CONNECTED) {
        Serial.println(F("Establishing initial connection..."));
        config.networks[0] = WiFiCredentials(TEST_WIFI_SSID, TEST_WIFI_PASSWORD);
        config.count = 1;
        wifiManager->connect(state, config);
        waitForConnection(TEST_TIMEOUT_MS);
    }

    TEST_ASSERT_EQUAL_MESSAGE(WiFiStatus::CONNECTED, wifiAdapter->status(),
                              "Should be connected before disconnect test");

    // Disconnect
    Serial.println(F("Disconnecting..."));
    bool disconnectResult = wifiAdapter->disconnect();
    TEST_ASSERT_TRUE_MESSAGE(disconnectResult, "disconnect() should return true");

    delay(2000); // Wait for disconnect to complete

    WiFiStatus statusAfterDisconnect = wifiAdapter->status();
    Serial.printf("Status after disconnect: %d\n", (int)statusAfterDisconnect);

    TEST_ASSERT_NOT_EQUAL_MESSAGE(WiFiStatus::CONNECTED, statusAfterDisconnect,
                                   "Should not be connected after disconnect");

    // Reconnect
    Serial.println(F("Reconnecting..."));
    wifiManager->connect(state, config);
    bool reconnected = waitForConnection(TEST_TIMEOUT_MS);

    TEST_ASSERT_TRUE_MESSAGE(reconnected, "Should reconnect successfully");

    Serial.println(F("✓ Disconnect and reconnect successful"));
}

/**
 * Test: Connection timeout behavior
 *
 * Validates that connection to non-existent network times out correctly.
 * Uses an invalid SSID that should not exist.
 */
void test_connection_timeout() {
    Serial.println(F("\n--- Test: Connection Timeout ---"));

    // Disconnect from current network
    wifiAdapter->disconnect();
    delay(2000);

    // Create config with non-existent network
    WiFiConfigFile timeoutConfig;
    timeoutConfig.networks[0] = WiFiCredentials("NonExistentNetwork_XYZ123", "password123");
    timeoutConfig.count = 1;

    Serial.println(F("Attempting connection to non-existent network..."));

    // Start connection attempt
    WiFiConnectionState timeoutState;
    timeoutState.currentNetworkIndex = 0;
    wifiManager->connect(timeoutState, timeoutConfig);

    unsigned long startTime = millis();

    // Wait for timeout (should not connect)
    bool connected = waitForConnection(TEST_TIMEOUT_MS);

    unsigned long elapsedTime = millis() - startTime;

    Serial.printf("Elapsed time: %lu ms\n", elapsedTime);

    TEST_ASSERT_FALSE_MESSAGE(connected, "Should NOT connect to non-existent network");

    // Verify timeout occurred (should be around 30 seconds)
    TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(25000, elapsedTime,
                                         "Should timeout after ~30 seconds");

    Serial.println(F("✓ Timeout behavior correct"));

    // Reconnect to valid network for subsequent tests
    Serial.println(F("Reconnecting to valid network..."));
    config.networks[0] = WiFiCredentials(TEST_WIFI_SSID, TEST_WIFI_PASSWORD);
    config.count = 1;
    wifiManager->connect(state, config);
    waitForConnection(TEST_TIMEOUT_MS);
}

/**
 * Test: LittleFS filesystem operations
 *
 * Validates that file operations work on actual hardware.
 */
void test_littlefs_operations() {
    Serial.println(F("\n--- Test: LittleFS Operations ---"));

    // Test write
    const char* testPath = "/hardware_test.txt";
    const char* testContent = "ESP32 hardware test data";

    bool writeResult = fileSystem->writeFile(testPath, testContent);
    TEST_ASSERT_TRUE_MESSAGE(writeResult, "File write should succeed");

    // Test exists
    bool exists = fileSystem->exists(testPath);
    TEST_ASSERT_TRUE_MESSAGE(exists, "File should exist after write");

    // Test read
    String readContent = fileSystem->readFile(testPath);
    TEST_ASSERT_EQUAL_STRING_MESSAGE(testContent, readContent.c_str(),
                                     "Read content should match written content");

    // Test delete
    bool deleteResult = fileSystem->deleteFile(testPath);
    TEST_ASSERT_TRUE_MESSAGE(deleteResult, "File delete should succeed");

    // Verify deleted
    exists = fileSystem->exists(testPath);
    TEST_ASSERT_FALSE_MESSAGE(exists, "File should not exist after delete");

    Serial.println(F("✓ LittleFS operations successful"));
}

/**
 * Test: WiFi config persistence
 *
 * Validates that WiFi configuration can be saved and loaded from flash.
 */
void test_config_persistence() {
    Serial.println(F("\n--- Test: Config Persistence ---"));

    // Create test config
    WiFiConfigFile saveConfig;
    saveConfig.networks[0] = WiFiCredentials("TestNet1", "password1");
    saveConfig.networks[1] = WiFiCredentials("TestNet2", "password2");
    saveConfig.count = 2;

    // Save
    bool saveResult = wifiManager->saveConfig(saveConfig);
    TEST_ASSERT_TRUE_MESSAGE(saveResult, "Config save should succeed");

    // Load
    WiFiConfigFile loadConfig;
    bool loadResult = wifiManager->loadConfig(loadConfig);
    TEST_ASSERT_TRUE_MESSAGE(loadResult, "Config load should succeed");

    // Verify
    TEST_ASSERT_EQUAL_MESSAGE(2, loadConfig.count, "Should load 2 networks");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("TestNet1", loadConfig.networks[0].ssid.c_str(),
                                     "First SSID should match");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("TestNet2", loadConfig.networks[1].ssid.c_str(),
                                     "Second SSID should match");

    Serial.println(F("✓ Config persistence successful"));
}

// ========================================================================
// TEST RUNNER
// ========================================================================

/**
 * @brief Main test setup (called by Unity framework)
 */
void setUp(void) {
    // Called before each test
}

/**
 * @brief Main test teardown (called by Unity framework)
 */
void tearDown(void) {
    // Called after each test
}

/**
 * @brief Main test execution
 */
void setup() {
    // Initialize Unity test framework
    UNITY_BEGIN();

    // Setup hardware
    hardware_test_setup();

    // Run tests in sequence
    RUN_TEST(test_wifi_adapter_initialization);
    RUN_TEST(test_actual_wifi_connection);
    RUN_TEST(test_wifi_disconnect_reconnect);
    RUN_TEST(test_littlefs_operations);
    RUN_TEST(test_config_persistence);
    RUN_TEST(test_connection_timeout);

    // Cleanup
    hardware_test_cleanup();

    // Finish Unity tests
    UNITY_END();
}

/**
 * @brief Main loop (required by Arduino framework, unused for tests)
 */
void loop() {
    // Tests run once in setup()
    delay(1000);
}
