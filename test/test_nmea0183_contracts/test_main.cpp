#include <unity.h>

// Test functions from test_iserialport.cpp
void test_iserialport_available_non_blocking();
void test_iserialport_read_consumes_bytes();
void test_iserialport_begin_idempotent();

void setUp() {
    // Set up before each test
}

void tearDown() {
    // Clean up after each test
}

int main(int argc, char** argv) {
    UNITY_BEGIN();

    // ISerialPort contract tests
    RUN_TEST(test_iserialport_available_non_blocking);
    RUN_TEST(test_iserialport_read_consumes_bytes);
    RUN_TEST(test_iserialport_begin_idempotent);

    return UNITY_END();
}
