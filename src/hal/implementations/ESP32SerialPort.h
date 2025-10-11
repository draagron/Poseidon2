#ifndef ESP32SERIALPORT_H
#define ESP32SERIALPORT_H

#include "hal/interfaces/ISerialPort.h"
#include <HardwareSerial.h>

/**
 * @file ESP32SerialPort.h
 * @brief ESP32 hardware implementation of ISerialPort interface
 *
 * Thin wrapper around Arduino HardwareSerial class (Serial2) for NMEA 0183 reception.
 * Delegates all operations directly to underlying HardwareSerial instance without
 * additional logic or buffering.
 *
 * Usage:
 * @code
 * ISerialPort* serial0183 = new ESP32SerialPort(&Serial2);
 * serial0183->begin(4800);
 * while (serial0183->available() > 0) {
 *     int byte = serial0183->read();
 *     // Process byte...
 * }
 * @endcode
 *
 * @note Does not take ownership of HardwareSerial pointer - caller must ensure
 *       Serial2 remains valid for lifetime of ESP32SerialPort instance.
 */
class ESP32SerialPort : public ISerialPort {
public:
    /**
     * @brief Constructor
     *
     * Creates adapter for existing HardwareSerial instance (typically &Serial2).
     * Does not take ownership - pointer must remain valid.
     *
     * @param serial Pointer to HardwareSerial instance (e.g., &Serial2)
     */
    explicit ESP32SerialPort(HardwareSerial* serial);

    /**
     * @brief Check number of bytes available to read
     *
     * Delegates to HardwareSerial::available().
     *
     * @return Number of bytes in receive buffer
     */
    int available() override;

    /**
     * @brief Read one byte from serial port
     *
     * Delegates to HardwareSerial::read().
     *
     * @return Byte value (0-255) or -1 if buffer empty
     */
    int read() override;

    /**
     * @brief Initialize serial port
     *
     * Delegates to HardwareSerial::begin(). For NMEA 0183 on Serial2:
     * - RX pin: GPIO25
     * - TX pin: GPIO27 (unused for read-only NMEA)
     * - Default config: 8N1 (8 data bits, no parity, 1 stop bit)
     *
     * @param baud Baud rate (4800 for NMEA 0183)
     */
    void begin(unsigned long baud) override;

private:
    HardwareSerial* serial_;  ///< Pointer to HardwareSerial instance (not owned)
};

#endif // ESP32SERIALPORT_H
