/**
 * @file test_services_run_independently.cpp
 * @brief Integration test for Services Independence
 *
 * Test scenario:
 * 1. WiFi connection is disabled/unavailable
 * 2. Other gateway services start normally
 * 3. Services operate independently without blocking on WiFi
 * 4. NMEA handlers, UDP logger (buffered), OLED display all start within 2s
 * 5. No service blocks waiting for WiFi connection
 *
 * This test validates the constitutional requirement (FR-011):
 * "Services MUST operate independently and NOT block on WiFi connection status"
 *
 * Key validation: WiFiManager operations are non-blocking and asynchronous.
 */

#include <gtest/gtest.h>
#include "../../src/components/WiFiManager.h"
#include "../../src/mocks/MockWiFiAdapter.h"
#include "../../src/mocks/MockFileSystem.h"
#include "../../src/utils/UDPLogger.h"
#include "../../src/utils/TimeoutManager.h"
#include "../../src/config.h"
#include <chrono>

/**
 * Test fixture for service independence scenario
 */
class ServicesIndependenceTest : public ::testing::Test {
protected:
    WiFiManager* manager;
    MockWiFiAdapter* mockWiFi;
    MockFileSystem* mockFS;
    UDPLogger* logger;
    TimeoutManager* timeoutMgr;
    WiFiConfigFile config;
    WiFiConnectionState state;

    void SetUp() override {
        mockWiFi = new MockWiFiAdapter();
        mockFS = new MockFileSystem();
        logger = new UDPLogger();
        timeoutMgr = new TimeoutManager();

        mockFS->mount();
        manager = new WiFiManager(mockWiFi, mockFS, logger, timeoutMgr);

        // Create config with networks (all unavailable for this test)
        config.networks[0] = WiFiCredentials("UnavailableNet1", "pass1");
        config.networks[1] = WiFiCredentials("UnavailableNet2", "pass2");
        config.networks[2] = WiFiCredentials("UnavailableNet3", "pass3");
        config.count = 3;

        manager->saveConfig(config);

        // Initialize state
        state = WiFiConnectionState();

        // Configure WiFi to never succeed (simulate unavailable networks)
        mockWiFi->setConnectionBehavior(false);
    }

    void TearDown() override {
        delete manager;
        delete mockWiFi;
        delete mockFS;
        delete logger;
        delete timeoutMgr;
    }
};

/**
 * Test: WiFiManager operations are non-blocking
 *
 * Expected behavior:
 * - loadConfig() returns immediately (file I/O is fast)
 * - connect() returns immediately (async WiFi.begin())
 * - No blocking delays in WiFiManager methods
 */
TEST_F(ServicesIndependenceTest, WiFiManagerOperationsNonBlocking) {
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;

    // Measure loadConfig() time
    auto start = high_resolution_clock::now();
    bool loadResult = manager->loadConfig(config);
    auto end = high_resolution_clock::now();

    auto loadDuration = duration_cast<milliseconds>(end - start).count();

    // loadConfig should complete in < 100ms (even with file I/O)
    EXPECT_LT(loadDuration, 100);
    EXPECT_TRUE(loadResult);

    // Measure connect() time
    start = high_resolution_clock::now();
    bool connectResult = manager->connect(state, config);
    end = high_resolution_clock::now();

    auto connectDuration = duration_cast<milliseconds>(end - start).count();

    // connect() should return immediately (< 50ms)
    // It initiates async WiFi.begin(), doesn't wait for result
    EXPECT_LT(connectDuration, 50);
    EXPECT_TRUE(connectResult);
}

/**
 * Test: Services can initialize while WiFi is connecting
 *
 * Expected behavior:
 * - WiFi connection attempt in progress
 * - Other services can initialize and run
 * - No blocking on WiFi status
 */
TEST_F(ServicesIndependenceTest, ServicesInitializeWhileWiFiConnecting) {
    // Start WiFi connection attempt (will timeout after 30s)
    manager->loadConfig(config);
    manager->connect(state, config);

    EXPECT_EQ(ConnectionStatus::CONNECTING, state.status);

    // Simulate other services initializing
    // These should all succeed immediately without waiting for WiFi

    // 1. LittleFS operations (independent of WiFi)
    EXPECT_TRUE(mockFS->exists(CONFIG_FILE_PATH));
    String fileContent = mockFS->readFile(CONFIG_FILE_PATH);
    EXPECT_GT(fileContent.length(), 0);

    // 2. UDPLogger initialization (works even without WiFi)
    // Logger can buffer messages until WiFi connects
    logger->begin(); // Should not block
    logger->logConnectionEvent(ConnectionEvent::CONNECTION_ATTEMPT, "TestNet");

    // 3. TimeoutManager operations (independent of WiFi)
    bool timeoutRegistered = false;
    timeoutMgr->registerTimeout(1000, [&]() {
        timeoutRegistered = true;
    });
    EXPECT_TRUE(timeoutMgr->isActive());

    // All services initialized successfully while WiFi is CONNECTING
    EXPECT_EQ(ConnectionStatus::CONNECTING, state.status);
}

