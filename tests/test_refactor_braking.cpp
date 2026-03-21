#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_refactor_braking_consistency, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    // 1. Set specific Braking values (current loose variables)
    engine.m_lockup_enabled = true;
    engine.m_lockup_gain = 1.2f;
    engine.m_lockup_start_pct = 2.0f;
    engine.m_lockup_full_pct = 8.0f;
    engine.m_lockup_rear_boost = 2.5f;
    engine.m_lockup_gamma = 0.5f;
    engine.m_lockup_prediction_sens = 25.0f;
    engine.m_lockup_bump_reject = 0.5f;
    engine.m_brake_load_cap = 3.0f;
    engine.m_lockup_freq_scale = 1.1f;
    engine.m_abs_pulse_enabled = true;
    engine.m_abs_gain = 1.5f;
    engine.m_abs_freq_hz = 30.0f;

    // 2. Create a telemetry state that triggers both lockup and ABS
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.05);
    data.mUnfilteredBrake = 0.8; // High brake pedal for ABS

    // Front wheels locking up (10% slip)
    data.mWheel[0].mLateralPatchVel = 0.0;
    data.mWheel[0].mLongitudinalPatchVel = -2.0;
    data.mWheel[0].mLongitudinalGroundVel = 20.0;
    data.mWheel[0].mRotation = 18.0 / 0.33;
    data.mWheel[0].mStaticUndeflectedRadius = 33;
    data.mWheel[0].mTireLoad = 4000.0;
    data.mWheel[0].mBrakePressure = 0.5;
    data.mSteeringShaftTorque = 5.0;

    // 3. Establish baseline - run multiple times to settle filters and safety
    // Use PumpEngineTime to simulate 3 seconds of driving to clear safety windows
    PumpEngineTime(engine, data, 3.0);

    // Now trigger the ABS oscillation
    data.mElapsedTime += 0.0025;
    data.mWheel[0].mBrakePressure = 0.8;
    double final_force = engine.calculate_force(&data, "GT3", "Ferrari", 0.0f, true, 0.0025);

    // 4. ASSERT THE EXACT VALUE
    // RUN THIS ON THE CURRENT CODEBASE FIRST.
    double EXPECTED_VALUE = 0.3532342416; // establishment result

    ASSERT_NEAR(final_force, EXPECTED_VALUE, 0.0001);
}

TEST_CASE(test_refactor_braking_roundtrip, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    Preset original("TestPreset", false);
    original.lockup_enabled = true;
    original.lockup_gain = 1.23f;
    original.lockup_start_pct = 2.34f;
    original.lockup_full_pct = 7.89f;
    original.lockup_rear_boost = 3.45f;
    original.lockup_gamma = 0.67f;
    original.lockup_prediction_sens = 33.0f;
    original.lockup_bump_reject = 1.2f;
    original.brake_load_cap = 4.5f;
    original.lockup_freq_scale = 1.2f;
    original.abs_pulse_enabled = true;
    original.abs_gain = 2.3f;
    original.abs_freq = 35.0f;

    // Apply to engine
    original.Apply(engine);

    // Verify engine received them (using current loose variables)
    ASSERT_EQ(engine.m_lockup_enabled, true);
    ASSERT_NEAR(engine.m_lockup_gain, 1.23f, 0.0001);
    ASSERT_NEAR(engine.m_lockup_start_pct, 2.34f, 0.0001);
    ASSERT_NEAR(engine.m_lockup_full_pct, 7.89f, 0.0001);
    ASSERT_NEAR(engine.m_lockup_rear_boost, 3.45f, 0.0001);
    ASSERT_NEAR(engine.m_lockup_gamma, 0.67f, 0.0001);
    ASSERT_NEAR(engine.m_lockup_prediction_sens, 33.0f, 0.0001);
    ASSERT_NEAR(engine.m_lockup_bump_reject, 1.2f, 0.0001);
    ASSERT_NEAR(engine.m_brake_load_cap, 4.5f, 0.0001);
    ASSERT_NEAR(engine.m_lockup_freq_scale, 1.2f, 0.0001);
    ASSERT_EQ(engine.m_abs_pulse_enabled, true);
    ASSERT_NEAR(engine.m_abs_gain, 2.3f, 0.0001);
    ASSERT_NEAR(engine.m_abs_freq_hz, 35.0f, 0.0001);

    // Extract back to a new preset
    Preset extracted("Extracted", false);
    extracted.UpdateFromEngine(engine);

    // They should be exactly equal
    ASSERT_TRUE(original.Equals(extracted));
}

TEST_CASE(test_refactor_braking_validation, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    Preset bad_preset("Bad", false);

    // Set malicious Braking values
    bad_preset.lockup_gain = -1.0f;
    bad_preset.lockup_start_pct = 0.0f;
    bad_preset.lockup_full_pct = 0.1f;
    bad_preset.lockup_gamma = 0.0f;
    bad_preset.lockup_prediction_sens = 0.0f;
    bad_preset.lockup_bump_reject = 0.0f;
    bad_preset.brake_load_cap = 0.0f;
    bad_preset.abs_gain = -5.0f;
    bad_preset.abs_freq = 0.0f;
    bad_preset.lockup_freq_scale = 0.0f;

    bad_preset.Apply(engine);

    // Verify engine clamped them (using current loose variables)
    ASSERT_NEAR(engine.m_lockup_gain, 0.0f, 0.0001);
    ASSERT_GE(engine.m_lockup_start_pct, 0.1f);
    ASSERT_GE(engine.m_lockup_full_pct, 0.2f);
    ASSERT_GE(engine.m_lockup_gamma, 0.1f);
    ASSERT_GE(engine.m_lockup_prediction_sens, 1.0f);
    ASSERT_GE(engine.m_lockup_bump_reject, 0.01f);
    ASSERT_GE(engine.m_brake_load_cap, 1.0f);
    ASSERT_NEAR(engine.m_abs_gain, 0.0f, 0.0001);
    ASSERT_GE(engine.m_abs_freq_hz, 1.0f);
    ASSERT_GE(engine.m_lockup_freq_scale, 0.1f);
}

}
