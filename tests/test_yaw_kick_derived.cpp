#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_yaw_kick_derived_from_rate, "YawKick") {
    std::cout << "\nTest: Yaw Kick Derived from Yaw Rate (Solution 2)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    
    engine.m_sop_yaw_gain = 1.0f;
    engine.m_yaw_accel_smoothing = 0.050f; // 50ms smoothing
    engine.m_sop_effect = 0.0f;
    engine.m_gain = 1.0f;
    engine.m_wheelbase_max_nm = 20.0f;
    engine.m_invert_force = false;
    
    // Scenario: Game reports 0 raw acceleration, but Yaw Rate is increasing.
    // This simulates a smooth yaw change that isn't captured in the game's noisy acceleration channel
    // or how we want to bypass it.
    
    data.mLocalRotAccel.y = 0.0; // Raw accel is zero
    data.mLocalRot.y = 0.0;      // Start at 0 rate
    data.mDeltaTime = 0.0025;    // 400Hz
    
    // Frame 1: Seed
    engine.calculate_force(&data);
    
    // Frame 2: Yaw Rate increases to 1.0 rad/s
    data.mLocalRot.y = 1.0;
    // Issue #397: Interpolation delay
    PumpEngineTime(engine, data, 0.0125);
    double force = engine.GetDebugBatch().back().total_output;
    
    // If derived from rate: 
    // derived_yaw_accel = (1.0 - 0.0) / 0.0025 = 400.0 rad/s^2
    // alpha = 0.0025 / (0.050 + 0.0025) = 0.0476
    // smoothed = 0.0 + 0.0476 * (400.0 - 0.0) = 19.04
    // force = -1.0 * 19.04 * 1.0 * 5.0 / 20.0 = -4.76 (clamped to -1.0)
    
    if (std::abs(force) > 0.1) {
        std::cout << "[PASS] Yaw Kick active from derived acceleration (Force: " << force << ")" << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Yaw Kick inactive or too weak. Derived acceleration ignored? Got: " << force);
    }
}

} // namespace FFBEngineTests
