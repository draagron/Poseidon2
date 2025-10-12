#ifndef MOCKSERIALPORT_H
#define MOCKSERIALPORT_H

#include "hal/interfaces/ISerialPort.h"
#include <cstring>

/**
 * @file MockSerialPort.h
 * @brief Mock implementation of ISerialPort for testing NMEA sentence processing
 *
 * This mock implementation simulates serial port byte-by-byte reading for testing
 * NMEA 0183 sentence parsing without requiring hardware. Supports loading predefined
 * NMEA sentence strings and simulates FIFO buffer behavior.
 *
 * Usage:
 * @code
 * MockSerialPort mock;
 * mock.setMockData("$APRSA,15.0,A*3C\r\n");
 * while (mock.available() > 0) {
 *     int byte = mock.read();
 *     // Process byte...
 * }
 * @endcode
 */
class MockSerialPort : public ISerialPort {
public:
    /**
     * @brief Constructor
     *
     * Initializes mock with empty buffer.
     */
    MockSerialPort();

    /**
     * @brief Set mock data for testing
     *
     * Loads predefined NMEA sentence string into mock buffer and resets read position.
     * Supports multiple calls to simulate multiple sentences.
     *
     * @param data Null-terminated string (e.g., "$APRSA,15.0,A*3C\r\n")
     * @note Does not copy data - stores pointer. Caller must ensure data remains valid.
     */
    void setMockData(const char* data);

    /**
     * @brief Check number of bytes available to read
     *
     * Returns number of unread bytes remaining in mock buffer.
     *
     * @return Number of bytes from current position to end of buffer
     * @note Non-destructive (does not advance position)
     */
    int available() override;

    /**
     * @brief Read one byte from mock buffer
     *
     * Returns next byte from mock buffer and advances position. Returns -1 if
     * no more bytes available.
     *
     * @return Byte value (0-255) or -1 if buffer empty
     * @note Consumes one byte (advances position)
     */
    int read() override;

    /**
     * @brief No-op for mock (no hardware to initialize)
     *
     * @param baud Ignored (mock doesn't use baud rate)
     */
    void begin(unsigned long baud) override;

    /**
     * @brief Get underlying Stream pointer (returns nullptr for mock)
     *
     * Mock implementation returns nullptr since there is no real Stream object.
     * Tests using this mock should handle nullptr or avoid calling this method.
     *
     * @return nullptr (no Stream in mock)
     */
    Stream* getStream() override;

    /**
     * @brief Reset read position to beginning
     *
     * Allows re-reading the same mock data without reloading.
     */
    void reset();

private:
    const char* mockData_;  ///< Pointer to mock data buffer
    size_t position_;       ///< Current read position in buffer
};

#endif // MOCKSERIALPORT_H
