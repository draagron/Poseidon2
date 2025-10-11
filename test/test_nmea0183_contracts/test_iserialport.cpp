#include <unity.h>
#include "mocks/MockSerialPort.h"

// T008: Contract Test - ISerialPort available() Non-Blocking
void test_iserialport_available_non_blocking() {
    MockSerialPort mockSerial;
    mockSerial.setMockData("ABC");

    // Test: available() returns correct byte count
    TEST_ASSERT_EQUAL_INT(3, mockSerial.available());

    // Test: available() is non-destructive (calling multiple times returns same value)
    TEST_ASSERT_EQUAL_INT(3, mockSerial.available());
    TEST_ASSERT_EQUAL_INT(3, mockSerial.available());

    // Test with empty mock
    MockSerialPort emptySerial;
    emptySerial.setMockData("");
    TEST_ASSERT_EQUAL_INT(0, emptySerial.available());
}

// T009: Contract Test - ISerialPort read() Consumes Bytes
void test_iserialport_read_consumes_bytes() {
    MockSerialPort mockSerial;
    mockSerial.setMockData("ABC");

    // Test: read() returns correct byte
    TEST_ASSERT_EQUAL_INT('A', mockSerial.read());

    // Test: available() decrements after read
    TEST_ASSERT_EQUAL_INT(2, mockSerial.available());

    // Test: FIFO order maintained
    TEST_ASSERT_EQUAL_INT('B', mockSerial.read());
    TEST_ASSERT_EQUAL_INT('C', mockSerial.read());

    // Test: read() returns -1 when empty
    TEST_ASSERT_EQUAL_INT(-1, mockSerial.read());

    // Test: available() returns 0 when empty
    TEST_ASSERT_EQUAL_INT(0, mockSerial.available());
}

// T010: Contract Test - ISerialPort begin() Idempotent
void test_iserialport_begin_idempotent() {
    MockSerialPort mockSerial;

    // Test: begin() can be called multiple times
    mockSerial.begin(4800);
    mockSerial.setMockData("TEST");

    // Call begin() again
    mockSerial.begin(4800);

    // Test: Mock still functional after multiple begin() calls
    TEST_ASSERT_EQUAL_INT(4, mockSerial.available());

    // Call begin() a third time
    mockSerial.begin(4800);
    TEST_ASSERT_EQUAL_INT(4, mockSerial.available());
}
