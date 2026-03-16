#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_dynamic_normalization_toggle_behavior, "Physics") {
    std::cout << "\nTest: Dynamic Normalization Toggle Behavior" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);

    // Default should be DISABLED (Issue #207)
    ASSERT_FALSE(engine.m_dynamic_normalization_enabled);

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mDeltaTime = 0.01f;
    data.mSteeringShaftTorque = 40.0f; // High torque
    data.mLocalAccel.x = 1.0f * 9.81f; // Clean state

    // Ensure m_target_rim_nm is something known
    engine.m_target_rim_nm = 10.0f;
    engine.m_wheelbase_max_nm = 15.0f;
    engine.m_torque_source = 0; // Shaft Torque

    // Initial peak from Preset::Apply (via constructor) would be default 25.0,
    // but InitializeEngine(engine) overrides it to 20.0 for legacy reasons.
    // Set it explicitly to 10.0 for this test.
    FFBEngineTestAccess::SetSessionPeakTorque(engine, 10.0);
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 10.0);
    ASSERT_NEAR(FFBEngineTestAccess::GetSessionPeakTorque(engine), 10.0, 0.001);

    // Settle rolling average to avoid spike rejection
    FFBEngineTestAccess::SetRollingAverageTorque(engine, 40.0);
    FFBEngineTestAccess::SetLastRawTorque(engine, 40.0);

    // 1. Run while DISABLED
    engine.calculate_force(&data);

    // Peak should NOT update
    ASSERT_NEAR(FFBEngineTestAccess::GetSessionPeakTorque(engine), 10.0, 0.001);

    // Multiplier should be converging to 1/10 = 0.1
    // (Wait longer for EMA to settle, STRUCT_MULT_SMOOTHING_TAU = 0.25s, so 100 frames at 0.01s = 1.0s = 4x tau)
    for(int i=0; i<400; i++) engine.calculate_force(&data);
    double mult1 = FFBEngineTestAccess::GetSmoothedStructuralMult(engine);
    std::cout << "DEBUG: Mult1 = " << mult1 << " (Expected ~0.1)" << std::endl;
    ASSERT_NEAR(mult1, 0.1, 0.01);

    // 2. ENABLE it
    engine.m_dynamic_normalization_enabled = true;
    engine.calculate_force(&data);

    // Peak SHOULD update to 40.0
    double peak2 = FFBEngineTestAccess::GetSessionPeakTorque(engine);
    std::cout << "DEBUG: Peak2 = " << peak2 << " (Expected 40.0)" << std::endl;
    ASSERT_NEAR(peak2, 40.0, 0.001);

    // Multiplier should start converging to 1/40 = 0.025
    for(int i=0; i<800; i++) engine.calculate_force(&data);
    double mult2 = FFBEngineTestAccess::GetSmoothedStructuralMult(engine);
    std::cout << "DEBUG: Mult2 = " << mult2 << " (Expected ~0.025)" << std::endl;
    ASSERT_NEAR(mult2, 0.025, 0.001);

    // 3. DISABLE it again
    engine.m_dynamic_normalization_enabled = false;
    data.mSteeringShaftTorque = 60.0f; // Even higher torque
    FFBEngineTestAccess::SetRollingAverageTorque(engine, 60.0);
    FFBEngineTestAccess::SetLastRawTorque(engine, 60.0);

    engine.calculate_force(&data);

    // Peak should NOT update (stays at 40.0 from previous step)
    ASSERT_NEAR(FFBEngineTestAccess::GetSessionPeakTorque(engine), 40.0, 0.001);

    // Multiplier should converge back to 1/10 = 0.1 (manual scaling)
    for(int i=0; i<400; i++) engine.calculate_force(&data);
    double mult3 = FFBEngineTestAccess::GetSmoothedStructuralMult(engine);
    std::cout << "DEBUG: Mult3 = " << mult3 << " (Expected ~0.1)" << std::endl;
    ASSERT_NEAR(mult3, 0.1, 0.01);
}

TEST_CASE(test_config_persistence_dynamic_normalization, "Logic") {
    std::cout << "\nTest: Config Persistence - Dynamic Normalization Toggle" << std::endl;
    FFBEngine engine;

    // 1. Test Global Save/Load
    engine.m_dynamic_normalization_enabled = true;
    Config::Save(engine, "test_normalization.ini");

    FFBEngine engine2;
    engine2.m_dynamic_normalization_enabled = false;
    Config::Load(engine2, "test_normalization.ini");
    ASSERT_TRUE(engine2.m_dynamic_normalization_enabled);

    // 2. Test Preset Save/Load
    Preset p("TestNorm");
    p.dynamic_normalization_enabled = true;

    FFBEngine engine3;
    engine3.m_dynamic_normalization_enabled = false;
    p.Apply(engine3);
    ASSERT_TRUE(engine3.m_dynamic_normalization_enabled);

    engine3.m_dynamic_normalization_enabled = false;
    p.UpdateFromEngine(engine3);
    ASSERT_FALSE(p.dynamic_normalization_enabled);
}

