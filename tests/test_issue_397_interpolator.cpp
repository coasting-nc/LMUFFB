#include "test_ffb_common.h"
#include "../src/utils/MathUtils.h"

using namespace LMUFFB::Utils;

namespace FFBEngineTests {

TEST_CASE(test_interpolator_continuity, "Math") {
    std::cout << "\nTest: LinearExtrapolator (Interpolator) Continuity" << std::endl;
    LinearExtrapolator interp;
    interp.Configure(0.01); // 100Hz game tick

    double dt = 0.0025; // 400Hz FFB loop

    // Frame 0: Initial Value 0.0
    double out0 = interp.Process(0.0, dt, true);
    ASSERT_NEAR(out0, 0.0, 0.0001);

    // Step change at T=10ms: Input 0.0 -> 100.0
    // The new interpolator should start ramping FROM 0.0 at T=10ms
    double out1 = interp.Process(100.0, dt, true);

    // OLD (Extrapolation): Rate = (100-0)/0.01 = 10,000. Snap to 100.0.
    // NEW (Interpolation): Rate = (100-0)/0.01 = 10,000. Start at 0.0.

    // We expect the NEW logic (delayed interpolation)
    ASSERT_NEAR(out1, 0.0, 0.0001); // Should start at previous value

    // T=12.5ms (2.5ms since update)
    double out2 = interp.Process(100.0, dt, false);
    ASSERT_NEAR(out2, 25.0, 0.0001); // 25% of the way to 100

    // T=15.0ms (5.0ms since update)
    double out3 = interp.Process(100.0, dt, false);
    ASSERT_NEAR(out3, 50.0, 0.0001); // 50% of the way to 100

    // T=17.5ms (7.5ms since update)
    double out3b = interp.Process(100.0, dt, false);
    ASSERT_NEAR(out3b, 75.0, 0.0001);

    // T=20.0ms (10.0ms since update)
    double out4 = interp.Process(100.0, dt, false);
    ASSERT_NEAR(out4, 100.0, 0.0001); // Should have reached target
}

TEST_CASE(test_interpolator_no_overshoot, "Math") {
    std::cout << "\nTest: LinearExtrapolator (Interpolator) No Overshoot" << std::endl;
    LinearExtrapolator interp;
    interp.Configure(0.01);

    double dt = 0.0025;

    // Pulse: 0.0 -> 100.0 -> 0.0
    interp.Process(0.0, dt, true);
    interp.Process(100.0, dt, true); // Target is now 100

    // Advance 5ms
    interp.Process(100.0, dt, false);
    interp.Process(100.0, dt, false);

    // New frame at T=20ms: Input drops back to 0.0
    double out_snap = interp.Process(0.0, dt, true);

    // OLD: Would have overshot (rate was +10,000), then snapped back.
    // NEW: Should be exactly 100.0 (previous target), now moving toward 0.0.
    ASSERT_NEAR(out_snap, 100.0, 0.0001);

    // T=25.0ms (5ms since update)
    interp.Process(0.0, dt, false);
    double out_half = interp.Process(0.0, dt, false);
    ASSERT_NEAR(out_half, 50.0, 0.0001); // Halfway back to 0
}

TEST_CASE(test_nan_transition_reset, "Transition") {
    std::cout << "\nTest: NaN Transition Reset (Issue #397 Fix)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);

    // 1. Simulate the end of a previous session
    TelemInfoV01 data = CreateBasicTestTelemetry(0.0);
    data.mElapsedTime = 150.0;
    engine.calculate_force(&data, "Test", "TestCar", 0.0f, false);
    
    // 2. Artificially set the last log time to 150.0, as if a NaN log just happened.
    // The rate limiter normally requires 5 seconds to pass before logging again.
    FFBEngineTestAccess::SetLastCoreNanLogTime(engine, 150.0);

    // 3. Simulate the start of a NEW session (allowed = true) 1 second later.
    // CRITICAL: The very first frame contains NaN data.
    data.mElapsedTime = 151.0;
    data.mUnfilteredSteering = std::numeric_limits<double>::quiet_NaN();

    // Call engine with allowed = true. 
    // Before Issue #397, the NaN check at the top would execute first:
    //   - It sees NaN, checks rate limiter: 151.0 > 150.0 + 5.0 is FALSE.
    //   - Logs nothing, returns 0.0. The state transition (m_was_allowed) NEVER happens.
    // After Issue #397:
    //   - The transition check executes first. m_last_core_nan_log_time is reset to -999.0.
    //   - Then the NaN check executes: 151.0 > -999.0 + 5.0 is TRUE.
    //   - Logs the warning, and sets the timer to 151.0.
    double output = engine.calculate_force(&data, "Test", "TestCar", 0.0f, true);

    // Force should be 0.0 safely
    ASSERT_EQ(output, 0.0);

    // If the fix is correct, the timer should now be 151.0 (log was permitted because of the reset)
    // If the fix was missing, it would still be 150.0 (the log was suppressed).
    ASSERT_EQ(FFBEngineTestAccess::GetLastCoreNanLogTime(engine), 151.0);
}

} // namespace FFBEngineTests
