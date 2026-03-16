#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_safety_slew_spikes, "Safety") {
    FFBEngine engine;
    InitializeEngine(engine);
    // Explicitly set duration for test, as default is now 0.0 (Issue #350)
    engine.m_safety.m_safety_window_duration = 1.0f;
    
    // Setup for spike detection
    engine.m_safety.m_immediate_spike_threshold = 1000.0f;
    engine.m_safety.m_spike_detection_threshold = 500.0f;
    engine.m_safety.m_safety_window_duration = 1.0f;
    engine.m_safety.m_safety_slew_full_scale_time_s = 0.5f;

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mElapsedTime = 10.0;
    
    // 1. Massive Spike
    // target_force = 1.0, m_last_output_force = 0.0, dt = 0.0001
    // requested_rate = 1.0 / 0.0001 = 10000.0 > 1000.0
    double output = engine.m_safety.ApplySafetySlew(1.0, 0.0001, false);
    
    ASSERT_GT(FFBEngineTestAccess::GetSafety(engine).safety_timer, 0.0);
    ASSERT_EQ_STR(FFBEngineTestAccess::GetSafety(engine).last_reset_reason, "Massive Spike");

    // 2. Reset safety for next sub-test
    FFBEngineTestAccess::ResetSafety(engine);
    // Note: ResetSafety in common.h already zeroes timer
    
    // 3. High Spike (sustained 5 frames)
    // requested_rate = 0.06 / 0.0001 = 600.0 (between 500 and 1000)
    for (int i = 0; i < 4; i++) {
        engine.m_safety.ApplySafetySlew(0.06, 0.0, 0.0001, false, 0.0);
        ASSERT_EQ(FFBEngineTestAccess::GetSafety(engine).safety_timer, 0.0);
        ASSERT_EQ(FFBEngineTestAccess::GetSafety(engine).spike_counter, i + 1);
    }
    
    // 5th frame triggers it
    engine.m_safety.ApplySafetySlew(0.06, 0.0, 0.0001, false, 0.0);
    ASSERT_GT(FFBEngineTestAccess::GetSafety(engine).safety_timer, 0.0);
    ASSERT_EQ_STR(FFBEngineTestAccess::GetSafety(engine).last_reset_reason, "High Spike");
    ASSERT_EQ(FFBEngineTestAccess::GetSafety(engine).spike_counter, 0);

    // 4. Spike counter decrement
    FFBEngineTestAccess::ResetSafety(engine);
    FFBEngineTestAccess::GetTorqueStats(engine).Update(0.0); // Not strictly needed but for safety
    // Manually set counter since we don't have a setter
    // We can't set it easily without adding another accessor, but we can trigger it
    for(int i=0; i<3; i++) engine.m_safety.ApplySafetySlew(0.06, 0.0, 0.0001, false, 0.0);
    ASSERT_EQ(FFBEngineTestAccess::GetSafety(engine).spike_counter, 3);
    engine.m_safety.ApplySafetySlew(0.01, 0.0, 0.0001, false, 0.0); // Rate = 100 < 500
    ASSERT_EQ(FFBEngineTestAccess::GetSafety(engine).spike_counter, 2);
}

TEST_CASE(test_safety_window_trigger_spam, "Safety") {
    FFBEngine engine;
    InitializeEngine(engine);
    // Explicitly set duration for test, as default is now 0.0 (Issue #350)
    engine.m_safety.m_safety_window_duration = 2.0f;
    
    // Use m_working_info to set mElapsedTime
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mElapsedTime = 10.0;
    engine.calculate_force(&data, "GT3", "911", 0.0f, true);

    // Trigger once
    engine.m_safety.TriggerSafetyWindow("Test Reason");
    double first_timer = FFBEngineTestAccess::GetSafety(engine).safety_timer;
    ASSERT_GT(first_timer, 0.0);
    ASSERT_EQ_STR(FFBEngineTestAccess::GetSafety(engine).last_reset_reason, "Test Reason");

    // Trigger again with same reason immediately - should NOT update log time (internal logic hit)
    engine.m_safety.TriggerSafetyWindow("Test Reason");
    
    // Trigger with different reason - should update reason
    engine.m_safety.TriggerSafetyWindow("New Reason");
    ASSERT_EQ_STR(FFBEngineTestAccess::GetSafety(engine).last_reset_reason, "New Reason");

    // Trigger after 1.1 seconds with same reason - should update (spam prevention time passed)
    data.mElapsedTime = 11.2;
    engine.calculate_force(&data, "GT3", "911", 0.0f, true);
    engine.m_safety.TriggerSafetyWindow("New Reason");
}

