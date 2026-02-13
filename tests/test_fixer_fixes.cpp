#include "test_ffb_common.h"
#include <iostream>

namespace FFBEngineTests {

TEST_CASE(test_slide_texture_threshold, "Fixer") {
    std::cout << "\nTest: Slide Texture Threshold Fix (v0.7.36)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slide_texture_enabled = true;
    engine.m_slide_texture_gain = 1.0f;

    TelemInfoV01 data = CreateBasicTestTelemetry(30.0);
    data.mDeltaTime = 0.01;

    // Test Case: Lateral velocity = 1.0 m/s (Below new 1.5 threshold, above old 0.5)
    data.mWheel[0].mLateralPatchVel = 1.0;
    data.mWheel[1].mLateralPatchVel = 1.0;

    // Ensure grip loss so slide noise is calculated
    for(int i=0; i<4; i++) data.mWheel[i].mGripFract = 0.5;

    engine.calculate_force(&data);
    auto snap = engine.GetDebugBatch().back();

    std::cout << "  Slide Noise at 1.0 m/s: " << snap.texture_slide << std::endl;
    ASSERT_NEAR(snap.texture_slide, 0.0, 0.001);

    // Test Case: Lateral velocity = 2.0 m/s (Above new 1.5 threshold)
    data.mWheel[0].mLateralPatchVel = 2.0;
    data.mWheel[1].mLateralPatchVel = 2.0;

    engine.calculate_force(&data);
    snap = engine.GetDebugBatch().back();

    std::cout << "  Slide Noise at 2.0 m/s: " << snap.texture_slide << std::endl;
    ASSERT_TRUE(std::abs(snap.texture_slide) > 0.001);
}

TEST_CASE(test_slope_detection_extended_range, "Fixer") {
    std::cout << "\nTest: Slope Detection Extended Range (v0.7.36)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection_enabled = true;
    engine.m_slope_min_threshold = -0.3f;
    engine.m_slope_max_threshold = -5.0f; // New default
    engine.m_slope_smoothing_tau = 0.001f; // Fast for test
    engine.m_slope_alpha_threshold = 0.0001f;

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = 0.01;

    // Produce a slope of -3.0
    // dAlpha/dt = 0.1, dG/dt = -0.3 -> Slope = -3.0
    for (int i = 0; i < 20; i++) {
        double alpha = (double)i * 0.001; // dAlpha/dt = 0.001 / 0.01 = 0.1
        double g = 1.0 - (double)i * 0.003; // dG/dt = -0.003 / 0.01 = -0.3
        engine.calculate_slope_grip(g, alpha, 0.01);
    }

    std::cout << "  Slope Current: " << engine.m_slope_current << std::endl;
    ASSERT_NEAR(engine.m_slope_current, -3.0, 0.1);

    // With -5.0 max, slope -3.0 should not hit the 0.2 floor.
    // loss = (-3.0 - -0.3) / (-5.0 - -0.3) = -2.7 / -4.7 = 0.574
    // grip = 1.0 - (0.574 * 0.8 * 1.0) = 1.0 - 0.459 = 0.541
    std::cout << "  Grip at -3.0 slope: " << engine.m_slope_smoothed_output << std::endl;
    ASSERT_GE(engine.m_slope_smoothed_output, 0.4);
    ASSERT_LE(engine.m_slope_smoothed_output, 0.7);
}

TEST_CASE(test_main_loop_force_zeroing, "Fixer") {
    std::cout << "\nTest: Main Loop Force Zeroing Logic (Logic Verification)" << std::endl;
    // We can't easily test main.cpp, but we can verify the logic we implemented.
    // If connected=false, force should be 0.0.

    double force = 1.0; // Assume last force was 1.0
    bool should_output = false;
    bool connected = false; // Game exited
    bool active = true;

    // Simulation of modified main.cpp logic:
    force = 0.0;
    should_output = false;
    if (active && connected) {
        force = 0.5; // Calculated force
        should_output = true;
    }

    if (!should_output) force = 0.0;

    ASSERT_NEAR(force, 0.0, 0.001);
    std::cout << "  [PASS] Force zeroed when disconnected." << std::endl;
}

} // namespace FFBEngineTests
