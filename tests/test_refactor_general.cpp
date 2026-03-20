#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_refactor_general_consistency, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    // 1. Set specific General FFB values (using current loose variables)
    engine.m_general.gain = 0.85f;
    engine.m_invert_force = true;
    engine.m_general.min_force = 0.05f;
    engine.m_general.wheelbase_max_nm = 12.0f;
    engine.m_general.target_rim_nm = 8.0f;
    engine.m_general.dynamic_normalization_enabled = true;
    engine.m_general.auto_load_normalization_enabled = true;

    // 2. Create a telemetry state
    TelemInfoV01 data = CreateBasicTestTelemetry(25.0, 0.08);
    data.mUnfilteredSteering = 0.5;
    data.mSteeringShaftTorque = 10.0;

    // 3. Pump the engine to settle filters
    PumpEngineSteadyState(engine, data);

    // 4. Get the final output force
    double final_force = engine.calculate_force(&data, "LMP2", "Oreca", 0.0f, true, 0.0025);

    // 5. ASSERT THE EXACT VALUE
    // RUN THIS ON THE CURRENT CODEBASE FIRST.
    double EXPECTED_VALUE = -0.448631;

    ASSERT_NEAR(final_force, EXPECTED_VALUE, 0.0001);
}

TEST_CASE(test_refactor_general_roundtrip, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    Preset original("TestPreset", false);
    // Set values for General variables (Note: invert_force is not currently in Preset)
    original.general.gain = 1.77f;
    original.general.min_force = 0.12f;
    original.general.wheelbase_max_nm = 20.0f;
    original.general.target_rim_nm = 15.0f;
    original.general.dynamic_normalization_enabled = true;
    original.general.auto_load_normalization_enabled = true;

    // Apply to engine
    original.Apply(engine);

    // Verify engine received them
    ASSERT_NEAR(engine.m_general.gain, 1.77f, 0.0001);
    ASSERT_NEAR(engine.m_general.min_force, 0.12f, 0.0001);
    ASSERT_NEAR(engine.m_general.wheelbase_max_nm, 20.0f, 0.0001);
    ASSERT_NEAR(engine.m_general.target_rim_nm, 15.0f, 0.0001);
    ASSERT_EQ(engine.m_general.dynamic_normalization_enabled, true);
    ASSERT_EQ(engine.m_general.auto_load_normalization_enabled, true);

    // Extract back to a new preset
    Preset extracted("Extracted", false);
    extracted.UpdateFromEngine(engine);

    // They should be exactly equal
    ASSERT_TRUE(original.Equals(extracted));
}

TEST_CASE(test_refactor_general_validation, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    Preset bad_preset("Bad", false);

    // Set malicious General values
    bad_preset.general.gain = -5.0f;                // Should clamp to 0.0
    bad_preset.general.wheelbase_max_nm = 0.0f;      // Should clamp to 1.0
    bad_preset.general.target_rim_nm = -10.0f;       // Should clamp to 1.0

    bad_preset.Apply(engine);

    // Verify engine clamped them
    ASSERT_NEAR(engine.m_general.gain, 0.0f, 0.0001);
    ASSERT_NEAR(engine.m_general.wheelbase_max_nm, 1.0f, 0.0001);
    ASSERT_NEAR(engine.m_general.target_rim_nm, 1.0f, 0.0001);
}

}
