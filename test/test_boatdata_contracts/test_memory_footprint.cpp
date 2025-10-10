/**
 * @file test_memory_footprint.cpp
 * @brief Contract test for BoatDataStructure memory footprint
 *
 * Validates that the enhanced BoatDataStructure stays within constitutional memory limits.
 * Target: ≤600 bytes (estimated ~560 bytes)
 *
 * Test Coverage:
 * - BoatDataStructure total size ≤ 600 bytes
 * - Individual structure sizes reasonable
 * - Static assertion for compile-time check
 *
 * Constitutional Compliance: Principle II (Resource Management)
 *
 * @see specs/008-enhanced-boatdata-following/data-model.md
 * @see specs/008-enhanced-boatdata-following/tasks.md (T007)
 * @see .specify/memory/constitution.md (Principle II)
 */

#include <unity.h>
#include "types/BoatDataTypes.h"

void setUp(void) {}
void tearDown(void) {}

/**
 * @test Verify BoatDataStructure total size is within budget
 */
void test_boatdatastructure_size_within_budget(void) {
    size_t structSize = sizeof(BoatDataStructure);
    
    // Log size for reference
    char msg[64];
    snprintf(msg, sizeof(msg), "BoatDataStructure size: %zu bytes", structSize);
    TEST_MESSAGE(msg);
    
    // Constitutional limit: ≤600 bytes
    TEST_ASSERT_LESS_OR_EQUAL_MESSAGE(600, structSize, 
        "BoatDataStructure exceeds 600 byte budget (Constitutional Principle II)");
}

/**
 * @test Verify GPSData size is reasonable
 */
void test_gpsdata_size_reasonable(void) {
    size_t size = sizeof(GPSData);
    
    char msg[64];
    snprintf(msg, sizeof(msg), "GPSData size: %zu bytes", size);
    TEST_MESSAGE(msg);
    
    // Should be ~56 bytes (7 doubles × 8B)
    TEST_ASSERT_LESS_OR_EQUAL(80, size);
}

/**
 * @test Verify CompassData size is reasonable
 */
void test_compassdata_size_reasonable(void) {
    size_t size = sizeof(CompassData);
    
    char msg[64];
    snprintf(msg, sizeof(msg), "CompassData size: %zu bytes", size);
    TEST_MESSAGE(msg);
    
    // Should be ~64 bytes (8 doubles × 8B)
    TEST_ASSERT_LESS_OR_EQUAL(96, size);
}

/**
 * @test Verify DSTData size is reasonable
 */
void test_dstdata_size_reasonable(void) {
    size_t size = sizeof(DSTData);
    
    char msg[64];
    snprintf(msg, sizeof(msg), "DSTData size: %zu bytes", size);
    TEST_MESSAGE(msg);
    
    // Should be ~40 bytes (5 doubles × 8B)
    TEST_ASSERT_LESS_OR_EQUAL(64, size);
}

/**
 * @test Verify EngineData size is reasonable
 */
void test_enginedata_size_reasonable(void) {
    size_t size = sizeof(EngineData);
    
    char msg[64];
    snprintf(msg, sizeof(msg), "EngineData size: %zu bytes", size);
    TEST_MESSAGE(msg);
    
    // Should be ~40 bytes (5 doubles × 8B)
    TEST_ASSERT_LESS_OR_EQUAL(64, size);
}

/**
 * @test Verify SaildriveData size is reasonable
 */
void test_saildrivedata_size_reasonable(void) {
    size_t size = sizeof(SaildriveData);
    
    char msg[64];
    snprintf(msg, sizeof(msg), "SaildriveData size: %zu bytes", size);
    TEST_MESSAGE(msg);
    
    // Should be ~12 bytes (2 bools + unsigned long + padding)
    TEST_ASSERT_LESS_OR_EQUAL(24, size);
}

/**
 * @test Verify BatteryData size is reasonable
 */
void test_batterydata_size_reasonable(void) {
    size_t size = sizeof(BatteryData);
    
    char msg[64];
    snprintf(msg, sizeof(msg), "BatteryData size: %zu bytes", size);
    TEST_MESSAGE(msg);
    
    // Should be ~80 bytes (6 doubles + 4 bools + metadata)
    TEST_ASSERT_LESS_OR_EQUAL(128, size);
}

/**
 * @test Verify ShorePowerData size is reasonable
 */
void test_shorepowerdata_size_reasonable(void) {
    size_t size = sizeof(ShorePowerData);
    
    char msg[64];
    snprintf(msg, sizeof(msg), "ShorePowerData size: %zu bytes", size);
    TEST_MESSAGE(msg);
    
    // Should be ~20 bytes (1 double + 2 bools + metadata)
    TEST_ASSERT_LESS_OR_EQUAL(32, size);
}

/**
 * @test Verify memory footprint delta from v1.0.0 baseline
 */
void test_memory_delta_from_baseline(void) {
    size_t currentSize = sizeof(BoatDataStructure);
    const size_t v1_0_0_baseline = 304;  // Documented v1.0.0 size
    
    int delta = (int)currentSize - (int)v1_0_0_baseline;
    
    char msg[128];
    snprintf(msg, sizeof(msg), 
        "Memory footprint delta: %+d bytes (%.1f%% increase from v1.0.0)", 
        delta, (delta / (double)v1_0_0_baseline) * 100.0);
    TEST_MESSAGE(msg);
    
    // Expected delta: +256 bytes (documented in data-model.md)
    // Allow some variance due to compiler padding
    TEST_ASSERT_GREATER_OR_EQUAL(200, delta);
    TEST_ASSERT_LESS_OR_EQUAL(320, delta);
}

/**
 * @test Verify ESP32 RAM percentage (informational)
 */
void test_esp32_ram_percentage_info(void) {
    size_t structSize = sizeof(BoatDataStructure);
    const size_t esp32_ram = 327680;  // 320KB SRAM
    
    double percentage = (structSize / (double)esp32_ram) * 100.0;
    
    char msg[128];
    snprintf(msg, sizeof(msg), 
        "BoatDataStructure uses %.3f%% of ESP32 RAM (%zu / %zu bytes)",
        percentage, structSize, esp32_ram);
    TEST_MESSAGE(msg);
    
    // Should be well under 1% of ESP32 RAM
    TEST_ASSERT_LESS_THAN(1.0, percentage);
}

/**
 * @test Compile-time static assertion (will fail if struct too large)
 */
void test_static_assertion_compile_time(void) {
    // This test passes if code compiles
    // The static_assert in BoatDataTypes.h will catch violations at compile time
    TEST_PASS_MESSAGE("Static assertion passed at compile time");
}

// Test runner
int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    RUN_TEST(test_boatdatastructure_size_within_budget);
    RUN_TEST(test_gpsdata_size_reasonable);
    RUN_TEST(test_compassdata_size_reasonable);
    RUN_TEST(test_dstdata_size_reasonable);
    RUN_TEST(test_enginedata_size_reasonable);
    RUN_TEST(test_saildrivedata_size_reasonable);
    RUN_TEST(test_batterydata_size_reasonable);
    RUN_TEST(test_shorepowerdata_size_reasonable);
    RUN_TEST(test_memory_delta_from_baseline);
    RUN_TEST(test_esp32_ram_percentage_info);
    RUN_TEST(test_static_assertion_compile_time);
    
    return UNITY_END();
}
