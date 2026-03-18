#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_sop_yaw_kick, "YawGyro") {
    std::cout << "\nTest: SoP Yaw Kick (v0.4.18 Smoothed)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine); // v0.5.12: Initialize with T300 defaults
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    
    // Setup
    engine.m_sop_yaw_gain = 1.0f;
    engine.m_yaw_accel_smoothing = 0.0225f; // v0.5.8: Explicitly set legacy value for test expectations
    engine.m_sop_effect = 0.0f; // Disable Base SoP
    engine.m_wheelbase_max_nm = 20.0f; engine.m_target_rim_nm = 20.0f; // Reference torque for normalization
    engine.m_gain = 1.0f;
    // Disable other effects
    engine.m_understeer_effect = 0.0f;
    engine.m_lockup_enabled = false;
    engine.m_spin_enabled = false;
    engine.m_slide_texture_enabled = false;
    engine.m_bottoming_enabled = false;
    engine.m_scrub_drag_gain = 0.0f;
    engine.m_rear_align_effect = 0.0f;
    engine.m_invert_force = false;
    
    // v0.4.18 UPDATE: With Low Pass Filter (alpha=0.1), the yaw acceleration
    // is smoothed over multiple frames. On the first frame with raw input = 1.0,
    // the smoothed value will be: 0.0 + 0.1 * (1.0 - 0.0) = 0.1
    // Formula: force = yaw_smoothed * gain * 5.0
    // First frame: 0.1 * 1.0 * 5.0 = 0.5 Nm
    // Norm: 0.5 / 20.0 = 0.025
    
    // Input: 1.0 rad/s^2 Yaw Accel (Derived from rate)
    // Seeding frame
    data.mLocalRot.y = 0.0;
    data.mElapsedTime = 1.0;
    engine.calculate_force(&data);
    
    // Test frame: rate moves to 1.0 * dt
    data.mLocalRot.y = 1.0 * 0.01;
    
    // Ensure no other inputs
    data.mSteeringShaftTorque = 0.0;
    data.mWheel[0].mRideHeight = 0.1;
    data.mWheel[1].mRideHeight = 0.1;
    data.mLocalVel.z = 20.0; // v0.4.42: Ensure speed > 5 m/s for Yaw Kick
    
    // Issue #397: Use PumpEngineTime
    PumpEngineTime(engine, data, 0.0125);
    double force = engine.GetDebugBatch().back().total_output;
    
    // v0.4.20 UPDATE: With force inversion, first frame should be ~-0.025 (10% of steady-state due to LPF)
    // The negative sign is correct - provides counter-steering cue
    if (force < -0.01) {
        std::cout << "[PASS] Yaw Kick first frame smoothed correctly (" << force << " < -0.01)." << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Yaw Kick first frame mismatch. Got " << force << " Expected ~-0.025.");
    }
}