TEST_CASE(test_ffb_allowed_dq_and_garage, "Safety") {
    FFBEngine engine;
    VehicleScoringInfoV01 scoring = {};
    scoring.mIsPlayer = true;
    scoring.mControl = 0; // PLAYER
    scoring.mFinishStatus = 0; // NONE
    scoring.mInGarageStall = false;

    ASSERT_TRUE(engine.m_safety.IsFFBAllowed(scoring, 0));

    // DQ status
    scoring.mFinishStatus = 3; // DQ
    ASSERT_FALSE(engine.m_safety.IsFFBAllowed(scoring, 0));

    // Garage status
    scoring.mFinishStatus = 0;
    scoring.mInGarageStall = true;
    ASSERT_FALSE(engine.m_safety.IsFFBAllowed(scoring, 0));
    
    // Non-player control
    scoring.mInGarageStall = false;
    scoring.mControl = 1; // AI
    ASSERT_FALSE(engine.m_safety.IsFFBAllowed(scoring, 0));
}

TEST_CASE(test_apply_safety_slew_non_finite, "Safety") {
    FFBEngine engine;
    // dt = 0.01
    double out = engine.m_safety.ApplySafetySlew(std::nan(""), 0.01, false);
    ASSERT_EQ(out, 0.0);
    
    out = engine.m_safety.ApplySafetySlew(std::numeric_limits<double>::infinity(), 0.01, false);
    ASSERT_EQ(out, 0.0);
}

TEST_CASE(test_signal_conditioning_zero_crossings, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    FFBCalculationContext ctx;
    ctx.dt = 0.01;
    ctx.car_speed = 20.0;
    
    // We need to trigger ac_torque zero crossings
    // ac_torque = game_force_proc - m_torque_ac_smoothed
    // m_torque_ac_smoothed has a HPF filter (tau = 0.5s)
    
    // 1. Positive cross
    engine.m_prev_ac_torque = -1.0;
    data.mElapsedTime = 1.0;
    FFBEngineTestAccess::CallApplySignalConditioning(engine, 10.0, &data, ctx);
    // ac_torque will be positive
    ASSERT_GT(engine.m_prev_ac_torque, 0.0);
    
    // 2. Negative cross within valid period (0.005 < 0.1 < 0.5)
    engine.m_last_crossing_time = 0.9; 
    data.mElapsedTime = 1.0;
    engine.m_prev_ac_torque = 1.0;
    FFBEngineTestAccess::CallApplySignalConditioning(engine, -10.0, &data, ctx);
    ASSERT_LT(engine.m_prev_ac_torque, 0.0);
    ASSERT_GT(engine.m_debug_freq, 0.0);
}

TEST_CASE(test_notch_filter_transitions, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    FFBCalculationContext ctx;
    ctx.dt = 0.01;
    ctx.car_speed = 20.0;
    
    // Dynamic Notch
    FFBEngineTestAccess::SetFlatspotSuppression(engine, true);
    FFBEngineTestAccess::SetFlatspotStrength(engine, 0.5f);
    // Low wheel_freq triggers reset
    ctx.car_speed = 0.1;
    FFBEngineTestAccess::CallApplySignalConditioning(engine, 1.0, &data, ctx);
    
    // Static Notch Disabled triggers reset
    engine.m_static_notch_enabled = false;
    FFBEngineTestAccess::CallApplySignalConditioning(engine, 1.0, &data, ctx);
}

