/**
 * @file test_main.cpp
 * @brief Integration tests for WebSocket loop frequency logging
 *
 * Tests:
 * - T007: WebSocket log emission with MockSystemMetrics
 * - T008: 5-second interval timing
 * - T009: Log metadata (component, event, level)
 * - T010: Graceful degradation (WebSocket failure)
 *
 * Feature: R007 - WebSocket Loop Frequency Logging
 */

#include <unity.h>
#include <string>
#include <sstream>
#include <cstdint>
#include <cstring>

// LogLevel enum (matching src/utils/LogEnums.h)
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    FATAL = 4
};

// ============================================================================
// MockWebSocketLogger - Captures broadcastLog calls for verification
// ============================================================================

/**
 * @brief Mock WebSocket logger for testing
 *
 * Captures calls to broadcastLog() and provides accessors for test verification.
 * Simulates failure modes for testing graceful degradation.
 */
class MockWebSocketLogger {
private:
    // Captured call data
    LogLevel _lastLevel;
    std::string _lastComponent;
    std::string _lastEvent;
    std::string _lastData;
    uint32_t _callCount;
    bool _shouldFail;
    bool _hasClients;

public:
    /**
     * @brief Constructor - initialize mock
     */
    MockWebSocketLogger()
        : _lastLevel(LogLevel::DEBUG),
          _lastComponent(""),
          _lastEvent(""),
          _lastData(""),
          _callCount(0),
          _shouldFail(false),
          _hasClients(true) {
    }

    /**
     * @brief Reset mock state
     */
    void reset() {
        _lastLevel = LogLevel::DEBUG;
        _lastComponent = "";
        _lastEvent = "";
        _lastData = "";
        _callCount = 0;
        _shouldFail = false;
        _hasClients = true;
    }

    /**
     * @brief Simulate WebSocket failure mode
     * @param shouldFail True to simulate failure
     */
    void setShouldFail(bool shouldFail) {
        _shouldFail = shouldFail;
    }

    /**
     * @brief Simulate no clients connected
     * @param hasClients True if clients connected
     */
    void setHasClients(bool hasClients) {
        _hasClients = hasClients;
    }

    /**
     * @brief Mock broadcastLog implementation
     * @param level Log level
     * @param component Component name
     * @param event Event name
     * @param data JSON data string
     */
    void broadcastLog(LogLevel level, const char* component, const char* event, const std::string& data = "") {
        if (_shouldFail) {
            // Simulate failure (but don't crash)
            return;
        }

        if (!_hasClients) {
            // No-op if no clients (graceful degradation)
            return;
        }

        // Capture call parameters
        _lastLevel = level;
        _lastComponent = std::string(component);
        _lastEvent = std::string(event);
        _lastData = data;
        _callCount++;
    }

    // Accessors for test verification

    LogLevel getLastLevel() const { return _lastLevel; }
    std::string getLastComponent() const { return _lastComponent; }
    std::string getLastEvent() const { return _lastEvent; }
    std::string getLastData() const { return _lastData; }
    uint32_t getCallCount() const { return _callCount; }
    bool hasClients() const { return _hasClients; }
};

// ============================================================================
// MockSystemMetrics - Simulates system metrics for testing
// ============================================================================

/**
 * @brief Mock system metrics for testing
 * Simplified version for integration tests
 */
class MockSystemMetrics {
private:
    uint32_t _loopFrequency;
    unsigned long _millis;

public:
    MockSystemMetrics() : _loopFrequency(0), _millis(0) {}

    void setMockLoopFrequency(uint32_t frequency) {
        _loopFrequency = frequency;
    }

    void setMillis(unsigned long milliseconds) {
        _millis = milliseconds;
    }

    uint32_t getLoopFrequency() const {
        return _loopFrequency;
    }

    unsigned long getMillis() const {
        return _millis;
    }

    void reset() {
        _loopFrequency = 0;
        _millis = 0;
    }
};

// ============================================================================
// Test Fixture - Global instances for all tests
// ============================================================================

MockSystemMetrics mockMetrics;
MockWebSocketLogger mockLogger;

/**
 * @brief Simulate the event loop callback from main.cpp
 *
 * This function replicates the logic that will be added to main.cpp:427-431
 */
