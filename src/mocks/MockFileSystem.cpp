/**
 * @file MockFileSystem.cpp
 * @brief Implementation of mock filesystem for unit testing
 */

#include "MockFileSystem.h"

MockFileSystem::MockFileSystem(size_t totalSize)
    : isMounted(false),
      totalSize(totalSize),
      mountShouldFail(false) {
}

bool MockFileSystem::mount() {
    if (mountShouldFail) {
        isMounted = false;
        return false;
    }
    isMounted = true;
    return true;
}

bool MockFileSystem::exists(const char* path) {
    if (!isMounted) {
        return false;
    }
    String pathStr(path);
    return files.find(pathStr) != files.end();
}

String MockFileSystem::readFile(const char* path) {
    if (!isMounted) {
        return "";
    }

    String pathStr(path);
    auto it = files.find(pathStr);
    if (it != files.end()) {
        return it->second;
    }
    return "";
}

bool MockFileSystem::writeFile(const char* path, const char* content) {
    if (!isMounted) {
        return false;
    }

    String pathStr(path);
    String contentStr(content);

    // Calculate space needed
    size_t currentUsed = usedBytes();
    size_t newContentSize = contentStr.length();

    // If file exists, subtract its current size
    if (exists(path)) {
        currentUsed -= files[pathStr].length();
    }

    // Check if we have enough space
    if (currentUsed + newContentSize > totalSize) {
        return false; // Not enough space
    }

    files[pathStr] = contentStr;
    return true;
}

bool MockFileSystem::deleteFile(const char* path) {
    if (!isMounted) {
        return false;
    }

    String pathStr(path);
    auto it = files.find(pathStr);
    if (it != files.end()) {
        files.erase(it);
        return true;
    }
    return false; // File not found
}

size_t MockFileSystem::totalBytes() {
    return totalSize;
}

size_t MockFileSystem::usedBytes() {
    size_t used = 0;
    for (const auto& pair : files) {
        used += pair.second.length();
    }
    return used;
}

void MockFileSystem::clear() {
    files.clear();
}

size_t MockFileSystem::getFileCount() {
    return files.size();
}

void MockFileSystem::setMountFailure(bool shouldFail) {
    mountShouldFail = shouldFail;
}

void MockFileSystem::reset() {
    files.clear();
    isMounted = false;
    mountShouldFail = false;
}
