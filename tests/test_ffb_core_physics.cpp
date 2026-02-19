#include "test_ffb_common.h"
#include <random>

namespace FFBEngineTests {

TEST_CASE(test_base_force_modes, "CorePhysics") {
    std::cout << "\nTest: Base Force Modes & Gain (v0.4.13)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine); 
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    data.mDeltaTime = 0.0025;
    data.mLocalVel.z = -20.0; 
    
    engine.m_wheelbase_max_nm = 20.0f; engine.m_target_rim_nm = 20.0f;
    engine.m_gain = 1.0f; 
    engine.m_steering_shaft_gain = 0.5f; 
    engine.m_invert_force = false;
    
    data.mSteeringShaftTorque = 10.0; 
    data.mWheel[0].mGripFract = 1.0; 
    data.mWheel[1].mGripFract = 1.0;
    data.mWheel[0].mRideHeight = 0.1; 
    data.mWheel[1].mRideHeight = 0.1;
    
    engine.m_base_force_mode = 0;
    double force_native = engine.calculate_force(&data);
    
    if (std::abs(force_native - 0.25) < 0.001) {
        std::cout << "[PASS] Native Mode: Correctly attenuated (0.25)." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Native Mode: Got " << force_native << " Expected 0.25." << std::endl;
        g_tests_failed++;
    }
    
    engine.m_base_force_mode = 1;
    double force_synthetic = engine.calculate_force(&data);
    
    if (std::abs(force_synthetic - 0.5) < 0.001) {
        std::cout << "[PASS] Synthetic Mode: Constant force applied (0.5)." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Synthetic Mode: Got " << force_synthetic << " Expected 0.5." << std::endl;
        g_tests_failed++;
    }
    
    data.mSteeringShaftTorque = 0.1; 
    double force_deadzone = engine.calculate_force(&data);
    if (std::abs(force_deadzone) < 0.001) {
        std::cout << "[PASS] Synthetic Mode: Deadzone respected." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Synthetic Mode: Deadzone failed." << std::endl;
        g_tests_failed++;
    }
    
    engine.m_base_force_mode = 2;
    data.mSteeringShaftTorque = 10.0; 
    double force_muted = engine.calculate_force(&data);
    
    if (std::abs(force_muted) < 0.001) {
        std::cout << "[PASS] Muted Mode: Output is zero." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Muted Mode: Got " << force_muted << " Expected 0.0." << std::endl;
        g_tests_failed++;
    }
}

TEST_CASE(test_grip_modulation, "CorePhysics") {
    std::cout << "\nTest: Grip Modulation (Understeer)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine); 
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    
    data.mWheel[0].mRideHeight = 0.1; data.mWheel[1].mRideHeight = 0.1;
    data.mLocalVel.z = -20.0; 

    engine.m_gain = 1.0; 
    engine.m_wheelbase_max_nm = 20.0f; engine.m_target_rim_nm = 20.0f;
    engine.m_invert_force = false;

    data.mSteeringShaftTorque = 10.0; 
    engine.m_sop_effect = 0.0;
    engine.m_slide_texture_enabled = false;
    engine.m_road_texture_enabled = false;

    data.mWheel[0].mGripFract = 1.0;
    data.mWheel[1].mGripFract = 1.0;
    data.mWheel[0].mRideHeight = 0.1; data.mWheel[1].mRideHeight = 0.1;
    engine.m_understeer_effect = 1.0;
    
    double force_full = engine.calculate_force(&data);
    ASSERT_NEAR(force_full, 0.5, 0.001);

    data.mWheel[0].mGripFract = 0.5;
    data.mWheel[1].mGripFract = 0.5;
    double force_half = engine.calculate_force(&data);
    ASSERT_NEAR(force_half, 0.25, 0.001);
}

TEST_CASE(test_min_force, "CorePhysics") {
    std::cout << "\nTest: Min Force" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine); 
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    
    data.mWheel[0].mRideHeight = 0.1; data.mWheel[1].mRideHeight = 0.1;
    data.mWheel[0].mGripFract = 1.0;
    data.mWheel[1].mGripFract = 1.0;