TEST_CASE(test_gyro_damping, "YawGyro") {
    std::cout << "\nTest: Gyroscopic Damping (v0.4.17)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine); // v0.5.12: Initialize with T300 defaults
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    
    // Setup
    engine.m_gyro_gain = 1.0f;
    engine.m_gyro_smoothing = 0.1f;
    engine.m_wheelbase_max_nm = 20.0f; engine.m_target_rim_nm = 20.0f; // Reference torque for normalization
    engine.m_gain = 1.0f;
    
    // Disable other effects to isolate gyro damping
    engine.m_understeer_effect = 0.0f;
    engine.m_sop_effect = 0.0f;
    engine.m_lockup_enabled = false;
    engine.m_spin_enabled = false;
    engine.m_slide_texture_enabled = false;
    engine.m_bottoming_enabled = false;
    engine.m_scrub_drag_gain = 0.0f;
    engine.m_rear_align_effect = 0.0f;
    engine.m_sop_yaw_gain = 0.0f;
    
    // Setup test data
    data.mLocalVel.z = 50.0; // Car speed (50 m/s)
    data.mPhysicalSteeringWheelRange = 9.4247f; // 540 degrees
    data.mDeltaTime = 0.0025; // 400Hz (2.5ms)
    
    // Ensure no other inputs
    data.mSteeringShaftTorque = 0.0;
    data.mWheel[0].mRideHeight = 0.1;
    data.mWheel[1].mRideHeight = 0.1;
    data.mWheel[0].mGripFract = 1.0;
    data.mWheel[1].mGripFract = 1.0;
    
    // Issue #397: Use peak-finding loop to capture damping force during the 10ms ramp
    auto get_peak_gyro = [&](double steer_target, double speed) {
        data.mLocalVel.z = speed;
        data.mUnfilteredSteering = (float)steer_target;
        data.mElapsedTime += 0.01;

        double peak = 0.0;
        for(int i=0; i<4; i++) {
            engine.calculate_force(&data, nullptr, nullptr, 0.0f, true, 0.0025);
            auto b = engine.GetDebugBatch();
            if (!b.empty()) {
                double g = b.back().ffb_gyro_damping;
                if (std::abs(g) > std::abs(peak)) peak = g;
            }
        }
        return peak;
    };

    // Frame 1: Steering at 0.0 (Steady)
    data.mUnfilteredSteering = 0.0f;
    PumpEngineSteadyState(engine, data);
    
    // Frame 2: Steering moves to 0.1
    double gyro_force = get_peak_gyro(0.1, 50.0);
    
    if (gyro_force < -0.01) {
        std::cout << "[PASS] Gyro force opposes steering movement (negative: " << gyro_force << ")" << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Gyro force should be negative. Got: " << gyro_force);
    }
    
    // Test opposite direction
    data.mUnfilteredSteering = 0.1f;
    PumpEngineSteadyState(engine, data);
    double gyro_force_reverse = get_peak_gyro(0.0, 50.0);
    
    if (gyro_force_reverse > 0.01) {
        std::cout << "[PASS] Gyro force reverses with steering direction (positive: " << gyro_force_reverse << ")" << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Gyro force should be positive for reverse movement. Got: " << gyro_force_reverse);
    }
    
    // Test speed scaling
    data.mUnfilteredSteering = 0.0f;
    PumpEngineSteadyState(engine, data);
    double gyro_force_slow = get_peak_gyro(0.1, 5.0);
    
    if (std::abs(gyro_force_slow) < std::abs(gyro_force) * 0.6) {
        std::cout << "[PASS] Gyro force scales with speed (slow: " << gyro_force_slow << " vs fast: " << gyro_force << ")" << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Gyro force should be weaker at low speed. Slow: " << gyro_force_slow << " Fast: " << gyro_force);
    }
}

