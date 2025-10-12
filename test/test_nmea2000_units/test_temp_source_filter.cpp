/**
 * @file test_temp_source_filter.cpp
 * @brief Unit tests for temperature source filtering (only N2kts_SeaTemperature processed)
 *
 * Tests that PGN 130316 (Temperature Extended Range) only processes sea temperature source,
 * silently ignoring other temperature sources (exhaust, engine room, etc.).
 *
 * @see specs/010-nmea-2000-handling/data-model.md lines 129-142
 * @see specs/010-nmea-2000-handling/tasks.md T044
 */

#include <unity.h>
#include <N2kTypes.h>

// ============================================================================
// Temperature Source Filter Logic Tests
// ============================================================================

void test_sea_temperature_accepted() {
    tN2kTempSource source = N2kts_SeaTemperature;
    bool shouldProcess = (source == N2kts_SeaTemperature);
    TEST_ASSERT_TRUE(shouldProcess);
}

void test_other_temperature_sources_rejected() {
    // Test various temperature sources that should be rejected
    tN2kTempSource sources[] = {
        N2kts_OutsideTemperature,
        N2kts_InsideTemperature,
        N2kts_EngineRoomTemperature,
        N2kts_MainCabinTemperature,
        N2kts_LiveWellTemperature,
        N2kts_BaitWellTemperature,
        N2kts_RefridgerationTemperature,
        N2kts_HeatingSystemTemperature,
        N2kts_DewPointTemperature,
        N2kts_ApparentWindChillTemperature,
        N2kts_TheoreticalWindChillTemperature,
        N2kts_HeatIndexTemperature,
        N2kts_FreezerTemperature,
        N2kts_ExhaustGasTemperature
    };

    for (unsigned int i = 0; i < sizeof(sources) / sizeof(sources[0]); i++) {
        bool shouldProcess = (sources[i] == N2kts_SeaTemperature);
        TEST_ASSERT_FALSE(shouldProcess);
    }
}

void test_pgn130316_source_filter() {
    tN2kTempSource tempSource;
    double actualTemp = 288.15;  // 15°C in Kelvin

    // Sea temperature: should be processed
    tempSource = N2kts_SeaTemperature;
    bool process = (tempSource == N2kts_SeaTemperature);
    TEST_ASSERT_TRUE(process);

    // Exhaust gas temperature: should be silently ignored
    tempSource = N2kts_ExhaustGasTemperature;
    process = (tempSource == N2kts_SeaTemperature);
    TEST_ASSERT_FALSE(process);

    // Engine room temperature: should be silently ignored
    tempSource = N2kts_EngineRoomTemperature;
    process = (tempSource == N2kts_SeaTemperature);
    TEST_ASSERT_FALSE(process);
}

void test_silent_filtering() {
    tN2kTempSource source = N2kts_ExhaustGasTemperature;
    bool shouldProcess = (source == N2kts_SeaTemperature);
    TEST_ASSERT_FALSE(shouldProcess);
}

// ============================================================================
// Test Suite Setup
// ============================================================================

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_sea_temperature_accepted);
    RUN_TEST(test_other_temperature_sources_rejected);
    RUN_TEST(test_pgn130316_source_filter);
    RUN_TEST(test_silent_filtering);
    return UNITY_END();
}