    engine.m_slide_texture_enabled = false;
    engine.m_road_texture_enabled = false;
    engine.m_sop_effect = 0.0;

    data.mSteeringShaftTorque = 0.05; 
    data.mLocalVel.z = -20.0; 
    engine.m_min_force = 0.10f; 
    engine.m_wheelbase_max_nm = 20.0f; engine.m_target_rim_nm = 20.0f;
    engine.m_invert_force = false;

    double force = engine.calculate_force(&data);
    ASSERT_NEAR(force, 0.10, 0.001);
}

TEST_CASE(test_zero_input, "CorePhysics") {
    std::cout << "\nTest: Zero Input" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine); 
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    
    data.mWheel[0].mGripFract = 1.0;
    data.mWheel[1].mGripFract = 1.0;
    data.mWheel[0].mRideHeight = 0.1;
    data.mWheel[1].mRideHeight = 0.1;
    
    double force = engine.calculate_force(&data);
    ASSERT_NEAR(force, 0.0, 0.001);
}

TEST_CASE(test_grip_low_speed, "CorePhysics") {
    std::cout << "\nTest: Grip Approximation Low Speed Cutoff" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine); 
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    
    engine.m_bottoming_enabled = false;
    engine.m_slide_texture_enabled = false;
    engine.m_road_texture_enabled = false;
    engine.m_invert_force = false;

    data.mWheel[0].mGripFract = 0.0; 
    data.mWheel[1].mGripFract = 0.0;
    data.mWheel[0].mTireLoad = 4000.0; 
    data.mWheel[1].mTireLoad = 4000.0;
    engine.m_gain = 1.0;
    engine.m_understeer_effect = 1.0;
    data.mSteeringShaftTorque = 40.0; 
    engine.m_wheelbase_max_nm = 40.0f; engine.m_target_rim_nm = 40.0f;
    
    data.mLocalVel.z = 1.0; 
    
    data.mWheel[0].mLateralPatchVel = 2.0;
    data.mWheel[1].mLateralPatchVel = 2.0;
    data.mWheel[0].mLongitudinalGroundVel = 1.0;
    data.mWheel[1].mLongitudinalGroundVel = 1.0;
    
    engine.m_steering_shaft_torque_smoothed = 40.0; 
    
    double force = engine.calculate_force(&data);
    
    if (std::abs(force - 1.0) < 0.001) {
        std::cout << "[PASS] Low speed grip forced to 1.0." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Low speed grip not forced. Got " << force << " Expected 1.0." << std::endl;
        g_tests_failed++;
    }
}

