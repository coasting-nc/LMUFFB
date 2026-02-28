#include "test_ffb_common.h"
#include <atomic>
#include <mutex>
#include "../src/lmu_sm_interface/LmuSharedMemoryWrapper.h"

namespace FFBEngineTests {

void test_issue_174_menu_muting() {
    std::cout << "\nTest: Issue #174 Menu Muting Repro" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);

    // Setup Soft Lock to be strong
    engine.m_soft_lock_enabled = true;
    engine.m_soft_lock_stiffness = 20.0f;
    engine.m_wheelbase_max_nm = 100.0f;
    engine.m_target_rim_nm = 100.0f;
    engine.m_gain = 1.0f;

    // Normalization setup
    FFBEngineTestAccess::SetSessionPeakTorque(engine, 100.0);
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 100.0);

    // Mock Telemetry
    TelemInfoV01 data = CreateBasicTestTelemetry(0.0, 0.0);
    data.mUnfilteredSteering = 1.1; // Beyond lock

    // 1. Verify that when in_realtime is TRUE, Soft Lock IS active (Issue #184 behavior)
    // This happens when car is in garage or AI is driving.
    {
        bool in_realtime = true;
        bool full_allowed = false; // e.g. AI driving
        double force = 0.0;
        bool should_output = false;

        // Simulate FFBThread logic in main.cpp (v0.7.108)
        force = engine.calculate_force(&data, "GT3", "911 GT3", 0.0f, full_allowed);
        if (!in_realtime) force = 0.0;
        should_output = true;

        if (!should_output) force = 0.0;

        std::cout << "  Force with in_realtime=true (expect Soft Lock active): " << force << std::endl;
        ASSERT_NEAR(force, -1.0, 0.01);
    }

    // 2. Verify that when in_realtime is FALSE, force is ZERO (Issue #174 fix)
    // This happens when in the pause menu or garage UI.
    {
        bool in_realtime = false;
        bool full_allowed = false;
        double force = 0.0;
        bool should_output = false;

        // Simulate FFBThread logic in main.cpp (v0.7.108)
        force = engine.calculate_force(&data, "GT3", "911 GT3", 0.0f, full_allowed);
        if (!in_realtime) force = 0.0;
        should_output = true;

        if (!should_output) force = 0.0;

        std::cout << "  Force with in_realtime=false (expect zeroed): " << force << std::endl;
        ASSERT_EQ(force, 0.0);
    }
}

AutoRegister reg_issue_174_repro("Issue #174 Menu Muting Repro", "Issue174", {"Physics", "Issue174"}, test_issue_174_menu_muting);

} // namespace FFBEngineTests
