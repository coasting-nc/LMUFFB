#include "test_ffb_common.h"

namespace FFBEngineTests {

// 1. Consistency Test: Ensures the math doesn't change when variables move
TEST_CASE(test_refactor_vibration_consistency, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    // Set specific Vibration values (using current loose variables)
    engine.m_vibration.vibration_gain = 0.85f;
    engine.m_vibration.texture_load_cap = 1.25f;
    engine.m_vibration.slide_enabled = true;
    engine.m_vibration.slide_gain = 0.55f;
    engine.m_vibration.slide_freq = 1.2f;
    engine.m_vibration.road_enabled = true;
    engine.m_vibration.road_gain = 0.45f;
    engine.m_vibration.spin_enabled = true;
    engine.m_vibration.spin_gain = 0.35f;
    engine.m_vibration.spin_freq_scale = 1.1f;
    engine.m_vibration.scrub_drag_gain = 0.25f;
    engine.m_vibration.bottoming_enabled = true;
    engine.m_vibration.bottoming_gain = 0.65f;
    engine.m_vibration.bottoming_method = 1;

    // Create a telemetry state that triggers vibration effects
    TelemInfoV01 data = CreateBasicTestTelemetry(30.0, 0.15); // 30m/s, 15% slip
    data.mWheel[0].mVerticalTireDeflection = 0.015;
    data.mWheel[1].mVerticalTireDeflection = 0.012;
    data.mWheel[2].mVerticalTireDeflection = 0.018;
    data.mWheel[3].mVerticalTireDeflection = 0.014;
    data.mLocalAccel.y = 10.0; // Vertical accel for road texture
    data.mWheel[2].mSuspForce = 8000.0; // High force for bottoming
    data.mWheel[3].mSuspForce = 7500.0;
    data.mWheel[2].mSuspensionDeflection = 0.12; // High deflection for bottoming
    data.mWheel[3].mSuspensionDeflection = 0.11;
    data.mWheel[2].mVerticalTireDeflection = 0.05; // Large deflection for bottoming
    data.mWheel[3].mVerticalTireDeflection = 0.045;

    // Pump the engine to settle filters and accumulators
    PumpEngineSteadyState(engine, data);
    double final_force = engine.calculate_force(&data, "LMP2", "Oreca", 0.0f, true, 0.0025);

    // This value is obtained by running the test on the current codebase.
    // [LOG] final_force: -0.0418642
    double EXPECTED_VALUE = -0.0418642;

    ASSERT_NEAR(final_force, EXPECTED_VALUE, 0.0001);
}

// 2. Round-Trip Test: Ensures Preset <-> Engine synchronization works
TEST_CASE(test_refactor_vibration_roundtrip, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    Preset original("TestPreset", false);
    // Set wild values for Vibration variables
    original.vibration.vibration_gain = 1.77f;
    original.vibration.texture_load_cap = 2.44f;
    original.vibration.slide_enabled = true;
    original.vibration.slide_gain = 1.33f;
    original.vibration.slide_freq = 2.1f;
    original.vibration.road_enabled = true;
    original.vibration.road_gain = 1.8f;
    original.vibration.spin_enabled = true;
    original.vibration.spin_gain = 1.22f;
    original.vibration.spin_freq_scale = 1.66f;
    original.vibration.scrub_drag_gain = 0.77f;
    original.vibration.bottoming_enabled = true;
    original.vibration.bottoming_gain = 1.44f;
    original.vibration.bottoming_method = 1;

    // Apply to engine
    original.Apply(engine);

    // Verify engine received them (using current loose variables)
    ASSERT_NEAR(engine.m_vibration.vibration_gain, 1.77f, 0.0001);
    ASSERT_NEAR(engine.m_vibration.texture_load_cap, 2.44f, 0.0001);
    ASSERT_EQ(engine.m_vibration.slide_enabled, true);
    ASSERT_NEAR(engine.m_vibration.slide_gain, 1.33f, 0.0001);
    ASSERT_NEAR(engine.m_vibration.slide_freq, 2.1f, 0.0001);
    ASSERT_EQ(engine.m_vibration.road_enabled, true);
    ASSERT_NEAR(engine.m_vibration.road_gain, 1.8f, 0.0001);
    ASSERT_EQ(engine.m_vibration.spin_enabled, true);
    ASSERT_NEAR(engine.m_vibration.spin_gain, 1.22f, 0.0001);
    ASSERT_NEAR(engine.m_vibration.spin_freq_scale, 1.66f, 0.0001);
    ASSERT_NEAR(engine.m_vibration.scrub_drag_gain, 0.77f, 0.0001);
    ASSERT_EQ(engine.m_vibration.bottoming_enabled, true);
    ASSERT_NEAR(engine.m_vibration.bottoming_gain, 1.44f, 0.0001);
    ASSERT_EQ(engine.m_vibration.bottoming_method, 1);

    // Extract back to a new preset
    Preset extracted("Extracted", false);
    extracted.UpdateFromEngine(engine);

    // They should be exactly equal
    ASSERT_TRUE(original.Equals(extracted));
}

// 3. Validation Test: Ensures clamping logic is preserved
TEST_CASE(test_refactor_vibration_validation, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    Preset bad_preset("Bad", false);

    // Set malicious Vibration values
    bad_preset.vibration.vibration_gain = -5.0f;     // Should clamp to 0.0
    bad_preset.vibration.texture_load_cap = 0.5f;     // Should clamp to 1.0
    bad_preset.vibration.slide_gain = -1.0f;          // Should clamp to 0.0
    bad_preset.vibration.slide_freq = 0.01f;          // Should clamp to 0.1
    bad_preset.vibration.road_gain = -2.0f;           // Should clamp to 0.0
    bad_preset.vibration.spin_gain = -3.0f;           // Should clamp to 0.0
    bad_preset.vibration.spin_freq_scale = 0.05f;     // Should clamp to 0.1
    bad_preset.vibration.scrub_drag_gain = -0.5f;     // Should clamp to 0.0
    bad_preset.vibration.bottoming_gain = 5.0f;       // Should clamp to 2.0

    bad_preset.Apply(engine);

    // Verify engine clamped them (using current loose variables)
    ASSERT_NEAR(engine.m_vibration.vibration_gain, 0.0f, 0.0001);
    ASSERT_NEAR(engine.m_vibration.texture_load_cap, 1.0f, 0.0001);
    ASSERT_NEAR(engine.m_vibration.slide_gain, 0.0f, 0.0001);
    ASSERT_NEAR(engine.m_vibration.slide_freq, 0.1f, 0.0001);
    ASSERT_NEAR(engine.m_vibration.road_gain, 0.0f, 0.0001);
    ASSERT_NEAR(engine.m_vibration.spin_gain, 0.0f, 0.0001);
    ASSERT_NEAR(engine.m_vibration.spin_freq_scale, 0.1f, 0.0001);
    ASSERT_NEAR(engine.m_vibration.scrub_drag_gain, 0.0f, 0.0001);
    ASSERT_NEAR(engine.m_vibration.bottoming_gain, 2.0f, 0.0001);
}

}
