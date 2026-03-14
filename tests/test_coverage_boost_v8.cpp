#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_ffb_engine_control_transition_extended, "Safety") {
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mElapsedTime = 1.0;

    // 1. First frame (m_safety.m_safety.last_mControl = -2)
    // Should NOT trigger "Control Transition"
    engine.calculate_force(&data, "GT3", "911", 0.0f, true, 0.0, 0 /* PLAYER */);
    ASSERT_EQ(FFBEngineTestAccess::GetSafety(engine).last_mControl, 0);
    ASSERT_EQ(FFBEngineTestAccess::GetSafety(engine).safety_timer, 0.0);

    // 2. Second frame: Transition 0 -> 1 (AI)
    // Should trigger "Control Transition"
    data.mElapsedTime = 1.1;
    engine.calculate_force(&data, "GT3", "911", 0.0f, true, 0.0, 1 /* AI */);
    ASSERT_EQ(FFBEngineTestAccess::GetSafety(engine).last_mControl, 1);
    ASSERT_GT(FFBEngineTestAccess::GetSafety(engine).safety_timer, 0.0);
    ASSERT_EQ_STR(FFBEngineTestAccess::GetSafety(engine).last_reset_reason, "Control Transition");
}

TEST_CASE(test_ffb_engine_mute_initialization_branch, "Safety") {
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    
    // mElapsedTime = 0 triggers "Initialization" reason in LogFile
    data.mElapsedTime = 0.0;
    engine.m_safety.SetLastAllowed(true);
    engine.calculate_force(&data, "GT3", "911", 0.0f, false);
    ASSERT_FALSE(FFBEngineTestAccess::GetSafety(engine).last_allowed);
}

TEST_CASE(test_ffb_engine_invalid_dt_warning, "Diagnostics") {
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    
    // Trigger m_warned_dt branch
    data.mDeltaTime = 0.0f; 
    engine.calculate_force(&data, "GT3", "911", 0.0f, true);
    ASSERT_TRUE(FFBEngineTestAccess::HasWarnings(engine));
    
    // Subsequent call should not log again (m_warned_dt is true)
    engine.calculate_force(&data, "GT3", "911", 0.0f, true);
}

TEST_CASE(test_ffb_engine_invalid_range_warning_multiple, "Diagnostics") {
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mPhysicalSteeringWheelRange = 0.0f;
    
    // 1. Trigger m_warned_invalid_range
    engine.calculate_force(&data, "GT3", "911", 0.0f, true);
    
    // 2. Subsequent call with same car - should not log
    engine.calculate_force(&data, "GT3", "911", 0.0f, true);
    
    // 3. Different car, but already warned - should still not log (it's a global flag in engine)
    engine.calculate_force(&data, "GTE", "488", 0.0f, true);
}

TEST_CASE(test_ffb_engine_contextual_spike_rejection, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_dynamic_normalization_enabled = true;
    engine.m_torque_source = 0; // Shaft
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    
    // 1. Seed rolling average
    data.mSteeringShaftTorque = 10.0;
    for(int i=0; i<10; i++) engine.calculate_force(&data);
    
    double avg_before = FFBEngineTestAccess::GetRollingAverageTorque(engine);
    ASSERT_GT(avg_before, 0.0);

    // 2. Trigger contextual spike (current > avg * 3 AND current > 15)
    data.mSteeringShaftTorque = 50.0; // 50 > 10*3 and 50 > 15
    double peak_before = FFBEngineTestAccess::GetSessionPeakTorque(engine);
    engine.calculate_force(&data);
    
    // Should NOT update session peak because it's a spike
    ASSERT_NEAR(FFBEngineTestAccess::GetSessionPeakTorque(engine), peak_before, 0.01);
}

TEST_CASE(test_ffb_engine_clean_state_false_branches, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_dynamic_normalization_enabled = true;
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    
    // 1. High Lat G (is_clean_state = false)
    // Threshold is 8.0G. 100.0 m/s2 / 9.81 approx 10.2G > 8.0G
    data.mLocalAccel.x = 100.0; 
    data.mSteeringShaftTorque = 30.0;
    double peak_before = FFBEngineTestAccess::GetSessionPeakTorque(engine); // 20.0
    engine.calculate_force(&data);
    // Should NOT update session peak because lat_g is too high
    ASSERT_NEAR(FFBEngineTestAccess::GetSessionPeakTorque(engine), peak_before, 0.01);

    // 2. High Torque Slew (is_clean_state = false)
    // Reset state for clean sub-test
    InitializeEngine(engine);
    engine.m_dynamic_normalization_enabled = true;
    data.mLocalAccel.x = 0.0;
    FFBEngineTestAccess::SetLastRawTorque(engine, 0.0);
    data.mSteeringShaftTorque = 50.0; // Slew = 50/0.01 = 5000 > 1000
    peak_before = FFBEngineTestAccess::GetSessionPeakTorque(engine);
    engine.calculate_force(&data);
    // Should NOT update session peak because slew is too high
    ASSERT_NEAR(FFBEngineTestAccess::GetSessionPeakTorque(engine), peak_before, 0.01);
}

TEST_CASE(test_ffb_engine_dynamic_normalization_decay, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_dynamic_normalization_enabled = true;
    FFBEngineTestAccess::SetSessionPeakTorque(engine, 50.0);
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mSteeringShaftTorque = 10.0; // < peak, should decay
    
    engine.calculate_force(&data);
    ASSERT_LT(FFBEngineTestAccess::GetSessionPeakTorque(engine), 50.0);
}