TEST_CASE(test_gain_compensation, "CorePhysics") {
    std::cout << "\nTest: FFB Signal Gain Compensation (Decoupling)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine); 
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));

    data.mDeltaTime = 0.0025; 
    data.mLocalVel.z = 20.0;
    data.mWheel[0].mRideHeight = 0.1;
    data.mWheel[1].mRideHeight = 0.1;
    data.mWheel[2].mRideHeight = 0.1;
    data.mWheel[3].mRideHeight = 0.1;
    data.mWheel[0].mTireLoad = 4000.0;
    data.mWheel[1].mTireLoad = 4000.0;
    engine.m_gain = 1.0;
    engine.m_invert_force = false;
    engine.m_understeer_effect = 0.0; 
    engine.m_oversteer_boost = 0.0;

    double ra1, ra2;
    {
        FFBEngine e1;
        e1.m_gain = 1.0; e1.m_invert_force = false; e1.m_understeer_effect = 0.0; e1.m_oversteer_boost = 0.0;
        e1.m_rear_align_effect = 1.0;
        e1.m_wheelbase_max_nm = 20.0f; e1.m_target_rim_nm = 20.0f;
        ra1 = e1.calculate_force(&data);
    }
    {
        FFBEngine e2;
        e2.m_gain = 1.0; e2.m_invert_force = false; e2.m_understeer_effect = 0.0; e2.m_oversteer_boost = 0.0;
        e2.m_rear_align_effect = 1.0;
        e2.m_wheelbase_max_nm = 60.0f; e2.m_target_rim_nm = 60.0f;
        ra2 = e2.calculate_force(&data);
    }

    if (std::abs(ra1 - ra2) < 0.001) {
        std::cout << "[PASS] Rear Align Torque correctly compensated (" << ra1 << " == " << ra2 << ")" << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Rear Align Torque compensation failed! 20Nm: " << ra1 << " 60Nm: " << ra2 << std::endl;
        g_tests_failed++;
    }

    double s1, s2;
    {
        FFBEngine e1;
        e1.m_gain = 1.0; e1.m_invert_force = false; e1.m_understeer_effect = 0.0; e1.m_oversteer_boost = 0.0;
        e1.m_slide_texture_enabled = true;
        e1.m_slide_texture_gain = 1.0;
        e1.m_wheelbase_max_nm = 20.0f; e1.m_target_rim_nm = 20.0f;
        e1.m_slide_phase = 0.5;
        s1 = e1.calculate_force(&data);
    }
    {
        FFBEngine e2;
        e2.m_gain = 1.0; e2.m_invert_force = false; e2.m_understeer_effect = 0.0; e2.m_oversteer_boost = 0.0;
        e2.m_slide_texture_enabled = true;
        e2.m_slide_texture_gain = 1.0;
        e2.m_wheelbase_max_nm = 100.0f; e2.m_target_rim_nm = 100.0f;
        e2.m_slide_phase = 0.5;
        s2 = e2.calculate_force(&data);
    }

    if (std::abs(s1 - s2) < 0.001) {
        std::cout << "[PASS] Slide Texture correctly compensated (" << s1 << " == " << s2 << ")" << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Slide Texture compensation failed! 20Nm: " << s1 << " 100Nm: " << s2 << std::endl;
        g_tests_failed++;
    }

    engine.m_slide_texture_enabled = false;
    engine.m_understeer_effect = 0.5; 
    data.mSteeringShaftTorque = 10.0;
    data.mWheel[0].mGripFract = 0.6; 
    data.mWheel[1].mGripFract = 0.6;

    engine.m_wheelbase_max_nm = 20.0f; engine.m_target_rim_nm = 20.0f;
    double u1 = engine.calculate_force(&data);

    engine.m_wheelbase_max_nm = 40.0f; engine.m_target_rim_nm = 40.0f;
    double u2 = engine.calculate_force(&data);

    // v0.7.67 Fix for Issue #152: Understeer is now normalized by session peak,
    // making it independent of m_wheelbase_max_nm. Expect u1 == u2.
    if (std::abs(u1 - u2) < 0.001) {
        std::cout << "[PASS] Understeer Modifier correctly normalized by session peak (" << u1 << " == " << u2 << ")" << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Understeer Modifier behavior unexpected! 20Nm: " << u1 << " 40Nm: " << u2 << std::endl;
        g_tests_failed++;
    }

    std::cout << "[SUMMARY] Gain Compensation verified for all effect types." << std::endl;
}

TEST_CASE(test_high_gain_stability, "CorePhysics") {
    std::cout << "\nTest: High Gain Stability (Max Ranges)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.15); 
    
    engine.m_gain = 2.0f; 
    engine.m_understeer_effect = 200.0f;
    engine.m_abs_gain = 10.0f;
    engine.m_lockup_gain = 3.0f;
    engine.m_brake_load_cap = 10.0f;
    engine.m_oversteer_boost = 4.0f;
    
    data.mWheel[0].mLongitudinalPatchVel = -15.0; 
    data.mUnfilteredBrake = 1.0;
    
    for(int i=0; i<1000; i++) {
        double force = engine.calculate_force(&data);
        if (std::isnan(force) || std::isinf(force)) {
            std::cout << "[FAIL] Stability failure at iteration " << i << std::endl;
            g_tests_failed++;
            return;
        }
    }
    std::cout << "[PASS] Engine stable at 200% Gain and 10.0 ABS Gain." << std::endl;
    g_tests_passed++;
}

