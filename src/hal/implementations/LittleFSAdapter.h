/**
 * @file LittleFSAdapter.h
 * @brief LittleFS filesystem adapter implementation
 *
 * Wraps the Arduino LittleFS library to implement IFileSystem interface.
 * Provides actual flash filesystem functionality for ESP32.
 */

#ifndef LITTLEFS_ADAPTER_H
#define LITTLEFS_ADAPTER_H

#include <LittleFS.h>
#include "../../hal/interfaces/IFileSystem.h"

/**
 * @brief LittleFS filesystem adapter implementation
 *
 * Real hardware implementation wrapping ESP32 LittleFS library.
 * Used in production (vs MockFileSystem for testing).
 */
class LittleFSAdapter : public IFileSystem {
private:
    bool isMounted;

public:
    /**
     * @brief Constructor
     */
    LittleFSAdapter();

    // IFileSystem interface implementation
    bool mount() override;
    bool exists(const char* path) override;
    String readFile(const char* path) override;
    bool writeFile(const char* path, const char* content) override;
    bool deleteFile(const char* path) override;
    size_t totalBytes() override;
    size_t usedBytes() override;
};

#endif // LITTLEFS_ADAPTER_H