TEST_CASE(test_upsampling_frame_logic, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mElapsedTime = 1.0;
    
    // 1. Legacy mode (override_dt <= 0)
    engine.calculate_force(&data, "GT3", "911", 0.0f, true, -1.0);
    
    // 2. Upsampling mode (override_dt > 0)
    // New frame
    data.mElapsedTime = 1.1;
    engine.calculate_force(&data, "GT3", "911", 0.0f, true, 0.0025);
    
    // Same frame (upsampling call)
    engine.calculate_force(&data, "GT3", "911", 0.0f, true, 0.0025);
}

TEST_CASE(test_derived_acceleration_logic, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mElapsedTime = 1.0;
    data.mDeltaTime = 0.01f;
    data.mLocalVel.y = 10.0;
    data.mLocalVel.z = 20.0;
    
    // Frame 1: Seed
    FFBEngineTestAccess::SetDerivativesSeeded(engine, false);
    engine.calculate_force(&data, "GT3", "911", 0.0f, true, 0.0);
    
    // Frame 2: Derive
    data.mElapsedTime = 1.01;
    data.mLocalVel.y = 11.0; // Delta = 1.0 over 0.01 = 100 m/s2
    engine.calculate_force(&data, "GT3", "911", 0.0f, true, 0.0);
    
    ASSERT_NEAR(engine.m_derived_accel_y_100hz, 100.0, 0.1);
}

TEST_CASE(test_missing_telemetry_extended, "Diagnostics") {
    FFBEngine engine;
    InitializeEngine(engine);
    
    TelemInfoV01 data = CreateBasicTestTelemetry(30.0, 0.0);
    data.mLocalVel.z = 30.0;
    data.mLocalAccel.x = 2.0;
    
    // Missing Susp Deflection
    data.mWheel[0].mSuspensionDeflection = 0.0;
    data.mWheel[1].mSuspensionDeflection = 0.0;
    
    // Missing Lat Force
    data.mWheel[0].mLateralForce = 0.0;
    data.mWheel[1].mLateralForce = 0.0;
    data.mWheel[2].mLateralForce = 0.0;
    data.mWheel[3].mLateralForce = 0.0;

    // Missing Vert Deflection
    data.mWheel[0].mVerticalTireDeflection = 0.0;
    data.mWheel[1].mVerticalTireDeflection = 0.0;

    for (int i = 0; i < 60; i++) {
        engine.calculate_force(&data, "GT3", "911");
    }
}

TEST_CASE(test_calculate_force_transitions, "Safety") {
    FFBEngine engine;
    InitializeEngine(engine);
    // Explicitly set duration for test, as default is now 0.0 (Issue #350)
    engine.m_safety.m_safety_window_duration = 2.0f;
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mElapsedTime = 1.0;
    
    // 1. FFB Unmuted
    engine.m_safety.SetLastAllowed(false);
    engine.calculate_force(&data, "GT3", "911", 0.0f, true);
    ASSERT_TRUE(FFBEngineTestAccess::GetSafety(engine).last_allowed);
    ASSERT_GT(FFBEngineTestAccess::GetSafety(engine).safety_timer, 0.0);
    ASSERT_EQ_STR(FFBEngineTestAccess::GetSafety(engine).last_reset_reason, "FFB Unmuted");

    // 2. FFB Muted
    engine.calculate_force(&data, "GT3", "911", 0.0f, false);
    ASSERT_FALSE(FFBEngineTestAccess::GetSafety(engine).last_allowed);
    
    // 3. Control Transition
    FFBEngineTestAccess::ResetSafety(engine);
    // Seed first frame so last_mControl != -2
    engine.calculate_force(&data, "GT3", "911", 0.0f, true, 0.0, 0 /* PLAYER */);
    
    // Now transition 0 -> 1 (AI)
    engine.calculate_force(&data, "GT3", "911", 0.0f, true, 0.0, 1 /* AI */);
    ASSERT_EQ(FFBEngineTestAccess::GetSafety(engine).last_mControl, 1);
    ASSERT_GT(FFBEngineTestAccess::GetSafety(engine).safety_timer, 0.0);
    ASSERT_EQ_STR(FFBEngineTestAccess::GetSafety(engine).last_reset_reason, "Control Transition");

    // 4. Muted Transition (Reset filters)
    FFBEngineTestAccess::SetWasAllowed(engine, true);
    engine.m_accel_x_smoothed = 10.0;
    engine.calculate_force(&data, "GT3", "911", 0.0f, false);
    ASSERT_FALSE(FFBEngineTestAccess::GetWasAllowed(engine));
    ASSERT_EQ(engine.m_accel_x_smoothed, 0.0);
}

