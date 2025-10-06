/**
 * @file TimeoutManager.h
 * @brief Timeout tracking manager for ReactESP event loops
 *
 * Manages timeout callbacks for WiFi connection attempts using ReactESP.
 * Integrates with ReactESP event loops for non-blocking timeout detection.
 *
 * Usage:
 * @code
 * TimeoutManager timeoutMgr;
 *
 * // Register timeout callback
 * timeoutMgr.registerTimeout(30000, []() {
 *     // Handle timeout after 30 seconds
 * });
 *
 * // Cancel timeout if connection succeeds
 * timeoutMgr.cancelTimeout();
 * @endcode
 */

#ifndef TIMEOUT_MANAGER_H
#define TIMEOUT_MANAGER_H

#include <Arduino.h>
#include <ReactESP.h>

/**
 * @brief Timeout callback function type
 */
typedef std::function<void()> TimeoutCallback;

/**
 * @brief Timeout manager for ReactESP integration
 *
 * Tracks timeout for WiFi connection attempts and triggers callbacks
 * when timeouts are exceeded. Non-blocking, event-driven approach.
 */
class TimeoutManager {
private:
    bool timeoutActive;
    unsigned long timeoutStart;
    unsigned long timeoutDuration;
    TimeoutCallback callback;

public:
    /**
     * @brief Constructor
     */
    TimeoutManager();

    /**
     * @brief Register a timeout callback
     * @param durationMs Timeout duration in milliseconds
     * @param cb Callback function to invoke on timeout
     *
     * Starts the timeout timer. Call this when connection attempt begins.
     */
    void registerTimeout(unsigned long durationMs, TimeoutCallback cb);

    /**
     * @brief Cancel active timeout
     *
     * Call this when connection succeeds to prevent timeout callback.
     */
    void cancelTimeout();

    /**
     * @brief Check if timeout has been exceeded
     * @return true if timeout expired
     *
     * Call this periodically from ReactESP event loop.
     * If timeout expired, invokes callback and deactivates timeout.
     */
    bool checkTimeout();

    /**
     * @brief Check if a timeout is currently active
     * @return true if timeout is active
     */
    bool isActive() const;

    /**
     * @brief Get elapsed time since timeout started
     * @return Elapsed time in milliseconds
     */
    unsigned long getElapsedTime() const;

    /**
     * @brief Get remaining time until timeout
     * @return Remaining time in milliseconds (0 if expired)
     */
    unsigned long getRemainingTime() const;
};

#endif // TIMEOUT_MANAGER_H
