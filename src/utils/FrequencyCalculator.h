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
     * Computes rolling average based on oldest and newest timestamps in circular buffer.
     * Formula: frequency (Hz) = 1000 / ((newest_timestamp - oldest_timestamp) / (count - 1))
     *
     * @param buffer Circular buffer of millis() timestamps (must contain ≥2 samples)
     * @param count Number of valid timestamps in buffer (2-10)
     * @param bufferIndex Current write position in circular buffer (0-9)
     *
     * @return Frequency in Hz (0.0 if invalid input)
     *
     * @pre count >= 2 (otherwise frequency undefined)
     * @pre count <= 10 (buffer size limit)
     * @pre buffer[0..9] contain valid millis() timestamps
     * @pre bufferIndex in range [0, 9]
     *
     * @note Stateless function - can be called with any buffer
     * @note Handles millis() rollover gracefully (incorrect for one cycle, self-corrects)
     * @note Returns 0.0 if count < 2 or average interval is 0
     * @note Correctly handles circular buffer wrap-around using bufferIndex
     *
     * @example
     * // Linear buffer (before wrap): [t1, t2, ..., t10], index=0
     * // Oldest at (0-1+10)%10=9, newest at (0+10-1)%10=9 → use linear logic
     * // Wrapped buffer: [t11, t2, t3, ..., t10], index=1
     * // Oldest at index=1 (t2), newest at index=0 (t11)
     */
    static double calculate(const uint32_t* buffer, uint8_t count, uint8_t bufferIndex);

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
