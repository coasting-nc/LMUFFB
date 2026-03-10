// tests/test_ffb_long_load.cpp
#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_load_weighted_grip, "Physics") {
    FFBEngine engine;

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);

    // Wheel 0 (Outside): Load = 10000N, Grip = 0.8 (Sliding)
    data.mWheel[0].mTireLoad = 10000.0;
    data.mWheel[0].mGripFract = 0.8;

    // Wheel 1 (Inside): Load = 500N, Grip = 1.0 (Not sliding)
    data.mWheel[1].mTireLoad = 500.0;
    data.mWheel[1].mGripFract = 1.0;

    double prev_slip1 = 0.0;
    double prev_slip2 = 0.0;
    bool warned = false;

    GripResult result = engine.calculate_axle_grip(
        data.mWheel[0], data.mWheel[1], 5250.0, warned,
        prev_slip1, prev_slip2, 20.0, 0.0025, "TestCar", &data, true
    );

    // Simple Average would be (0.8 + 1.0) / 2.0 = 0.9
    // Weighted Average should be (0.8 * 10000 + 1.0 * 500) / 10500 = (8000 + 500) / 10500 = 8500 / 10500 approx 0.8095

    std::cout << "[INFO] Load-Weighted Grip Result: " << result.original << " (Simple Average would be 0.9)" << std::endl;

    ASSERT_NEAR(result.original, 0.8095, 0.01);
}

TEST_CASE(test_long_load_scaling, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_auto_load_normalization_enabled = true;
    Preset p;
    p.long_load_effect = 1.0f; // Enable
    p.long_load_smoothing = 0.0f; // Disable smoothing for instant test
    p.steering_shaft_gain = 1.0f;
    p.understeer = 0.0f; // Disable understeer drop for pure gain test
    engine.m_invert_force = false; // Easier to test
    p.wheelbase_max_nm = 100.0f;
    p.target_rim_nm = 100.0f;
    p.Apply(engine);

    // v0.7.67 Fix for Issue #152: Ensure consistent scaling for test
    FFBEngineTestAccess::SetSessionPeakTorque(engine, 100.0);
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 100.0);
    FFBEngineTestAccess::SetRollingAverageTorque(engine, 100.0);
    FFBEngineTestAccess::SetLastRawTorque(engine, 100.0);

    // Now heavy braking (G-force based)
    TelemInfoV01 data = CreateBasicTestTelemetry(30.0, 0.0);
    data.mLocalAccel.z = 9.81; // 1G Braking
    data.mSteeringShaftTorque = 5.0;

    // Freeze chassis inertia to use our input directly
    engine.m_chassis_inertia_smoothing = 1000.0f;
    engine.m_accel_z_smoothed = 9.81;

    double output = engine.calculate_force(&data);

    // Master Gain = 1.0, MaxTorqueRef = 100.0
    // base_input = 5.0
    // long_g = 1.0
    // Factor = 1.0 + 1.0 * 1.0 = 2.0
    // Total Nm = 5.0 * 2.0 = 10.0
    // Norm Force = 10.0 / 100.0 = 0.1

    std::cout << "[INFO] Longitudinal G Output: " << output << " (Expected 0.1)" << std::endl;

    ASSERT_NEAR(output, 0.1, 0.01);
}

// [SKIP_TEST] Feature removed: Longitudinal load no longer uses safety gate to avoid constant dropping out on bumps
// TEST_CASE(test_long_load_safety_gate, "Physics") {
//     FFBEngine engine;
//     InitializeEngine(engine);
//     engine.m_understeer_effect = 0.0f; // Disable understeer for pure gate test
//     engine.m_auto_load_normalization_enabled = true;
//     engine.m_invert_force = false;
//     Preset p;
//     p.long_load_effect = 1.0f;
//     p.wheelbase_max_nm = 100.0f;
//     p.target_rim_nm = 100.0f;
//     p.Apply(engine);
// 
//     // v0.7.67 Fix for Issue #152: Ensure consistent scaling for test
//     FFBEngineTestAccess::SetSessionPeakTorque(engine, 100.0);
//     FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 100.0);
//     FFBEngineTestAccess::SetRollingAverageTorque(engine, 100.0);
//     FFBEngineTestAccess::SetLastRawTorque(engine, 100.0);
// 
//     TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
//     data.mWheel[0].mTireLoad = 0.0; // Trigger fallback
//     data.mWheel[1].mTireLoad = 0.0;
//     data.mSteeringShaftTorque = 5.0;
// 
//     // Run multiple frames to trigger warned_load hysteresis
//     // Need to use a car name so InitializeLoadReference doesn't reset us every frame
//     for(int i=0; i<30; ++i) {
//         engine.calculate_force(&data, "GT3", "911");
//     }
// 
//     double output = engine.calculate_force(&data);
// 
//     // warned_load should be true, dynamic weight should be disabled (factor 1.0)
//     // base_input = 5.0
//     // factor = 1.0
//     // Structural Multiplier = 1/100 = 0.01 (session peak 100)
//     // di_structural = 5.0 * 0.01 * (100 / 100) = 0.05
//     // Norm Force = 0.032 (due to refactored summation order and gain interaction)
// 
//     std::cout << "[INFO] Safety Gate Output: " << output << " (Expected 0.032)" << std::endl;
//     ASSERT_NEAR(output, 0.032, 0.01);
// }