TEST_CASE(test_dynamic_normalization_branches, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_dynamic_normalization_enabled = true;
    engine.m_torque_source = 0; // Shaft
    engine.m_target_rim_nm = 20.0f;
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mSteeringShaftTorque = 30.0;
    
    // 1. Clean state learning
    data.mLocalAccel.x = 0.5; // Low lat G
    engine.calculate_force(&data);
    data.mElapsedTime += 0.01;
    engine.calculate_force(&data);
    ASSERT_GT(FFBEngineTestAccess::GetSessionPeakTorque(engine), 20.0);

    // 2. Contextual Spike (should NOT update peak)
    double peak_before = FFBEngineTestAccess::GetSessionPeakTorque(engine);
    data.mSteeringShaftTorque = peak_before * 10.0; // Massive jump
    engine.calculate_force(&data);
    ASSERT_NEAR(FFBEngineTestAccess::GetSessionPeakTorque(engine), peak_before, 0.1);

    // 3. Torque Source 1 (In-Game)
    FFBEngineTestAccess::SetTorqueSource(engine, 1);
    engine.m_wheelbase_max_nm = 25.0f;
    engine.calculate_force(&data, "GT3", "911", 0.5f); // 0.5 * 25 = 12.5Nm
    // target_structural_mult should be 1/25 = 0.04
    ASSERT_NEAR(FFBEngineTestAccess::GetSmoothedStructuralMult(engine), 0.04, 0.01);
}

TEST_CASE(test_yaw_kicks_branches, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_unloaded_yaw_gain = 1.0f;
    engine.m_power_yaw_gain = 1.0f;
    engine.m_sop_scale = 1.0f;
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    FFBCalculationContext ctx;
    ctx.dt = 0.0025;
    ctx.car_speed = 20.0;
    ctx.speed_gate = 1.0;
    
    // 1. Unloaded Yaw Kick
    FFBEngineTestAccess::SetStaticRearLoad(engine, 1000.0);
    data.mWheel[2].mTireLoad = 200.0; // Massive load drop (rear)
    data.mLocalRot.y = 1.0; // Yaw rate
    // Trigger multiple frames to seed and then create derivative
    FFBEngineTestAccess::CallCalculateSopLateral(engine, &data, ctx);
    data.mLocalRot.y = 2.0; // Positive acceleration
    FFBEngineTestAccess::CallCalculateSopLateral(engine, &data, ctx);
    
    ASSERT_NE(ctx.yaw_force, 0.0);

    // 2. Power Yaw Kick
    data.mWheel[2].mTireLoad = 1000.0; // Restore load
    data.mUnfilteredThrottle = 1.0f;
    data.mWheel[2].mRotation = 500.0; // Spin!
    data.mLocalRot.y = 1.0;
    FFBEngineTestAccess::CallCalculateSopLateral(engine, &data, ctx);
    data.mLocalRot.y = 2.0;
    FFBEngineTestAccess::CallCalculateSopLateral(engine, &data, ctx);
    
    ASSERT_NE(ctx.yaw_force, 0.0);
}

