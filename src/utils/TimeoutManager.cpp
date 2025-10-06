/**
 * @file TimeoutManager.cpp
 * @brief Implementation of timeout manager for ReactESP
 */

#include "TimeoutManager.h"

TimeoutManager::TimeoutManager()
    : timeoutActive(false),
      timeoutStart(0),
      timeoutDuration(0),
      callback(nullptr) {
}

void TimeoutManager::registerTimeout(unsigned long durationMs, TimeoutCallback cb) {
    timeoutActive = true;
    timeoutStart = millis();
    timeoutDuration = durationMs;
    callback = cb;
}

void TimeoutManager::cancelTimeout() {
    timeoutActive = false;
    timeoutStart = 0;
    timeoutDuration = 0;
    callback = nullptr;
}

bool TimeoutManager::checkTimeout() {
    if (!timeoutActive) {
        return false;
    }

    unsigned long elapsed = millis() - timeoutStart;

    if (elapsed >= timeoutDuration) {
        // Timeout exceeded
        timeoutActive = false;

        // Invoke callback if registered
        if (callback != nullptr) {
            callback();
        }

        return true;
    }

    return false;
}

bool TimeoutManager::isActive() const {
    return timeoutActive;
}

unsigned long TimeoutManager::getElapsedTime() const {
    if (!timeoutActive) {
        return 0;
    }

    return millis() - timeoutStart;
}

unsigned long TimeoutManager::getRemainingTime() const {
    if (!timeoutActive) {
        return 0;
    }

    unsigned long elapsed = millis() - timeoutStart;

    if (elapsed >= timeoutDuration) {
        return 0;
    }

    return timeoutDuration - elapsed;
}
