#include "test_ffb_common.h"
#include "../src/FFBEngine.h"

namespace FFBEngineTests {

TEST_CASE(test_issue_332_mcontrol_glitch, "Regression") {
    std::cout << "\nTest: Issue #332 - mControl Glitch during player join/leave" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry();

    // 1. Normal driving
    engine.calculate_force(&data, "GT3", "Ferrari", 0.0f, true, 0.0025, 0);
    ASSERT_EQ(engine.m_safety.safety_timer, 0.0);

    // 2. Glitch: mControl becomes -1 (NONE) for one frame
    // In current implementation, this triggers safety because -1 != 0.
    // In NEW implementation, it should NOT trigger safety.
    engine.calculate_force(&data, "GT3", "Ferrari", 0.0f, false, 0.0025, -1);

    // 3. Recovery: mControl becomes 0 again
    engine.calculate_force(&data, "GT3", "Ferrari", 0.0f, true, 0.0025, 0);

    // Check if safety was triggered
    ASSERT_EQ(engine.m_safety.safety_timer, 0.0);
    std::cout << "  [PASS] Safety NOT triggered for 0 -> -1 -> 0 transition." << std::endl;

    // 4. Genuine transition: mControl becomes 1 (AI)
    engine.calculate_force(&data, "GT3", "Ferrari", 0.0f, false, 0.0025, 1);
    ASSERT_GT(engine.m_safety.safety_timer, 0.0);
    std::cout << "  [PASS] Safety correctly triggered for 0 -> 1 transition." << std::endl;
}

TEST_CASE(test_issue_332_soft_lock_pits, "Regression") {
    std::cout << "\nTest: Issue #332 - Soft Lock in Pits" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry();
    engine.m_soft_lock_enabled = true;
    engine.m_wheelbase_max_nm = 10.0f;
    engine.m_target_rim_nm = 10.0f;
    engine.m_gain = 1.0f;

    // Normalization setup
    FFBEngineTestAccess::SetSessionPeakTorque(engine, 10.0);
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 10.0);

    // Simulate being in garage with a messy steering value
    data.mUnfilteredSteering = 1.5; // Way past lock

    // Test 1: In Pits/Pause but NOT in Garage Stall (Allowed=false, inGarage=false)
    // This should still have Soft Lock (Issue #184)
    double force_pause = engine.calculate_force(&data, "GT3", "Ferrari", 0.0f, false, 0.0025, 0, false);
    ASSERT_NE(force_pause, 0.0);
    std::cout << "  Force in pause (allowed=false, inGarage=false): " << force_pause << " (Expected non-zero soft lock)" << std::endl;

    // Test 2: In Garage Stall (Allowed=false, inGarage=true)
    // This should NOT have Soft Lock (Fix for #332)
    double force_garage = engine.calculate_force(&data, "GT3", "Ferrari", 0.0f, false, 0.0025, 0, true);
    ASSERT_EQ(force_garage, 0.0);
    std::cout << "  Force in garage (allowed=false, inGarage=true): " << force_garage << " (Expected zero)" << std::endl;
}

} // namespace FFBEngineTests