TEST_CASE(test_stress_stability, "CorePhysics") {
    std::cout << "\nTest: Stress Stability (Fuzzing)" << std::endl;
    FFBEngine engine;
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    
    // Enable EVERYTHING
    engine.m_lockup_enabled = true;
    engine.m_spin_enabled = true;
    engine.m_slide_texture_enabled = true;
    engine.m_road_texture_enabled = true;
    engine.m_bottoming_enabled = true;
    engine.m_scrub_drag_gain = 1.0;
    
    std::default_random_engine generator;
    std::uniform_real_distribution<double> distribution(-100000.0, 100000.0);
    std::uniform_real_distribution<double> dist_small(-1.0, 1.0);
    
    bool failed = false;
    
    for(int i=0; i<1000; i++) {
        data.mSteeringShaftTorque = distribution(generator);
        data.mLocalAccel.x = distribution(generator);
        data.mLocalVel.z = distribution(generator);
        data.mDeltaTime = std::abs(dist_small(generator) * 0.1); 
        
        for(int w=0; w<4; w++) {
            data.mWheel[w].mTireLoad = distribution(generator);
            data.mWheel[w].mGripFract = dist_small(generator); 
            data.mWheel[w].mSuspForce = distribution(generator);
            data.mWheel[w].mVerticalTireDeflection = distribution(generator);
            data.mWheel[w].mLateralPatchVel = distribution(generator);
            data.mWheel[w].mLongitudinalGroundVel = distribution(generator);
        }
        
        double force = engine.calculate_force(&data);
        
        if (std::isnan(force) || std::isinf(force)) {
            std::cout << "[FAIL] Iteration " << i << " produced NaN/Inf!" << std::endl;
            failed = true;
            break;
        }
        
        if (force > 1.00001 || force < -1.00001) {
            std::cout << "[FAIL] Iteration " << i << " exceeded bounds: " << force << std::endl;
            failed = true;
            break;
        }
    }
    
    if (!failed) {
        std::cout << "[PASS] Survived 1000 iterations of random input." << std::endl;
        g_tests_passed++;
    } else {
        g_tests_failed++;
    }
}

TEST_CASE(test_smoothing_step_response, "CorePhysics") {
    std::cout << "\nTest: SoP Smoothing Step Response" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine); 
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    
    data.mWheel[0].mRideHeight = 0.1; data.mWheel[1].mRideHeight = 0.1;

    engine.m_sop_smoothing_factor = 0.5;
    engine.m_sop_scale = 1.0;  
    engine.m_sop_effect = 1.0;
    engine.m_wheelbase_max_nm = 20.0f; engine.m_target_rim_nm = 20.0f;
    engine.m_invert_force = false;
    
    data.mLocalAccel.x = 9.81; 
    data.mDeltaTime = 0.0025;
    
    double force1 = engine.calculate_force(&data);
    
    if (force1 > 0.0 && force1 < 0.005) {
        std::cout << "[PASS] Smoothing Step 1 correct (" << force1 << ", small positive)." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Smoothing Step 1 mismatch. Got " << force1 << std::endl;
        g_tests_failed++;
    }
    
    for (int i = 0; i < 100; i++) {
        force1 = engine.calculate_force(&data);
    }
    
    if (force1 > 0.02 && force1 < 0.06) {
        std::cout << "[PASS] Smoothing settled to steady-state (" << force1 << ", near 0.05)." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Smoothing did not settle. Value: " << force1 << std::endl;
        g_tests_failed++;
    }
}