TEST_CASE(test_yaw_accel_smoothing, "YawGyro") {
    std::cout << "\nTest: Yaw Acceleration Smoothing (v0.4.18)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine); // v0.5.12: Initialize with T300 defaults
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    
    // Setup: Isolate Yaw Kick effect
    engine.m_sop_yaw_gain = 1.0f;
    engine.m_yaw_accel_smoothing = 0.0225f; // v0.5.8: Legacy value
    engine.m_sop_effect = 0.0f;
    engine.m_wheelbase_max_nm = 20.0f; engine.m_target_rim_nm = 20.0f;
    engine.m_gain = 1.0f;
    engine.m_understeer_effect = 0.0f;
    engine.m_lockup_enabled = false;
    engine.m_spin_enabled = false;
    engine.m_slide_texture_enabled = false;
    engine.m_bottoming_enabled = false;
    engine.m_scrub_drag_gain = 0.0f;
    engine.m_rear_align_effect = 0.0f;
    engine.m_gyro_gain = 0.0f;
    engine.m_invert_force = false;
    
    data.mWheel[0].mRideHeight = 0.1;
    data.mWheel[1].mRideHeight = 0.1;
    data.mSteeringShaftTorque = 0.0;
    data.mLocalVel.z = 20.0; // v0.4.42: Ensure speed > 5 m/s for Yaw Kick
    
    // Test 1: Verify smoothing reduces first-frame response
    // Raw input: 10.0 rad/s^2 (large spike)
    // Seeding
    data.mLocalRot.y = 0.0;
    data.mElapsedTime = 1.0;
    engine.calculate_force(&data);
    
    // Spike: rate moves to 10.0 * dt
    data.mLocalRot.y = 10.0 * 0.01;
    
    // Issue #397: Use PumpEngineTime
    PumpEngineTime(engine, data, 0.0125);
    double force_frame1 = engine.GetDebugBatch().back().total_output;
    
    // v0.4.20 UPDATE: With force inversion, values are negative
    // Without smoothing, this would be -10.0 * 1.0 * 5.0 / 20.0 = -2.5 (clamped to -1.0)
    // With smoothing (alpha=0.1), first frame = -0.25
    if (force_frame1 < -0.1) {
        std::cout << "[PASS] First frame smoothed correctly (" << force_frame1 << " < -0.1)." << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("First frame smoothing incorrect. Got " << force_frame1 << " Expected ~-0.25.");
    }
    
    // v0.4.20 UPDATE: With force inversion, values are negative
    // Smoothed (frame 2): -1.0 + 0.1 * (-10.0 - (-1.0)) = -1.0 + 0.1 * (-9.0) = -1.9
    // Force: -1.9 * 1.0 * 5.0 = -9.5 Nm
    // Normalized: -9.5 / 20.0 = -0.475
    data.mLocalRot.y += 10.0 * 0.01;
    PumpEngineTime(engine, data, 0.01);
    double force_frame2 = engine.GetDebugBatch().back().total_output;
    
    if (force_frame2 < force_frame1) {
        std::cout << "[PASS] Second frame accumulated correctly (" << force_frame2 << " < " << force_frame1 << ")." << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Second frame accumulation incorrect. Got " << force_frame2 << " Expected ~-0.475.");
    }
    
    // Test 3: Verify high-frequency noise rejection
    // Simulate rapid oscillation (noise from Slide Rumble)
    // Alternate between +5.0 and -5.0 every frame
    // The smoothed value should remain close to 0 (averaging out the noise)
    FFBEngine engine2;
    InitializeEngine(engine2); // v0.5.12: Initialize with T300 defaults
    engine2.m_sop_yaw_gain = 1.0f;
    engine2.m_sop_effect = 0.0f;
    engine2.m_wheelbase_max_nm = 20.0f; engine2.m_target_rim_nm = 20.0f;
    engine2.m_gain = 1.0f;
    engine2.m_understeer_effect = 0.0f;
    engine2.m_lockup_enabled = false;
    engine2.m_spin_enabled = false;
    engine2.m_slide_texture_enabled = false;
    engine2.m_bottoming_enabled = false;
    engine2.m_scrub_drag_gain = 0.0f;
    engine2.m_rear_align_effect = 0.0f;
    engine2.m_gyro_gain = 0.0f;
    
    TelemInfoV01 data2;
    std::memset(&data2, 0, sizeof(data2));
    data2.mWheel[0].mRideHeight = 0.1;
    data2.mWheel[1].mRideHeight = 0.1;
    data2.mSteeringShaftTorque = 0.0;
    
    // Run 20 frames of alternating noise
    double max_force = 0.0;
    data2.mDeltaTime = 0.0025;
    data2.mLocalRot.y = 0.0;
    engine2.calculate_force(&data2); // Seed
    
    for (int i = 0; i < 20; i++) {
        double accel = (i % 2 == 0) ? 5.0 : -5.0;
        data2.mLocalRot.y += accel * 0.0025;
        data2.mElapsedTime += 0.0025;
        double force = engine2.calculate_force(&data2);
        max_force = (std::max)(max_force, std::abs(force));
    }
    
    // With smoothing, the max force should be much smaller than the raw input would produce
    // Raw would give: 5.0 * 1.0 * 5.0 / 20.0 = 1.25 (clamped to 1.0)
    // Smoothed should stay well below 0.5
    if (max_force < 0.5) {
        std::cout << "[PASS] High-frequency noise rejected (max force " << max_force << " < 0.5)." << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("High-frequency noise not rejected. Max force: " << max_force);
    }
}

