#include "test_ffb_common.h"
#include <iostream>

namespace FFBEngineTests {

/**
 * Test for Issue #281: Fix FFB Spikes on Driving State Transition
 *
 * Verifies that when IsPlayerActivelyDriving() is false (e.g., Paused),
 * any persistent forces (like Soft Lock) are correctly slewed to zero.
 */
void test_issue_281_transition_smoothing() {
    std::cout << "\nTest: Issue #281 Transition Smoothing" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);

    // Setup: High torque wheelbase, Soft Lock enabled
    engine.m_soft_lock_enabled = true;
    engine.m_soft_lock_stiffness = 20.0f; // Standard stiffness
    engine.m_wheelbase_max_nm = 20.0f;     // 20Nm base
    engine.m_target_rim_nm = 20.0f;
    engine.m_gain = 1.0f;

    // Normalization setup (ensure structural mult is valid)
    FFBEngineTestAccess::SetSessionPeakTorque(engine, 20.0);
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 20.0);

    // Telemetry: Wheel beyond lock limit (triggers Soft Lock)
    TelemInfoV01 data = CreateBasicTestTelemetry(0.0, 0.0);
    data.mUnfilteredSteering = 1.1; // 110% lock

    // Simulate main.cpp logic:
    // is_driving = GameConnector::Get().IsPlayerActivelyDriving();
    // full_allowed = g_engine.IsFFBAllowed(...) && is_driving;
    // force = g_engine.calculate_force(..., full_allowed);
    // [FIX] if (!is_driving) force = 0.0;
    // force = g_engine.ApplySafetySlew(force, 0.0025, !full_allowed);

    // Scenario 1: Active Driving (is_driving = true), but AI/Stationary (full_allowed = false)
    // Soft Lock SHOULD be active.
    {
        bool is_driving = true;
        bool full_allowed = false; // e.g., AI driving or in garage stall

        double slewed_force = 0.0;
        // Run for 10 frames to let the slew rate limiter reach the target
        for (int i = 0; i < 10; i++) {
            double force = engine.calculate_force(&data, "GT3", "911 GT3", 0.0f, full_allowed);
            if (!is_driving) force = 0.0;
            slewed_force = engine.ApplySafetySlew(force, 0.0025, !full_allowed);
        }

        std::cout << "  Active Driving, Muted Physics (Garage/AI) - Force (expect Soft Lock): " << slewed_force << std::endl;
        // Soft Lock at 1.1 with stiffness 20 and 20Nm base should be significant.
        // It's capped by -1.0 to 1.0. At 1.1 steer, it should be -1.0.
        ASSERT_LT(slewed_force, -0.9);
    }

    // Scenario 2: Paused (is_driving = false) - WITH FIX IN main.cpp
    // This test simulates the actual logic now present in main.cpp.
    {
        bool is_driving = false; // Paused
        bool full_allowed = false;

        // Run many frames of "Paused" logic WITH the fix
        double slewed_force = 0.0;
        for (int i = 0; i < 50; i++) {
            double force = engine.calculate_force(&data, "GT3", "911 GT3", 0.0f, full_allowed);
            // Fix logic as in main.cpp:
            if (!is_driving) force = 0.0;
            slewed_force = engine.ApplySafetySlew(force, 0.0025, true);
        }

        std::cout << "  Paused (With Fix) - Final Slewed Force: " << slewed_force << std::endl;

        // With the fix, the force reaches zero.
        ASSERT_NEAR(slewed_force, 0.0, 0.01);
    }

}

AutoRegister reg_issue_281_spikes("Issue #281 Transition Spikes", "Issue281", {"Physics", "Regression"}, test_issue_281_transition_smoothing);

} // namespace FFBEngineTests
