#include "MockSerialPort.h"

MockSerialPort::MockSerialPort()
    : mockData_(nullptr), position_(0) {
}

void MockSerialPort::setMockData(const char* data) {
    mockData_ = data;
    position_ = 0;
}

int MockSerialPort::available() {
    if (mockData_ == nullptr) {
        return 0;
    }
    size_t dataLength = strlen(mockData_);
    if (position_ >= dataLength) {
        return 0;
    }
    return static_cast<int>(dataLength - position_);
}

int MockSerialPort::read() {
    if (mockData_ == nullptr) {
        return -1;
    }
    if (position_ >= strlen(mockData_)) {
        return -1;
    }
    return static_cast<int>(static_cast<unsigned char>(mockData_[position_++]));
}

void MockSerialPort::begin(unsigned long baud) {
    // No-op for mock implementation
    (void)baud;  // Suppress unused parameter warning
}

void MockSerialPort::reset() {
    position_ = 0;
}