TEST_CASE(test_yaw_accel_convergence, "YawGyro") {
    std::cout << "\nTest: Yaw Acceleration Convergence (v0.4.18)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine); // v0.5.12: Initialize with T300 defaults
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    
    // Setup
    engine.m_sop_yaw_gain = 1.0f;
    engine.m_yaw_accel_smoothing = 0.0225f; // v0.5.8: Explicitly set legacy value
    engine.m_sop_effect = 0.0f;
    engine.m_wheelbase_max_nm = 20.0f; engine.m_target_rim_nm = 20.0f;
    engine.m_gain = 1.0f;
    engine.m_invert_force = false;
    engine.m_understeer_effect = 0.0f;
    engine.m_lockup_enabled = false;
    engine.m_spin_enabled = false;
    engine.m_slide_texture_enabled = false;
    engine.m_bottoming_enabled = false;
    engine.m_scrub_drag_gain = 0.0f;
    engine.m_rear_align_effect = 0.0f;
    engine.m_gyro_gain = 0.0f;
    
    data.mWheel[0].mRideHeight = 0.1;
    data.mLocalVel.z = 20.0; // v0.4.42: Ensure speed > 5 m/s for Yaw Kick
    data.mWheel[1].mRideHeight = 0.1;
    data.mSteeringShaftTorque = 0.0;
    
    // Test: Verify convergence to steady-state value
    // Constant input: 1.0 rad/s^2
    // Expected steady-state: 1.0 * 1.0 * 5.0 / 20.0 = 0.25
    data.mLocalRot.y = 0.0;
    data.mElapsedTime = 1.0;
    engine.calculate_force(&data); // Seed
    
    // Run for 50 frames (should converge with alpha=0.1)
    double force = 0.0;
    for (int i = 0; i < 50; i++) {
        data.mLocalRot.y += 1.0 * 0.01;
        force = PumpEngineTime(engine, data, 0.01);
    }
    
    // v0.4.20 UPDATE: With force inversion, steady-state is negative
    // Expected steady-state: -1.0 * 1.0 * 5.0 / 20.0 = -0.25
    // After 50 frames with alpha=0.1, should be very close to steady-state (-0.25)
    // Formula: smoothed = target * (1 - (1-alpha)^n)
    // After 50 frames: smoothed ~= -1.0 * (1 - 0.9^50) ~= -0.9948
    // Force: -0.9948 * 1.0 * 5.0 / 20.0 ~= -0.2487
    if (std::abs(force - (-0.25)) < 0.01) {
        std::cout << "[PASS] Converged to steady-state after 50 frames (" << force << " ~= -0.25)." << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Did not converge. Got " << force << " Expected ~-0.25.");
    }
    
    // Test: Verify response to step change
    // Change input from 1.0 to 0.0 (rotation stops accelerating, remains at current rate)
    // No change in mLocalRot.y means 0 acceleration.
    // data.mLocalRot.y stays at last value.
    
    // First frame after change
    double force_after_change = PumpEngineTime(engine, data, 0.01);
    
    // v0.4.20 UPDATE: With force inversion, decay is toward zero from negative
    // Smoothed should decay: prev_smoothed + 0.1 * (0.0 - prev_smoothed)
    // If prev_smoothed ~= -0.9948, new = -0.9948 + 0.1 * (0.0 - (-0.9948)) = -0.8953
    // Force: -0.8953 * 1.0 * 5.0 / 20.0 ~= -0.224
    if (force_after_change > force || std::abs(force_after_change - force) < 0.01) {
        std::cout << "[PASS] Smoothly decaying after step change (" << force_after_change << ")." << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Decay behavior incorrect. Got " << force_after_change);
    }
}

TEST_CASE(test_regression_yaw_slide_feedback, "YawGyro") {
    std::cout << "\nTest: Regression - Yaw/Slide Feedback Loop (v0.4.18)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine); // v0.5.12: Initialize with T300 defaults
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    
    // Setup: Enable BOTH Yaw Kick and Slide Rumble (the problematic combination)
    engine.m_sop_yaw_gain = 1.0f;  // Yaw Kick enabled
    engine.m_slide_texture_enabled = true;  // Slide Rumble enabled
    engine.m_slide_texture_gain = 1.0f;
    
    engine.m_sop_effect = 0.0f;
    engine.m_wheelbase_max_nm = 20.0f; engine.m_target_rim_nm = 20.0f;
    engine.m_gain = 1.0f;
    engine.m_understeer_effect = 0.0f;
    engine.m_lockup_enabled = false;
    engine.m_spin_enabled = false;
    engine.m_bottoming_enabled = false;
    engine.m_scrub_drag_gain = 0.0f;
    engine.m_rear_align_effect = 0.0f;
    engine.m_gyro_gain = 0.0f;
    
    data.mWheel[0].mRideHeight = 0.1;
    data.mWheel[1].mRideHeight = 0.1;
    data.mWheel[0].mTireLoad = 4000.0;
    data.mWheel[1].mTireLoad = 4000.0;
    data.mSteeringShaftTorque = 0.0;
    data.mDeltaTime = 0.0025; // 400Hz
    
    // Simulate the bug scenario:
    // 1. Slide Rumble generates high-frequency vibration (sawtooth wave)
    // 2. This would cause yaw acceleration to spike (if not smoothed)
    // 3. Yaw Kick would amplify the spikes
    // 4. Feedback loop: wheel shakes harder
    
    // Set up lateral sliding (triggers Slide Rumble)
    data.mWheel[0].mLateralPatchVel = 5.0;
    data.mWheel[1].mLateralPatchVel = 5.0;
    
    // Simulate high-frequency yaw acceleration noise (what Slide Rumble would cause)
    // Alternate between +10 and -10 rad/s^2 (extreme noise)
    double max_force = 0.0;
    double sum_force = 0.0;
    int frames = 50;
    
    data.mLocalRot.y = 0.0;
    engine.calculate_force(&data); // Seed
    
    for (int i = 0; i < frames; i++) {
        // Simulate noise that would come from vibrations
        double accel = (i % 2 == 0) ? 10.0 : -10.0;
        data.mLocalRot.y += accel * 0.0025;
        data.mElapsedTime += 0.0025;
        
        double force = engine.calculate_force(&data);
        max_force = (std::max)(max_force, std::abs(force));
        sum_force += std::abs(force);
    }
    
    double avg_force = sum_force / frames;
    
    // CRITICAL TEST: With smoothing, the system should remain stable
    // Without smoothing (v0.4.16), this would create a feedback loop with forces > 1.0
    // With smoothing (v0.4.18), max force should stay reasonable (< 1.0, ideally < 0.8)
    if (max_force < 1.0) {
        std::cout << "[PASS] No feedback loop detected (max force " << max_force << " < 1.0)." << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Potential feedback loop! Max force: " << max_force);
    }
    
    // Additional check: Average force should be low (noise should cancel out)
    if (avg_force < 0.5) {
        std::cout << "[PASS] Average force remains low (avg " << avg_force << " < 0.5)." << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Average force too high: " << avg_force);
    }
    
    // Verify that the smoothing state doesn't explode
    // Check internal state by running a few more frames with zero input
    // Stop increasing rate
    data.mWheel[0].mLateralPatchVel = 0.0;
    data.mWheel[1].mLateralPatchVel = 0.0;
    
    for (int i = 0; i < 10; i++) {
        data.mElapsedTime += 0.0025;
        engine.calculate_force(&data);
    }
    
    // After settling, force should decay to near zero
    double final_force = engine.calculate_force(&data);
    if (std::abs(final_force) < 0.1) {
        std::cout << "[PASS] System settled after noise removed (final force " << final_force << ")." << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("System did not settle. Final force: " << final_force);
    }
}

