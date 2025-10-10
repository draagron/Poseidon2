/**
 * @file test_bugfix_001_animation_icon.cpp
 * @brief Regression tests for bugfix-001 - Animation icon brackets removal
 *
 * Tests: Animation icon rendering without square brackets
 * Bug: Animation shows "[ / ]" instead of clean icon "/"
 * Fix: Remove hardcoded bracket prints, adjust cursor position
 *
 * These tests MUST FAIL before the fix is applied (TDD approach)
 *
 * @version 1.0.0
 * @date 2025-10-10
 */

#include <unity.h>
#include "mocks/MockDisplayAdapter.h"
#include "mocks/MockSystemMetrics.h"
#include "components/DisplayManager.h"
#include "types/DisplayTypes.h"

extern MockDisplayAdapter* g_mockDisplay;
extern MockSystemMetrics* g_mockMetrics;

/**
 * @brief Test: Animation icon renders WITHOUT square brackets
 *
 * Regression test for bugfix-001 part 4:
 * - renderStatusPage() or updateAnimationIcon() called
 * - Animation icon should render as single character: /
 * - Should NOT render brackets: [ / ]
 */
void test_animation_icon_has_no_brackets() {
    // Arrange
    DisplayManager dm(g_mockDisplay, g_mockMetrics);
    dm.init();

    // Act: Render status page with animation
    g_mockDisplay->reset();
    dm.renderStatusPage();

    // Assert: No brackets printed
    const char* renderedText = g_mockDisplay->getRenderedText();

    TEST_ASSERT_FALSE_MESSAGE(
        g_mockDisplay->wasTextRendered("[ "),
        "Animation icon should NOT have opening bracket '[ '"
    );
    TEST_ASSERT_FALSE_MESSAGE(
        g_mockDisplay->wasTextRendered(" ]"),
        "Animation icon should NOT have closing bracket ' ]'"
    );

    // Verify icon character IS rendered (one of: /, -, \, |)
    bool hasIcon = g_mockDisplay->wasTextRendered("/") ||
                   g_mockDisplay->wasTextRendered("-") ||
                   g_mockDisplay->wasTextRendered("\\") ||
                   g_mockDisplay->wasTextRendered("|");

    TEST_ASSERT_TRUE_MESSAGE(
        hasIcon,
        "Animation icon should render one of: /, -, \\, |"
    );
}

/**
 * @brief Test: Animation icon cursor position is adjusted
 *
 * Regression test for bugfix-001 part 5:
 * - Cursor X-position should be 118 (not 108)
 * - Old: 128 - 20 pixels = 108 (for "[ X ]")
 * - New: 128 - 10 pixels = 118 (for " X ")
 */
void test_animation_icon_cursor_position_correct() {
    // Arrange
    DisplayManager dm(g_mockDisplay, g_mockMetrics);
    dm.init();

    // Act: Update animation
    g_mockDisplay->reset();
    dm.updateAnimationIcon();

    // Assert: Cursor set to X=118, Y=50 (line 5)
    uint8_t cursorX = g_mockDisplay->getCursorX();
    uint8_t cursorY = g_mockDisplay->getCursorY();

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(
        118,
        cursorX,
        "Animation icon cursor X-position should be 118 (not 108)"
    );
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(
        50,
        cursorY,
        "Animation icon cursor Y-position should be 50 (line 5)"
    );
}

/**
 * @brief Test: Animation cycles through 4 states without brackets
 *
 * Regression test for bugfix-001 part 6:
 * - Animation should cycle: / → - → \ → | → /
 * - Each state should render as single character (no brackets)
 */
void test_animation_cycles_through_states_without_brackets() {
    // Arrange
    DisplayManager dm(g_mockDisplay, g_mockMetrics);
    dm.init();

    // Expected icon sequence (one full cycle + wrap)
    const char* expectedIcons[] = {"/", "-", "\\", "|", "/"};

    // Act & Assert: Cycle through 5 states
    for (int i = 0; i < 5; i++) {
        g_mockDisplay->reset();
        dm.updateAnimationIcon();

        // Verify icon character rendered
        bool hasExpectedIcon = g_mockDisplay->wasTextRendered(expectedIcons[i]);

        char msg[128];
        snprintf(msg, sizeof(msg),
                 "Animation state %d should render '%s'", i, expectedIcons[i]);
        TEST_ASSERT_TRUE_MESSAGE(hasExpectedIcon, msg);

        // Verify NO brackets
        TEST_ASSERT_FALSE_MESSAGE(
            g_mockDisplay->wasTextRendered("[ "),
            "No opening bracket in any animation state"
        );
        TEST_ASSERT_FALSE_MESSAGE(
            g_mockDisplay->wasTextRendered(" ]"),
            "No closing bracket in any animation state"
        );
    }
}

/**
 * @brief Test: updateAnimationIcon() with explicit metrics
 *
 * Regression test for updateAnimationIcon(const DisplayMetrics&) overload
 */
void test_update_animation_icon_with_metrics_no_brackets() {
    // Arrange
    DisplayManager dm(g_mockDisplay, g_mockMetrics);
    dm.init();

    DisplayMetrics metrics;
    metrics.animationState = 2;  // Should render '\'

    // Act
    g_mockDisplay->reset();
    dm.updateAnimationIcon(metrics);

    // Assert
    TEST_ASSERT_TRUE_MESSAGE(
        g_mockDisplay->wasTextRendered("\\"),
        "Animation state 2 should render '\\'"
    );
    TEST_ASSERT_FALSE_MESSAGE(
        g_mockDisplay->wasTextRendered("[ "),
        "No opening bracket with explicit metrics"
    );
    TEST_ASSERT_FALSE_MESSAGE(
        g_mockDisplay->wasTextRendered(" ]"),
        "No closing bracket with explicit metrics"
    );
}
