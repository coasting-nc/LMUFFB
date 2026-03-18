#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_issue_241_rectification_fix, "YawGyro") {
    std::cout << "\nTest: Issue #241 Yaw Kick Rectification Fix" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);

    // Setup: Isolate Yaw Kick effect
    engine.m_sop_yaw_gain = 1.0f;
    engine.m_yaw_kick_threshold = 1.0f; // Threshold = 1.0
    engine.m_yaw_accel_smoothing = 0.05f; // Smoothing tau
    engine.m_wheelbase_max_nm = 20.0f;
    engine.m_target_rim_nm = 20.0f;
    engine.m_gain = 1.0f;
    engine.m_invert_force = false;

    // Disable other effects
    engine.m_understeer_effect = 0.0f;
    engine.m_sop_effect = 0.0f;
    engine.m_lat_load_effect = 0.0f;
    engine.m_gyro_gain = 0.0f;
    engine.m_rear_align_effect = 0.0f;

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0); // Speed = 20.0 (> 5.0 cutoff)
    data.mDeltaTime = 0.0025; // 400Hz

    // Simulate high-frequency noise that would be rectified by the old code.
    // Oscillate between +2.0 and -1.5 around a small drift of 0.25.
    // Mean = (2.0 - 1.5) / 2 = 0.25.
    // Old code (if threshold=1.0):
    // +2.0 passes -> 2.0
    // -1.5 fails -> 0.0
    // Rectified mean = (2.0 + 0.0) / 2 = 1.0. (DC OFFSET!)

    // New code:
    // Smoothing first: 0.25 (average of +2.0 and -1.5)
    // Threshold (1.0) applied to 0.25 -> 0.0. (CORRECT!)

    double sum_force = 0.0;
    int frames = 100;
    data.mLocalRot.y = 0.0;
    engine.calculate_force(&data); // Seed
    
    for (int i = 0; i < frames; i++) {
        double accel = (i % 2 == 0) ? 2.0 : -1.5;
        data.mLocalRot.y += accel * 0.0025;
        sum_force += engine.calculate_force(&data);
    }

    double avg_force = sum_force / frames;

    // With the fix, the high-frequency noise is smoothed to its mean (0.25)
    // BEFORE thresholding (1.0). Since 0.25 < 1.0, the output should be 0.0.
    ASSERT_NEAR(avg_force, 0.0, 0.01);
    std::cout << "[PASS] High-frequency noise correctly filtered without developing DC offset (avg force: " << avg_force << ")" << std::endl;
}

TEST_CASE(test_issue_241_continuous_deadzone, "YawGyro") {
    std::cout << "\nTest: Issue #241 Continuous Deadzone" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);

    engine.m_sop_yaw_gain = 1.0f;
    engine.m_yaw_kick_threshold = 1.0f; // Threshold = 1.0
    engine.m_yaw_accel_smoothing = 0.0001f; // Fast response for testing
    engine.m_wheelbase_max_nm = 20.0f;
    engine.m_target_rim_nm = 20.0f;
    engine.m_gain = 1.0f;
    engine.m_invert_force = false;

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = 0.0025;

    // Case 1: Just above threshold (1.1)
    // Old code (gate): input = 1.1 -> force = -1.1 * 1.0 * 5.0 = -5.5 Nm -> norm = -0.275
    // New code (deadzone): input = 1.1 - 1.0 = 0.1 -> force = -0.1 * 1.0 * 5.0 = -0.5 Nm -> norm = -0.025
    data.mLocalRot.y = 0.0;
    FFBEngineTestAccess::ResetYawDerivedState(engine);
    engine.calculate_force(&data); // Seed

    // To hit 1.1 rad/s^2 derived accel WITH interpolation:
    // Rate change over 10ms needs to be 1.1 * 0.01 = 0.011 rad/s
    data.mLocalRot.y = 0.011;
    PumpEngineTime(engine, data, 0.01); // Frame 1 interpolation completes. Current output is 0.011. Rate reached.
    // Now LPF alpha=0.1. We need one more frame to hit target.
    // Wait, the interpolator produces linear ramp.
    // After 10ms of PumpEngineTime, derived accel was constant 1.1.
    // But LPF smoothed it.
    PumpEngineTime(engine, data, 0.05); // Let LPF converge
    double force_low = engine.GetDebugBatch().back().total_output;

    // We expect -0.025
    ASSERT_NEAR(force_low, -0.025, 0.01);
    std::cout << "[PASS] Continuous deadzone correctly subtracts threshold (force: " << force_low << " ~= -0.025)" << std::endl;

    // Case 2: Negative yaw accel (-1.1)
    data.mLocalRot.y = 0.0;
    FFBEngineTestAccess::ResetYawDerivedState(engine); // reset
    engine.calculate_force(&data); // Seed
    data.mLocalRot.y = -0.011;
    // Issue #397: Interpolation delay
    PumpEngineTime(engine, data, 0.06); // Let interpolation and LPF converge
    double force_neg = engine.GetDebugBatch().back().total_output;

    ASSERT_NEAR(force_neg, 0.025, 0.01);
    std::cout << "[PASS] Continuous deadzone handles negative values correctly (force: " << force_neg << " ~= 0.025)" << std::endl;
}

} // namespace FFBEngineTests
