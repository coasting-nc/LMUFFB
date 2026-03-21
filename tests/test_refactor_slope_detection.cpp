#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_refactor_slope_detection_consistency, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    // Set specific Slope Detection values (using current loose variables)
    engine.m_slope_detection.enabled = true;
    engine.m_slope_detection.sg_window = 21;
    engine.m_slope_detection.sensitivity = 0.8f;
    engine.m_slope_detection.smoothing_tau = 0.05f;
    engine.m_slope_detection.alpha_threshold = 0.03f;
    engine.m_slope_detection.decay_rate = 4.0f;
    engine.m_slope_detection.confidence_enabled = true;
    engine.m_slope_detection.confidence_max_rate = 0.15f;
    engine.m_slope_detection.min_threshold = -0.5f;
    engine.m_slope_detection.max_threshold = -1.5f;
    engine.m_slope_detection.g_slew_limit = 40.0f;
    engine.m_slope_detection.use_torque = true;
    engine.m_slope_detection.torque_sensitivity = 0.6f;

    // Create a telemetry state that triggers slope detection logic
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.10);
    data.mLocalAccel.x = 12.0;
    data.mUnfilteredSteering = 0.4;
    data.mSteeringShaftTorque = 8.0;

    PumpEngineSteadyState(engine, data);
    double final_force = engine.calculate_force(&data, "LMP2", "Oreca", 0.0f, true, 0.0025);

    // Baseline value for current codebase
    double EXPECTED_VALUE = 0.4;

    ASSERT_NEAR(final_force, EXPECTED_VALUE, 0.0001);
}

TEST_CASE(test_refactor_slope_detection_roundtrip, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    Preset original("TestPreset", false);
    // Set wild values for Slope Detection variables
    original.slope_detection.enabled = true;
    original.slope_detection.sg_window = 31;
    original.slope_detection.sensitivity = 1.23f;
    original.slope_detection.smoothing_tau = 0.088f;
    original.slope_detection.alpha_threshold = 0.045f;
    original.slope_detection.decay_rate = 7.5f;
    original.slope_detection.confidence_enabled = false;
    original.slope_detection.confidence_max_rate = 0.22f;
    original.slope_detection.min_threshold = -0.7f;
    original.slope_detection.max_threshold = -2.5f;
    original.slope_detection.g_slew_limit = 65.0f;
    original.slope_detection.use_torque = false;
    original.slope_detection.torque_sensitivity = 0.77f;

    // Apply to engine
    original.Apply(engine);

    // Verify engine received them (using current loose variables)
    ASSERT_EQ(engine.m_slope_detection.enabled, true);
    ASSERT_EQ(engine.m_slope_detection.sg_window, 31);
    ASSERT_NEAR(engine.m_slope_detection.sensitivity, 1.23f, 0.0001);
    ASSERT_NEAR(engine.m_slope_detection.smoothing_tau, 0.088f, 0.0001);
    ASSERT_NEAR(engine.m_slope_detection.alpha_threshold, 0.045f, 0.0001);
    ASSERT_NEAR(engine.m_slope_detection.decay_rate, 7.5f, 0.0001);
    ASSERT_EQ(engine.m_slope_detection.confidence_enabled, false);
    ASSERT_NEAR(engine.m_slope_detection.confidence_max_rate, 0.22f, 0.0001);
    ASSERT_NEAR(engine.m_slope_detection.min_threshold, -0.7f, 0.0001);
    ASSERT_NEAR(engine.m_slope_detection.max_threshold, -2.5f, 0.0001);
    ASSERT_NEAR(engine.m_slope_detection.g_slew_limit, 65.0f, 0.0001);
    ASSERT_EQ(engine.m_slope_detection.use_torque, false);
    ASSERT_NEAR(engine.m_slope_detection.torque_sensitivity, 0.77f, 0.0001);

    // Extract back to a new preset
    Preset extracted("Extracted", false);
    extracted.UpdateFromEngine(engine);

    // They should be exactly equal
    ASSERT_TRUE(original.Equals(extracted));
}

TEST_CASE(test_refactor_slope_detection_validation, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    Preset bad_preset("Bad", false);

    // Set malicious Slope Detection values
    bad_preset.slope_detection.sg_window = 2;              // Should clamp to 5 (and then 5 is odd)
    bad_preset.slope_detection.sensitivity = 0.0f;         // Should clamp to 0.1
    bad_preset.slope_detection.smoothing_tau = 0.0f;       // Should clamp to 0.001
    bad_preset.slope_detection.alpha_threshold = 0.0f;     // Should clamp to 0.001
    bad_preset.slope_detection.decay_rate = 0.0f;          // Should clamp to 0.1
    bad_preset.slope_detection.g_slew_limit = 0.0f;        // Should clamp to 1.0
    bad_preset.slope_detection.torque_sensitivity = 0.0f;  // Should clamp to 0.01

    bad_preset.Apply(engine);

    // Verify engine clamped them (using current loose variables)
    ASSERT_EQ(engine.m_slope_detection.sg_window, 5);
    ASSERT_NEAR(engine.m_slope_detection.sensitivity, 0.1f, 0.0001);
    ASSERT_NEAR(engine.m_slope_detection.smoothing_tau, 0.001f, 0.0001);
    ASSERT_NEAR(engine.m_slope_detection.alpha_threshold, 0.001f, 0.0001);
    ASSERT_NEAR(engine.m_slope_detection.decay_rate, 0.1f, 0.0001);
    ASSERT_NEAR(engine.m_slope_detection.g_slew_limit, 1.0f, 0.0001);
    ASSERT_NEAR(engine.m_slope_detection.torque_sensitivity, 0.01f, 0.0001);
}

}
