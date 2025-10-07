/**
 * @file test_icalibration_contract.cpp
 * @brief Contract test for ICalibration interface
 *
 * This test verifies the ICalibration interface contract:
 * - getCalibration() returns current parameters
 * - setCalibration() updates parameters in memory
 * - validateCalibration() rejects invalid values (K ≤ 0, offset out of range)
 * - loadFromFlash() loads persisted values
 * - saveToFlash() persists values to filesystem
 * - Default values used when file missing
 *
 * @see specs/003-boatdata-feature-as/contracts/README.md lines 44-54
 * @test T006
 */

#include <unity.h>
#include "../../src/types/BoatDataTypes.h"
#include <map>
#include <string>

// Forward declaration - interface will be implemented in Phase 3.3
class ICalibration {
public:
    virtual ~ICalibration() {}

    /**
     * @brief Get current calibration parameters
     * @return Current calibration parameters
     */
    virtual CalibrationParameters getCalibration() = 0;

    /**
     * @brief Set calibration parameters (in memory)
     * @param params New calibration parameters
     * @return true if set successfully, false if invalid
     */
    virtual bool setCalibration(const CalibrationParameters& params) = 0;

    /**
     * @brief Validate calibration parameters
     * @param params Parameters to validate
     * @return true if valid, false if invalid
     */
    virtual bool validateCalibration(const CalibrationParameters& params) = 0;

    /**
     * @brief Load calibration from flash filesystem
     * @return true if loaded successfully, false if file missing or invalid
     */
    virtual bool loadFromFlash() = 0;

    /**
     * @brief Save calibration to flash filesystem
     * @param params Parameters to persist
     * @return true if saved successfully, false on error
     */
    virtual bool saveToFlash(const CalibrationParameters& params) = 0;
};

// Mock filesystem for testing (simulates LittleFS)
class MockFileSystem {
private:
    std::map<std::string, std::string> files;

public:
    bool fileExists(const char* path) {
        return files.find(path) != files.end();
    }

    bool writeFile(const char* path, const char* content) {
        files[path] = std::string(content);
        return true;
    }

    bool readFile(const char* path, char* buffer, size_t maxSize) {
        if (!fileExists(path)) return false;
        std::string content = files[path];
        strncpy(buffer, content.c_str(), maxSize);
        return true;
    }

    void deleteFile(const char* path) {
        files.erase(path);
    }

    void clear() {
        files.clear();
    }
};

// Mock implementation for testing the interface contract
class MockCalibrationManager : public ICalibration {
private:
    CalibrationParameters currentParams;
    MockFileSystem* fs;

public:
    MockCalibrationManager(MockFileSystem* filesystem) : fs(filesystem) {
        // Initialize with defaults
        currentParams.leewayCalibrationFactor = DEFAULT_LEEWAY_K_FACTOR;
        currentParams.windAngleOffset = DEFAULT_WIND_ANGLE_OFFSET;
        currentParams.version = 1;
        currentParams.lastModified = 0;
        currentParams.valid = false;
    }

    CalibrationParameters getCalibration() override {
        return currentParams;
    }

    bool setCalibration(const CalibrationParameters& params) override {
        if (!validateCalibration(params)) {
            return false;
        }
        currentParams = params;
        currentParams.valid = true;
        return true;
    }

    bool validateCalibration(const CalibrationParameters& params) override {
        // K factor must be positive
        if (params.leewayCalibrationFactor <= 0.0) {
            return false;
        }

        // Wind angle offset must be in range [-2π, 2π]
        if (params.windAngleOffset < -2 * M_PI || params.windAngleOffset > 2 * M_PI) {
            return false;
        }

        return true;
    }

