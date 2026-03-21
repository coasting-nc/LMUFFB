#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_yaw_kick_derived_from_rate, "YawKick") {
    std::cout << "\nTest: Yaw Kick Derived from Yaw Rate (Solution 2)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    
    engine.m_rear_axle.sop_yaw_gain = 1.0f;
    engine.m_rear_axle.yaw_accel_smoothing = 0.050f; // 50ms smoothing
    engine.m_rear_axle.sop_effect = 0.0f;
    engine.m_general.gain = 1.0f;
    engine.m_general.wheelbase_max_nm = 20.0f;
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
    data.mElapsedTime += 0.01;
    
    // Issue #397: Force propagates through 10ms interpolator
    double force = 0.0;
    for(int i=0; i<4; i++) {
        force = engine.calculate_force(&data, nullptr, nullptr, 0.0f, true, 0.0025);
    }
    
    if (std::abs(force) > 0.05) {
        std::cout << "[PASS] Yaw Kick active from derived acceleration (Force: " << force << ")" << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Yaw Kick inactive or too weak. Derived acceleration ignored? Got: " << force);
    }
}

} // namespace FFBEngineTests