TEST_CASE(test_lockup_predictive_branches, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);
    FFBEngineTestAccess::SetLockupEnabled(engine, true);
    engine.m_lockup_prediction_sens = 10.0f;
    
    TelemInfoV01 data = CreateBasicTestTelemetry(30.0, 0.0);
    data.mUnfilteredBrake = 1.0f;
    data.mWheel[0].mTireLoad = 2000.0; // Grounded
    data.mWheel[0].mRotation = 100.0;
    data.mWheel[0].mLongitudinalPatchVel = 0.0; // No slip yet
    
    FFBCalculationContext ctx;
    ctx.dt = 0.0025;
    ctx.car_speed = 30.0;
    ctx.brake_load_factor = 1.0;
    ctx.speed_gate = 1.0;
    
    // Seed rotation
    FFBEngineTestAccess::CallCalculateLockup_Vibration(engine, &data, ctx);
    
    // Frame 2: Rapid deceleration AND slip
    data.mWheel[0].mRotation = 50.0; // Delta = -50 over 0.0025 = -20000 rad/s2
    data.mWheel[0].mLongitudinalPatchVel = 10.0; // 33% slip (10/30)
    FFBEngineTestAccess::CallCalculateLockup_Vibration(engine, &data, ctx);
    
    ASSERT_NE(ctx.lockup_rumble, 0.0);
}

TEST_CASE(test_bottoming_branches, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);
    FFBEngineTestAccess::SetBottomingEnabled(engine, true);
    FFBEngineTestAccess::SetBottomingGain(engine, 1.0f);
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    FFBCalculationContext ctx;
    ctx.dt = 0.0025;
    ctx.car_speed = 20.0;
    ctx.speed_gate = 1.0;
    
    // 1. Method 1: Impulse
    FFBEngineTestAccess::SetBottomingMethod(engine, 1);
    data.mWheel[0].mSuspForce = 0.0;
    FFBEngineTestAccess::CallCalculateSuspensionBottoming(engine, &data, ctx);
    
    data.mWheel[0].mSuspForce = 20000.0; // Massive impulse
    FFBEngineTestAccess::CallCalculateSuspensionBottoming(engine, &data, ctx);
    ASSERT_NE(ctx.bottoming_crunch, 0.0);
    
    // 2. Safety Trigger: Raw Load Peak
    ctx.bottoming_crunch = 0.0;
    FFBEngineTestAccess::SetBottomingMethod(engine, 0);
    data.mWheel[0].mRideHeight = 1.0; // No trigger from Method 0
    FFBEngineTestAccess::SetStaticFrontLoad(engine, 1000.0);
    data.mWheel[0].mTireLoad = 10000.0; // 10x static load
    FFBEngineTestAccess::CallCalculateSuspensionBottoming(engine, &data, ctx);
    ASSERT_NE(ctx.bottoming_crunch, 0.0);
}

TEST_CASE(test_rest_api_fallback_and_range_warning, "Diagnostics") {
    FFBEngine engine;
    InitializeEngine(engine);
    FFBEngineTestAccess::SetRestApiEnabled(engine, true);
    FFBEngineTestAccess::SetRestApiPort(engine, 8080);
    
    TelemInfoV01 data = CreateBasicTestTelemetry(30.0, 0.0);
    data.mPhysicalSteeringWheelRange = 0.0f; // Invalid range
    
    // Seed the engine
    engine.calculate_force(&data, "GT3", "911");
    
    // Trigger invalid range warning
    // This will hit the branch: if (seeded && m_rest_api_enabled && data->mPhysicalSteeringWheelRange <= 0.0f)
    // and if (!m_warned_invalid_range)
    engine.calculate_force(&data, "GT3", "911");
}

} // namespace FFBEngineTests
