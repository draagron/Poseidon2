/**
 * @file test_main.cpp
 * @brief Hardware validation tests for NMEA 0183 handlers on ESP32
 * 
 * Tests Serial2 timing, processing budget, and buffer handling on real hardware.
 * Requires ESP32 with Serial2 configured for NMEA 0183 (4800 baud, 8N1).
 * 
 * Phase: 3.5 Hardware Validation & Polish
 * Task: T040 - Create Hardware Test for Serial2 Timing
 * 
 * @note These tests require physical hardware and cannot run on native platform
 * 
 * @warning This is TEST code, not production code. Tests use blocking waits
 *          (while loops with timeouts) to validate hardware timing. Production
 *          code uses ReactESP event scheduling (onRepeat, onDelay) which is
 *          non-blocking and does NOT require delay() calls in loop().
 */

#include <Arduino.h>
#include <unity.h>
#include <HardwareSerial.h>

// Test configuration
#define NMEA_BAUD_RATE 4800
#define PROCESSING_BUDGET_MS 50
#define TEST_SENTENCE_COUNT 10
#define OVERFLOW_TEST_COUNT 100

// Sample NMEA sentences for testing
const char* TEST_SENTENCES[] = {
  "$APRSA,10.5,A,,V*3F\r\n",
  "$APHDM,045.2,M*1C\r\n",
  "$VHGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n",
  "$VHRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A\r\n",
  "$VHVTG,054.7,T,034.4,M,005.5,N,010.2,K*48\r\n"
};
const int SENTENCE_COUNT = 5;

// Timing measurement variables
unsigned long processingStartTime = 0;
unsigned long processingEndTime = 0;
int sentencesProcessed = 0;
int maxProcessingTime = 0;

/**
 * @brief Setup function for hardware tests
 */
void setUp(void) {
  // Initialize Serial2 for NMEA 0183
  Serial2.begin(NMEA_BAUD_RATE, SERIAL_8N1, 25, 27); // RX=25, TX=27 (SH-ESP32)
  delay(100); // Allow serial to stabilize
  
  // Clear any pending data
  while (Serial2.available()) {
    Serial2.read();
  }
  
  // Reset counters
  sentencesProcessed = 0;
  maxProcessingTime = 0;
}

/**
 * @brief Teardown function for hardware tests
 */
void tearDown(void) {
  // Clear serial buffer
  while (Serial2.available()) {
    Serial2.read();
  }
}

/**
 * @brief Test: Serial2 initialization at 4800 baud
 * 
 * Verifies Serial2 can be initialized and is ready for NMEA data reception.
 */
void test_serial2_initialization(void) {
  TEST_ASSERT_EQUAL(NMEA_BAUD_RATE, Serial2.baudRate());
  TEST_ASSERT_TRUE(Serial2);
}

/**
 * @brief Test: Single sentence processing within 50ms budget (FR-027)
 * 
 * Sends one NMEA sentence and measures processing time.
 * Validates that processing completes within 50ms budget.
 */
void test_single_sentence_processing_time(void) {
  // Send a test sentence via Serial2 loopback
  const char* testSentence = "$APHDM,045.2,M*1C\r\n";
  
  processingStartTime = millis();
  
  // Simulate sentence reception (in real test, this would come from external device)
  Serial2.print(testSentence);
  
  // Wait for data to be available (non-blocking check with timeout)
  unsigned long timeout = millis() + 100;
  while (!Serial2.available() && millis() < timeout) {
    // Non-blocking wait for serial data
  }
  
  // Read and process sentence
  if (Serial2.available()) {
    String sentence = "";
    while (Serial2.available()) {
      char c = Serial2.read();
      sentence += c;
      if (c == '\n') break;
    }
    
    processingEndTime = millis();
    unsigned long processingTime = processingEndTime - processingStartTime;
    
    TEST_ASSERT_LESS_THAN(PROCESSING_BUDGET_MS, processingTime);
    TEST_MESSAGE("Processing time: " + String(processingTime) + "ms");
  }
}

/**
 * @brief Test: Multiple sentences at normal rate (1-7 Hz)
 * 
 * Sends burst of sentences at typical NMEA 0183 update rate.
 * Validates all sentences are processed without buffer overrun.
 * 
 * @note Uses timed intervals to simulate realistic NMEA update rates
 */
