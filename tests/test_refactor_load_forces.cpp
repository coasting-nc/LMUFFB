#include "test_ffb_common.h"

namespace FFBEngineTests {

// 1. Consistency Test: Ensures the math doesn't change when variables move
TEST_CASE(test_refactor_load_forces_consistency, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    // Set specific Load Forces values (using current loose variables)
    engine.m_load_forces.lat_load_effect = 0.5f;
    engine.m_load_forces.lat_load_transform = (int)LoadTransform::CUBIC;
    engine.m_load_forces.long_load_effect = 2.0f;
    engine.m_load_forces.long_load_smoothing = 0.1f;
    engine.m_load_forces.long_load_transform = (int)LoadTransform::QUADRATIC;

    // Create a telemetry state that triggers load forces logic
    TelemInfoV01 data = CreateBasicTestTelemetry(25.0, 0.05); // 25 m/s, 5% slip
    data.mLocalAccel.x = 10.0; // Lateral G
    data.mLocalAccel.z = -5.0;  // Longitudinal G (Braking)
    data.mSteeringShaftTorque = 10.0;

    // Set some initial smoothed states to avoid long ramp-up
    FFBEngineTestAccess::SetLongitudinalLoadSmoothed(engine, 1.0);
    FFBEngineTestAccess::SetFrontGripSmoothedState(engine, 1.0);

    PumpEngineTime(engine, data, 0.5); // Advance to let smoothing settle
    double final_force = engine.calculate_force(&data, "LMP2", "Oreca", 0.0f, true, 0.0025);

    // TODO: Run this test ONCE on the current code, look at the output,
    // and hardcode the exact result here.
    // Result from first run: 0.4577109289110196
    double EXPECTED_VALUE = 0.4577109289110196;

    ASSERT_NEAR(final_force, EXPECTED_VALUE, 0.0001);
}

// 2. Round-Trip Test: Ensures Preset <-> Engine synchronization works
TEST_CASE(test_refactor_load_forces_roundtrip, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    Preset original("TestPreset", false);
    // Set wild values for Load Forces variables
    original.load_forces.lat_load_effect = 0.77f;
    original.load_forces.lat_load_transform = 2; // QUADRATIC
    original.load_forces.long_load_effect = 4.5f;
    original.load_forces.long_load_smoothing = 0.25f;
    original.load_forces.long_load_transform = 3; // HERMITE

    // Apply to engine
    original.Apply(engine);

    // Verify engine received them (using current loose variables)
    ASSERT_NEAR(engine.m_load_forces.lat_load_effect, 0.77f, 0.0001);
    ASSERT_EQ(static_cast<int>(engine.m_load_forces.lat_load_transform), 2);
    ASSERT_NEAR(engine.m_load_forces.long_load_effect, 4.5f, 0.0001);
    ASSERT_NEAR(engine.m_load_forces.long_load_smoothing, 0.25f, 0.0001);
    ASSERT_EQ(static_cast<int>(engine.m_load_forces.long_load_transform), 3);

    // Extract back to a new preset
    Preset extracted("Extracted", false);
    extracted.UpdateFromEngine(engine);

    // They should be exactly equal
    ASSERT_TRUE(original.Equals(extracted));
}

// 3. Validation Test: Ensures clamping logic is preserved
TEST_CASE(test_refactor_load_forces_validation, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    Preset bad_preset("Bad", false);

    // Set malicious Load Forces values
    bad_preset.load_forces.lat_load_effect = -1.0f;       // Should clamp to 0.0
    bad_preset.load_forces.lat_load_transform = 5;    // Should clamp to 3
    bad_preset.load_forces.long_load_effect = 15.0f;  // Should clamp to 10.0
    bad_preset.load_forces.long_load_smoothing = -0.5f; // Should clamp to 0.0
    bad_preset.load_forces.long_load_transform = -2;  // Should clamp to 0

    bad_preset.Apply(engine);

    // Verify engine clamped them (using current loose variables)
    ASSERT_NEAR(engine.m_load_forces.lat_load_effect, 0.0f, 0.0001);
    ASSERT_EQ(static_cast<int>(engine.m_load_forces.lat_load_transform), 3);
    ASSERT_NEAR(engine.m_load_forces.long_load_effect, 10.0f, 0.0001);
    ASSERT_NEAR(engine.m_load_forces.long_load_smoothing, 0.0f, 0.0001);
    ASSERT_EQ(static_cast<int>(engine.m_load_forces.long_load_transform), 0);
}

}