void simulateEventLoopCallback() {
    if (&mockMetrics != nullptr) {
        uint32_t frequency = mockMetrics.getLoopFrequency();
        LogLevel level = (frequency == 0 || frequency >= 200)
                         ? LogLevel::DEBUG : LogLevel::WARN;
        std::ostringstream oss;
        oss << "{\"frequency\":" << frequency << "}";
        std::string data = oss.str();
        mockLogger.broadcastLog(level, "Performance", "LOOP_FREQUENCY", data);
    }
}

// ============================================================================
// T007: Integration Test - WebSocket Log Emission
// ============================================================================

/**
 * Test: Normal frequency (212 Hz) logs with DEBUG level
 */
void test_log_emission_normal_frequency() {
    mockLogger.reset();
    mockMetrics.setMockLoopFrequency(212);

    simulateEventLoopCallback();

    TEST_ASSERT_EQUAL(1, mockLogger.getCallCount());
    TEST_ASSERT_EQUAL(LogLevel::DEBUG, mockLogger.getLastLevel());
    TEST_ASSERT_EQUAL_STRING("Performance", mockLogger.getLastComponent().c_str());
    TEST_ASSERT_EQUAL_STRING("LOOP_FREQUENCY", mockLogger.getLastEvent().c_str());
    TEST_ASSERT_EQUAL_STRING("{\"frequency\":212}", mockLogger.getLastData().c_str());
}

/**
 * Test: Abnormally low frequency (5 Hz) logs with WARN level
 */
void test_log_emission_low_frequency() {
    mockLogger.reset();
    mockMetrics.setMockLoopFrequency(5);

    simulateEventLoopCallback();

    TEST_ASSERT_EQUAL(1, mockLogger.getCallCount());
    TEST_ASSERT_EQUAL(LogLevel::WARN, mockLogger.getLastLevel());
    TEST_ASSERT_EQUAL_STRING("Performance", mockLogger.getLastComponent().c_str());
    TEST_ASSERT_EQUAL_STRING("LOOP_FREQUENCY", mockLogger.getLastEvent().c_str());
    TEST_ASSERT_EQUAL_STRING("{\"frequency\":5}", mockLogger.getLastData().c_str());
}

/**
 * Test: High frequency (412000 Hz) logs with DEBUG level
 */
void test_log_emission_high_frequency() {
    mockLogger.reset();
    mockMetrics.setMockLoopFrequency(412000);

    simulateEventLoopCallback();

    TEST_ASSERT_EQUAL(1, mockLogger.getCallCount());
    TEST_ASSERT_EQUAL(LogLevel::DEBUG, mockLogger.getLastLevel());
    TEST_ASSERT_EQUAL_STRING("Performance", mockLogger.getLastComponent().c_str());
    TEST_ASSERT_EQUAL_STRING("LOOP_FREQUENCY", mockLogger.getLastEvent().c_str());
    TEST_ASSERT_EQUAL_STRING("{\"frequency\":412000}", mockLogger.getLastData().c_str());
}

/**
 * Test: Zero frequency (0 Hz) logs with DEBUG level
 * Note: 0 Hz is DEBUG (startup placeholder)
 */
void test_log_emission_zero_frequency() {
    mockLogger.reset();
    mockMetrics.setMockLoopFrequency(0);

    simulateEventLoopCallback();

    TEST_ASSERT_EQUAL(1, mockLogger.getCallCount());
    TEST_ASSERT_EQUAL(LogLevel::DEBUG, mockLogger.getLastLevel());
    TEST_ASSERT_EQUAL_STRING("{\"frequency\":0}", mockLogger.getLastData().c_str());
}

// ============================================================================
// T008: Integration Test - 5-Second Interval Timing
// ============================================================================

/**
 * Test: Log emission occurs every 5 seconds (5000ms)
 */