    bool loadFromFlash() override {
        // Simulate reading from /calibration.json
        char buffer[256];
        if (!fs->readFile("/calibration.json", buffer, sizeof(buffer))) {
            // File not found - use defaults
            return false;
        }

        // Simplified JSON parsing (real implementation would use ArduinoJson)
        // Format: {"leewayKFactor":0.65,"windAngleOffset":0.087}
        CalibrationParameters loaded;
        if (sscanf(buffer, "{\"leewayKFactor\":%lf,\"windAngleOffset\":%lf}",
                   &loaded.leewayCalibrationFactor,
                   &loaded.windAngleOffset) == 2) {
            loaded.version = 1;
            loaded.lastModified = millis();
            loaded.valid = true;

            if (validateCalibration(loaded)) {
                currentParams = loaded;
                return true;
            }
        }

        return false;
    }

    bool saveToFlash(const CalibrationParameters& params) override {
        if (!validateCalibration(params)) {
            return false;
        }

        // Simplified JSON writing (real implementation would use ArduinoJson)
        char buffer[256];
        snprintf(buffer, sizeof(buffer),
                 "{\"leewayKFactor\":%.2f,\"windAngleOffset\":%.3f}",
                 params.leewayCalibrationFactor,
                 params.windAngleOffset);

        return fs->writeFile("/calibration.json", buffer);
    }
};

// Test fixture
static MockFileSystem* mockFS = nullptr;
static MockCalibrationManager* calibMgr = nullptr;

void setUp(void) {
    mockFS = new MockFileSystem();
    calibMgr = new MockCalibrationManager(mockFS);
}

void tearDown(void) {
    delete calibMgr;
    delete mockFS;
    calibMgr = nullptr;
    mockFS = nullptr;
}

// =============================================================================
// GET/SET CALIBRATION TESTS
// =============================================================================

void test_getCalibration_returns_defaults() {
    // Act: Get calibration (no file, should be defaults)
    CalibrationParameters params = calibMgr->getCalibration();

    // Assert: Default values
    TEST_ASSERT_EQUAL_DOUBLE(DEFAULT_LEEWAY_K_FACTOR, params.leewayCalibrationFactor);
    TEST_ASSERT_EQUAL_DOUBLE(DEFAULT_WIND_ANGLE_OFFSET, params.windAngleOffset);
    TEST_ASSERT_FALSE(params.valid);  // Not loaded from flash
}

void test_setCalibration_updates_in_memory() {
    // Arrange: Create new calibration
    CalibrationParameters newParams;
    newParams.leewayCalibrationFactor = 0.65;
    newParams.windAngleOffset = 0.087;  // 5° masthead offset
    newParams.version = 1;
    newParams.lastModified = 12345;

    // Act: Set calibration
    bool result = calibMgr->setCalibration(newParams);

    // Assert: Set succeeded
    TEST_ASSERT_TRUE(result);

    // Assert: Calibration updated
    CalibrationParameters retrieved = calibMgr->getCalibration();
    TEST_ASSERT_EQUAL_DOUBLE(0.65, retrieved.leewayCalibrationFactor);
    TEST_ASSERT_EQUAL_DOUBLE(0.087, retrieved.windAngleOffset);
    TEST_ASSERT_TRUE(retrieved.valid);
}

void test_setCalibration_round_trip() {
    // Arrange
    CalibrationParameters params;
    params.leewayCalibrationFactor = 1.5;
    params.windAngleOffset = -0.05;  // -3° offset
    params.version = 1;

    // Act: Write and read back
    calibMgr->setCalibration(params);
    CalibrationParameters retrieved = calibMgr->getCalibration();

    // Assert: Values match
    TEST_ASSERT_EQUAL_DOUBLE(1.5, retrieved.leewayCalibrationFactor);
    TEST_ASSERT_EQUAL_DOUBLE(-0.05, retrieved.windAngleOffset);
}

// =============================================================================
// VALIDATION TESTS
// =============================================================================

void test_validateCalibration_accepts_valid_params() {
    // Arrange: Valid parameters
    CalibrationParameters params;
    params.leewayCalibrationFactor = 0.65;
    params.windAngleOffset = 0.087;
    params.version = 1;

    // Act: Validate
    bool result = calibMgr->validateCalibration(params);

    // Assert: Validation passes
    TEST_ASSERT_TRUE(result);
}