TEST_CASE(test_long_load_transformations, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_long_load_effect = 1.0f;
    engine.m_long_load_smoothing = 0.0f;
    engine.m_invert_force = false;
    engine.m_chassis_inertia_smoothing = 1000.0f;

    // Use high scaling to see Nm directly
    FFBEngineTestAccess::SetSessionPeakTorque(engine, 100.0);
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 100.0);
    engine.m_wheelbase_max_nm = 100.0f;
    engine.m_target_rim_nm = 100.0f;

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mSteeringShaftTorque = 10.0;

    auto get_long_load_force = [&](LoadTransform transform, double g_force) {
        engine.m_long_load_transform = transform;
        engine.m_accel_z_smoothed = g_force;
        engine.calculate_force(&data);
        auto snap = engine.GetDebugBatch().back();
        return snap.long_load_force;
    };

    // Case: g_force = 0.5G (4.905 m/s2). long_g = 0.5
    // base_force = 10.0
    // long_load_force = 10.0 * transform(0.5)

    // Linear: 10 * 0.5 = 5.0
    ASSERT_NEAR(get_long_load_force(LoadTransform::LINEAR, 0.5 * 9.81), 5.0f, 0.1f);

    // Cubic: 10 * 0.6875 = 6.875
    ASSERT_NEAR(get_long_load_force(LoadTransform::CUBIC, 0.5 * 9.81), 6.875f, 0.1f);

    // Quadratic: 10 * 0.75 = 7.5
    ASSERT_NEAR(get_long_load_force(LoadTransform::QUADRATIC, 0.5 * 9.81), 7.5f, 0.1f);

    // Hermite: 10 * 0.625 = 6.25
    ASSERT_NEAR(get_long_load_force(LoadTransform::HERMITE, 0.5 * 9.81), 6.25f, 0.1f);
}

TEST_CASE(test_long_load_multiplier_behavior, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_long_load_effect = 1.0f;
    engine.m_long_load_smoothing = 0.0f;
    engine.m_invert_force = false;
    engine.m_chassis_inertia_smoothing = 1000.0f;

    // Use high scaling to see Nm directly
    FFBEngineTestAccess::SetSessionPeakTorque(engine, 100.0);
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 100.0);
    engine.m_wheelbase_max_nm = 100.0f;
    engine.m_target_rim_nm = 100.0f;

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);

    // Case 1: Straight line (zero steering torque)
    data.mSteeringShaftTorque = 0.0;
    engine.m_accel_z_smoothed = 9.81; // 1G braking

    engine.calculate_force(&data);
    auto snap1 = engine.GetDebugBatch().back();

    // Physical Requirement: output and isolated component MUST be zero in straight line
    ASSERT_NEAR(snap1.total_output, 0.0f, 0.001f);
    ASSERT_NEAR(snap1.long_load_force, 0.0f, 0.001f);

    // Case 2: Cornering (non-zero steering torque)
    data.mSteeringShaftTorque = 10.0;
    engine.m_accel_z_smoothed = 9.81; // 1G braking
    engine.calculate_force(&data);
    auto snap2 = engine.GetDebugBatch().back();

    // factor = 1.0 + 1.0 * 1.0 = 2.0
    // isolated = 10.0 * (2.0 - 1.0) = 10.0 Nm
    // total Nm = 10.0 * 2.0 = 20.0 Nm
    // output = 20 / 100 = 0.2

    ASSERT_GT(snap2.long_load_force, 0.001f);
    ASSERT_NEAR(snap2.long_load_force, 10.0f, 0.1f);
    ASSERT_NEAR(snap2.total_output, 0.2f, 0.01f);
}

} // namespace FFBEngineTests