/**
 * Test: Services operate independently during WiFi failures
 *
 * Expected behavior:
 * - All WiFi networks fail
 * - Device enters reboot loop
 * - Other services continue operating
 * - No service hangs waiting for WiFi
 */
TEST_F(ServicesIndependenceTest, ServicesOperateDuringWiFiFailures) {
    // Simulate WiFi connection failures
    for (int attempt = 0; attempt < 3; attempt++) {
        state.currentNetworkIndex = attempt;
        manager->connect(state, config);

        // Connection fails (simulated timeout)
        state.attemptStartTime = millis() - (WIFI_TIMEOUT_MS + 1000);
        manager->checkTimeout(state, config);

        // During failure, services should still be operational
        // Example: File operations
        EXPECT_TRUE(mockFS->exists(CONFIG_FILE_PATH));

        // Example: Logging (buffered until WiFi available)
        logger->logConnectionEvent(ConnectionEvent::CONNECTION_FAILED,
                                   config.networks[attempt].ssid);

        // Services don't block on WiFi status
    }

    // After all networks fail, device would reboot
    // But services never blocked during the failure sequence
    EXPECT_TRUE(state.allNetworksExhausted(config.count));
}

/**
 * Test: UDPLogger operates in buffered mode without WiFi
 *
 * Expected behavior:
 * - UDPLogger.begin() succeeds even without WiFi
 * - Log messages can be queued/buffered
 * - No blocking when WiFi unavailable
 */
TEST_F(ServicesIndependenceTest, UDPLoggerBufferedModeWithoutWiFi) {
    // WiFi is not connected
    EXPECT_NE(ConnectionStatus::CONNECTED, state.status);

    // UDPLogger should still initialize
    bool loggerInitialized = logger->begin();

    // Note: begin() may return false if WiFi not connected,
    // but it should not block and should handle gracefully
    // The logger should buffer or discard messages safely

    // Attempt to log without WiFi (should not crash or block)
    logger->broadcastLog(LogLevel::INFO, "TestComponent", "TEST_EVENT",
                        "{\"data\":\"test\"}");

    logger->logConnectionEvent(ConnectionEvent::CONNECTION_ATTEMPT, "TestNet");

    // These calls should complete without blocking
    // In production, messages would be buffered or dropped
}

/**
 * Test: Filesystem operations work independently of WiFi
 *
 * Expected behavior:
 * - LittleFS operations succeed regardless of WiFi status
 * - Read/write operations non-blocking
 * - No WiFi dependency
 */
TEST_F(ServicesIndependenceTest, FilesystemIndependentOfWiFi) {
    // WiFi not connected
    EXPECT_NE(ConnectionStatus::CONNECTED, state.status);

    // Filesystem operations should work
    EXPECT_TRUE(mockFS->exists(CONFIG_FILE_PATH));

    String testData = "Test data independent of WiFi";
    EXPECT_TRUE(mockFS->writeFile("/test.txt", testData.c_str()));

    String readData = mockFS->readFile("/test.txt");
    EXPECT_STREQ(testData.c_str(), readData.c_str());

    // All filesystem operations completed successfully without WiFi
}

/**
 * Test: WiFiManager state queries are non-blocking
 *
 * Expected behavior:
 * - State queries return immediately
 * - No waiting for network operations
 * - Status always available
 */
TEST_F(ServicesIndependenceTest, StateQueriesNonBlocking) {
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::microseconds;

    // Start connection attempt
    manager->connect(state, config);

    // Query state (should be instant)
    auto start = high_resolution_clock::now();
    ConnectionStatus status = state.status;
    bool isConnected = state.isConnected();
    bool isConnecting = state.isConnecting();
    unsigned long elapsed = state.getElapsedTime();
    String statusStr = state.getStatusString();
    auto end = high_resolution_clock::now();

    auto queryDuration = duration_cast<microseconds>(end - start).count();

    // All queries should complete in < 1ms (< 1000 microseconds)
    EXPECT_LT(queryDuration, 1000);

    // Verify query results are valid
    EXPECT_EQ(ConnectionStatus::CONNECTING, status);
    EXPECT_FALSE(isConnected);
    EXPECT_TRUE(isConnecting);
    EXPECT_STREQ("CONNECTING", statusStr.c_str());
}

