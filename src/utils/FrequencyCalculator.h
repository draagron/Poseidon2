/**
 * @file FrequencyCalculator.h
 * @brief Stateless utility for calculating update frequencies from circular timestamp buffers
 *
 * Provides functions for:
 * - Calculating rolling-average frequency (Hz) from 10-sample circular buffer
 * - Managing circular timestamp buffer insertion
 *
 * Algorithm: frequency = 1000ms / avg_interval
 * where avg_interval = (last_timestamp - first_timestamp) / (sample_count - 1)
 *
 * Accuracy: ±10% target (SC-002)
 * Performance: O(1) time complexity, <1µs on ESP32 @ 240MHz
 *
 * @see specs/012-sources-stats-and/contracts/FrequencyCalculatorContract.md
 * @version 1.0.0
 * @date 2025-10-13
 */

#ifndef FREQUENCY_CALCULATOR_H
#define FREQUENCY_CALCULATOR_H

#include <stdint.h>

class FrequencyCalculator {
public:
    /**
     * @brief Calculate update frequency from circular timestamp buffer
     *
     * Computes rolling average based on first and last timestamp in buffer.
     * Formula: frequency (Hz) = 1000 / ((last_timestamp - first_timestamp) / (count - 1))
     *
     * @param buffer Circular buffer of millis() timestamps (must contain ≥2 samples)
     * @param count Number of valid timestamps in buffer (2-10)
     *
     * @return Frequency in Hz (0.0 if invalid input)
     *
     * @pre count >= 2 (otherwise frequency undefined)
     * @pre count <= 10 (buffer size limit)
     * @pre buffer[0..count-1] contain valid millis() timestamps
     *
     * @note Stateless function - can be called with any buffer
     * @note Handles millis() rollover gracefully (incorrect for one cycle, self-corrects)
     * @note Returns 0.0 if count < 2 or average interval is 0
     *
     * @example
     * uint32_t buffer[10] = {1000, 1100, 1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900};
     * double freq = FrequencyCalculator::calculate(buffer, 10);
     * // freq = 1000 / ((1900 - 1000) / 9) = 1000 / 100 = 10.0 Hz
     */
    static double calculate(const uint32_t* buffer, uint8_t count);

    /**
     * @brief Add timestamp to circular buffer
     *
     * Inserts timestamp at current index, advances index with wrap-around,
     * sets full flag when 10 samples collected.
     *
     * @param buffer Circular buffer of millis() timestamps (size 10)
     * @param index Current buffer index (0-9), updated by reference
     * @param full Buffer full flag, set to true when 10 samples collected
     * @param timestamp Timestamp to insert (typically millis())
     *
     * @post buffer[index] = timestamp
     * @post index = (index + 1) % 10
     * @post full = true if buffer wrapped around
     *
     * @note Helper function for MessageSource timestamp management
     */
    static void addTimestamp(uint32_t* buffer, uint8_t& index, bool& full, uint32_t timestamp);
};

#endif // FREQUENCY_CALCULATOR_H
