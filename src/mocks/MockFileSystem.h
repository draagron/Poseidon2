/**
 * @file MockFileSystem.h
 * @brief Mock filesystem for unit testing
 *
 * This mock implementation provides in-memory file storage without
 * requiring actual flash hardware. Allows testing of file operations
 * in native test environment.
 */

#ifndef MOCK_FILE_SYSTEM_H
#define MOCK_FILE_SYSTEM_H

#include "../hal/interfaces/IFileSystem.h"
#include <map>

/**
 * @brief Mock filesystem for testing
 *
 * Simulates filesystem operations using in-memory storage.
 * All files are stored in a map and persist only during test execution.
 */
class MockFileSystem : public IFileSystem {
private:
    std::map<String, String> files; // In-memory file storage
    bool isMounted;
    size_t totalSize;

public:
    /**
     * @brief Constructor
     * @param totalSize Simulated total filesystem size (default 512KB)
     */
    MockFileSystem(size_t totalSize = 512 * 1024);

    // IFileSystem interface implementation
    bool mount() override;
    bool exists(const char* path) override;
    String readFile(const char* path) override;
    bool writeFile(const char* path, const char* content) override;
    bool deleteFile(const char* path) override;
    size_t totalBytes() override;
    size_t usedBytes() override;

    // Test helper methods
    /**
     * @brief Clear all files from mock filesystem
     */
    void clear();

    /**
     * @brief Get number of files in filesystem
     * @return File count
     */
    size_t getFileCount();

    /**
     * @brief Simulate mount failure
     * @param shouldFail If true, mount() will return false
     */
    void setMountFailure(bool shouldFail);

    /**
     * @brief Reset mock to initial state
     */
    void reset();

private:
    bool mountShouldFail;
};

#endif // MOCK_FILE_SYSTEM_H
