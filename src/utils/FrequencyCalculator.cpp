/**
 * @file FrequencyCalculator.cpp
 * @brief Implementation of frequency calculation utilities
 *
 * @see FrequencyCalculator.h for interface documentation
 */

#include "FrequencyCalculator.h"

double FrequencyCalculator::calculate(const uint32_t* buffer, uint8_t count, uint8_t bufferIndex) {
    // Validate input: need at least 2 samples for interval calculation
    if (count < 2 || count > 10) {
        return 0.0;
    }

    // Find oldest and newest timestamps in circular buffer
    uint8_t oldestIdx, newestIdx;

    if (count == 10) {
        // Buffer is full - circular behavior
        // Oldest: at bufferIndex (next to be overwritten)
        // Newest: at (bufferIndex - 1 + 10) % 10 (just written)
        oldestIdx = bufferIndex;
        newestIdx = (bufferIndex + 9) % 10;  // Same as (bufferIndex - 1 + 10) % 10
    } else {
        // Buffer not full - linear behavior
        // Oldest: at index 0
        // Newest: at (count - 1) or (bufferIndex - 1)
        oldestIdx = 0;
        newestIdx = count - 1;
    }

    // Calculate total interval from oldest to newest sample
    uint32_t totalInterval = buffer[newestIdx] - buffer[oldestIdx];

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