TEST_CASE(test_auto_load_normalization_reset_behavior, "Physics") {
    std::cout << "\nTest: Auto Load Normalization Reset Behavior" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);

    // Default should be DISABLED
    ASSERT_FALSE(engine.m_auto_load_normalization_enabled);

    // Seed as GT3 (5000N)
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    engine.calculate_force(&data, "GT3");
    data.mElapsedTime += 0.01;
    engine.calculate_force(&data, "GT3");
    ASSERT_NEAR(FFBEngineTestAccess::GetAutoPeakLoad(engine), 5000.0, 1.0);

    // 1. Enable and "Learn" a higher peak
    engine.m_auto_load_normalization_enabled = true;
    data.mWheel[0].mTireLoad = 6000.0;
    data.mWheel[1].mTireLoad = 6000.0;
    data.mElapsedTime += 0.01;
    engine.calculate_force(&data, "GT3");
    ASSERT_NEAR(FFBEngineTestAccess::GetAutoPeakLoad(engine), 6000.0, 1.0);

    // 2. Disable and verify it resets to class default (4800N)
    // In real app, UI calls ResetNormalization() when toggled OFF.
    engine.m_auto_load_normalization_enabled = false;
    engine.ResetNormalization();

    ASSERT_NEAR(FFBEngineTestAccess::GetAutoPeakLoad(engine), 5000.0, 1.0);

    // 3. Verify static load learning is also reset
    // Default static load for GT3 is 4800 * 0.5 = 2400
    ASSERT_NEAR(FFBEngineTestAccess::GetStaticFrontLoad(engine), 2500.0, 1.0);
}

TEST_CASE(test_load_normalization_disabled_no_learning, "Physics") {
    std::cout << "\nTest: Load Normalization Disabled No Learning" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);

    // Default is DISABLED
    ASSERT_FALSE(engine.m_auto_load_normalization_enabled);

    TelemInfoV01 data = CreateBasicTestTelemetry(10.0); // 10 m/s is learning range
    data.mWheel[0].mTireLoad = 8000.0;
    data.mWheel[1].mTireLoad = 8000.0;

    // Seed as GT3 (5000N baseline)
    // We expect the auto peak to stay at 5000, but static load WILL learn
    // immediately because it's required for other effects like Longitudinal Load.
    engine.calculate_force(&data, "GT3");
    data.mElapsedTime += 0.01;
    engine.calculate_force(&data, "GT3");
    double initial_peak = FFBEngineTestAccess::GetAutoPeakLoad(engine);
    double initial_static = FFBEngineTestAccess::GetStaticFrontLoad(engine);

    ASSERT_NEAR(initial_peak, 5000.0, 1.0);
    // Static load started at 2400 and immediately learned a bit from the 8000N input
    ASSERT_GT(initial_static, 2500.0);

    // Drive for many frames
    for(int i=0; i<100; i++) engine.calculate_force(&data, "GT3");

    // Peak should not have changed because normalization is disabled
    ASSERT_NEAR(FFBEngineTestAccess::GetAutoPeakLoad(engine), 5000.0, 1.0);
    
    // Static SHOULD have learned significantly
    ASSERT_GT(FFBEngineTestAccess::GetStaticFrontLoad(engine), 3000.0);

    // Enable it
    engine.m_auto_load_normalization_enabled = true;
    for(int i=0; i<100; i++) engine.calculate_force(&data, "GT3");

    // Now peak should learn
    ASSERT_GT(FFBEngineTestAccess::GetAutoPeakLoad(engine), 5000.0);
    ASSERT_GT(FFBEngineTestAccess::GetStaticFrontLoad(engine), 3000.0);

    // Disable it and Reset
    engine.m_auto_load_normalization_enabled = false;
    engine.ResetNormalization();

    ASSERT_NEAR(FFBEngineTestAccess::GetAutoPeakLoad(engine), 5000.0, 1.0);
    ASSERT_NEAR(FFBEngineTestAccess::GetStaticFrontLoad(engine), 2500.0, 1.0);
}

} // namespace FFBEngineTests
