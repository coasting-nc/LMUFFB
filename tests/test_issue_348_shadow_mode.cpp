#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_issue_348_slope_shadow_mode, "SlopeDetection") {
    std::cout << "\nTest: Issue #348 Slope Detection Shadow Mode" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);

    // Enable slope detection
    engine.m_slope_detection_enabled = true;
    engine.m_slope_sg_window = 9;

    // Simulate unencrypted telemetry (high grip value)
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = 0.01;
    data.mWheel[0].mGripFract = 0.95f; // Valid grip
    data.mWheel[1].mGripFract = 0.95f;

    // Verify that slope detection still updates its internal state even when grip is valid
    // We'll create a scenario with negative slope (grip loss)
    for (int i = 0; i < 20; i++) {
        double slip = 0.05 + (double)i * 0.002;
        double g = 1.5 - (double)i * 0.02;

        data.mWheel[0].mLateralPatchVel = slip * 20.0;
        data.mWheel[1].mLateralPatchVel = slip * 20.0;
        data.mLocalAccel.x = g * 9.81;

        engine.calculate_force(&data);
    }

    // Slope should be negative even though FFB is using raw grip
    std::cout << "  Slope Current (Shadow): " << engine.m_slope_current << std::endl;
    std::cout << "  Slope Smoothed (Shadow): " << engine.m_slope_smoothed_output << std::endl;

    ASSERT_LE(engine.m_slope_current, -5.0);
    ASSERT_LE(engine.m_slope_smoothed_output, 0.9);
}

} // namespace FFBEngineTests
