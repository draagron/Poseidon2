/**
 * @file LittleFSAdapter.cpp
 * @brief Implementation of LittleFS filesystem adapter
 */

#include "LittleFSAdapter.h"

LittleFSAdapter::LittleFSAdapter() : isMounted(false) {
}

bool LittleFSAdapter::mount() {
    if (isMounted) {
        return true; // Already mounted
    }

    // Mount LittleFS with format on fail option
    // formatOnFail=true ensures first-time setup works
    isMounted = LittleFS.begin(true);

    return isMounted;
}

bool LittleFSAdapter::exists(const char* path) {
    if (!isMounted) {
        return false;
    }

    return LittleFS.exists(path);
}

String LittleFSAdapter::readFile(const char* path) {
    if (!isMounted) {
        return "";
    }

    if (!LittleFS.exists(path)) {
        return "";
    }

    // Open file for reading
    File file = LittleFS.open(path, "r");
    if (!file) {
        return "";
    }

    // Read entire file content
    String content = "";
    while (file.available()) {
        content += (char)file.read();
    }

    file.close();
    return content;
}

bool LittleFSAdapter::writeFile(const char* path, const char* content) {
    if (!isMounted) {
        return false;
    }

    // Open file for writing (overwrites existing)
    File file = LittleFS.open(path, "w");
    if (!file) {
        return false;
    }

    // Write content
    size_t bytesWritten = file.print(content);
    file.close();

    // Verify all bytes were written
    return bytesWritten == strlen(content);
}

bool LittleFSAdapter::deleteFile(const char* path) {
    if (!isMounted) {
        return false;
    }

    if (!LittleFS.exists(path)) {
        return false; // File doesn't exist
    }

    return LittleFS.remove(path);
}

size_t LittleFSAdapter::totalBytes() {
    if (!isMounted) {
        return 0;
    }

    return LittleFS.totalBytes();
}

size_t LittleFSAdapter::usedBytes() {
    if (!isMounted) {
        return 0;
    }

    return LittleFS.usedBytes();
}