TEST_CASE(test_time_corrected_smoothing, "CorePhysics") {
    std::cout << "\nTest: Time Corrected Smoothing (v0.4.37)" << std::endl;
    FFBEngine engine_fast; // 400Hz
    InitializeEngine(engine_fast); 
    FFBEngine engine_slow; // 50Hz
    InitializeEngine(engine_slow); 
    
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    data.mLocalRotAccel.y = 10.0; 
    
    data.mDeltaTime = 0.0025;
    for(int i=0; i<80; i++) engine_fast.calculate_force(&data);
    
    data.mDeltaTime = 0.02;
    for(int i=0; i<10; i++) engine_slow.calculate_force(&data);
    
    double val_fast = engine_fast.m_yaw_accel_smoothed;
    double val_slow = engine_slow.m_yaw_accel_smoothed;
    
    std::cout << "Fast Yaw (400Hz): " << val_fast << " Slow Yaw (50Hz): " << val_slow << std::endl;
    
    if (std::abs(val_fast - val_slow) < 0.5) {
        std::cout << "[PASS] Smoothing is consistent across frame rates." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Smoothing diverges! Time correction failed." << std::endl;
        g_tests_failed++;
    }
}

TEST_CASE(test_abs_frequency_scaling, "CorePhysics") {
    std::cout << "\nTest: ABS Frequency Scaling" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(10.0);
    engine.m_abs_pulse_enabled = true;
    engine.m_abs_gain = 1.0f;
    data.mDeltaTime = 0.001; 
    
    engine.m_abs_freq_hz = 20.0f;
    engine.m_abs_phase = 0.0;
    engine.calculate_force(&data); 
    double start_phase = engine.m_abs_phase;
    engine.calculate_force(&data);
    double delta_phase_20 = engine.m_abs_phase - start_phase;
    
    engine.m_abs_freq_hz = 40.0f;
    engine.m_abs_phase = 0.0;
    engine.calculate_force(&data);
    start_phase = engine.m_abs_phase;
    engine.calculate_force(&data);
    double delta_phase_40 = engine.m_abs_phase - start_phase;
    
    ASSERT_NEAR(delta_phase_40, delta_phase_20 * 2.0, 0.0001);
}

TEST_CASE(test_lockup_pitch_scaling, "CorePhysics") {
    std::cout << "\nTest: Lockup Pitch Scaling" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    engine.m_lockup_enabled = true;
    data.mWheel[0].mLongitudinalPatchVel = -5.0; 
    data.mDeltaTime = 0.001;
    
    engine.m_lockup_freq_scale = 1.0f;
    engine.m_lockup_phase = 0.0;
    engine.calculate_force(&data);
    double start_phase = engine.m_lockup_phase;
    engine.calculate_force(&data);
    double delta_1 = engine.m_lockup_phase - start_phase;
    
    engine.m_lockup_freq_scale = 2.0f;
    engine.m_lockup_phase = 0.0;
    engine.calculate_force(&data);
    start_phase = engine.m_lockup_phase;
    engine.calculate_force(&data);
    double delta_2 = engine.m_lockup_phase - start_phase;
    
    ASSERT_NEAR(delta_2, delta_1 * 2.0, 0.0001);
}

TEST_CASE(test_sop_effect, "CorePhysics") {
    std::cout << "\nTest: SoP Effect" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    engine.m_sop_effect = 0.5f;
    engine.m_sop_scale = 10.0f;
    engine.m_sop_smoothing_factor = 1.0f;
    data.mLocalAccel.x = 4.905; // 0.5G
    for (int i = 0; i < 60; i++) engine.calculate_force(&data);
    double force = engine.calculate_force(&data);
    ASSERT_NEAR(force, 0.125, 0.05);
}

