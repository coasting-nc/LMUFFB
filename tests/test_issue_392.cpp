#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_issue_392_default_value, "Physics") {
    FFBEngine engine;
    // Default should be true as per plan
    ASSERT_TRUE(engine.m_load_sensitivity_enabled == true);
}

TEST_CASE(test_issue_392_physics_toggle_impact, "Physics") {
    FFBEngine engine;
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);

    // Setup baseline car state
    data.mElapsedTime = 1.0;

    // Use a car name that triggers fallback (Encrypted)
    const char* carName = "Ferrari 499P";

    // Setup wheel data with significant slip angle to ensure grip < 1.0
    for(int i=0; i<4; i++) {
        data.mWheel[i].mLongitudinalGroundVel = 20.0;
        data.mWheel[i].mLateralPatchVel = 2.0; // ~0.1 rad slip
        data.mWheel[i].mGripFract = 0.0; // Force fallback
    }

    FFBEngineTestAccess::SetStaticLoadLatched(engine, true);
    FFBEngineTestAccess::SetStaticFrontLoad(engine, 3000.0);

    // 1. ENABLED (Default)
    engine.m_load_sensitivity_enabled = true;

    // Low Load (1500N)
    data.mWheel[0].mTireLoad = 1500.0;
    data.mWheel[1].mTireLoad = 1500.0;
    PumpEngineTime(engine, data, 1.0); // Let it settle
    double grip_low_load = engine.GetDebugBatch().back().calc_front_grip;

    // High Load (6000N)
    data.mWheel[0].mTireLoad = 6000.0;
    data.mWheel[1].mTireLoad = 6000.0;
    PumpEngineTime(engine, data, 1.0); // Let it settle
    double grip_high_load = engine.GetDebugBatch().back().calc_front_grip;

    // With load sensitivity, higher load should result in higher grip (for same slip angle)
    // because optimal slip angle increases.
    ASSERT_GT(grip_high_load, grip_low_load);

    // 2. DISABLED
    engine.m_load_sensitivity_enabled = false;

    // Low Load
    data.mWheel[0].mTireLoad = 1500.0;
    data.mWheel[1].mTireLoad = 1500.0;
    PumpEngineTime(engine, data, 1.0);
    double grip_disabled_low = engine.GetDebugBatch().back().calc_front_grip;

    // High Load
    data.mWheel[0].mTireLoad = 6000.0;
    data.mWheel[1].mTireLoad = 6000.0;
    PumpEngineTime(engine, data, 1.0);
    double grip_disabled_high = engine.GetDebugBatch().back().calc_front_grip;

    // Without load sensitivity, grip should be identical regardless of load
    ASSERT_NEAR(grip_disabled_high, grip_disabled_low, 0.001);
}

TEST_CASE(test_issue_392_persistence, "Config") {
    FFBEngine engine;
    Preset p("TestPreset");

    // Toggle off
    engine.m_load_sensitivity_enabled = false;

    // Update preset from engine
    p.UpdateFromEngine(engine);
    ASSERT_FALSE(p.load_sensitivity_enabled);

    // Toggle engine back on
    engine.m_load_sensitivity_enabled = true;

    // Apply preset
    p.Apply(engine);
    ASSERT_FALSE(engine.m_load_sensitivity_enabled);

    // Equality check
    Preset p2 = p;
    ASSERT_TRUE(p.Equals(p2));
    p2.load_sensitivity_enabled = true;
    ASSERT_FALSE(p.Equals(p2));
}

} // namespace FFBEngineTests
