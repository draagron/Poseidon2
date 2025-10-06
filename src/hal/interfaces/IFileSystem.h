/**
 * @file IFileSystem.h
 * @brief Hardware abstraction interface for flash filesystem operations
 *
 * This interface abstracts filesystem operations (LittleFS/SPIFFS) to enable
 * testing without physical flash storage. Implementations must provide methods
 * for file existence checks, reading, and writing.
 *
 * Usage:
 * @code
 * IFileSystem* fs = new LittleFSAdapter();
 * if (fs->exists("/wifi.conf")) {
 *   String content = fs->readFile("/wifi.conf");
 *   // Process content...
 * }
 * @endcode
 */

#ifndef I_FILE_SYSTEM_H
#define I_FILE_SYSTEM_H

#include <Arduino.h>

/**
 * @brief File open modes
 */
enum class FileMode {
    READ,       ///< Open for reading only
    WRITE,      ///< Open for writing (overwrite existing)
    APPEND      ///< Open for appending to existing content
};

/**
 * @brief Abstract interface for filesystem operations
 *
 * All filesystem interactions must go through this interface to
 * maintain testability and hardware abstraction (HAL Principle I).
 */
class IFileSystem {
public:
    virtual ~IFileSystem() {}

    /**
     * @brief Mount the filesystem
     * @return true if mount successful, false otherwise
     */
    virtual bool mount() = 0;

    /**
     * @brief Check if a file exists
     * @param path Absolute path to file (e.g., "/wifi.conf")
     * @return true if file exists
     */
    virtual bool exists(const char* path) = 0;

    /**
     * @brief Read entire file contents
     * @param path Absolute path to file
     * @return File contents as String (empty if file doesn't exist)
     */
    virtual String readFile(const char* path) = 0;

    /**
     * @brief Write content to file (overwrites existing)
     * @param path Absolute path to file
     * @param content Content to write
     * @return true if write successful
     */
    virtual bool writeFile(const char* path, const char* content) = 0;

    /**
     * @brief Delete a file
     * @param path Absolute path to file
     * @return true if deletion successful
     */
    virtual bool deleteFile(const char* path) = 0;

    /**
     * @brief Get total filesystem size in bytes
     * @return Total size in bytes
     */
    virtual size_t totalBytes() = 0;

    /**
     * @brief Get used filesystem space in bytes
     * @return Used space in bytes
     */
    virtual size_t usedBytes() = 0;
};

#endif // I_FILE_SYSTEM_H
