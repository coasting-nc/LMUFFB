#include "test_ffb_common.h"

namespace FFBEngineTests {

// 1. Consistency Test: Ensures the math doesn't change when variables move
TEST_CASE(test_refactor_grip_estimation_consistency, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    // Set specific Grip Estimation values (using current loose variables)
    engine.m_grip_estimation.optimal_slip_angle = 0.15f;
    engine.m_grip_estimation.optimal_slip_ratio = 0.18f;
    engine.m_grip_estimation.slip_angle_smoothing = 0.005f;
    engine.m_grip_estimation.chassis_inertia_smoothing = 0.01f;
    engine.m_grip_estimation.load_sensitivity_enabled = true;
    engine.m_grip_estimation.grip_smoothing_steady = 0.08f;
    engine.m_grip_estimation.grip_smoothing_fast = 0.01f;
    engine.m_grip_estimation.grip_smoothing_sensitivity = 0.15f;

    // Create a telemetry state that triggers grip estimation logic
    TelemInfoV01 data = CreateBasicTestTelemetry(30.0, 0.12); // 30m/s, 12% slip
    data.mLocalAccel.x = 12.0; // Moderate lateral G
    data.mUnfilteredSteering = 0.4;
    data.mSteeringShaftTorque = 8.0;

    // Add some load
    for(int i=0; i<4; ++i) {
        data.mWheel[i].mSuspensionDeflection = 0.02;
    }

    PumpEngineSteadyState(engine, data);
    double final_force = engine.calculate_force(&data, "LMP2", "Oreca", 0.0f, true, 0.0025);

    // Baseline value for current codebase
    // After running this once, I will update EXPECTED_VALUE with the actual output.
    double EXPECTED_VALUE = 0.2444766863618698; // baseline

    ASSERT_NEAR(final_force, EXPECTED_VALUE, 0.0001);
}

// 2. Round-Trip Test: Ensures Preset <-> Engine synchronization works
TEST_CASE(test_refactor_grip_estimation_roundtrip, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    Preset original("TestPreset", false);
    // Set wild values for Grip Estimation variables
    original.grip_estimation.optimal_slip_angle = 0.17f;
    original.grip_estimation.optimal_slip_ratio = 0.19f;
    original.grip_estimation.slip_angle_smoothing = 0.007f;
    original.grip_estimation.chassis_inertia_smoothing = 0.013f;
    original.grip_estimation.load_sensitivity_enabled = false;
    original.grip_estimation.grip_smoothing_steady = 0.09f;
    original.grip_estimation.grip_smoothing_fast = 0.008f;
    original.grip_estimation.grip_smoothing_sensitivity = 0.2f;

    // Apply to engine
    original.Apply(engine);

    // Verify engine received them (using current loose variables)
    ASSERT_NEAR(engine.m_grip_estimation.optimal_slip_angle, 0.17f, 0.0001);
    ASSERT_NEAR(engine.m_grip_estimation.optimal_slip_ratio, 0.19f, 0.0001);
    ASSERT_NEAR(engine.m_grip_estimation.slip_angle_smoothing, 0.007f, 0.0001);
    ASSERT_NEAR(engine.m_grip_estimation.chassis_inertia_smoothing, 0.013f, 0.0001);
    ASSERT_EQ(engine.m_grip_estimation.load_sensitivity_enabled, false);
    ASSERT_NEAR(engine.m_grip_estimation.grip_smoothing_steady, 0.09f, 0.0001);
    ASSERT_NEAR(engine.m_grip_estimation.grip_smoothing_fast, 0.008f, 0.0001);
    ASSERT_NEAR(engine.m_grip_estimation.grip_smoothing_sensitivity, 0.2f, 0.0001);

    // Extract back to a new preset
    Preset extracted("Extracted", false);
    extracted.UpdateFromEngine(engine);

    // They should be exactly equal
    ASSERT_TRUE(original.Equals(extracted));
}

// 3. Validation Test: Ensures clamping logic is preserved
TEST_CASE(test_refactor_grip_estimation_validation, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    Preset bad_preset("Bad", false);

    // Set malicious Grip Estimation values
    bad_preset.grip_estimation.optimal_slip_angle = 0.005f; // Should clamp to 0.01
    bad_preset.grip_estimation.optimal_slip_ratio = 0.005f; // Should clamp to 0.01
    bad_preset.grip_estimation.slip_angle_smoothing = 0.0f;       // Should clamp to 0.0001
    bad_preset.grip_estimation.chassis_inertia_smoothing = -1.0f;  // Should clamp to 0.0
    bad_preset.grip_estimation.grip_smoothing_steady = -1.0f; // Should clamp to 0.0
    bad_preset.grip_estimation.grip_smoothing_fast = -1.0f;   // Should clamp to 0.0
    bad_preset.grip_estimation.grip_smoothing_sensitivity = 0.0001f; // Should clamp to 0.001

    bad_preset.Apply(engine);

    // Verify engine clamped them (using current loose variables)
    ASSERT_NEAR(engine.m_grip_estimation.optimal_slip_angle, 0.01f, 0.0001);
    ASSERT_NEAR(engine.m_grip_estimation.optimal_slip_ratio, 0.01f, 0.0001);
    ASSERT_NEAR(engine.m_grip_estimation.slip_angle_smoothing, 0.0001f, 0.0001);
    ASSERT_NEAR(engine.m_grip_estimation.chassis_inertia_smoothing, 0.0f, 0.0001);
    ASSERT_NEAR(engine.m_grip_estimation.grip_smoothing_steady, 0.0f, 0.0001);
    ASSERT_NEAR(engine.m_grip_estimation.grip_smoothing_fast, 0.0f, 0.0001);
    ASSERT_NEAR(engine.m_grip_estimation.grip_smoothing_sensitivity, 0.001f, 0.0001);
}

}
