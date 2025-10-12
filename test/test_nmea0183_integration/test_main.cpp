#include <unity.h>

// Integration test functions
void test_rsa_to_boatdata();
void test_hdm_to_boatdata();
void test_gga_to_boatdata();
void test_rmc_to_boatdata();
void test_vtg_to_boatdata();
void test_multi_source_priority();
void test_talker_id_filter();
void test_message_type_filter();
void test_invalid_sentences();

void setUp() {
    // Set up before each test
}

void tearDown() {
    // Clean up after each test
}

int main(int argc, char** argv) {
    UNITY_BEGIN();

    // Integration scenarios
    RUN_TEST(test_rsa_to_boatdata);
    RUN_TEST(test_hdm_to_boatdata);
    RUN_TEST(test_gga_to_boatdata);
    RUN_TEST(test_rmc_to_boatdata);
    RUN_TEST(test_vtg_to_boatdata);
    RUN_TEST(test_multi_source_priority);
    RUN_TEST(test_talker_id_filter);
    RUN_TEST(test_message_type_filter);
    RUN_TEST(test_invalid_sentences);

    return UNITY_END();
}
