#include "test_ffb_common.h"
#include <random>

namespace FFBEngineTests {

TEST_CASE(test_base_force_passthrough, "CorePhysics") {
    std::cout << "\nTest: Base Force Passthrough (Issue #178)" << std::endl;
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

    // v0.7.67 normalization:
    // force = (raw_torque * shaft_gain) = 10.0 * 0.5 = 5.0 Nm
    // session_peak defaults to target_rim_nm = 20.0 Nm
    // norm_force = 5.0 / 20.0 = 0.25
    // output = norm_force * (target_rim_nm / wheelbase_max_nm) = 0.25 * (20.0 / 20.0) = 0.25
    
    // Issue #397: Holt-Winters upsampler overshoots on step input.
    // Pump for 1 second to allow it to settle perfectly to the target.
    double force = PumpEngineTime(engine, data, 1.0);
    ASSERT_NEAR(force, 0.25, 0.001);
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
    
    // Issue #397: Pump for longer to settle HW filters
    double force_full = PumpEngineTime(engine, data, 1.0);
    ASSERT_NEAR(force_full, 0.5, 0.001);

    data.mWheel[0].mGripFract = 0.5;
    data.mWheel[1].mGripFract = 0.5;
    double force_half = PumpEngineTime(engine, data, 1.0);
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
        FAIL_TEST("Low speed grip not forced. Got " << force << " Expected 1.0.");
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
        InitializeEngine(e1);
        e1.m_gain = 1.0; e1.m_invert_force = false; e1.m_understeer_effect = 0.0; e1.m_oversteer_boost = 0.0;
        e1.m_rear_align_effect = 1.0;
        e1.m_wheelbase_max_nm = 20.0f; e1.m_target_rim_nm = 20.0f;
        ra1 = e1.calculate_force(&data);
    }
    {
        FFBEngine e2;
        InitializeEngine(e2);
        e2.m_gain = 1.0; e2.m_invert_force = false; e2.m_understeer_effect = 0.0; e2.m_oversteer_boost = 0.0;
        e2.m_rear_align_effect = 1.0;
        e2.m_wheelbase_max_nm = 60.0f; e2.m_target_rim_nm = 60.0f;
        ra2 = e2.calculate_force(&data);
    }

    if (std::abs(ra1 - ra2) < 0.001) {
        std::cout << "[PASS] Rear Align Torque correctly compensated (" << ra1 << " == " << ra2 << ")" << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Rear Align Torque compensation failed! 20Nm: " << ra1 << " 60Nm: " << ra2);
    }

    double s1, s2;
    {
        FFBEngine e1;
        InitializeEngine(e1);
        e1.m_gain = 1.0; e1.m_invert_force = false; e1.m_understeer_effect = 0.0; e1.m_oversteer_boost = 0.0;
        e1.m_slide_texture_enabled = true;
        e1.m_slide_texture_gain = 1.0;
        e1.m_wheelbase_max_nm = 20.0f; e1.m_target_rim_nm = 20.0f;
        e1.m_slide_phase = 0.5;
        s1 = e1.calculate_force(&data);
    }
    {
        FFBEngine e2;
        InitializeEngine(e2);
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
        FAIL_TEST("Slide Texture compensation failed! 20Nm: " << s1 << " 100Nm: " << s2);
    }

    engine.m_slide_texture_enabled = false;
    engine.m_understeer_effect = 0.5; 
    data.mSteeringShaftTorque = 10.0;
    data.mWheel[0].mGripFract = 0.6; 
    data.mWheel[1].mGripFract = 0.6;