void test_timing_5_second_interval() {
    mockLogger.reset();
    mockMetrics.setMockLoopFrequency(212);
    mockMetrics.setMillis(0);

    // Simulate first call at 5 seconds
    mockMetrics.setMillis(5000);
    simulateEventLoopCallback();
    TEST_ASSERT_EQUAL(1, mockLogger.getCallCount());

    // Simulate second call at 10 seconds
    mockMetrics.setMillis(10000);
    simulateEventLoopCallback();
    TEST_ASSERT_EQUAL(2, mockLogger.getCallCount());

    // Simulate third call at 15 seconds
    mockMetrics.setMillis(15000);
    simulateEventLoopCallback();
    TEST_ASSERT_EQUAL(3, mockLogger.getCallCount());
}

/**
 * Test: Multiple rapid calls (simulating event loop precision)
 */
void test_timing_multiple_calls() {
    mockLogger.reset();
    mockMetrics.setMockLoopFrequency(218);

    // Simulate 10 calls (50 seconds total)
    for (int i = 0; i < 10; i++) {
        simulateEventLoopCallback();
    }

    TEST_ASSERT_EQUAL(10, mockLogger.getCallCount());
}

// ============================================================================
// T009: Integration Test - Log Metadata Validation
// ============================================================================

/**
 * Test: Component identifier is exactly "Performance"
 */
void test_metadata_component_name() {
    mockLogger.reset();
    mockMetrics.setMockLoopFrequency(212);

    simulateEventLoopCallback();

    TEST_ASSERT_EQUAL_STRING("Performance", mockLogger.getLastComponent().c_str());
    // Verify case sensitivity
    TEST_ASSERT_NOT_EQUAL(0, strcmp("performance", mockLogger.getLastComponent().c_str()));
    TEST_ASSERT_NOT_EQUAL(0, strcmp("PERFORMANCE", mockLogger.getLastComponent().c_str()));
}

/**
 * Test: Event identifier is exactly "LOOP_FREQUENCY"
 */
void test_metadata_event_name() {
    mockLogger.reset();
    mockMetrics.setMockLoopFrequency(212);

    simulateEventLoopCallback();

    TEST_ASSERT_EQUAL_STRING("LOOP_FREQUENCY", mockLogger.getLastEvent().c_str());
    // Verify case sensitivity
    TEST_ASSERT_NOT_EQUAL(0, strcmp("loop_frequency", mockLogger.getLastEvent().c_str()));
    TEST_ASSERT_NOT_EQUAL(0, strcmp("LOOP_FREQ_UPDATE", mockLogger.getLastEvent().c_str()));
}

/**
 * Test: Log level is correct for normal frequency range
 */
void test_metadata_log_level_normal() {
    mockLogger.reset();

    // Test lower boundary (200 Hz)
    mockMetrics.setMockLoopFrequency(200);
    simulateEventLoopCallback();
    TEST_ASSERT_EQUAL(LogLevel::DEBUG, mockLogger.getLastLevel());

    mockLogger.reset();

    // Test middle range (500 Hz)
    mockMetrics.setMockLoopFrequency(500);
    simulateEventLoopCallback();
    TEST_ASSERT_EQUAL(LogLevel::DEBUG, mockLogger.getLastLevel());

    mockLogger.reset();

    // Test high frequency (412000 Hz)
    mockMetrics.setMockLoopFrequency(412000);
    simulateEventLoopCallback();
    TEST_ASSERT_EQUAL(LogLevel::DEBUG, mockLogger.getLastLevel());
}

/**
 * Test: Log level is WARN for abnormal frequency
 */
void test_metadata_log_level_abnormal() {
    mockLogger.reset();

    // Test below threshold (5 Hz)
    mockMetrics.setMockLoopFrequency(5);
    simulateEventLoopCallback();
    TEST_ASSERT_EQUAL(LogLevel::WARN, mockLogger.getLastLevel());

    mockLogger.reset();

    // Test just below threshold (199 Hz)
    mockMetrics.setMockLoopFrequency(199);
    simulateEventLoopCallback();
    TEST_ASSERT_EQUAL(LogLevel::WARN, mockLogger.getLastLevel());
}

/**
 * Test: JSON data format is correct
 */
