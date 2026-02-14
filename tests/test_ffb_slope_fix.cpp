#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_slope_singularity_rejection, "SlopeFix") {
    std::cout << "\nTest: Slope Singularity Rejection (Projected Slope)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection_enabled = true;

    // Setup telemetry where SlipAngle is constant (dAlpha ~ 0)
    // but LateralG has a spike (dG >> 0)
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.05);
    data.mDeltaTime = 0.01;

    // Fill buffer with constant values
    for (int i = 0; i < 40; i++) {
        engine.calculate_force(&data);
    }

    // Inject spike in Lateral G
    data.mLocalAccel.x = 5.0 * 9.81; // 5G spike
    engine.calculate_force(&data);

    // Old behavior might explode. New behavior should stay near 0.
    // m_slope_current should be near 0 because (dG * dAlpha) is near 0
    ASSERT_NEAR(engine.m_slope_current, 0.0, 0.1);
    ASSERT_NEAR(engine.m_debug_slope_den, 0.0, 0.01); // Denominator (dAlpha^2 + epsilon)
}

TEST_CASE(test_slope_steady_state_hold, "SlopeFix") {
    std::cout << "\nTest: Slope Steady State Hold" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection_enabled = true;
    engine.m_slope_alpha_threshold = 0.02f;

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = 0.01;

    // 1. Frames 1-20 (Transient): Ramp SlipAngle and LateralG
    for (int i = 0; i < 20; i++) {
        double slip = 0.01 + (double)i * 0.01; // dAlpha/dt = 1.0 rad/s
        double g = 0.5 + (double)i * 0.05;
        data.mWheel[0].mLateralPatchVel = slip * 20.0;
        data.mWheel[1].mLateralPatchVel = slip * 20.0;
        data.mLocalAccel.x = g * 9.81;
        engine.calculate_force(&data);
    }

    double slope_transient = engine.m_slope_current;
    ASSERT_NEAR(engine.m_slope_hold_timer, 0.25, 0.001); // Reset to SLOPE_HOLD_TIME

    // 2. Frames 21-60 (Steady): Hold SlipAngle and LateralG constant
    // We need enough frames to clear the SG window (15 frames) plus some more to see the timer decrease.
    for (int i = 0; i < 25; i++) {
        engine.calculate_force(&data);
    }

    // Check: m_slope_hold_timer decreases but > 0
    ASSERT_TRUE(engine.m_slope_hold_timer < 0.25);
    ASSERT_TRUE(engine.m_slope_hold_timer > 0.0);

    // Check: m_slope_current does NOT decay yet (it should be exactly the same as last update)
    ASSERT_NEAR(engine.m_slope_current, slope_transient, 0.1); // Allow some small change due to SG trailing

    // 3. Continue holding until decay
    // Run for another 500 frames to ensure timer expires and significant decay happens
    for (int i = 0; i < 500; i++) {
        engine.calculate_force(&data);
    }

    // Check: Once timer expires, m_slope_current decays toward 0
    ASSERT_NEAR(engine.m_slope_hold_timer, 0.0, 0.001);
    ASSERT_NEAR(engine.m_slope_current, 0.0, 0.1);
}

TEST_CASE(test_input_smoothing, "SlopeFix") {
    std::cout << "\nTest: Slope Input Smoothing" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection_enabled = true;

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = 0.0025; // 400Hz

    // Feed noisy SlipAngle signal (alternating 0.05 and 0.06)
    double last_smoothed = 0.0;
    for (int i = 0; i < 100; i++) {
        double slip = (i % 2 == 0) ? 0.05 : 0.06;
        data.mWheel[0].mLateralPatchVel = slip * 20.0;
        data.mWheel[1].mLateralPatchVel = slip * 20.0;
        engine.calculate_force(&data);
        last_smoothed = engine.m_slope_slip_smoothed;
    }

    // Smoothed value should be between 0.05 and 0.06, and stable
    ASSERT_TRUE(last_smoothed > 0.051 && last_smoothed < 0.059);
}

} // namespace FFBEngineTests