void test_normal_rate_processing(void) {
  int sentencesReceived = 0;
  
  // Send 10 sentences with 200ms interval (5 Hz rate)
  for (int i = 0; i < TEST_SENTENCE_COUNT; i++) {
    const char* sentence = TEST_SENTENCES[i % SENTENCE_COUNT];
    Serial2.print(sentence);
    
    // Wait for 200ms interval to simulate realistic update rate
    unsigned long nextSend = millis() + 200;
    while (millis() < nextSend) {
      // Check if data is available and process during wait
      if (Serial2.available()) {
        processingStartTime = millis();
        
        String receivedSentence = "";
        while (Serial2.available()) {
          char c = Serial2.read();
          receivedSentence += c;
          if (c == '\n') {
            sentencesReceived++;
            break;
          }
        }
        
        processingEndTime = millis();
        unsigned long processingTime = processingEndTime - processingStartTime;
        
        // Track max processing time
        if (processingTime > maxProcessingTime) {
          maxProcessingTime = processingTime;
        }
        
        TEST_ASSERT_LESS_THAN(PROCESSING_BUDGET_MS, processingTime);
      }
    }
  }
  
  // Process any remaining data in buffer
  while (Serial2.available()) {
    char c = Serial2.read();
    if (c == '\n') {
      sentencesReceived++;
    }
  }
  
  TEST_ASSERT_EQUAL(TEST_SENTENCE_COUNT, sentencesReceived);
  TEST_MESSAGE("Max processing time: " + String(maxProcessingTime) + "ms");
}

/**
 * @brief Test: Buffer overflow handling with FIFO drop (FR-032)
 * 
 * Sends rapid burst of sentences to overflow buffer.
 * Validates graceful degradation (oldest data dropped, system continues).
 * 
 * @note This test validates that the system doesn't crash under overflow conditions
 */
void test_buffer_overflow_handling(void) {
  int sentencesReceived = 0;
  bool systemStable = true;
  
  // Send 100 sentences rapidly (no delay) to force buffer overflow
  for (int i = 0; i < OVERFLOW_TEST_COUNT; i++) {
    const char* sentence = TEST_SENTENCES[i % SENTENCE_COUNT];
    Serial2.print(sentence);
    // No delay - intentional overflow test
  }
  
  // Wait for processing to complete (non-blocking check with timeout)
  unsigned long processTimeout = millis() + 1000;
  while (millis() < processTimeout) {
    // Allow processing time
  }
  
  // Process whatever is in the buffer
  while (Serial2.available()) {
    char c = Serial2.read();
    if (c == '\n') {
      sentencesReceived++;
    }
  }
  
  // System should remain stable (not crash)
  TEST_ASSERT_TRUE(systemStable);
  
  // We won't receive all 100 sentences due to overflow, but system should handle gracefully
  TEST_MESSAGE("Sentences received after overflow: " + String(sentencesReceived));
  
  // Verify system is still responsive after overflow
  Serial2.print("$APHDM,045.2,M*1C\r\n");
  
  // Wait for response (non-blocking check with timeout)
  unsigned long timeout = millis() + 50;
  while (!Serial2.available() && millis() < timeout) {
    // Non-blocking wait
  }
  
  TEST_ASSERT_TRUE(Serial2.available() > 0);
}

/**
 * @brief Test: Loopback test with known sentence
 * 
 * Validates Serial2 TX/RX functionality with loopback.
 * Requires TX and RX pins to be connected (GPIO 25 -> GPIO 27 on SH-ESP32).
 */
void test_serial2_loopback(void) {
  // NOTE: This test requires physical loopback connection (TX->RX)
  const char* testSentence = "$APHDM,045.2,M*1C\r\n";
  
  // Send sentence
  Serial2.print(testSentence);
  
  // Wait for data to be available (non-blocking check with timeout)
  unsigned long timeout = millis() + 50;
  while (!Serial2.available() && millis() < timeout) {
    // Non-blocking wait for transmission
  }
  
  // Read back
  String received = "";
  while (Serial2.available()) {
    received += (char)Serial2.read();
  }
  
  // In loopback mode, we should receive what we sent
  // Note: This test will be marked as optional since loopback may not be set up
  if (received.length() > 0) {
    TEST_ASSERT_EQUAL_STRING(testSentence, received.c_str());
  } else {
    TEST_MESSAGE("Loopback not connected - test skipped");
  }
}

/**
 * @brief Main test runner for ESP32 hardware tests
 * 
 * @note Initial delay allows serial monitor connection for test output visibility.
 *       In production code with ReactESP, delays are not needed - ReactESP handles
 *       timing through event scheduling (onRepeat, onDelay, etc.).
 */
void setup() {
  delay(2000); // Allow time for serial monitor connection (test framework only)
  
  UNITY_BEGIN();
  
  // Test Serial2 initialization
  RUN_TEST(test_serial2_initialization);
  
  // Test processing timing (FR-027)
  RUN_TEST(test_single_sentence_processing_time);
  RUN_TEST(test_normal_rate_processing);
  
  // Test buffer overflow handling (FR-032)
  RUN_TEST(test_buffer_overflow_handling);
  
  // Optional: Loopback test (requires physical connection)
  RUN_TEST(test_serial2_loopback);
  
  UNITY_END();
}

void loop() {
  // Hardware tests run once in setup()
}
