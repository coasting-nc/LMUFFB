#include "test_ffb_common.h"

namespace FFBEngineTests {

// 1. Consistency Test: Ensures the math doesn't change when variables move
TEST_CASE(test_refactor_rear_axle_consistency, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    // Set specific Rear Axle values (using current loose variables)
    engine.m_rear_axle.sop_effect = 1.23f;
    engine.m_rear_axle.oversteer_boost = 2.0f;
    engine.m_rear_axle.sop_yaw_gain = 0.5f;
    engine.m_rear_axle.rear_align_effect = 0.8f;
    engine.m_rear_axle.yaw_kick_threshold = 0.1f;
    engine.m_rear_axle.yaw_accel_smoothing = 0.01f;

    // Unloaded Yaw Kick
    engine.m_rear_axle.unloaded_yaw_gain = 0.3f;
    engine.m_rear_axle.unloaded_yaw_threshold = 0.05f;
    engine.m_rear_axle.unloaded_yaw_sens = 1.5f;
    engine.m_rear_axle.unloaded_yaw_gamma = 1.2f;
    engine.m_rear_axle.unloaded_yaw_punch = 0.1f;

    // Create a telemetry state that triggers SoP and Yaw Kick logic
    TelemInfoV01 data = CreateBasicTestTelemetry(25.0, 0.1); // 25 m/s, some slip
    data.mLocalAccel.x = 10.0; // Lateral G
    data.mLocalRot.y = 0.5; // Yaw rate change

    PumpEngineSteadyState(engine, data);

    // Advance one more frame with changing yaw rate to trigger yaw acceleration
    data.mLocalRot.y = 1.0;
    double final_force = engine.calculate_force(&data, "LMP2", "Oreca", 0.0f, true, 0.0025);

    // Baseline value for current codebase
    double EXPECTED_VALUE = -0.118749;

    ASSERT_NEAR(final_force, EXPECTED_VALUE, 0.0001);
}

// 2. Round-Trip Test: Ensures Preset <-> Engine synchronization works
TEST_CASE(test_refactor_rear_axle_roundtrip, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    Preset original("TestPreset", false);
    // Set wild values for Rear Axle variables
    original.rear_axle.sop_effect = 1.44f;
    original.rear_axle.sop_scale = 1.5f;
    original.rear_axle.sop_smoothing_factor = 0.05f;
    original.rear_axle.oversteer_boost = 3.1f;
    original.rear_axle.rear_align_effect = 1.2f;
    original.rear_axle.kerb_strike_rejection = 0.5f;
    original.rear_axle.sop_yaw_gain = 0.77f;
    original.rear_axle.yaw_kick_threshold = 0.25f;
    original.rear_axle.yaw_accel_smoothing = 0.02f;

    original.rear_axle.unloaded_yaw_gain = 0.4f;
    original.rear_axle.unloaded_yaw_threshold = 0.15f;
    original.rear_axle.unloaded_yaw_sens = 2.0f;
    original.rear_axle.unloaded_yaw_gamma = 1.8f;
    original.rear_axle.unloaded_yaw_punch = 0.12f;

    original.rear_axle.power_yaw_gain = 0.6f;
    original.rear_axle.power_yaw_threshold = 0.1f;
    original.rear_axle.power_slip_threshold = 0.2f;
    original.rear_axle.power_yaw_gamma = 1.5f;
    original.rear_axle.power_yaw_punch = 0.08f;

    // Apply to engine
    original.Apply(engine);

    // Verify engine received them
    ASSERT_NEAR(engine.m_rear_axle.sop_effect, 1.44f, 0.0001);
    ASSERT_NEAR(engine.m_rear_axle.sop_scale, 1.5f, 0.0001);
    ASSERT_NEAR(engine.m_rear_axle.sop_smoothing_factor, 0.05f, 0.0001);
    ASSERT_NEAR(engine.m_rear_axle.oversteer_boost, 3.1f, 0.0001);
    ASSERT_NEAR(engine.m_rear_axle.rear_align_effect, 1.2f, 0.0001);
    ASSERT_NEAR(engine.m_rear_axle.kerb_strike_rejection, 0.5f, 0.0001);
    ASSERT_NEAR(engine.m_rear_axle.sop_yaw_gain, 0.77f, 0.0001);
    ASSERT_NEAR(engine.m_rear_axle.yaw_kick_threshold, 0.25f, 0.0001);
    ASSERT_NEAR(engine.m_rear_axle.yaw_accel_smoothing, 0.02f, 0.0001);

    ASSERT_NEAR(engine.m_rear_axle.unloaded_yaw_gain, 0.4f, 0.0001);
    ASSERT_NEAR(engine.m_rear_axle.unloaded_yaw_threshold, 0.15f, 0.0001);
    ASSERT_NEAR(engine.m_rear_axle.unloaded_yaw_sens, 2.0f, 0.0001);
    ASSERT_NEAR(engine.m_rear_axle.unloaded_yaw_gamma, 1.8f, 0.0001);
    ASSERT_NEAR(engine.m_rear_axle.unloaded_yaw_punch, 0.12f, 0.0001);

    ASSERT_NEAR(engine.m_rear_axle.power_yaw_gain, 0.6f, 0.0001);
    ASSERT_NEAR(engine.m_rear_axle.power_yaw_threshold, 0.1f, 0.0001);
    ASSERT_NEAR(engine.m_rear_axle.power_slip_threshold, 0.2f, 0.0001);
    ASSERT_NEAR(engine.m_rear_axle.power_yaw_gamma, 1.5f, 0.0001);
    ASSERT_NEAR(engine.m_rear_axle.power_yaw_punch, 0.08f, 0.0001);

    // Extract back to a new preset
    Preset extracted("Extracted", false);
    extracted.UpdateFromEngine(engine);

    // They should be exactly equal
    ASSERT_TRUE(original.Equals(extracted));
}

// 3. Validation Test: Ensures clamping logic is preserved
TEST_CASE(test_refactor_rear_axle_validation, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    Preset bad_preset("Bad", false);

    // Set malicious Rear Axle values
    bad_preset.rear_axle.sop_effect = 5.0f;                 // Should clamp to 2.0
    bad_preset.rear_axle.sop_scale = -1.0f;          // Should clamp to 0.01
    bad_preset.rear_axle.sop_smoothing_factor = 5.0f;       // Should clamp to 1.0
    bad_preset.rear_axle.oversteer_boost = -1.0f;    // Should clamp to 0.0
    bad_preset.rear_axle.kerb_strike_rejection = 5.0f; // Should clamp to 1.0
    bad_preset.rear_axle.sop_yaw_gain = -1.0f;       // Should clamp to 0.0

    bad_preset.rear_axle.unloaded_yaw_sens = -1.0f;  // Should clamp to 0.1
    bad_preset.rear_axle.unloaded_yaw_gamma = 10.0f; // Should clamp to 4.0
    bad_preset.rear_axle.power_slip_threshold = 5.0f; // Should clamp to 1.0

    bad_preset.Apply(engine);

    // Verify engine clamped them
    ASSERT_NEAR(engine.m_rear_axle.sop_effect, 2.0f, 0.0001);
    ASSERT_NEAR(engine.m_rear_axle.sop_scale, 0.01f, 0.0001);
    ASSERT_NEAR(engine.m_rear_axle.sop_smoothing_factor, 1.0f, 0.0001);
    ASSERT_NEAR(engine.m_rear_axle.oversteer_boost, 0.0f, 0.0001);
    ASSERT_NEAR(engine.m_rear_axle.kerb_strike_rejection, 1.0f, 0.0001);
    ASSERT_NEAR(engine.m_rear_axle.sop_yaw_gain, 0.0f, 0.0001);

    ASSERT_NEAR(engine.m_rear_axle.unloaded_yaw_sens, 0.1f, 0.0001);
    ASSERT_NEAR(engine.m_rear_axle.unloaded_yaw_gamma, 4.0f, 0.0001);
    ASSERT_NEAR(engine.m_rear_axle.power_slip_threshold, 1.0f, 0.0001);
}

}
