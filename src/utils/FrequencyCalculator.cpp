/**
 * @file FrequencyCalculator.cpp
 * @brief Implementation of frequency calculation utilities
 *
 * @see FrequencyCalculator.h for interface documentation
 */

#include "FrequencyCalculator.h"

double FrequencyCalculator::calculate(const uint32_t* buffer, uint8_t count) {
    // Validate input: need at least 2 samples for interval calculation
    if (count < 2) {
        return 0.0;
    }

    // Calculate total interval from first to last sample
    uint32_t totalInterval = buffer[count - 1] - buffer[0];

    // Calculate average interval between consecutive samples
    double avgInterval = (double)totalInterval / (count - 1);

    // Guard against division by zero (all timestamps identical)
    if (avgInterval == 0.0) {
        return 0.0;
    }

    // Convert average interval (ms) to frequency (Hz)
    // Hz = 1000ms / avg_interval_ms
    return 1000.0 / avgInterval;
}

void FrequencyCalculator::addTimestamp(uint32_t* buffer, uint8_t& index, bool& full, uint32_t timestamp) {
    // Insert timestamp at current index
    buffer[index] = timestamp;

    // Advance index with wrap-around (circular buffer)
    index = (index + 1) % 10;

    // Mark buffer as full when we've wrapped around to index 0
    if (index == 0) {
        full = true;
    }
}