TEST_CASE(test_regression_rear_torque_lpf, "CorePhysics") {
    std::cout << "\nTest: Regression - Rear Torque LPF Continuity" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine); // v0.5.12: Initialize with T300 defaults
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    
    engine.m_rear_align_effect = 1.0;
    engine.m_sop_effect = 0.0; // Isolate rear torque
    engine.m_oversteer_boost = 0.0;
    engine.m_wheelbase_max_nm = 20.0f; engine.m_target_rim_nm = 20.0f;
    engine.m_invert_force = false;
    engine.m_gain = 1.0f; // Explicit gain for clarity
    
    // Setup: Car is sliding sideways (5 m/s) but has Grip (1.0)
    // This means Rear Torque is 0.0 (because grip is good), BUT LPF should be tracking the slide.
    data.mWheel[2].mLateralPatchVel = 5.0;
    data.mWheel[3].mLateralPatchVel = 5.0;
    data.mWheel[2].mLongitudinalGroundVel = 20.0;
    data.mWheel[3].mLongitudinalGroundVel = 20.0;
    data.mWheel[2].mGripFract = 1.0; // Good grip
    data.mWheel[3].mGripFract = 1.0;
    data.mWheel[2].mTireLoad = 4000.0;
    data.mWheel[3].mTireLoad = 4000.0;
    data.mWheel[2].mSuspForce = 3700.0; // For load calc
    data.mWheel[3].mSuspForce = 3700.0;
    data.mDeltaTime = 0.01;
    
    // Run 50 frames. The LPF should settle on the slip angle (~0.24 rad).
    for(int i=0; i<50; i++) {
        engine.calculate_force(&data);
    }
    
    // Now, suddenly drop grip to 0.0 (Oversteer event)
    data.mWheel[2].mGripFract = 0.0;
    data.mWheel[3].mGripFract = 0.0;
    
    // The LPF should ALREADY be charged, so force should be immediate.
    double force = engine.calculate_force(&data);
    
    // With 0.24 rad slip angle and 4000N load, we expect significant force.
    // Normalized ~ -0.3
    if (std::abs(force) > 0.1) {
        std::cout << "[PASS] LPF was running in background. Force: " << force << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] LPF was idle! Cold start lag detected. Force: " << force << std::endl;
        g_tests_failed++;
    }
}

TEST_CASE(test_steering_shaft_smoothing, "CorePhysics") {
    std::cout << "\nTest: Steering Shaft Smoothing (v0.5.7)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine); // v0.5.12: Initialize with T300 defaults
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    data.mDeltaTime = 0.01; // 100Hz for this test math
    data.mLocalVel.z = -20.0;

    engine.m_steering_shaft_smoothing = 0.050f; // 50ms tau
    engine.m_gain = 1.0;
    engine.m_wheelbase_max_nm = 1.0; engine.m_target_rim_nm = 1.0;
    // v0.7.67 Fix for Issue #152: Ensure normalization matches the test scaling
    FFBEngineTestAccess::SetSessionPeakTorque(engine, 1.0);
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0);
    FFBEngineTestAccess::SetRollingAverageTorque(engine, 1.0);
    FFBEngineTestAccess::SetLastRawTorque(engine, 1.0);

    engine.m_understeer_effect = 0.0; // Neutralize modifiers
    engine.m_sop_effect = 0.0f;      // Disable SoP
    engine.m_invert_force = false;   // Disable inversion
    data.mDeltaTime = 0.01; // 100Hz

    // Step input: 0.0 -> 1.0
    data.mSteeringShaftTorque = 1.0;

    // After 1 frame (10ms) with 50ms tau:
    // alpha = dt / (tau + dt) = 10 / (50 + 10) = 1/6 â‰ˆ 0.166
    // Expected force: 0.166
    double force = engine.calculate_force(&data);

    if (std::abs(force - 0.166) < 0.01) {
        std::cout << "[PASS] Shaft Smoothing delayed the step input (Frame 1: " << force << ")." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Shaft Smoothing mismatch. Got " << force << " Expected ~0.166." << std::endl;
        g_tests_failed++;
    }

    // After 20 frames (200ms) it should be near 1.0 (approx 97% of target)
    for (int i = 0; i < 19; i++) engine.calculate_force(&data);
    force = engine.calculate_force(&data);

    if (force > 0.8 && force < 0.99) {
        std::cout << "[PASS] Shaft Smoothing converged correctly (Frame 11: " << force << ")." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Shaft Smoothing convergence failure. Got " << force << std::endl;
        g_tests_failed++;
    }
}



} // namespace FFBEngineTests
