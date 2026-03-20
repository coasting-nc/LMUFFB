#include "test_ffb_common.h"
#include "../src/ffb/FFBEngine.h"
#include <iostream>

namespace FFBEngineTests {

TEST_CASE(test_soft_lock_normalization_consistency, "Regression") {
    std::cout << "Test: Soft Lock Normalization Consistency (#181)" << std::endl;

    auto get_soft_lock_output = [](FFBEngine& engine, double steer, double peak) {
        TelemInfoV01 data = CreateBasicTestTelemetry();
        data.mUnfilteredSteering = steer;
        data.mDeltaTime = 0.0025;

        // Manually set session peak to simulate learned state
        FFBEngineTestAccess::SetSessionPeakTorque(engine, peak);
        // Update smoothed mult accordingly
        FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / peak);

        // Issue #397: Flush the 10ms transient ramp
        return PumpEngineTime(engine, data, 0.015);
    };

    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_soft_lock_enabled = true;
    engine.m_soft_lock_stiffness = 100.0f; // 100%
    engine.m_soft_lock_damping = 0.0f;
    engine.m_general.wheelbase_max_nm = 15.0f;
    engine.m_general.target_rim_nm = 10.0f;
    engine.m_general.gain = 1.0f;
    engine.m_steering_shaft_gain = 0.0f; // Mute other forces

    // Small excess to test non-clamped behavior if necessary,
    // but at 100% stiffness it should be strong.
    double force_low_peak = get_soft_lock_output(engine, 1.001, 1.0);
    double force_high_peak = get_soft_lock_output(engine, 1.001, 50.0);

    std::cout << "  Force at 1.001 steer, 1.0Nm peak: " << force_low_peak << std::endl;
    std::cout << "  Force at 1.001 steer, 50.0Nm peak: " << force_high_peak << std::endl;

    // After fix, these should be identical because soft lock is scaled by wheelbase_max_nm only.
    // Stiffness 100 -> excess_for_max = 5 / 10000 = 0.0005
    // SoftLockNm at 1.001 (0.001 excess):
    // spring_nm = min(1, 0.001 / 0.0005) * 15 * 2 = 1.0 * 30 = 30 Nm.
    // di_texture = 30 / 15 = 2.0 -> clamped to -1.0
    ASSERT_NEAR(force_low_peak, -1.0, 0.001);
    ASSERT_NEAR(force_high_peak, force_low_peak, 0.000001);

    // Verify it reaches full force at 1% excess
    double force_full = get_soft_lock_output(engine, 1.01, 25.0);
    std::cout << "  Force at 1.01 steer (1% excess): " << force_full << std::endl;
    ASSERT_NEAR(force_full, -1.0, 0.001);
}

} // namespace
