#include "test_ffb_common.h"
#include <iostream>

namespace FFBEngineTests {

/**
 * Test for Issue #314: Safety Fixes for FFB Spikes (2)
 */
void test_immediate_spike_detection() {
    std::cout << "\nTest: Issue #314 Immediate Spike Detection" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_safety.m_safety_window_duration = 2.0f;
    FFBEngineTestAccess::ResetSafety(engine);

    // Large jump: 0 to 5.0 in 2.5ms = 2000 units/s
    // Exceeds proposed IMMEDIATE_SPIKE_THRESHOLD (1500)
    FFBEngineTestAccess::SetLastOutputForce(engine, 0.0);

    // First call should trigger safety immediately
    engine.m_safety.ApplySafetySlew(5.0, 0.0025, false);

    // Should have triggered safety window immediately
    ASSERT_GT(engine.m_safety.safety_timer, 0.0);
}

void test_safety_timer_reset() {
    std::cout << "\nTest: Issue #314 Safety Timer Reset" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_safety.m_safety_window_duration = 2.0f;
    FFBEngineTestAccess::ResetSafety(engine);

    // Initial trigger
    engine.m_safety.TriggerSafetyWindow("Reason 1");
    ASSERT_GT(engine.m_safety.safety_timer, 1.9);

    // Simulate time passing (1s)
    engine.m_safety.safety_timer = 1.0;

    // Trigger again
    engine.m_safety.TriggerSafetyWindow("Reason 2");

    // Should be reset to full duration
    ASSERT_GT(engine.m_safety.safety_timer, 1.9);
}

void test_safety_exit_state() {
    std::cout << "\nTest: Issue #314 Safety Exit State" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_safety.m_safety_window_duration = 2.0f;
    FFBEngineTestAccess::ResetSafety(engine);

    // Trigger safety
    engine.m_safety.TriggerSafetyWindow("Test Exit");
    ASSERT_GT(engine.m_safety.safety_timer, 0.0);

    // Simulate time passing beyond duration (2s)
    TelemInfoV01 data = CreateBasicTestTelemetry(10.0, 0.0);
    data.mElapsedTime = 100.0;

    // Warmup
    FFBEngineTestAccess::SetDerivativesSeeded(engine, false);
    engine.calculate_force(&data, "GT3", "911 GT3", 0.0f, true, 0.0025, 0);

    // Run for 2.1s (840 frames @ 0.0025s)
    for (int i = 0; i < 840; i++) {
        data.mElapsedTime += 0.0025;
        engine.calculate_force(&data, "GT3", "911 GT3", 0.0f, true, 0.0025, 0);
    }

    // Timer should be zero
    ASSERT_EQ(engine.m_safety.safety_timer, 0.0);
}

void test_safety_restrictiveness() {
    std::cout << "\nTest: Issue #314 Safety Restrictiveness" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_safety.m_safety_window_duration = 2.0f;
    engine.m_gain = 1.0f;
    engine.m_wheelbase_max_nm = 10.0f;
    engine.m_target_rim_nm = 10.0f;
    FFBEngineTestAccess::SetSessionPeakTorque(engine, 10.0);
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0/10.0);

    TelemInfoV01 data = CreateBasicTestTelemetry(10.0, 0.0);
    data.mSteeringShaftTorque = 5.0; // 5Nm -> 0.5 normalized force

    // Trigger safety window
    engine.m_safety.TriggerSafetyWindow("Test Restrictiveness");

    // Issue #397: Flush the 10ms transient ramp
    // Seed and let settle
    PumpEngineTime(engine, data, 1.0);
    double safety_force = engine.GetDebugBatch().back().total_output;

    std::cout << "  Safety Force: " << safety_force << std::endl;

    // Should be near 0.15 (0.5 * 0.3)
    // Issue #397: Holt-Winters (Shaft Torque) also has settled state slightly different
    ASSERT_NEAR(std::abs(safety_force), 0.15, 0.05);

    // Test Slew Rate Limitation during safety (capped at 1.0)
    FFBEngineTestAccess::SetLastOutputForce(engine, 0.0);
    // Request a large jump (from 0 to 1.0)
    double slewed = engine.m_safety.ApplySafetySlew(1.0, 0.0025, false);
    // Max slew in safety window is 1.0 unit/s. In 2.5ms, max change is 1.0 * 0.0025 = 0.0025
    ASSERT_NEAR(slewed, 0.0025, 0.001);
}