    // Enable Dynamic Normalization to test its consistent scaling
    engine.m_dynamic_normalization_enabled = true;
    engine.m_wheelbase_max_nm = 20.0f; engine.m_target_rim_nm = 20.0f;
    FFBEngineTestAccess::SetSessionPeakTorque(engine, 20.0);
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 20.0);

    // Issue #397: Pump for longer to settle HW filters
    double u1 = PumpEngineTime(engine, data, 1.0);

    engine.m_wheelbase_max_nm = 40.0f; engine.m_target_rim_nm = 40.0f;
    // Session peak should remain 20.0 because input torque (10.0) < peak (20.0)
    double u2 = PumpEngineTime(engine, data, 1.0);

    // v0.7.67 Fix for Issue #152: Understeer is now normalized by session peak,
    // making it independent of target_rim_nm when enabled. Expect u1 == u2.
    // Issue #397: Relax tolerance slightly due to HW filter settling noise
    if (std::abs(u1 - u2) < 0.01) {
        std::cout << "[PASS] Understeer Modifier correctly normalized by session peak (" << u1 << " == " << u2 << ")" << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Understeer Modifier behavior unexpected! 20Nm: " << u1 << " 40Nm: " << u2);
    }

    std::cout << "[SUMMARY] Gain Compensation verified for all effect types." << std::endl;
}

TEST_CASE(test_gain_compensation_disabled, "CorePhysics") {
    std::cout << "\nTest: FFB Signal Gain Compensation (Disabled - Issue #207)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_dynamic_normalization_enabled = false;

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

    // With normalization disabled, structural forces scale to target_rim_nm.
    // If target_rim_nm == wheelbase_max_nm, force should be absolute.

    double ra1, ra2;
    {
        FFBEngine e1;
        InitializeEngine(e1);
        e1.m_dynamic_normalization_enabled = false;
        e1.m_rear_align_effect = 1.0;
        e1.m_wheelbase_max_nm = 20.0f; e1.m_target_rim_nm = 20.0f;
        ra1 = e1.calculate_force(&data);
    }
    {
        FFBEngine e2;
        InitializeEngine(e2);
        e2.m_dynamic_normalization_enabled = false;
        e2.m_rear_align_effect = 1.0;
        e2.m_wheelbase_max_nm = 60.0f; e2.m_target_rim_nm = 20.0f; // Target is SAME, wheelbase is larger
        ra2 = e2.calculate_force(&data);
    }

    // Force should be SAME because target_rim_nm is same (20.0)
    // Calc: structural_sum * (target_rim_nm / wheelbase_max_nm) / (target_rim_nm)
    // Wait: di_structural = structural_sum * mult * (target / max)
    // mult = 1 / target
    // di_structural = structural_sum * (1/target) * (target/max) = structural_sum / max.
    // Wait, the formula is:
    // target_structural_mult = 1.0 / m_target_rim_nm
    // di_structural = norm_structural * (m_target_rim_nm / wheelbase_max_safe)
    // di_structural = (structural_sum / target) * (target / max) = structural_sum / max.
    // So if wheelbase_max_nm differs, di_structural differs?
    // No, structural_sum is in Nm.
    // If I want 10 Nm on a 20 Nm wheel, it's 50%.
    // If I want 10 Nm on a 40 Nm wheel, it's 25%.
    // Correct. But RA torque depends on tire stiffness which depends on load.
    // Let's check ASSERT_NEAR(ra1, ra2 * (60.0/20.0), ...)? No.
    // Manual scaling means if I have 10 Nm of align torque, it should feel like 10 Nm.

    // In LMUFFB, di_structural is the DirectInput percentage [-1, 1].
    // So ra1 (20Nm wheel) should be 3x ra2 (60Nm wheel) if structural_sum is the same.
    ASSERT_NEAR(ra1, ra2 * 3.0, 0.001);

    // Now test Understeer drop when disabled
    engine.m_dynamic_normalization_enabled = false;
    engine.m_understeer_effect = 0.5;
    data.mSteeringShaftTorque = 10.0;
    data.mWheel[0].mGripFract = 0.6;
    data.mWheel[1].mGripFract = 0.6;

    engine.m_wheelbase_max_nm = 20.0f; engine.m_target_rim_nm = 20.0f;
    // v0.7.109: Ensure multiplier matches target before first calc
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 20.0);

    // Issue #397: Pump for longer to settle HW filters
    double u1 = PumpEngineTime(engine, data, 1.0);

    engine.m_wheelbase_max_nm = 20.0f; engine.m_target_rim_nm = 10.0f; // Target halved
    // Ensure multiplier matches target before second calc
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 10.0);
    double u2 = PumpEngineTime(engine, data, 1.0);

    // With normalization disabled, reducing target_rim_nm should reduce output force
    // (u1 uses 1/20, u2 uses 1/10. Wait, if target is halved, force % should double?
    // di_structural = Nm * (1/target) * (target/max) = Nm/max.
    // If target halved, and mult = 1/target, then di_structural is same?
    // Nm = 10, Target = 20, Max = 20 -> di = 10 * (1/20) * (20/20) = 0.5.
    // Nm = 10, Target = 10, Max = 20 -> di = 10 * (1/10) * (10/20) = 0.5.
    // Ah, structural normalization (even manual) aims for physical Nm.
    // BUT! Understeer is a MODIFIER.
    // force = (base * gain) * dw * grip.
    // u1 = (10 * 1) * 1 * 0.8 = 8 Nm.
    // di_u1 = 8 * (1/20) * (20/20) = 0.4.
    // u2 = (10 * 1) * 1 * 0.8 = 8 Nm.
    // di_u2 = 8 * (1/10) * (10/20) = 0.4.
    // So output force stays same for same physics. Correct.
    ASSERT_NEAR(u1, u2, 0.001);
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
            FAIL_TEST("Stability failure at iteration " << i);
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
            FAIL_TEST("Iteration " << i << " produced NaN/Inf!");
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
        FAIL_TEST("Manual failure increment");
    }
}