TEST_CASE(test_yaw_kick_signal_conditioning, "YawGyro") {
    std::cout << "\nTest: Yaw Kick Signal Conditioning (v0.4.42)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine); // v0.5.12: Initialize with T300 defaults
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = 0.0025;
    
    // Setup: Isolate Yaw Kick effect
    engine.m_sop_yaw_gain = 1.0f;
    engine.m_sop_effect = 0.0f;
    engine.m_wheelbase_max_nm = 20.0f; engine.m_target_rim_nm = 20.0f;
    engine.m_gain = 1.0f;
    engine.m_understeer_effect = 0.0f;
    engine.m_lockup_enabled = false;
    engine.m_spin_enabled = false;
    engine.m_slide_texture_enabled = false;
    engine.m_bottoming_enabled = false;
    engine.m_scrub_drag_gain = 0.0f;
    engine.m_rear_align_effect = 0.0f;
    engine.m_gyro_gain = 0.0f;
    engine.m_invert_force = false;
    engine.m_yaw_kick_threshold = 0.2f;
    engine.m_yaw_accel_smoothing = 0.0f; // Fast response
    
    // Test Case 1: Idle Noise - Below Deadzone Threshold (0.2 rad/s^2)
    std::cout << "  Case 1: Idle Noise (YawAccel = 0.1, below threshold)" << std::endl;
    data.mLocalRot.y = 0.0;
    engine.calculate_force(&data); // Seed
    data.mLocalRot.y = 0.1 * 0.0025; // Below 0.2 threshold
    
    // Ensure all effects that could mask are off
    engine.m_sop_effect = 0.0f;
    engine.m_understeer_effect = 0.0f;
    engine.m_min_force = 0.0f;
    
    double force_idle = engine.calculate_force(&data);
    
    // Should be zero because raw_yaw_accel is zeroed by noise gate
    if (std::abs(force_idle) < 0.01) {
        std::cout << "[PASS] Idle noise filtered (force = " << force_idle << " ~= 0.0)." << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Idle noise not filtered. Got " << force_idle << " Expected ~0.0.");
    }
    
    // Test Case 2: Low Speed Cutoff
    std::cout << "  Case 2: Low Speed (YawAccel = 5.0, Speed = 1.0 m/s)" << std::endl;
    FFBEngineTestAccess::ResetYawDerivedState(engine);
    data.mLocalRot.y = 0.0;
    engine.calculate_force(&data); // Seed
    data.mLocalRot.y = 5.0 * 0.0025; // High yaw accel
    data.mLocalVel.z = 1.0; // Below 5 m/s cutoff
    
    double force_low_speed = engine.calculate_force(&data);
    
    // Should be zero because speed < 5.0 m/s
    if (std::abs(force_low_speed) < 0.01) {
        std::cout << "[PASS] Low speed cutoff active (force = " << force_low_speed << " ~= 0.0)." << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Low speed cutoff failed. Got " << force_low_speed << " Expected ~0.0.");
    }
    
    // Test Case 3: Valid Kick - High Speed + High Yaw Accel
    std::cout << "  Case 3: Valid Kick (YawAccel = 5.0, Speed = 20.0 m/s)" << std::endl;
    FFBEngineTestAccess::ResetYawDerivedState(engine);
    data.mLocalRot.y = 0.0;
    data.mLocalVel.z = 20.0; // Ensure speed is above cutoff
    engine.calculate_force(&data); // Seed
    
    // Run for multiple frames to let smoothing settle
    double force_valid = 0.0;
    for (int i = 0; i < 100; i++) {
        data.mLocalRot.y += 5.0 * 0.0025;
        data.mElapsedTime += 0.0025;
        force_valid = engine.calculate_force(&data);
    }
    
    // Should be non-zero and negative (due to inversion)
    if (force_valid < -0.1) {
        std::cout << "[PASS] Valid kick detected (force = " << force_valid << ")." << std::endl;
        g_tests_passed++;
        } else {
            FAIL_TEST("Valid kick not detected correctly. Got " << force_valid << "." << std::endl
                << "DEBUG: m_yaw_accel_smoothed: " << FFBEngineTestAccess::GetYawAccelSmoothed(engine) << std::endl
                << "DEBUG: m_yaw_kick_threshold: " << engine.m_yaw_kick_threshold << std::endl
                << "DEBUG: m_sop_yaw_gain: " << engine.m_sop_yaw_gain);
        }
}