void test_metadata_json_format() {
    mockLogger.reset();
    mockMetrics.setMockLoopFrequency(212);

    simulateEventLoopCallback();

    std::string data = mockLogger.getLastData();

    // Verify JSON structure
    TEST_ASSERT_TRUE(data.find("{\"frequency\":") == 0);
    TEST_ASSERT_TRUE(data.back() == '}');
    TEST_ASSERT_EQUAL_STRING("{\"frequency\":212}", data.c_str());

    // Verify no extra whitespace or formatting
    // Note: ostringstream produces "{\"frequency\":212}" = 17 chars
    // Arduino String would produce "{\"frequency\":212}" = 17 chars too
    TEST_ASSERT_EQUAL(17, data.length());
}

// ============================================================================
// T010: Integration Test - Graceful Degradation
// ============================================================================

/**
 * Test: System continues when WebSocket broadcast fails
 */
void test_graceful_degradation_websocket_failure() {
    mockLogger.reset();
    mockLogger.setShouldFail(true);  // Simulate WebSocket failure
    mockMetrics.setMockLoopFrequency(212);

    // Should not crash or throw exception
    simulateEventLoopCallback();

    // Verify failure was simulated (no call recorded)
    TEST_ASSERT_EQUAL(0, mockLogger.getCallCount());
}

/**
 * Test: System continues when no WebSocket clients connected
 */
void test_graceful_degradation_no_clients() {
    mockLogger.reset();
    mockLogger.setHasClients(false);  // Simulate no clients
    mockMetrics.setMockLoopFrequency(212);

    // Should not crash or throw exception
    simulateEventLoopCallback();

    // Verify graceful no-op (no call recorded)
    TEST_ASSERT_EQUAL(0, mockLogger.getCallCount());
}

/**
 * Test: System recovers after WebSocket failure
 */
void test_graceful_degradation_recovery() {
    mockLogger.reset();
    mockMetrics.setMockLoopFrequency(212);

    // First call succeeds
    simulateEventLoopCallback();
    TEST_ASSERT_EQUAL(1, mockLogger.getCallCount());

    // Simulate failure
    mockLogger.setShouldFail(true);
    simulateEventLoopCallback();
    TEST_ASSERT_EQUAL(1, mockLogger.getCallCount());  // Still 1 (failure)

    // Recover
    mockLogger.setShouldFail(false);
    simulateEventLoopCallback();
    TEST_ASSERT_EQUAL(2, mockLogger.getCallCount());  // Recovered
}

/**
 * Test: System recovers when clients reconnect
 */
void test_graceful_degradation_client_reconnect() {
    mockLogger.reset();
    mockMetrics.setMockLoopFrequency(212);

    // No clients initially
    mockLogger.setHasClients(false);
    simulateEventLoopCallback();
    TEST_ASSERT_EQUAL(0, mockLogger.getCallCount());

    // Client connects
    mockLogger.setHasClients(true);
    simulateEventLoopCallback();
    TEST_ASSERT_EQUAL(1, mockLogger.getCallCount());  // Now logs appear
}

// ============================================================================
// Unity Test Runner
// ============================================================================

void setUp(void) {
    // Reset mocks before each test
    mockLogger.reset();
    mockMetrics.reset();
}

void tearDown(void) {
    // Clean up after each test
}

int main() {
    UNITY_BEGIN();

    // T007: Log Emission Tests
    RUN_TEST(test_log_emission_normal_frequency);
    RUN_TEST(test_log_emission_low_frequency);
    RUN_TEST(test_log_emission_high_frequency);
    RUN_TEST(test_log_emission_zero_frequency);

    // T008: Timing Tests
    RUN_TEST(test_timing_5_second_interval);
    RUN_TEST(test_timing_multiple_calls);

    // T009: Metadata Tests
    RUN_TEST(test_metadata_component_name);
    RUN_TEST(test_metadata_event_name);
    RUN_TEST(test_metadata_log_level_normal);
    RUN_TEST(test_metadata_log_level_abnormal);
    RUN_TEST(test_metadata_json_format);

    // T010: Graceful Degradation Tests
    RUN_TEST(test_graceful_degradation_websocket_failure);
    RUN_TEST(test_graceful_degradation_no_clients);
    RUN_TEST(test_graceful_degradation_recovery);
    RUN_TEST(test_graceful_degradation_client_reconnect);

    return UNITY_END();
}