void test_safety_log_throttling() {
    std::cout << "\nTest: Issue #314 Safety Log Throttling" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);
    FFBEngineTestAccess::ResetSafety(engine);

    // Initial trigger at t=0
    engine.m_working_info.mElapsedTime = 100.0;
    engine.m_safety.TriggerSafetyWindow("Test Reason");
    ASSERT_EQ(engine.m_safety.last_reset_log_time, 100.0); // Updated on first entry now
    ASSERT_EQ(std::string(engine.m_safety.last_reset_reason), std::string("Test Reason"));

    // Reset with SAME reason at t=100.1
    engine.m_working_info.mElapsedTime = 100.1;
    engine.m_safety.TriggerSafetyWindow("Test Reason");
    ASSERT_EQ(engine.m_safety.last_reset_log_time, 100.0); // Should NOT have logged (throttled)

    // Reset with DIFFERENT reason at t=0.2
    engine.m_working_info.mElapsedTime = 100.2;
    engine.m_safety.TriggerSafetyWindow("New Reason");
    ASSERT_EQ(engine.m_safety.last_reset_log_time, 100.2); // Should HAVE logged (reason changed)
    ASSERT_EQ(std::string(engine.m_safety.last_reset_reason), std::string("New Reason"));

    // Reset with SAME reason at t=1.3 (>1s later)
    engine.m_working_info.mElapsedTime = 101.3;
    engine.m_safety.TriggerSafetyWindow("New Reason");
    ASSERT_EQ(engine.m_safety.last_reset_log_time, 101.3); // Should HAVE logged (time passed)
}

void test_safety_reentry_smoothing() {
    std::cout << "\nTest: Issue #314 Safety Re-entry Smoothing Reset" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_safety.m_safety_window_duration = 2.0f;
    engine.m_gain = 1.0f;
    engine.m_wheelbase_max_nm = 10.0f;
    engine.m_target_rim_nm = 10.0f;
    FFBEngineTestAccess::SetSessionPeakTorque(engine, 10.0);
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0/10.0);

    TelemInfoV01 data = CreateBasicTestTelemetry(10.0, 0.0);

    // 1. First safety event at high force
    data.mSteeringShaftTorque = 10.0; // 1.0 normalized
    engine.m_safety.TriggerSafetyWindow("High Force Event");

    // Issue #397: Flush the 10ms transient ramp
    PumpEngineTime(engine, data, 1.0);
    double force1 = engine.GetDebugBatch().back().total_output;
    ASSERT_NEAR(std::abs(force1), 0.3, 0.1); // 1.0 * 0.3 gain

    // 2. Clear safety
    engine.m_safety.safety_timer = 0.0;

    // 3. Second safety event at low force
    data.mSteeringShaftTorque = 0.0;
    // Advance time significantly to ensure all filters and upsamplers settle to 0
    for(int i=0; i<500; i++) {
        data.mElapsedTime += 0.0025;
        engine.calculate_force(&data, "GT3", "911 GT3", 0.0f, true, 0.0025, 0);
    }

    ASSERT_EQ(engine.m_safety.safety_timer, 0.0);

    // Check that we are actually at 0 before triggering safety
    double force_before = engine.calculate_force(&data, "GT3", "911 GT3", 0.0f, true, 0.0025, 0);
    ASSERT_NEAR(force_before, 0.0, 0.001);

    engine.m_safety.TriggerSafetyWindow("Low Force Event");
    // safety_is_seeded is now false.

    // Warmup/Seed for this new context
    FFBEngineTestAccess::SetDerivativesSeeded(engine, false);
    engine.calculate_force(&data, "GT3", "911 GT3", 0.0f, true, 0.0025, 0);

    data.mElapsedTime += 0.0025;
    double force2 = engine.calculate_force(&data, "GT3", "911 GT3", 0.0f, true, 0.0025, 0);

    // If it didn't reset, it would be smoothed from 0.3 (old force1) towards 0.0
    // If it reset, it should be seeded from norm_force (which is 0.0)
    ASSERT_NEAR(force2, 0.0, 0.001);
}

AutoRegister reg_issue_314_safety_v2("Issue #314 Safety Fixes V2", "Issue314", {"Safety", "Logging"}, []() {
    test_immediate_spike_detection();
    test_safety_timer_reset();
    test_safety_exit_state();
    test_safety_restrictiveness();
    test_safety_log_throttling();
    test_safety_reentry_smoothing();
});

} // namespace FFBEngineTests
