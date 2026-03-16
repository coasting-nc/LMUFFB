#include "test_ffb_common.h"
#include "../src/ffb/UpSampler.h"
#include <vector>

namespace FFBEngineTests {

using namespace ffb_math;

// Regression test for Issue #393: Phase Alignment (The "Stutter" Bug)
// Currently, the resampler shifts its history buffer as soon as a new physics sample arrives.
// It SHOULD wait until the phase wraps around (m_phase >= 5).
TEST_CASE(test_upsampler_issue_393_resampler_alignment, "UpSampler") {
    PolyphaseResampler resampler;
    resampler.Reset();

    // The resampler starts at phase 0.
    // 1000Hz ticks:
    // Tick 0: is_new_physics_tick = true. Phase 0 -> 2.
    // Tick 1: is_new_physics_tick = false. Phase 2 -> 4.
    // Tick 2: is_new_physics_tick = false. Phase 4 -> 1. (WRAP! Needs shift for Tick 3)
    // Tick 3: is_new_physics_tick = true. Phase 1 -> 3.
    // Tick 4: is_new_physics_tick = false. Phase 3 -> 0. (WRAP! Needs shift for Tick 5)

    // Initial state: History [0, 0, 0]

    // Tick 0: New physics sample P0 = 1.0 arrives.
    // CORRECT BEHAVIOR: History remains [0, 0, 0] for this tick.
    // Output = c[0]*0 + c[1]*0 + c[2]*0 = 0.0

    double p0 = 1.0;
    double out0 = resampler.Process(p0, true);

    ASSERT_NEAR(out0, 0.0, 0.0001);
}

} // namespace FFBEngineTests
