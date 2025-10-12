#include "ESP32SerialPort.h"

ESP32SerialPort::ESP32SerialPort(HardwareSerial* serial, int8_t rxPin, int8_t txPin)
    : serial_(serial), rxPin_(rxPin), txPin_(txPin) {
}

int ESP32SerialPort::available() {
    return serial_->available();
}

int ESP32SerialPort::read() {
    return serial_->read();
}

void ESP32SerialPort::begin(unsigned long baud) {
    // Pass GPIO pins to HardwareSerial::begin() for ESP32
    // Format: begin(baud, config, rxPin, txPin)
    serial_->begin(baud, SERIAL_8N1, rxPin_, txPin_);
}

Stream* ESP32SerialPort::getStream() {
    return serial_;
}
