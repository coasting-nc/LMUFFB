#include "test_ffb_common.h"
#include "ffb/FFBSafetyMonitor.h"
#include <iostream>
#include <vector>

namespace FFBEngineTests {

/**
 * Test for Issue #426: Pause/Menu Spikes when the wheel is off-center
 * Verifies that the restricted slew rate allows for a smooth fade-out.
 */
void test_issue_426_restricted_slew_rate() {
    std::cout << "\nTest: Issue #426 Restricted Slew Rate (Fade-out)" << std::endl;

    FFBSafetyMonitor safety;
    double dt = 0.0025; // 400Hz
    double current_force = 1.0;
    double target_force = 0.0;

    // Scenario A: Normal Slew (Restricted = false)
    // SAFETY_SLEW_NORMAL is 1000.0.
    // Max delta = 1000 * 0.0025 = 2.5
    // From 1.0 to 0.0 should take 1 frame.
    double next_force_normal = safety.ApplySafetySlew(target_force, current_force, dt, false);
    std::cout << "  Normal Slew (1.0 -> 0.0): " << next_force_normal << " (Expected: 0.0)" << std::endl;
    ASSERT_EQ(next_force_normal, 0.0);

    // Scenario B: Restricted Slew (Restricted = true)
    // Old value: 100.0 -> Max delta = 100 * 0.0025 = 0.25. Reaches 0 in 4 frames (10ms).
    // New value: 2.0 -> Max delta = 2.0 * 0.0025 = 0.005. Reaches 0 in 200 frames (500ms).

    double next_force_restricted = safety.ApplySafetySlew(target_force, current_force, dt, true);
    double delta = std::abs(current_force - next_force_restricted);

    std::cout << "  Restricted Slew (1.0 -> 0.0) first frame delta: " << delta << std::endl;

    // We expect the delta to match the slew limit * dt
    // If the fix is NOT applied (100.0), delta will be 0.25
    // If the fix IS applied (2.0), delta will be 0.005

    // To confirm it takes ~0.5s to reach zero with the fix:
    int frames_to_zero = 0;
    double f = 1.0;
    while (f > 0.0 && frames_to_zero < 1000) {
        f = safety.ApplySafetySlew(0.0, f, dt, true);
        frames_to_zero++;
    }

    double time_to_zero = frames_to_zero * dt;
    std::cout << "  Frames to reach 0.0 (Restricted): " << frames_to_zero << " (" << time_to_zero << "s)" << std::endl;

    // If SAFETY_SLEW_RESTRICTED is 100.0 (Buggy state):
    // frames = 1.0 / 0.25 = 4 frames.
    // If SAFETY_SLEW_RESTRICTED is 2.0 (Fixed state):
    // frames = 1.0 / 0.005 = 200 frames.

    // This assertion will FAIL before the fix if we assert it takes > 0.1s
    ASSERT_GT(time_to_zero, 0.1);
    ASSERT_NEAR(time_to_zero, 0.5, 0.01);
}

AutoRegister reg_issue_426_spikes("Issue #426 Pause Spikes", "Issue426", {"Safety", "Slew"}, []() {
    test_issue_426_restricted_slew_rate();
});

} // namespace FFBEngineTests
