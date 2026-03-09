#include "test_ffb_common.h"
#include <iostream>

namespace FFBEngineTests {

/**
 * Test for Issue #303: Safety Fixes for FFB Spikes
 */
void test_issue_303_safety_window_activation() {
    std::cout << "\nTest: Issue #303 Safety Window Activation" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);

    TelemInfoV01 data = CreateBasicTestTelemetry(10.0, 0.0);

    // Scenario 1: mControl Transition
    {
        // First frame: PLAYER
        engine.calculate_force(&data, "GT3", "911 GT3", 0.0f, true, 0.0025, static_cast<signed char>(ControlMode::PLAYER));
        ASSERT_EQ(engine.m_safety.safety_timer, 0.0);

        // Second frame: AI takeover
        engine.calculate_force(&data, "GT3", "911 GT3", 0.0f, true, 0.0025, static_cast<signed char>(ControlMode::AI));
        ASSERT_GT(engine.m_safety.safety_timer, 1.9); // Should be near 2.0
    }

    // Scenario 2: Manual Trigger (e.g. from Lost Frames)
    {
        engine.m_safety.safety_timer = 0.0;
        engine.TriggerSafetyWindow("Test Lost Frames");
        ASSERT_GT(engine.m_safety.safety_timer, 1.9);
    }
}

void test_issue_303_safety_mitigation() {
    std::cout << "\nTest: Issue #303 Safety Mitigation (Gain/Slew)" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_gain = 1.0f;
    engine.m_wheelbase_max_nm = 10.0f;
    engine.m_target_rim_nm = 10.0f;
    FFBEngineTestAccess::SetSessionPeakTorque(engine, 10.0);
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0/10.0);

    TelemInfoV01 data = CreateBasicTestTelemetry(10.0, 0.0);
    data.mSteeringShaftTorque = 5.0; // 5Nm

    // Normal output (no safety window)
    double normal_force = engine.calculate_force(&data, "GT3", "911 GT3", 0.0f, true, 0.0025, static_cast<signed char>(ControlMode::PLAYER));

    // Trigger safety window
    engine.TriggerSafetyWindow("Test");

    // Safety output (reduced gain)
    double safety_force = engine.calculate_force(&data, "GT3", "911 GT3", 0.0f, true, 0.0025, static_cast<signed char>(ControlMode::PLAYER));

    std::cout << "  Normal Force: " << normal_force << " | Safety Force: " << safety_force << std::endl;

    // Should be significantly lower (Gain reduction 0.3x)
    // On first frame of safety window, smoothing is seeded with reduced gain force.
    ASSERT_NEAR(std::abs(safety_force), std::abs(normal_force) * 0.3, 0.01);

    // Test Slew Rate Limitation during safety
    FFBEngineTestAccess::SetLastOutputForce(engine, 0.0);
    // Request a large jump (from 0 to 1.0)
    double slewed = engine.ApplySafetySlew(1.0, 0.0025, false);
    // Max slew in safety window is 100 units/s. In 2.5ms, max change is 100 * 0.0025 = 0.25
    ASSERT_NEAR(slewed, 0.25, 0.01);
}

void test_issue_303_spike_detection() {
    std::cout << "\nTest: Issue #303 Spike Detection" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);
    FFBEngineTestAccess::ResetSafety(engine);

    // Large jump: 0 to 10.0 in 2.5ms = 4000 units/s
    // Exceeds SPIKE_DETECTION_THRESHOLD (500)
    FFBEngineTestAccess::SetLastOutputForce(engine, 0.0);
    for (int i = 0; i < 10; i++) {
        // Must use large enough target to ensure delta/dt > threshold
        // And reset last output force to keep delta large
        FFBEngineTestAccess::SetLastOutputForce(engine, 0.0);
        engine.ApplySafetySlew(10.0, 0.0025, false);
    }

    // Should have triggered safety window
    ASSERT_GT(engine.m_safety.safety_timer, 0.0);
}

void test_issue_303_full_tock_timer() {
    std::cout << "\nTest: Issue #303 Full Tock Timer" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_gain = 1.0f;
    engine.m_target_rim_nm = 10.0f;
    engine.m_wheelbase_max_nm = 10.0f;
    FFBEngineTestAccess::SetSessionPeakTorque(engine, 10.0);
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0/10.0);

    TelemInfoV01 data = CreateBasicTestTelemetry(10.0, 0.0);
    data.mUnfilteredSteering = 1.0; // 100% lock
    data.mSteeringShaftTorque = 10.0; // Max torque
    data.mElapsedTime = 100.0; // Avoid initialization state

    // Run for 0.5s (200 frames @ 0.0025s)
    for (int i = 0; i < 200; i++) {
        data.mElapsedTime += 0.0025;
        engine.calculate_force(&data, "GT3", "911 GT3", 0.0f, true, 0.0025, 0);
    }
    ASSERT_NEAR(engine.m_safety.tock_timer, 0.5, 0.05);

    // Run for another 0.6s
    for (int i = 0; i < 240; i++) {
        data.mElapsedTime += 0.0025;
        engine.calculate_force(&data, "GT3", "911 GT3", 0.0f, true, 0.0025, 0);
    }

    // Timer should have reset after triggering log (which we can't easily check here but we check timer behavior)
    ASSERT_LT(engine.m_safety.tock_timer, 0.1);
}

AutoRegister reg_issue_303_safety("Issue #303 Safety Fixes", "Issue303", {"Safety", "Logging"}, []() {
    test_issue_303_safety_window_activation();
    test_issue_303_safety_mitigation();
    test_issue_303_spike_detection();
    test_issue_303_full_tock_timer();
});

} // namespace FFBEngineTests
