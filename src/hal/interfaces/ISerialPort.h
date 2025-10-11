#ifndef ISERIALPORT_H
#define ISERIALPORT_H

/**
 * @file ISerialPort.h
 * @brief Hardware Abstraction Layer interface for serial port communication
 *
 * This interface abstracts Serial2 hardware access for NMEA 0183 sentence reception,
 * enabling mock-first testing on native platforms and hardware independence.
 *
 * Behavioral Contract:
 * - available() returns number of bytes in receive buffer without consuming them
 * - available() is non-blocking and must return immediately (< 1ms)
 * - read() consumes and returns one byte from FIFO buffer
 * - read() returns -1 if no data available (non-blocking)
 * - begin() initializes serial port and is idempotent (safe to call multiple times)
 *
 * Reference: specs/006-nmea-0183-handlers/contracts/ISerialPort.contract.md
 */
class ISerialPort {
public:
    /**
     * @brief Check number of bytes available to read
     *
     * Non-blocking operation that returns the number of bytes currently in the
     * receive buffer. Does not consume or modify buffer contents. Can be called
     * multiple times and will return the same value until read() is called.
     *
     * @return Number of bytes available (0 if buffer empty)
     * @note Must execute in < 1ms
     * @note Non-destructive (does not consume bytes)
     */
    virtual int available() = 0;

    /**
     * @brief Read one byte from serial port
     *
     * Consumes and returns the next byte from the FIFO receive buffer.
     * If buffer is empty, returns -1 immediately without blocking.
     *
     * @return Byte value (0-255) if data available, -1 if buffer empty
     * @note Consumes one byte from buffer (destructive operation)
     * @note Non-blocking operation
     * @note FIFO order guaranteed (first byte received is first byte returned)
     */
    virtual int read() = 0;

    /**
     * @brief Initialize serial port
     *
     * Initializes serial port hardware at specified baud rate. For NMEA 0183,
     * typical baud rate is 4800. Idempotent - safe to call multiple times with
     * same or different baud rates. Implementation may reconfigure port if called
     * with different baud rate.
     *
     * @param baud Baud rate (typically 4800 for NMEA 0183)
     * @note Idempotent operation (safe to call multiple times)
     * @note May reconfigure port if baud rate changes
     */
    virtual void begin(unsigned long baud) = 0;

    /**
     * @brief Virtual destructor for proper cleanup
     *
     * Ensures derived classes can properly clean up resources when deleted
     * through base class pointer.
     */
    virtual ~ISerialPort() = default;
};

#endif // ISERIALPORT_H