TEST_CASE(test_yaw_kick_threshold, "YawGyro") {
    std::cout << "\nTest: Yaw Kick Threshold (v0.6.10)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    
    engine.m_sop_yaw_gain = 1.0f;
    engine.m_yaw_kick_threshold = 5.0f;
    engine.m_yaw_accel_smoothing = 0.0001f; // Fast response for test
    
    // Case 1: Yaw Accel below threshold (2.0 < 5.0)
    data.mDeltaTime = 0.0025;
    data.mLocalRot.y = 0.0;
    engine.calculate_force(&data); // Seed
    data.mLocalRot.y = 2.0 * 0.0025;
    engine.calculate_force(&data); // 1st frame smoothing
    data.mLocalRot.y += 2.0 * 0.0025;
    double force_low = engine.calculate_force(&data);
    
    ASSERT_NEAR(force_low, 0.0, 0.001);

    // Case 2: Yaw Accel above threshold (6.0 > 5.0)
    data.mLocalRot.y = 0.0;
    FFBEngineTestAccess::ResetYawDerivedState(engine);
    engine.calculate_force(&data); // Seed
    
    // Run for 40 frames to ensure convergence
    double force_high = 0.0;
    for (int i = 0; i < 40; i++) {
        data.mLocalRot.y += 6.0 * 0.0025;
        data.mElapsedTime += 0.0025;
        force_high = engine.calculate_force(&data);
    }
    
    // v0.4.18 (Issue #241): Continuous deadzone means force is based on (6.0 - 5.0) = 1.0
    // formula: force = -1.0 * processed * gain * 5.0 / wheelbase_max
    // force = -1.0 * 1.0 * 1.0 * 5.0 / 20.0 = -0.25
    ASSERT_NEAR(force_high, -0.25, 0.01);
}

