#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_refactor_front_axle_consistency, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    // Set specific Front Axle values (using current loose variables)
    engine.m_front_axle.understeer_effect = 1.5f;
    engine.m_front_axle.understeer_gamma = 0.8f;
    engine.m_front_axle.steering_shaft_gain = 0.9f;
    engine.m_front_axle.flatspot_suppression = true;
    engine.m_front_axle.notch_q = 4.0f;

    // Create a telemetry state that triggers understeer and flatspot logic
    TelemInfoV01 data = CreateBasicTestTelemetry(25.0, 0.15); // High slip = understeer
    data.mUnfilteredSteering = 0.5;
    data.mSteeringShaftTorque = 12.0;

    PumpEngineSteadyState(engine, data);
    double final_force = engine.calculate_force(&data, "LMP2", "Oreca", 0.0f, true, 0.0025);

    // Baseline value for current codebase
    double EXPECTED_VALUE = 0.0;

    ASSERT_NEAR(final_force, EXPECTED_VALUE, 0.0001);
}

TEST_CASE(test_refactor_front_axle_roundtrip, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    Preset original("TestPreset", false);
    // Set wild values for Front Axle variables
    original.front_axle.understeer_effect = 0.44f;
    original.front_axle.understeer_gamma = 2.1f;
    original.front_axle.steering_shaft_gain = 1.8f;
    original.front_axle.torque_source = 1;
    original.front_axle.static_notch_enabled = true;
    original.front_axle.static_notch_freq = 45.0f;

    // Apply to engine
    original.Apply(engine);

    // Verify engine received them (using current loose variables)
    ASSERT_NEAR(engine.m_front_axle.understeer_effect, 0.44f, 0.0001);
    ASSERT_NEAR(engine.m_front_axle.understeer_gamma, 2.1f, 0.0001);
    ASSERT_NEAR(engine.m_front_axle.steering_shaft_gain, 1.8f, 0.0001);
    ASSERT_EQ(engine.m_front_axle.torque_source, 1);
    ASSERT_EQ(engine.m_front_axle.static_notch_enabled, true);
    ASSERT_NEAR(engine.m_front_axle.static_notch_freq, 45.0f, 0.0001);

    // Extract back to a new preset
    Preset extracted("Extracted", false);
    extracted.UpdateFromEngine(engine);

    // They should be exactly equal
    ASSERT_TRUE(original.Equals(extracted));
}

TEST_CASE(test_refactor_front_axle_validation, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    Preset bad_preset("Bad", false);

    // Set malicious Front Axle values
    bad_preset.front_axle.understeer_effect = 5.0f;         // Should clamp to 2.0
    bad_preset.front_axle.understeer_gamma = -1.0f;  // Should clamp to 0.1
    bad_preset.front_axle.steering_shaft_gain = -5.0f; // Should clamp to 0.0
    bad_preset.front_axle.torque_source = 5;         // Should clamp to 1
    bad_preset.front_axle.notch_q = 0.0f;            // Should clamp to 0.1

    bad_preset.Apply(engine);

    // Verify engine clamped them (using current loose variables)
    ASSERT_NEAR(engine.m_front_axle.understeer_effect, 2.0f, 0.0001);
    ASSERT_NEAR(engine.m_front_axle.understeer_gamma, 0.1f, 0.0001);
    ASSERT_NEAR(engine.m_front_axle.steering_shaft_gain, 0.0f, 0.0001);
    ASSERT_EQ(engine.m_front_axle.torque_source, 1);
    ASSERT_NEAR(engine.m_front_axle.notch_q, 0.1f, 0.0001);
}

}
