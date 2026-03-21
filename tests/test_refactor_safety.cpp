#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_refactor_safety_consistency, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    // Set specific Safety values (using current loose variables)
    engine.m_safety.m_config.window_duration = 2.5f;
    engine.m_safety.m_config.gain_reduction = 0.5f;
    engine.m_safety.m_config.smoothing_tau = 0.1f;
    engine.m_safety.m_config.spike_detection_threshold = 400.0f;
    engine.m_safety.m_config.immediate_spike_threshold = 1200.0f;
    engine.m_safety.m_config.slew_full_scale_time_s = 0.5f;
    engine.m_safety.m_config.stutter_safety_enabled = true;
    engine.m_safety.m_config.stutter_threshold = 1.8f;

    // Create a telemetry state that might trigger safety logic
    TelemInfoV01 data = CreateBasicTestTelemetry(25.0, 0.08);
    data.mSteeringShaftTorque = 15.0;

    // Pump engine to settle filters
    PumpEngineSteadyState(engine, data);

    // Force a spike to trigger safety window
    engine.m_safety.TriggerSafetyWindow("Test Spike", data.mElapsedTime);

    double final_force = engine.calculate_force(&data, "LMP2", "Oreca", 0.0f, true, 0.0025);

    // Baseline value for current codebase
    // After first run, we will hardcode the captured value.
    double EXPECTED_VALUE = 0.2379815042; // Captured from pre-refactor run

    ASSERT_NEAR(final_force, EXPECTED_VALUE, 0.0001);
}

TEST_CASE(test_refactor_safety_roundtrip, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    Preset original("TestPreset", false);
    // Set wild values for Safety variables
    original.safety.window_duration = 3.3f;
    original.safety.gain_reduction = 0.15f;
    original.safety.smoothing_tau = 0.25f;
    original.safety.spike_detection_threshold = 888.0f;
    original.safety.immediate_spike_threshold = 2222.0f;
    original.safety.slew_full_scale_time_s = 0.75f;
    original.safety.stutter_safety_enabled = true;
    original.safety.stutter_threshold = 2.5f;

    // Apply to engine
    original.Apply(engine);

    // Verify engine received them (using current loose variables)
    ASSERT_NEAR(engine.m_safety.m_config.window_duration, 3.3f, 0.0001);
    ASSERT_NEAR(engine.m_safety.m_config.gain_reduction, 0.15f, 0.0001);
    ASSERT_NEAR(engine.m_safety.m_config.smoothing_tau, 0.25f, 0.0001);
    ASSERT_NEAR(engine.m_safety.m_config.spike_detection_threshold, 888.0f, 0.0001);
    ASSERT_NEAR(engine.m_safety.m_config.immediate_spike_threshold, 2222.0f, 0.0001);
    ASSERT_NEAR(engine.m_safety.m_config.slew_full_scale_time_s, 0.75f, 0.0001);
    ASSERT_EQ(engine.m_safety.m_config.stutter_safety_enabled, true);
    ASSERT_NEAR(engine.m_safety.m_config.stutter_threshold, 2.5f, 0.0001);

    // Extract back to a new preset
    Preset extracted("Extracted", false);
    extracted.UpdateFromEngine(engine);

    // They should be exactly equal
    ASSERT_TRUE(original.Equals(extracted));
}

TEST_CASE(test_refactor_safety_validation, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    Preset bad_preset("Bad", false);

    // Set malicious Safety values
    bad_preset.safety.window_duration = -1.0f;         // Should clamp to 0.0
    bad_preset.safety.gain_reduction = 5.0f;          // Should clamp to 1.0
    bad_preset.safety.smoothing_tau = 0.0f;           // Should clamp to 0.001
    bad_preset.safety.spike_detection_threshold = 0.5f;      // Should clamp to 1.0
    bad_preset.safety.immediate_spike_threshold = 0.1f;      // Should clamp to 1.0
    bad_preset.safety.slew_full_scale_time_s = 0.0f;  // Should clamp to 0.01
    bad_preset.safety.stutter_threshold = 0.5f;              // Should clamp to 1.01

    bad_preset.Apply(engine);

    // Verify engine clamped them (using current loose variables)
    ASSERT_NEAR(engine.m_safety.m_config.window_duration, 0.0f, 0.0001);
    ASSERT_NEAR(engine.m_safety.m_config.gain_reduction, 1.0f, 0.0001);
    ASSERT_NEAR(engine.m_safety.m_config.smoothing_tau, 0.001f, 0.0001);
    ASSERT_NEAR(engine.m_safety.m_config.spike_detection_threshold, 1.0f, 0.0001);
    ASSERT_NEAR(engine.m_safety.m_config.immediate_spike_threshold, 1.0f, 0.0001);
    ASSERT_NEAR(engine.m_safety.m_config.slew_full_scale_time_s, 0.01f, 0.0001);
    ASSERT_NEAR(engine.m_safety.m_config.stutter_threshold, 1.01f, 0.0001);
}

}
