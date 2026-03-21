#include "test_ffb_common.h"
#include <iostream>

namespace FFBEngineTests {

/**
 * Test for Issue #281: Fix FFB Spikes on Driving State Transition
 *
 * Verifies that when player is not in control (mControl != 0),
 * all FFB is zeroed, while Soft Lock remains active in the garage and pause.
 */
void test_issue_281_transition_smoothing() {
    std::cout << "\nTest: Issue #281 Transition Smoothing" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);

    // Setup: High torque wheelbase, Soft Lock enabled
    engine.m_advanced.soft_lock_enabled = true;
    engine.m_advanced.soft_lock_stiffness = 20.0f; // Standard stiffness
    engine.m_general.wheelbase_max_nm = 20.0f;     // 20Nm base
    engine.m_general.target_rim_nm = 20.0f;
    engine.m_general.gain = 1.0f;

    // Normalization setup (ensure structural mult is valid)
    FFBEngineTestAccess::SetSessionPeakTorque(engine, 20.0);
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 20.0);

    // Telemetry: Wheel beyond lock limit (triggers Soft Lock)
    TelemInfoV01 data = CreateBasicTestTelemetry(0.0, 0.0);
    data.mUnfilteredSteering = 1.1; // 110% lock

    // Simulate main.cpp logic:
    // is_driving = GameConnector::Get().IsPlayerActivelyDriving();
    // full_allowed = g_engine.m_safety.IsFFBAllowed(...) && is_driving;
    // force = g_engine.calculate_force(..., full_allowed);
    // [FIX] if (scoring.mControl != 0) force = 0.0;
    // force = g_engine.m_safety.ApplySafetySlew(force, 0.0025, !full_allowed);

    // Scenario 1: Stationary in Garage / Paused (is_driving = false, mControl = ControlMode::PLAYER)
    // Soft Lock SHOULD be active.
    {
        bool is_driving = false;
        signed char mControl = static_cast<signed char>(ControlMode::PLAYER);
        bool full_allowed = false; // Muted physics because we are in garage or paused

        double slewed_force = 0.0;
        // Run for frames to let the slew rate limiter reach the target
        // Reaching -1.0 at 2.0 u/s takes 200 frames.
        for (int i = 0; i < 210; i++) {
            double force = engine.calculate_force(&data, "GT3", "911 GT3", 0.0f, full_allowed);
            if (mControl != static_cast<signed char>(ControlMode::PLAYER)) force = 0.0;
            slewed_force = engine.m_safety.ApplySafetySlew(force, 0.0025, !full_allowed);
        }

        std::cout << "  Garage/Paused (mControl=PLAYER) - Force (expect Soft Lock): " << slewed_force << std::endl;
        // Soft Lock should be active (~ -1.0)
        ASSERT_LT(slewed_force, -0.9);
    }

    // Scenario 2: AI Takeover / Transition to Menu (mControl != PLAYER) - WITH IMPROVED FIX
    {
        bool is_driving = false;
        signed char mControl = static_cast<signed char>(ControlMode::AI);
        bool full_allowed = false;

        // Run many frames to verify it slews to zero
        double slewed_force = -1.0; // Start with high force from previous state
        FFBEngineTestAccess::SetLastOutputForce(engine, -1.0);

        // Reaching 0.0 from -1.0 at 2.0 u/s takes 0.5s = 200 frames at 400Hz
        for (int i = 0; i < 210; i++) {
            double force = engine.calculate_force(&data, "GT3", "911 GT3", 0.0f, full_allowed);
            // Fix logic as in main.cpp:
            if (mControl != static_cast<signed char>(ControlMode::PLAYER)) force = 0.0;
            slewed_force = engine.m_safety.ApplySafetySlew(force, 0.0025, true);
        }

        std::cout << "  AI Takeover (mControl=AI) - Final Slewed Force: " << slewed_force << std::endl;

        // With the fix, the force reaches zero despite Soft Lock wanting to push.
        ASSERT_NEAR(slewed_force, 0.0, 0.01);
    }
}

AutoRegister reg_issue_281_spikes("Issue #281 Transition Spikes", "Issue281", {"Physics", "Regression"}, test_issue_281_transition_smoothing);

} // namespace FFBEngineTests