TEST_CASE(test_yaw_kick_edge_cases, "YawGyro") {
    std::cout << "\nTest: Yaw Kick Threshold Edge Cases (v0.6.10)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    
    engine.m_sop_yaw_gain = 1.0f;
    engine.m_yaw_accel_smoothing = 0.0001f; // Fast response for testing
    
    // Edge Case 1: Zero Threshold (0.0) - All signals pass through
    engine.m_yaw_kick_threshold = 0.0f;
    
    // Use a reasonable signal (not tiny) to test threshold behavior
    data.mDeltaTime = 0.0025;
    data.mLocalRot.y = 0.0;
    FFBEngineTestAccess::ResetYawDerivedState(engine);
    engine.calculate_force(&data); // Seed
    double force_tiny = 0.0;
    for (int i = 0; i < 40; i++) {
        data.mLocalRot.y += 1.0 * 0.0025;
        data.mElapsedTime += 0.0025;
        force_tiny = engine.calculate_force(&data);
    }
    
    ASSERT_TRUE(std::abs(force_tiny) > 0.001); // With zero threshold, signals pass
    
    // Edge Case 2: Maximum Threshold (10.0) - Only extreme signals pass
    engine.m_yaw_kick_threshold = 10.0f;
    
    // Reset smoothing state
    FFBEngineTestAccess::ResetYawDerivedState(engine);
    
    // Large but below threshold (9.0 < 10.0)
    data.mLocalRot.y = 0.0;
    data.mElapsedTime = 0.0;
    engine.calculate_force(&data); // Seed
    double force_below_max = 0.0;
    for (int i = 0; i < 40; i++) {
        data.mLocalRot.y += 9.0 * 0.0025;
        data.mElapsedTime += 0.0025;
        force_below_max = engine.calculate_force(&data);
    }
    
    ASSERT_NEAR(force_below_max, 0.0, 0.001); // Below max threshold = gated
    
    // Above maximum threshold (11.0 > 10.0)
    data.mLocalRot.y = 0.0;
    data.mElapsedTime = 0.0;
    FFBEngineTestAccess::ResetYawDerivedState(engine);
    engine.calculate_force(&data); // Seed
    double force_above_max = 0.0;
    for (int i = 0; i < 40; i++) {
        data.mLocalRot.y += 11.0 * 0.0025;
        data.mElapsedTime += 0.0025;
        force_above_max = engine.calculate_force(&data);
    }
    
    // v0.4.18 (Issue #241): Continuous deadzone means force is based on (11.0 - 10.0) = 1.0
    // force = -1.0 * 1.0 * 1.0 * 5.0 / 20.0 = -0.25
    ASSERT_NEAR(force_above_max, -0.25, 0.01);
    
    // Edge Case 3: Negative yaw acceleration (should use absolute value)
    engine.m_yaw_kick_threshold = 5.0f;
    engine.m_yaw_accel_smoothed = 0.0; // Reset
    
    // Negative value with magnitude above threshold
    data.mLocalRot.y = 0.0;
    data.mElapsedTime = 0.0;
    FFBEngineTestAccess::ResetYawDerivedState(engine);
    engine.calculate_force(&data); // Seed
    double force_negative = 0.0;
    for (int i = 0; i < 40; i++) {
        data.mLocalRot.y -= 6.0 * 0.0025;
        data.mElapsedTime += 0.0025;
        force_negative = engine.calculate_force(&data);
    }
    
    // v0.4.18: Continuous deadzone means force is based on (-6.0 + 5.0) = -1.0
    // force = -1.0 * (-1.0) * 1.0 * 5.0 / 20.0 = 0.25
    ASSERT_NEAR(force_negative, 0.25, 0.01);
    
    // Negative value with magnitude below threshold
    FFBEngineTestAccess::ResetYawDerivedState(engine); // Reset
    data.mLocalRot.y = 0.0;
    data.mElapsedTime = 0.0;
    engine.calculate_force(&data); // Seed
    double force_negative_below = 0.0;
    for (int i = 0; i < 40; i++) {
        data.mLocalRot.y -= 4.0 * 0.0025;
        data.mElapsedTime += 0.0025;
        force_negative_below = engine.calculate_force(&data);
    }
    
    ASSERT_NEAR(force_negative_below, 0.0, 0.001); // Below threshold = gated
    
    // Edge Case 4: Interaction with low-speed cutoff
    // Low speed cutoff (< 5.0 m/s) should override threshold
    engine.m_yaw_kick_threshold = 0.0f; // Zero threshold (all pass)
    FFBEngineTestAccess::ResetYawDerivedState(engine); // Reset
    data.mLocalRot.y = 0.0;
    engine.calculate_force(&data); // Seed
    data.mLocalRot.y = 10.0 * 0.0025; // Large acceleration
    data.mLocalVel.z = 3.0; // Below 5.0 m/s cutoff
    
    engine.calculate_force(&data);
    data.mLocalRot.y += 10.0 * 0.0025;
    double force_low_speed = engine.calculate_force(&data);
    
    ASSERT_NEAR(force_low_speed, 0.0, 0.001); // Low speed cutoff takes precedence
}

TEST_CASE(test_gyro_stability, "YawGyro") {
    std::cout << "\nTest: Gyro Stability (Clamp Check)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine); // v0.5.12: Initialize with T300 defaults
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    
    engine.m_gyro_gain = 1.0;
    engine.m_gyro_smoothing = -1.0; // Malicious input (should be clamped to 0.0 internally) 
    
    data.mDeltaTime = 0.01;
    data.mLocalVel.z = 20.0;
    
    // Run
    engine.calculate_force(&data);
    
    // Check if exploded
    if (std::abs(engine.m_steering_velocity_smoothed) < 1000.0 && !std::isnan(engine.m_steering_velocity_smoothed)) {
         std::cout << "[PASS] Gyro stable with negative smoothing." << std::endl;
         g_tests_passed++;
    } else {
         FAIL_TEST("Gyro exploded!");
    }
}