TEST_CASE(test_ffb_engine_missing_load_fallback_frames, "Diagnostics") {
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    
    // 1. Missing load
    data.mWheel[0].mTireLoad = 0.0;
    data.mWheel[1].mTireLoad = 0.0;
    
    // Need to hit threshold (50 frames)
    for(int i=0; i<60; i++) {
        engine.calculate_force(&data);
    }
    
    // Should have triggered fallback and warning
    ASSERT_TRUE(FFBEngineTestAccess::HasWarnings(engine));
}

TEST_CASE(test_ffb_engine_get_debug_batch_branches, "Diagnostics") {
    FFBEngine engine;
    FFBSnapshot s = {};
    FFBEngineTestAccess::AddSnapshot(engine, s);
    
    auto batch = engine.GetDebugBatch();
    ASSERT_EQ(batch.size(), 1);
    
    auto empty_batch = engine.GetDebugBatch();
    ASSERT_EQ(empty_batch.size(), 0);
}

TEST_CASE(test_ffb_engine_soft_knee_compression_branches, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);
    
    // T = 1.5, W = 0.5, R = 4.0
    // lower_bound = 1.5 - 0.25 = 1.25
    // upper_bound = 1.5 + 0.25 = 1.75
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    FFBCalculationContext ctx;
    ctx.dt = 0.01;
    
    // 1. Above upper bound (x = 2.0)
    FFBEngineTestAccess::SetStaticFrontLoad(engine, 1000.0);
    ctx.avg_front_load = 2000.0; 
    // This part of calculate_force is hard to test in isolation without calling calculate_force
    // But calculate_force is too big. I'll use calculate_force with specific loads.
    
    data.mWheel[0].mTireLoad = 2000.0;
    data.mWheel[1].mTireLoad = 2000.0;
    engine.calculate_force(&data);
    double mult_high = FFBEngineTestAccess::GetSmoothedVibrationMult(engine);
    
    // 2. In knee region (x = 1.5)
    FFBEngineTestAccess::SetSmoothedVibrationMult(engine, 1.0);
    data.mWheel[0].mTireLoad = 1500.0;
    data.mWheel[1].mTireLoad = 1500.0;
    engine.calculate_force(&data);
    double mult_knee = FFBEngineTestAccess::GetSmoothedVibrationMult(engine);
    
    ASSERT_NE(mult_high, mult_knee);
}

TEST_CASE(test_ffb_engine_long_load_transform_branches, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_long_load_effect = 1.0f;
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    
    // QUADRATIC
    engine.m_long_load_transform = LoadTransform::QUADRATIC;
    engine.calculate_force(&data);
    
    // CUBIC
    engine.m_long_load_transform = LoadTransform::CUBIC;
    engine.calculate_force(&data);
    
    // HERMITE
    engine.m_long_load_transform = LoadTransform::HERMITE;
    engine.calculate_force(&data);
}

TEST_CASE(test_ffb_engine_torque_passthrough_branch, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_torque_passthrough = true;
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    engine.calculate_force(&data);
    // Should hit branches at L656 and L694
}

TEST_CASE(test_ffb_engine_safety_seeded_branch, "Safety") {
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    
    // Trigger safety window
    engine.m_safety.TriggerSafetyWindow("Test");
    ASSERT_TRUE(FFBEngineTestAccess::GetSafety(engine).safety_timer > 0.0);
    ASSERT_FALSE(FFBEngineTestAccess::GetSafety(engine).safety_is_seeded);
    
    // Frame 1: Seed
    engine.calculate_force(&data);
    ASSERT_TRUE(FFBEngineTestAccess::GetSafety(engine).safety_is_seeded);
    
    // Frame 2: Apply smoothing
    engine.calculate_force(&data);
}

TEST_CASE(test_ffb_engine_significant_soft_lock_allowed_branch, "Safety") {
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(0.0, 0.0);
    data.mUnfilteredSteering = 1.1; // Beyond limit
    
    // Case 1: Allowed = false, but significant soft lock
    // We need to make sure calculate_force is called with allowed=false
    // but soft_lock_force > 0.5
    engine.m_soft_lock_enabled = true;
    engine.m_soft_lock_stiffness = 20.0f;
    engine.m_wheelbase_max_nm = 20.0f; // Spring force will be high
    engine.m_min_force = 0.1f;
    
    // Call with allowed = false (AI/Garage)
    engine.calculate_force(&data, "GT3", "911", 0.0f, false);
}

TEST_CASE(test_ffb_engine_invert_force_branch, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_invert_force = true;
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    engine.calculate_force(&data);
}

TEST_CASE(test_ffb_engine_full_tock_detection, "Safety") {
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mUnfilteredSteering = 1.0; // Pinned
    data.mSteeringShaftTorque = 100.0; // High force
    
    // Need to pin for 1 second at 100Hz = 100 frames
    for(int i=0; i<110; i++) {
        data.mElapsedTime += 0.01;
        engine.calculate_force(&data);
    }
}

TEST_CASE(test_ffb_engine_signal_conditioning_notch_low_freq, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);
    FFBEngineTestAccess::SetFlatspotSuppression(engine, true);
    
    TelemInfoV01 data = CreateBasicTestTelemetry(0.1, 0.0); // Very low speed -> low wheel_freq
    FFBCalculationContext ctx;
    ctx.dt = 0.01;
    ctx.car_speed = 0.1;
    
    // wheel_freq = 0.1 / (2*PI*0.3) approx 0.05 < 1.0
    FFBEngineTestAccess::CallApplySignalConditioning(engine, 1.0, &data, ctx);
    // Should hit the 'else' branch and Reset() notch filter
}

} // namespace FFBEngineTests