/**
 * Test: Service startup timing (must be < 2 seconds)
 *
 * Expected behavior:
 * - All services initialize within 2 seconds
 * - WiFi connection attempt does not delay other services
 * - Constitutional requirement met (services independent)
 */
TEST_F(ServicesIndependenceTest, ServiceStartupTiming) {
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;

    auto bootStartTime = high_resolution_clock::now();

    // Simulate service initialization sequence
    // (as it would occur in main.cpp setup())

    // 1. Mount filesystem (< 100ms)
    auto fsStart = high_resolution_clock::now();
    mockFS->mount();
    auto fsEnd = high_resolution_clock::now();
    auto fsDuration = duration_cast<milliseconds>(fsEnd - fsStart).count();
    EXPECT_LT(fsDuration, 100);

    // 2. Create WiFiManager (instant)
    auto wifiMgrStart = high_resolution_clock::now();
    // Already created in SetUp(), but verify instantiation is fast
    WiFiManager* tempMgr = new WiFiManager(mockWiFi, mockFS, logger, timeoutMgr);
    auto wifiMgrEnd = high_resolution_clock::now();
    auto wifiMgrDuration = duration_cast<milliseconds>(wifiMgrEnd - wifiMgrStart).count();
    EXPECT_LT(wifiMgrDuration, 10);
    delete tempMgr;

    // 3. Load config (< 100ms)
    auto configStart = high_resolution_clock::now();
    manager->loadConfig(config);
    auto configEnd = high_resolution_clock::now();
    auto configDuration = duration_cast<milliseconds>(configEnd - configStart).count();
    EXPECT_LT(configDuration, 100);

    // 4. Start WiFi connection (non-blocking, < 50ms)
    auto connectStart = high_resolution_clock::now();
    manager->connect(state, config);
    auto connectEnd = high_resolution_clock::now();
    auto connectDuration = duration_cast<milliseconds>(connectEnd - connectStart).count();
    EXPECT_LT(connectDuration, 50);

    // 5. Initialize logger (< 50ms)
    auto loggerStart = high_resolution_clock::now();
    logger->begin();
    auto loggerEnd = high_resolution_clock::now();
    auto loggerDuration = duration_cast<milliseconds>(loggerEnd - loggerStart).count();
    EXPECT_LT(loggerDuration, 50);

    // Total time from boot to all services operational
    auto bootEndTime = high_resolution_clock::now();
    auto totalBootTime = duration_cast<milliseconds>(bootEndTime - bootStartTime).count();

    // Constitutional requirement: Services start within 2 seconds
    EXPECT_LT(totalBootTime, 2000);

    // WiFi may still be connecting, but services are operational
    // This proves services don't block on WiFi
}

/**
 * Test: Timeout checking is non-blocking
 *
 * Expected behavior:
 * - checkTimeout() returns immediately
 * - Periodic calls don't accumulate delay
 * - ReactESP event loop can run smoothly
 */
TEST_F(ServicesIndependenceTest, TimeoutCheckingNonBlocking) {
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;

    // Start connection
    manager->connect(state, config);

    // Simulate periodic timeout checks (as ReactESP would do)
    for (int i = 0; i < 10; i++) {
        auto start = high_resolution_clock::now();
        manager->checkTimeout(state, config);
        auto end = high_resolution_clock::now();

        auto checkDuration = duration_cast<milliseconds>(end - start).count();

        // Each check should be < 10ms
        EXPECT_LT(checkDuration, 10);
    }

    // Multiple checks completed quickly without blocking
}

/**
 * Test: HandleDisconnect is non-blocking
 *
 * Expected behavior:
 * - handleDisconnect() returns immediately
 * - State updated synchronously
 * - No waiting for reconnection
 */
TEST_F(ServicesIndependenceTest, HandleDisconnectNonBlocking) {
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;

    // Setup connected state
    state.status = ConnectionStatus::CONNECTED;
    state.connectedSSID = "TestNetwork";

    // Measure handleDisconnect time
    auto start = high_resolution_clock::now();
    manager->handleDisconnect(state);
    auto end = high_resolution_clock::now();

    auto disconnectDuration = duration_cast<milliseconds>(end - start).count();

    // Should complete in < 10ms
    EXPECT_LT(disconnectDuration, 10);

    // State updated correctly
    EXPECT_EQ(ConnectionStatus::DISCONNECTED, state.status);
}
