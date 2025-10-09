/**
 * @file test_idisplayadapter.cpp
 * @brief Contract validation tests for IDisplayAdapter interface
 *
 * Validates that MockDisplayAdapter correctly implements the IDisplayAdapter
 * interface contract. These tests ensure the interface behaves as expected.
 *
 * @version 1.0.0
 * @date 2025-10-09
 */

#include <unity.h>
#include "mocks/MockDisplayAdapter.h"

// Global mock instance for tests
static MockDisplayAdapter* mockDisplay = nullptr;

/**
 * @brief Test: init() returns true on success
 */
void test_init_returns_true_on_success() {
    mockDisplay = new MockDisplayAdapter();
    mockDisplay->setInitResult(true);  // Simulate successful init

    bool result = mockDisplay->init();

    TEST_ASSERT_TRUE(result);
    delete mockDisplay;
}

/**
 * @brief Test: init() returns false on failure (I2C error)
 */
void test_init_returns_false_on_failure() {
    mockDisplay = new MockDisplayAdapter();
    mockDisplay->setInitResult(false);  // Simulate I2C error

    bool result = mockDisplay->init();

    TEST_ASSERT_FALSE(result);
    delete mockDisplay;
}

/**
 * @brief Test: isReady() returns false before init()
 */
void test_isReady_false_before_init() {
    mockDisplay = new MockDisplayAdapter();

    bool ready = mockDisplay->isReady();

    TEST_ASSERT_FALSE(ready);
    delete mockDisplay;
}

/**
 * @brief Test: isReady() returns true after successful init()
 */
void test_isReady_true_after_init() {
    mockDisplay = new MockDisplayAdapter();
    mockDisplay->setInitResult(true);
    mockDisplay->init();

    bool ready = mockDisplay->isReady();

    TEST_ASSERT_TRUE(ready);
    delete mockDisplay;
}

/**
 * @brief Test: clear() does not crash before init()
 *
 * Graceful degradation: display operations should be safe even if init fails
 */
void test_clear_does_not_crash_before_init() {
    mockDisplay = new MockDisplayAdapter();

    // Should not crash even if not initialized
    mockDisplay->clear();

    // Verify clear was tracked
    TEST_ASSERT_TRUE(mockDisplay->wasCleared());
    delete mockDisplay;
}

/**
 * @brief Test: setCursor() accepts valid coordinates
 */
void test_setCursor_accepts_valid_coordinates() {
    mockDisplay = new MockDisplayAdapter();

    // Test corner coordinates
    mockDisplay->setCursor(0, 0);
    TEST_ASSERT_EQUAL(0, mockDisplay->getCursorX());
    TEST_ASSERT_EQUAL(0, mockDisplay->getCursorY());

    // Test mid-screen coordinates
    mockDisplay->setCursor(64, 32);
    TEST_ASSERT_EQUAL(64, mockDisplay->getCursorX());
    TEST_ASSERT_EQUAL(32, mockDisplay->getCursorY());

    // Test bottom-right coordinates
    mockDisplay->setCursor(127, 63);
    TEST_ASSERT_EQUAL(127, mockDisplay->getCursorX());
    TEST_ASSERT_EQUAL(63, mockDisplay->getCursorY());

    delete mockDisplay;
}

/**
 * @brief Test: setTextSize() accepts valid sizes (1-4)
 */
void test_setTextSize_accepts_valid_sizes() {
    mockDisplay = new MockDisplayAdapter();

    mockDisplay->setTextSize(1);
    TEST_ASSERT_EQUAL(1, mockDisplay->getTextSize());

    mockDisplay->setTextSize(2);
    TEST_ASSERT_EQUAL(2, mockDisplay->getTextSize());

    mockDisplay->setTextSize(3);
    TEST_ASSERT_EQUAL(3, mockDisplay->getTextSize());

    mockDisplay->setTextSize(4);
    TEST_ASSERT_EQUAL(4, mockDisplay->getTextSize());

    delete mockDisplay;
}

/**
 * @brief Test: print() renders text to display buffer
 */
void test_print_renders_text() {
    mockDisplay = new MockDisplayAdapter();
    mockDisplay->clear();

    mockDisplay->print("Hello");

    TEST_ASSERT_TRUE(mockDisplay->wasTextRendered("Hello"));

    mockDisplay->print(" World");
    TEST_ASSERT_TRUE(mockDisplay->wasTextRendered("Hello World"));

    delete mockDisplay;
}

/**
 * @brief Test: display() pushes buffer to hardware
 */
void test_display_pushes_buffer() {
    mockDisplay = new MockDisplayAdapter();

    mockDisplay->clear();
    mockDisplay->print("Test");
    mockDisplay->display();

    TEST_ASSERT_TRUE(mockDisplay->wasDisplayCalled());

    delete mockDisplay;
}
