#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_refactor_braking_consistency, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    // 1. Set specific Braking values
    engine.m_braking.lockup_enabled = true;
    engine.m_braking.lockup_gain = 1.2f;
    engine.m_braking.lockup_start_pct = 2.0f;
    engine.m_braking.lockup_full_pct = 8.0f;
    engine.m_braking.lockup_rear_boost = 2.5f;
    engine.m_braking.lockup_gamma = 0.5f;
    engine.m_braking.lockup_prediction_sens = 25.0f;
    engine.m_braking.lockup_bump_reject = 0.5f;
    engine.m_braking.brake_load_cap = 3.0f;
    engine.m_braking.lockup_freq_scale = 1.1f;
    engine.m_braking.abs_pulse_enabled = true;
    engine.m_braking.abs_gain = 1.5f;
    engine.m_braking.abs_freq = 30.0f;

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

    // 3. Establish baseline - run exactly 40 frames to settle
    for(int i=0; i<40; ++i) {
        data.mElapsedTime += 0.0025;
        engine.calculate_force(&data, "GT3", "Ferrari", 0.0f, true, 0.0025);
    }

    // Now trigger the ABS oscillation
    data.mElapsedTime += 0.0025;
    data.mWheel[0].mBrakePressure = 0.8;
    double final_force = engine.calculate_force(&data, "GT3", "Ferrari", 0.0f, true, 0.0025);

    // 4. ASSERT THE VALUE
    // establishment result
    double EXPECTED_VALUE = 0.29835;

    ASSERT_NEAR(final_force, EXPECTED_VALUE, 0.001);
}

TEST_CASE(test_refactor_braking_roundtrip, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    Preset original("TestPreset", false);
    original.braking.lockup_enabled = true;
    original.braking.lockup_gain = 1.23f;
    original.braking.lockup_start_pct = 2.34f;
    original.braking.lockup_full_pct = 7.89f;
    original.braking.lockup_rear_boost = 3.45f;
    original.braking.lockup_gamma = 0.67f;
    original.braking.lockup_prediction_sens = 33.0f;
    original.braking.lockup_bump_reject = 1.2f;
    original.braking.brake_load_cap = 4.5f;
    original.braking.lockup_freq_scale = 1.2f;
    original.braking.abs_pulse_enabled = true;
    original.braking.abs_gain = 2.3f;
    original.braking.abs_freq = 35.0f;

    // Apply to engine
    original.Apply(engine);

    // Verify engine received them
    ASSERT_EQ(engine.m_braking.lockup_enabled, true);
    ASSERT_NEAR(engine.m_braking.lockup_gain, 1.23f, 0.0001);
    ASSERT_NEAR(engine.m_braking.lockup_start_pct, 2.34f, 0.0001);
    ASSERT_NEAR(engine.m_braking.lockup_full_pct, 7.89f, 0.0001);
    ASSERT_NEAR(engine.m_braking.lockup_rear_boost, 3.45f, 0.0001);
    ASSERT_NEAR(engine.m_braking.lockup_gamma, 0.67f, 0.0001);
    ASSERT_NEAR(engine.m_braking.lockup_prediction_sens, 33.0f, 0.0001);
    ASSERT_NEAR(engine.m_braking.lockup_bump_reject, 1.2f, 0.0001);
    ASSERT_NEAR(engine.m_braking.brake_load_cap, 4.5f, 0.0001);
    ASSERT_NEAR(engine.m_braking.lockup_freq_scale, 1.2f, 0.0001);
    ASSERT_EQ(engine.m_braking.abs_pulse_enabled, true);
    ASSERT_NEAR(engine.m_braking.abs_gain, 2.3f, 0.0001);
    ASSERT_NEAR(engine.m_braking.abs_freq, 35.0f, 0.0001);

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
    bad_preset.braking.lockup_gain = -1.0f;
    bad_preset.braking.lockup_start_pct = 0.0f;
    bad_preset.braking.lockup_full_pct = 0.1f;
    bad_preset.braking.lockup_gamma = 0.0f;
    bad_preset.braking.lockup_prediction_sens = 0.0f;
    bad_preset.braking.lockup_bump_reject = 0.0f;
    bad_preset.braking.brake_load_cap = 0.0f;
    bad_preset.braking.abs_gain = -5.0f;
    bad_preset.braking.abs_freq = 0.0f;
    bad_preset.braking.lockup_freq_scale = 0.0f;

    bad_preset.Apply(engine);

    // Verify engine clamped them
    ASSERT_NEAR(engine.m_braking.lockup_gain, 0.0f, 0.0001);
    ASSERT_GE(engine.m_braking.lockup_start_pct, 0.1f);
    ASSERT_GE(engine.m_braking.lockup_full_pct, 0.2f);
    ASSERT_GE(engine.m_braking.lockup_gamma, 0.1f);
    ASSERT_GE(engine.m_braking.lockup_prediction_sens, 1.0f);
    ASSERT_GE(engine.m_braking.lockup_bump_reject, 0.01f);
    ASSERT_GE(engine.m_braking.brake_load_cap, 1.0f);
    ASSERT_NEAR(engine.m_braking.abs_gain, 0.0f, 0.0001);
    ASSERT_GE(engine.m_braking.abs_freq, 1.0f);
    ASSERT_GE(engine.m_braking.lockup_freq_scale, 0.1f);
}

}
