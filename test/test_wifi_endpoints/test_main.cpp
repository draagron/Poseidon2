/**
 * @file test_main.cpp
 * @brief WiFi API Endpoint Tests - Main Entry Point
 *
 * This test suite validates WiFi HTTP API endpoints:
 * - GET /wifi-config
 * - GET /wifi-status
 * - POST /upload-wifi-config
 *
 * @see user_requirements/R001 - foundation.md
 */

#ifdef UNIT_TEST

#include <unity.h>

// Forward declarations from individual test files
void test_upload_config_endpoint();
void test_get_config_endpoint();
void test_status_endpoint();
void test_invalid_config_handling();

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_upload_config_endpoint);
    RUN_TEST(test_get_config_endpoint);
    RUN_TEST(test_status_endpoint);
    RUN_TEST(test_invalid_config_handling);

    return UNITY_END();
}

#endif // UNIT_TEST