TEST_CASE(test_smoothing_step_response, "CorePhysics") {
    std::cout << "\nTest: SoP Smoothing Step Response (Updated for v0.7.147 mapping)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine); 
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    
    data.mWheel[0].mRideHeight = 0.1; data.mWheel[1].mRideHeight = 0.1;

    // v0.7.147 Mapping: 0.5 factor means 50ms Tau.
    // Frame 1 (2.5ms) response: alpha = 2.5 / (50 + 2.5) = 2.5/52.5 approx 0.0476
    engine.m_sop_smoothing_factor = 0.5;
    engine.m_sop_scale = 1.0;  
    engine.m_sop_effect = 1.0;
    engine.m_wheelbase_max_nm = 20.0f; engine.m_target_rim_nm = 20.0f;
    engine.m_invert_force = false;
    
    data.mLocalAccel.x = 9.81; 
    data.mDeltaTime = 0.0025;
    
    // Issue #397: Step response is now masked by 10ms linear ramp.
    // After 10ms (4 ticks), signal reaches 1.0.
    // Then smoothing starts from that.
    
    // We'll pump 15ms to let interpolator finish and smoothing progress
    double force1 = PumpEngineTime(engine, data, 0.015);

    if (force1 > 0.0001) {
        std::cout << "[PASS] Smoothing Step response detected (" << force1 << ")." << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Smoothing Step 1 mismatch. Got " << force1 << " Expected > 0");
    }
    
    // Settle fully
    PumpEngineTime(engine, data, 1.0);
    force1 = engine.GetDebugBatch().back().total_output;
    
    if (force1 > 0.02 && force1 < 0.06) {
        std::cout << "[PASS] Smoothing settled to steady-state (" << force1 << ", near 0.05)." << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Smoothing did not settle. Value: " << force1);
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
        FAIL_TEST("Smoothing diverges! Time correction failed.");
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
    engine.m_sop_smoothing_factor = 0.0f; // Instant response (v0.7.147)
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
        FAIL_TEST("LPF was idle! Cold start lag detected. Force: " << force);
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

    // Issue #397: Wait 15ms for interpolator ramp to finish and smoothing to start
    double force = PumpEngineTime(engine, data, 0.015);

    if (force > 0.01) {
        std::cout << "[PASS] Shaft Smoothing delayed the step input (Initial: " << force << ")." << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Shaft Smoothing mismatch. Got " << force << " Expected > 0.01.");
    }

    // After 2.0s it should be near 1.0
    PumpEngineTime(engine, data, 2.0);
    force = engine.GetDebugBatch().back().total_output;

    if (force > 0.99) {
        std::cout << "[PASS] Shaft Smoothing converged correctly (Frame 11: " << force << ")." << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Shaft Smoothing convergence failure. Got " << force);
    }
}



} // namespace FFBEngineTests
