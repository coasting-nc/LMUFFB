#include "test_ffb_common.h"

namespace FFBEngineTests {

// 1. Consistency Test: Ensures the math doesn't change when variables move
TEST_CASE(test_refactor_braking_consistency, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    // Set specific Braking values (using current loose variables)
    engine.m_lockup_enabled = true;
    engine.m_lockup_gain = 0.8f;
    engine.m_lockup_start_pct = 2.0f;
    engine.m_lockup_full_pct = 10.0f;
    engine.m_lockup_gamma = 1.5f;
    engine.m_brake_load_cap = 2.5f;
    engine.m_abs_pulse_enabled = true;
    engine.m_abs_gain = 1.2f;
    engine.m_abs_freq_hz = 30.0f;
    engine.m_front_axle.steering_shaft_gain = 1.0f;

    // Create a telemetry state that triggers braking logic
    TelemInfoV01 data = CreateBasicTestTelemetry(30.0, 0.1); // 30m/s
    data.mSteeringShaftTorque = 10.0;
    data.mUnfilteredBrake = 0.8;
    // Set wheel rotations to trigger lockup prediction
    for(int i=0; i<4; i++) {
        data.mWheel[i].mRotation = 10.0;
        data.mWheel[i].mBrakePressure = 0.6;
    }

    PumpEngineSteadyState(engine, data);

    // One more frame with slower rotation to trigger lockup and pressure change for ABS
    data.mElapsedTime += 0.01;
    for(int i=0; i<4; i++) {
        data.mWheel[i].mRotation = 1.0;
        data.mWheel[i].mBrakePressure = 0.9;
    }

    // Run a few frames to let the oscillator advance
    engine.calculate_force(&data, "GT3", "Ferrari", 0.0f, true, 0.0025);
    data.mElapsedTime += 0.0025;
    engine.calculate_force(&data, "GT3", "Ferrari", 0.0f, true, 0.0025);
    data.mElapsedTime += 0.0025;
    double final_force = engine.calculate_force(&data, "GT3", "Ferrari", 0.0f, true, 0.0025);

    // Initial run to capture EXPECTED_VALUE
    // After running, replace 0.0 with the actual value.
    double EXPECTED_VALUE = 0.0250997624;

    ASSERT_NEAR(final_force, EXPECTED_VALUE, 0.0001);
}

// 2. Round-Trip Test: Ensures Preset <-> Engine synchronization works
TEST_CASE(test_refactor_braking_roundtrip, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    Preset original("TestPreset", false);
    // Set wild values for Braking variables
    original.lockup_enabled = true;
    original.lockup_gain = 1.44f;
    original.lockup_start_pct = 3.3f;
    original.abs_pulse_enabled = true;
    original.abs_gain = 4.5f;
    original.abs_freq = 33.3f;
    original.brake_load_cap = 4.2f;

    // Apply to engine
    original.Apply(engine);

    // Verify engine received them (using current loose variables)
    ASSERT_EQ(engine.m_lockup_enabled, true);
    ASSERT_NEAR(engine.m_lockup_gain, 1.44f, 0.0001);
    ASSERT_NEAR(engine.m_lockup_start_pct, 3.3f, 0.0001);
    ASSERT_EQ(engine.m_abs_pulse_enabled, true);
    ASSERT_NEAR(engine.m_abs_gain, 4.5f, 0.0001);
    ASSERT_NEAR(engine.m_abs_freq_hz, 33.3f, 0.0001);
    ASSERT_NEAR(engine.m_brake_load_cap, 4.2f, 0.0001);

    // Extract back to a new preset
    Preset extracted("Extracted", false);
    extracted.UpdateFromEngine(engine);

    // They should be exactly equal
    ASSERT_TRUE(original.Equals(extracted));
}

// 3. Validation Test: Ensures clamping logic is preserved
TEST_CASE(test_refactor_braking_validation, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    Preset bad_preset("Bad", false);

    // Set malicious Braking values
    bad_preset.lockup_gain = -1.0f;      // Should clamp to 0.0
    bad_preset.lockup_start_pct = 0.0f;  // Should clamp to 0.1
    bad_preset.lockup_gamma = 0.0f;      // Should clamp to 0.1
    bad_preset.brake_load_cap = 0.5f;    // Should clamp to 1.0
    bad_preset.abs_gain = -5.0f;         // Should clamp to 0.0
    bad_preset.abs_freq = 0.5f;          // Should clamp to 1.0

    bad_preset.Apply(engine);

    // Verify engine clamped them (using current loose variables)
    ASSERT_NEAR(engine.m_lockup_gain, 0.0f, 0.0001);
    ASSERT_NEAR(engine.m_lockup_start_pct, 0.1f, 0.0001);
    ASSERT_NEAR(engine.m_lockup_gamma, 0.1f, 0.0001);
    ASSERT_NEAR(engine.m_brake_load_cap, 1.0f, 0.0001);
    ASSERT_NEAR(engine.m_abs_gain, 0.0f, 0.0001);
    ASSERT_NEAR(engine.m_abs_freq_hz, 1.0f, 0.0001);
}

}