TEST_CASE(test_sop_yaw_kick_direction, "YawGyro") {
    std::cout << "\nTest: SoP Yaw Kick Direction (v0.4.20)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine); // v0.5.12: Initialize with T300 defaults
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    
    engine.m_sop_yaw_gain = 1.0f;
    engine.m_gain = 1.0f;
    engine.m_wheelbase_max_nm = 20.0f; engine.m_target_rim_nm = 20.0f;
    engine.m_invert_force = false;
    
    // Case: Car rotates Right (+Yaw Accel)
    // This implies rear is sliding Left.
    // We want Counter-Steer Left (Negative Torque).
    data.mDeltaTime = 0.01;
    data.mLocalRot.y = 0.0;
    data.mElapsedTime = 1.0;
    engine.calculate_force(&data); // Seed
    data.mLocalRot.y = 5.0 * 0.01;
    data.mLocalVel.z = 20.0; // v0.4.42: Ensure speed > 5 m/s for Yaw Kick 
    
    // Issue #397: Use PumpEngineTime
    PumpEngineTime(engine, data, 0.02);
    double force = engine.GetDebugBatch().back().total_output;
    
    // We expect counter-steer (negative force) but it might be small due to delay
    if (force < -0.000001) {
        std::cout << "[PASS] Yaw Kick provides counter-steer (Negative Force: " << force << ")" << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Yaw Kick direction wrong. Got: " << force << " Expected Negative.");
    }
}

TEST_CASE(test_chassis_inertia_smoothing_convergence, "YawGyro") {
    std::cout << "\nTest: Chassis Inertia Smoothing Convergence (v0.4.39)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine); // v0.5.12: Initialize with T300 defaults
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    
    // Setup: Apply constant acceleration via velocity changes for Issue #278
    // To get 1G (9.81 m/s^2) at 400Hz (dt=0.0025): dv = 9.81 * 0.0025 = 0.024525
    data.mLocalVel.y = 0.0;
    data.mLocalVel.z = 0.0;
    data.mLocalAccel.x = 9.81; // Lateral still raw
    data.mDeltaTime = 0.0025; // 400Hz
    
    // Chassis tau = 0.035s, alpha = dt / (tau + dt)
    // At 400Hz: alpha = 0.0025 / (0.035 + 0.0025) Ã¢â€°Ë† 0.0667
    // After 50 frames (~125ms), should be near steady-state
    
    for (int i = 0; i < 50; i++) {
        data.mLocalVel.y += 0.0; // Y is 0
        data.mLocalVel.z += 9.81 * 0.0025;
        data.mElapsedTime += 0.0025; // Update time to ensure is_new_frame works
        engine.calculate_force(&data, nullptr, nullptr, 0.0f, true, 0.0025);
    }
    
    // Check convergence
    double smoothed_x = engine.m_accel_x_smoothed;
    double smoothed_z = engine.m_accel_z_smoothed;
    
    // Should be close to input (9.81) after 50 frames
    // Exponential decay: y(t) = target * (1 - e^(-t/tau))
    // At t = 125ms, tau = 35ms: y = 9.81 * (1 - e^(-3.57)) Ã¢â€°Ë† 9.81 * 0.972 Ã¢â€°Ë† 9.53
    double expected = 9.81 * 0.95; // Allow 5% error
    
    if (smoothed_x > expected && smoothed_z > expected) {
        std::cout << "[PASS] Smoothing converged (X: " << smoothed_x << ", Z: " << smoothed_z << ")" << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Smoothing did not converge. X: " << smoothed_x << " Z: " << smoothed_z << " Expected > " << expected);
    }
    
    // Test decay
    data.mLocalAccel.x = 0.0;
    // To maintain 0 acceleration, velocity must stop changing
    
    for (int i = 0; i < 50; i++) {
        data.mElapsedTime += 0.0025;
        engine.calculate_force(&data, nullptr, nullptr, 0.0f, true, 0.0025);
    }
    
    smoothed_x = engine.m_accel_x_smoothed;
    smoothed_z = engine.m_accel_z_smoothed;
    
    // Should decay to near zero
    if (smoothed_x < 0.5 && smoothed_z < 0.5) {
        std::cout << "[PASS] Smoothing decayed correctly (X: " << smoothed_x << ", Z: " << smoothed_z << ")" << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Smoothing did not decay. X: " << smoothed_x << " Z: " << smoothed_z);
    }
}



} // namespace FFBEngineTests
