#include "test_ffb_common.h"

namespace FFBEngineTests {

// 1. Consistency Test: Ensures the math doesn't change when variables move
TEST_CASE(test_refactor_advanced_consistency, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    // Set specific Advanced values (using future struct paths)
    engine.m_advanced.gyro_gain = 0.2f;
    engine.m_advanced.gyro_smoothing = 0.01f;
    engine.m_advanced.stationary_damping = 0.5f;
    engine.m_advanced.soft_lock_enabled = true;
    engine.m_advanced.soft_lock_stiffness = 10.0f;
    engine.m_advanced.soft_lock_damping = 0.3f;
    engine.m_advanced.speed_gate_lower = 1.0f;
    engine.m_advanced.speed_gate_upper = 5.0f;
    engine.m_advanced.rest_api_enabled = false;
    engine.m_advanced.rest_api_port = 6397;
    engine.m_advanced.road_fallback_scale = 0.05f;
    engine.m_advanced.understeer_affects_sop = false;

    // Create a telemetry state
    TelemInfoV01 data = CreateBasicTestTelemetry(10.0, 0.05); // 10m/s
    data.mUnfilteredSteering = 0.2;
    data.mLocalAccel.x = 4.0;
    data.mSteeringShaftTorque = 2.0;

    // Pump engine
    PumpEngineSteadyState(engine, data);
    double final_force = engine.calculate_force(&data, "LMP2", "Oreca", 0.0f, true, 0.0025);

    // This value was obtained by running the baseline test on the pre-refactored codebase.
    double EXPECTED_VALUE = -0.5022886336;

    ASSERT_NEAR(final_force, EXPECTED_VALUE, 0.0001);
}

// 2. Round-Trip Test: Ensures Preset <-> Engine synchronization works
TEST_CASE(test_refactor_advanced_roundtrip, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    Preset original("TestPreset", false);
    // Set wild values for Advanced variables
    original.advanced.gyro_gain = 0.77f;
    original.advanced.gyro_smoothing = 0.044f;
    original.advanced.stationary_damping = 0.88f;
    original.advanced.soft_lock_enabled = true;
    original.advanced.soft_lock_stiffness = 55.0f;
    original.advanced.soft_lock_damping = 1.2f;
    original.advanced.speed_gate_lower = 2.5f;
    original.advanced.speed_gate_upper = 8.0f;
    original.advanced.rest_api_enabled = true;
    original.advanced.rest_api_port = 1234;
    original.advanced.road_fallback_scale = 0.15f;
    original.advanced.understeer_affects_sop = true;

    // Apply to engine
    original.Apply(engine);

    // Verify engine received them
    ASSERT_NEAR(engine.m_advanced.gyro_gain, 0.77f, 0.0001);
    ASSERT_NEAR(engine.m_advanced.gyro_smoothing, 0.044f, 0.0001);
    ASSERT_NEAR(engine.m_advanced.stationary_damping, 0.88f, 0.0001);
    ASSERT_EQ(engine.m_advanced.soft_lock_enabled, true);
    ASSERT_NEAR(engine.m_advanced.soft_lock_stiffness, 55.0f, 0.0001);
    ASSERT_NEAR(engine.m_advanced.soft_lock_damping, 1.2f, 0.0001);
    ASSERT_NEAR(engine.m_advanced.speed_gate_lower, 2.5f, 0.0001);
    ASSERT_NEAR(engine.m_advanced.speed_gate_upper, 8.0f, 0.0001);
    ASSERT_EQ(engine.m_advanced.rest_api_enabled, true);
    ASSERT_EQ(engine.m_advanced.rest_api_port, 1234);
    ASSERT_NEAR(engine.m_advanced.road_fallback_scale, 0.15f, 0.0001);
    ASSERT_EQ(engine.m_advanced.understeer_affects_sop, true);

    // Extract back to a new preset
    Preset extracted("Extracted", false);
    extracted.UpdateFromEngine(engine);

    // They should be exactly equal
    ASSERT_TRUE(original.Equals(extracted));
}

// 3. Validation Test: Ensures clamping logic is preserved
TEST_CASE(test_refactor_advanced_validation, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    Preset bad_preset("Bad", false);

    // Set malicious Advanced values
    bad_preset.advanced.gyro_gain = -5.0f;           // Should clamp to 0.0
    bad_preset.advanced.stationary_damping = 2.0f;   // Should clamp to 1.0
    bad_preset.advanced.soft_lock_stiffness = -10.0f; // Should clamp to 0.0
    bad_preset.advanced.speed_gate_upper = 0.0f;     // Should clamp to 0.1
    bad_preset.advanced.rest_api_port = -1;          // Should clamp to 1

    bad_preset.Apply(engine);

    // Verify engine clamped them
    ASSERT_NEAR(engine.m_advanced.gyro_gain, 0.0f, 0.0001);
    ASSERT_NEAR(engine.m_advanced.stationary_damping, 1.0f, 0.0001);
    ASSERT_NEAR(engine.m_advanced.soft_lock_stiffness, 0.0f, 0.0001);
    ASSERT_NEAR(engine.m_advanced.speed_gate_upper, 0.1f, 0.0001);
    ASSERT_EQ(engine.m_advanced.rest_api_port, 1);
}

}
