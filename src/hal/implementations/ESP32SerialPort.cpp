#include "ESP32SerialPort.h"

ESP32SerialPort::ESP32SerialPort(HardwareSerial* serial)
    : serial_(serial) {
}

int ESP32SerialPort::available() {
    return serial_->available();
}

int ESP32SerialPort::read() {
    return serial_->read();
}

void ESP32SerialPort::begin(unsigned long baud) {
    serial_->begin(baud);
}
