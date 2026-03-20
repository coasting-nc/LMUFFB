#include "test_ffb_common.h"

namespace FFBEngineTests {

// 1. Consistency Test: Ensures the math doesn't change when variables move
TEST_CASE(test_refactor_general_config_consistency, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    // Set specific General Config values (using current loose variables)
    engine.m_gain = 0.85f;
    engine.m_invert_force = true;
    engine.m_min_force = 0.05f;
    engine.m_wheelbase_max_nm = 12.0f;
    engine.m_target_rim_nm = 8.0f;
    engine.m_dynamic_normalization_enabled = true;
    engine.m_auto_load_normalization_enabled = true;

    // Create a telemetry state
    TelemInfoV01 data = CreateBasicTestTelemetry(25.0, 0.08);
    data.mLocalAccel.x = 15.0;
    data.mUnfilteredSteering = 0.5;
    data.mSteeringShaftTorque = 10.0;
    
    PumpEngineSteadyState(engine, data);
    double final_force = engine.calculate_force(&data, "LMP2", "Oreca", 0.0f, true, 0.0025);

    // Hardcode the expected value after first run
    double EXPECTED_VALUE = -0.44863114643038; 
    
    ASSERT_NEAR(final_force, EXPECTED_VALUE, 0.0001);
    std::cout << "[INFO] final_force: " << final_force << std::endl;
}

// 2. Round-Trip Test: Ensures Preset <-> Engine synchronization works
TEST_CASE(test_refactor_general_config_roundtrip, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    Preset original("TestPreset", false);
    // Set wild values for General Config variables
    original.gain = 1.77f;
    original.min_force = 0.12f;
    original.wheelbase_max_nm = 25.0f;
    original.target_rim_nm = 12.0f;
    original.dynamic_normalization_enabled = true;
    original.auto_load_normalization_enabled = true;

    // Apply to engine
    original.Apply(engine);

    // Verify engine received them (using current loose variables)
    ASSERT_NEAR(engine.m_gain, 1.77f, 0.0001);
    ASSERT_NEAR(engine.m_min_force, 0.12f, 0.0001);
    ASSERT_NEAR(engine.m_wheelbase_max_nm, 25.0f, 0.0001);
    ASSERT_NEAR(engine.m_target_rim_nm, 12.0f, 0.0001);
    ASSERT_EQ(engine.m_dynamic_normalization_enabled, true);
    ASSERT_EQ(engine.m_auto_load_normalization_enabled, true);

    // Extract back to a new preset
    Preset extracted("Extracted", false);
    extracted.UpdateFromEngine(engine);

    // They should be exactly equal
    ASSERT_TRUE(original.Equals(extracted));
}

// 3. Validation Test: Ensures clamping logic is preserved
TEST_CASE(test_refactor_general_config_validation, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    Preset bad_preset("Bad", false);
    
    // Set malicious General Config values
    bad_preset.gain = -5.0f;           // Should clamp to 0.0
    bad_preset.min_force = -1.0f;      // Should clamp to 0.0
    bad_preset.wheelbase_max_nm = 0.0f; // Should clamp to 1.0 (from Preset::Apply)
    bad_preset.target_rim_nm = 0.0f;    // Should clamp to 1.0 (from Preset::Apply)

    bad_preset.Apply(engine);

    // Verify engine clamped them (using current loose variables)
    ASSERT_NEAR(engine.m_gain, 0.0f, 0.0001);
    ASSERT_NEAR(engine.m_min_force, 0.0f, 0.0001);
    ASSERT_NEAR(engine.m_wheelbase_max_nm, 1.0f, 0.0001);
    ASSERT_NEAR(engine.m_target_rim_nm, 1.0f, 0.0001);
}

} // namespace FFBEngineTests