void test_validateCalibration_rejects_zero_K_factor() {
    // Arrange: Invalid K factor (K = 0)
    CalibrationParameters params;
    params.leewayCalibrationFactor = 0.0;  // INVALID: K must be > 0
    params.windAngleOffset = 0.087;
    params.version = 1;

    // Act: Validate
    bool result = calibMgr->validateCalibration(params);

    // Assert: Validation fails
    TEST_ASSERT_FALSE(result);
}

void test_validateCalibration_rejects_negative_K_factor() {
    // Arrange: Invalid K factor (K < 0)
    CalibrationParameters params;
    params.leewayCalibrationFactor = -0.5;  // INVALID: K must be > 0
    params.windAngleOffset = 0.087;
    params.version = 1;

    // Act: Validate
    bool result = calibMgr->validateCalibration(params);

    // Assert: Validation fails
    TEST_ASSERT_FALSE(result);
}

void test_validateCalibration_rejects_offset_out_of_range() {
    // Arrange: Invalid wind offset (> 2π)
    CalibrationParameters params;
    params.leewayCalibrationFactor = 1.0;
    params.windAngleOffset = 7.0;  // INVALID: > 2π (6.283)
    params.version = 1;

    // Act: Validate
    bool result = calibMgr->validateCalibration(params);

    // Assert: Validation fails
    TEST_ASSERT_FALSE(result);
}

void test_setCalibration_rejects_invalid_params() {
    // Arrange: Invalid parameters (K <= 0)
    CalibrationParameters params;
    params.leewayCalibrationFactor = 0.0;  // INVALID
    params.windAngleOffset = 0.087;
    params.version = 1;

    // Act: Try to set
    bool result = calibMgr->setCalibration(params);

    // Assert: Set rejected
    TEST_ASSERT_FALSE(result);

    // Assert: Calibration unchanged (still defaults)
    CalibrationParameters current = calibMgr->getCalibration();
    TEST_ASSERT_EQUAL_DOUBLE(DEFAULT_LEEWAY_K_FACTOR, current.leewayCalibrationFactor);
}

// =============================================================================
// PERSISTENCE TESTS
// =============================================================================

void test_loadFromFlash_returns_false_when_file_missing() {
    // Act: Try to load (no file exists)
    bool result = calibMgr->loadFromFlash();

    // Assert: Load failed
    TEST_ASSERT_FALSE(result);

    // Assert: Still using defaults
    CalibrationParameters params = calibMgr->getCalibration();
    TEST_ASSERT_EQUAL_DOUBLE(DEFAULT_LEEWAY_K_FACTOR, params.leewayCalibrationFactor);
}

void test_saveToFlash_persists_calibration() {
    // Arrange: Create calibration
    CalibrationParameters params;
    params.leewayCalibrationFactor = 0.65;
    params.windAngleOffset = 0.087;
    params.version = 1;

    // Act: Save to flash
    bool result = calibMgr->saveToFlash(params);

    // Assert: Save succeeded
    TEST_ASSERT_TRUE(result);

    // Assert: File exists in mock filesystem
    TEST_ASSERT_TRUE(mockFS->fileExists("/calibration.json"));
}

void test_saveToFlash_rejects_invalid_params() {
    // Arrange: Invalid parameters
    CalibrationParameters params;
    params.leewayCalibrationFactor = -1.0;  // INVALID
    params.windAngleOffset = 0.087;
    params.version = 1;

    // Act: Try to save
    bool result = calibMgr->saveToFlash(params);

    // Assert: Save rejected
    TEST_ASSERT_FALSE(result);
}

void test_loadFromFlash_restores_saved_calibration() {
    // Arrange: Save calibration first
    CalibrationParameters saved;
    saved.leewayCalibrationFactor = 0.75;
    saved.windAngleOffset = 0.1;
    saved.version = 1;
    calibMgr->saveToFlash(saved);

    // Create new manager instance (simulates reboot)
    delete calibMgr;
    calibMgr = new MockCalibrationManager(mockFS);

    // Act: Load from flash
    bool result = calibMgr->loadFromFlash();

    // Assert: Load succeeded
    TEST_ASSERT_TRUE(result);

    // Assert: Calibration restored
    CalibrationParameters loaded = calibMgr->getCalibration();
    TEST_ASSERT_EQUAL_DOUBLE(0.75, loaded.leewayCalibrationFactor);
    TEST_ASSERT_EQUAL_DOUBLE(0.1, loaded.windAngleOffset);
    TEST_ASSERT_TRUE(loaded.valid);
}

void test_calibration_persists_across_reboot() {
    // Arrange: Set and save calibration
    CalibrationParameters params;
    params.leewayCalibrationFactor = 1.2;
    params.windAngleOffset = -0.05;
    params.version = 1;

    calibMgr->setCalibration(params);
    calibMgr->saveToFlash(params);

    // Simulate reboot: destroy and recreate manager
    delete calibMgr;
    calibMgr = new MockCalibrationManager(mockFS);

    // Act: Load persisted calibration
    calibMgr->loadFromFlash();

    // Assert: Calibration survives reboot
    CalibrationParameters restored = calibMgr->getCalibration();
    TEST_ASSERT_EQUAL_DOUBLE(1.2, restored.leewayCalibrationFactor);
    TEST_ASSERT_EQUAL_DOUBLE(-0.05, restored.windAngleOffset);
}

// =============================================================================
// EDGE CASES
// =============================================================================

void test_validateCalibration_accepts_boundary_values() {
    // Arrange: K factor at minimum boundary
    CalibrationParameters params1;
    params1.leewayCalibrationFactor = 0.001;  // Very small but > 0
    params1.windAngleOffset = 0.0;
    params1.version = 1;

    // Act & Assert: Should accept
    TEST_ASSERT_TRUE(calibMgr->validateCalibration(params1));

    // Arrange: Wind offset at boundaries
    CalibrationParameters params2;
    params2.leewayCalibrationFactor = 1.0;
    params2.windAngleOffset = 2 * M_PI;  // Maximum positive
    params2.version = 1;

    TEST_ASSERT_TRUE(calibMgr->validateCalibration(params2));

    CalibrationParameters params3;
    params3.leewayCalibrationFactor = 1.0;
    params3.windAngleOffset = -2 * M_PI;  // Maximum negative
    params3.version = 1;

    TEST_ASSERT_TRUE(calibMgr->validateCalibration(params3));
}

// =============================================================================
// TEST RUNNER
// =============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // Get/Set calibration tests
    RUN_TEST(test_getCalibration_returns_defaults);
    RUN_TEST(test_setCalibration_updates_in_memory);
    RUN_TEST(test_setCalibration_round_trip);

    // Validation tests
    RUN_TEST(test_validateCalibration_accepts_valid_params);
    RUN_TEST(test_validateCalibration_rejects_zero_K_factor);
    RUN_TEST(test_validateCalibration_rejects_negative_K_factor);
    RUN_TEST(test_validateCalibration_rejects_offset_out_of_range);
    RUN_TEST(test_setCalibration_rejects_invalid_params);

    // Persistence tests
    RUN_TEST(test_loadFromFlash_returns_false_when_file_missing);
    RUN_TEST(test_saveToFlash_persists_calibration);
    RUN_TEST(test_saveToFlash_rejects_invalid_params);
    RUN_TEST(test_loadFromFlash_restores_saved_calibration);
    RUN_TEST(test_calibration_persists_across_reboot);

    // Edge cases
    RUN_TEST(test_validateCalibration_accepts_boundary_values);

    return UNITY_END();
}
