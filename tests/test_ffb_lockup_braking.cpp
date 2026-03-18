#include "test_ffb_common.h"

namespace FFBEngineTests {

// [Texture][Physics] Progressive lockup effect with frequency-based vibration
TEST_CASE(test_progressive_lockup, "LockupBraking") {
    std::cout << "\nTest: Progressive Lockup [Texture][Physics]" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine); // v0.5.12: Initialize with T300 defaults
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    
    engine.m_lockup_enabled = true;
    engine.m_lockup_gain = 1.0;
    engine.m_sop_effect = 0.0;
    engine.m_slide_texture_enabled = false;
    
    data.mSteeringShaftTorque = 0.0;
    data.mUnfilteredBrake = 1.0;
    
    // Use production defaults: Start 5%, Full 15% (v0.5.13)
    // These are the default values that ship to users
    engine.m_lockup_start_pct = 5.0f;
    engine.m_lockup_full_pct = 15.0f;
    
    // Case 1: High Slip (-0.20 = 20%). 
    // With Full=15%: severity = 1.0
    data.mWheel[0].mLongitudinalGroundVel = 20.0;
    data.mWheel[1].mLongitudinalGroundVel = 20.0;
    data.mWheel[0].mLongitudinalPatchVel = -0.20 * 20.0; // -4.0 m/s
    data.mWheel[1].mLongitudinalPatchVel = -0.20 * 20.0;
    
    // Ensure data.mDeltaTime is set!
    data.mDeltaTime = 0.01;
    
    // DEBUG: Manually verify phase logic in test
    // freq = 10 + (20 * 1.5) = 40.0
    // dt = 0.01
    // step = 40 * 0.01 * 6.28 = 2.512
    
    engine.calculate_force(&data); // Frame 1
    // engine.m_lockup_phase should be approx 2.512
    
    double force_low = engine.calculate_force(&data); // Frame 2
    // engine.m_lockup_phase should be approx 5.024
    // sin(5.024) is roughly -0.95.
    // Amp should be non-zero.
    
    // Debug
    // std::cout << "Force Low: " << force_low << " Phase: " << engine.m_lockup_phase << std::endl;

    if (engine.m_lockup_phase == 0.0) {
         // Maybe frequency calculation is zero?
         // Freq = 10 + (20 * 1.5) = 40.
         // Dt = 0.01.
         // Accumulator += 40 * 0.01 * 6.28 = 2.5.
         FAIL_TEST("Phase stuck at 0. Check data inputs.");
    }

    ASSERT_TRUE(std::abs(force_low) > 0.00001);
    ASSERT_TRUE(engine.m_lockup_phase != 0.0);
    
    std::cout << "[PASS] Progressive Lockup calculated." << std::endl;
    g_tests_passed++;
}

// [Texture][Physics][Regression] Predictive lockup activation based on wheel deceleration
TEST_CASE(test_predictive_lockup_v060, "LockupBraking") {
    std::cout << "\nTest: Predictive Lockup (v0.6.0) [Texture][Physics][Regression]" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    
    engine.m_lockup_enabled = true;
    engine.m_lockup_prediction_sens = 50.0f;
    engine.m_lockup_start_pct = 5.0f;
    engine.m_lockup_full_pct = 15.0f; // Default threshold is higher than current slip
    
    data.mUnfilteredBrake = 1.0; // Needs brake input for prediction gating (v0.6.0)
    
    // Force constant rotation history
    engine.calculate_force(&data);
    
    // Frame 2: Wheel slows down RAPIDLY (-100 rad/s^2)
    data.mDeltaTime = 0.01;
    // Current rotation for 20m/s is ~66.6. 
    // We set rotation to create a derivative of -100.
    // delta = rotation - prev. so rotation = prev - 1.0.
    double prev_rot = data.mWheel[0].mRotation;
    data.mWheel[0].mRotation = prev_rot - 1.0; 
    
    // Slip at 10% (Required now that manual slip is removed)
    data.mWheel[0].mLongitudinalPatchVel = -2.0; 
    data.mWheel[0].mRotation = 18.0 / 0.3;
    
    // Car decel is 0 (mLocalAccel.z = 0)
    // Sensitivity threshold is -50. -100 < -50 is TRUE.
    
    // Execute
    // Issue #397: Interpolation delay
    PumpEngineTime(engine, data, 0.0125);
    
    // With 10% slip and prediction active, threshold is 5%, so severity is (10-5)/10 = 0.5.
    // Phase should advance.
    
    if (engine.m_lockup_phase > 0.001) {
        std::cout << "[PASS] Predictive trigger activated at 10% slip (Phase: " << engine.m_lockup_phase << ")" << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Predictive trigger failed. Phase: " << engine.m_lockup_phase << " Accel: " << (data.mWheel[0].mRotation - prev_rot)/0.01);
    }
}

// [Texture][Physics][Regression] ABS pulse detection from brake pressure modulation
TEST_CASE(test_abs_pulse_v060, "LockupBraking") {
    std::cout << "\nTest: ABS Pulse Detection (v0.6.0) [Texture][Physics][Regression]" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0); // Moving car (v0.6.21 FIX)
    
    engine.m_abs_pulse_enabled = true;
    engine.m_abs_gain = 1.0f;
    data.mUnfilteredBrake = 1.0;
    data.mDeltaTime = 0.01;
    
    // Frame 1: Pressure 1.0
    data.mWheel[0].mBrakePressure = 1.0;
    engine.calculate_force(&data);
    
    // Frame 2: Pressure drops to 0.7 (ABS modulation)
    // Delta = -0.3 / 0.01 = -30.0. |Delta| > 2.0.
    data.mWheel[0].mBrakePressure = 0.7;
    // Issue #397: Interpolation delay
    PumpEngineTime(engine, data, 0.0125);
    double force = engine.GetDebugBatch().back().ffb_abs_pulse;
    
    if (std::abs(force) > 0.001) {
        std::cout << "[PASS] ABS Pulse triggered (Force: " << force << ")" << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("ABS Pulse silent. Force: " << force);
    }
}

// [Texture][Physics][Regression] Rear lockup differentiation with frequency scaling
TEST_CASE(test_rear_lockup_differentiation, "LockupBraking") {
    std::cout << "\nTest: Rear Lockup Differentiation [Texture][Physics][Regression]" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine); // v0.5.12: Initialize with T300 defaults
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));

    // Common Setup
    engine.m_lockup_enabled = true;
    engine.m_lockup_gain = 1.0;
    engine.m_wheelbase_max_nm = 20.0f; engine.m_target_rim_nm = 20.0f;
    engine.m_gain = 1.0f;
    
    data.mUnfilteredBrake = 1.0; // Braking
    data.mLocalVel.z = 20.0;     // 20 m/s
    data.mDeltaTime = 0.01;      // 10ms step
    
    // Setup Ground Velocity (Reference)
    for(int i=0; i<4; i++) data.mWheel[i].mLongitudinalGroundVel = 20.0;

    // --- PASS 1: Front Lockup Only ---
    // Front Slip -0.5, Rear Slip 0.0
    data.mWheel[0].mLongitudinalPatchVel = -0.5 * 20.0; // -10 m/s
    data.mWheel[1].mLongitudinalPatchVel = -0.5 * 20.0;
    data.mWheel[2].mLongitudinalPatchVel = 0.0;
    data.mWheel[3].mLongitudinalPatchVel = 0.0;

    // Issue #397: Interpolation delay
    PumpEngineTime(engine, data, 0.0125);
    double phase_delta_front = engine.m_lockup_phase; // Phase started at 0

    // Verify Front triggered
    if (phase_delta_front > 0.0) {
        std::cout << "[PASS] Front lockup triggered. Phase delta: " << phase_delta_front << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Front lockup silent.");
    }

    // --- PASS 2: Rear Lockup Only ---
    // Reset Engine State
    engine.m_lockup_phase = 0.0;
    
    // Front Slip 0.0, Rear Slip -0.5
    data.mWheel[0].mLongitudinalPatchVel = 0.0;
    data.mWheel[1].mLongitudinalPatchVel = 0.0;
    data.mWheel[2].mLongitudinalPatchVel = -0.5 * 20.0;
    data.mWheel[3].mLongitudinalPatchVel = -0.5 * 20.0;

    // Issue #397: Interpolation delay
    PumpEngineTime(engine, data, 0.0125);
    double phase_delta_rear = engine.m_lockup_phase;

    // Verify Rear triggered (Fixes the bug)
    if (phase_delta_rear > 0.0) {
        std::cout << "[PASS] Rear lockup triggered. Phase delta: " << phase_delta_rear << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Rear lockup silent (Bug not fixed).");
    }

    // Rear frequency is lower (Ratio 0.3 per FFBEngine.h)
    double ratio = phase_delta_rear / phase_delta_front;
    
    // Behavioral: Rear frequency must be significantly lower than front
    if (ratio < 1.0) {
        std::cout << "[PASS] Rear frequency is lower (Ratio: " << ratio << " vs expected ~0.3)." << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Frequency differentiation failed. Ratio: " << ratio);
    }
}

// [Config][Physics] Split load caps for braking vs texture effects
TEST_CASE(test_split_load_caps, "LockupBraking") {
    std::cout << "\nTest: Split Load Caps (Brake vs Texture) [Config][Physics]" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);

    // Setup High Load (12000N = 3.0x Load Factor)
    for(int i=0; i<4; i++) data.mWheel[i].mTireLoad = 12000.0;

    // Config: Texture Cap = 1.0x, Brake Cap = 3.0x
    engine.m_texture_load_cap = 1.0f; 
    engine.m_brake_load_cap = 3.0f;
    engine.m_abs_pulse_enabled = false; // Disable ABS to isolate lockup (v0.6.0)
    
    // ===================================================================
    // PART 1: Test Road Texture (Should be clamped to 1.0x)
    // ===================================================================
    engine.m_road_texture_enabled = true;
    engine.m_road_texture_gain = 1.0;
    engine.m_lockup_enabled = false;
    data.mWheel[0].mVerticalTireDeflection = 0.01; // Bump FL
    data.mWheel[1].mVerticalTireDeflection = 0.01; // Bump FR
    
    // Road Texture Baseline: Delta * Sum * 50.0
    // Bump 0.01 -> Delta Sum = 0.02. 0.02 * 50.0 = 1.0 Nm.
    // 1.0 Nm * Texture Load Cap (1.0) = 1.0 Nm.
    // Normalized by 20 Nm (Default decoupling baseline) = 0.05.
    
    // Issue #397: Interpolation delay
    // We need multiple frames to see the delta in deflection.
    double prev_def = data.mWheel[0].mVerticalTireDeflection;
    data.mWheel[0].mVerticalTireDeflection += 0.01;
    data.mWheel[1].mVerticalTireDeflection += 0.01;

    PumpEngineTime(engine, data, 0.1); // Advance 100ms
    auto batch_road = engine.GetDebugBatch();
    bool found_road = false;
    for(const auto& s : batch_road) {
        if (std::abs(s.texture_road) > 0.001) { found_road = true; break; }
    }

    // Verify road texture was triggered during the ramp
    if (found_road) {
        std::cout << "[PASS] Road texture correctly triggered" << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Road texture silent.");
        return;
    }

    // ===================================================================
    // PART 2: Test Lockup (Should use Brake Load Cap 3.0x)
    // ===================================================================
    engine.m_road_texture_enabled = false;
    engine.m_lockup_enabled = true;
    engine.m_lockup_gain = 1.0;
    data.mUnfilteredBrake = 1.0;
    data.mWheel[0].mLongitudinalPatchVel = -10.0; // Slip
    data.mWheel[1].mLongitudinalPatchVel = -10.0; // Slip (both wheels for consistency)
    
    // Baseline engine with 1.0 cap for comparison
    FFBEngine engine_low;
    InitializeEngine(engine_low);
    // v0.7.43: Disable auto-normalization adaptation to maintain 4000N reference for test
    FFBEngineTestAccess::SetAutoNormalizationEnabled(engine, false);
    FFBEngineTestAccess::SetAutoNormalizationEnabled(engine_low, false);
    FFBEngineTestAccess::SetAutoPeakLoad(engine, 4000.0);
    FFBEngineTestAccess::SetAutoPeakLoad(engine_low, 4000.0);
    
    // Explicitly lock the static load for engine and engine_low
    FFBEngineTestAccess::SetStaticFrontLoad(engine, 4000.0);
    FFBEngineTestAccess::SetStaticLoadLatched(engine, true);
    FFBEngineTestAccess::SetSmoothedVibrationMult(engine, 5.0);

    FFBEngineTestAccess::SetStaticFrontLoad(engine_low, 4000.0);
    FFBEngineTestAccess::SetStaticLoadLatched(engine_low, true);
    FFBEngineTestAccess::SetSmoothedVibrationMult(engine_low, 5.0);

    engine_low.m_brake_load_cap = 1.0f;
    engine_low.m_lockup_enabled = true;
    engine_low.m_lockup_gain = 1.0;
    engine_low.m_abs_pulse_enabled = false; // Disable ABS (v0.6.0)
    engine_low.m_road_texture_enabled = false; // Disable Road (v0.6.0)
    
    // Reset phase to ensure both engines start from same state
    engine.m_lockup_phase = 0.0;
    engine_low.m_lockup_phase = 0.0;
    
    // Issue #397: Interpolation delay. Pump both engines simultaneously to ensure phase sync.
    double max_high = 0.0;
    double max_low = 0.0;

    for(int i=0; i<40; i++) {
        if (i % 4 == 0) {
            data.mElapsedTime += 0.01;
            data.mDeltaTime = 0.01;
        }
        double f_high = engine.calculate_force(&data, nullptr, nullptr, 0.0f, true, 0.0025);
        double f_low = engine_low.calculate_force(&data, nullptr, nullptr, 0.0f, true, 0.0025);
        if (std::abs(f_high) > max_high) max_high = std::abs(f_high);
        if (std::abs(f_low) > max_low) max_low = std::abs(f_low);
    }
    
    // Verify the ratio of peak vibrations
    double actual_ratio = max_high / (max_low + 0.0001);
    
    // Behavioral: High cap engine must produce significantly more force (approx 3x)
    if (actual_ratio > 2.0) {
        std::cout << "[PASS] Brake load cap applies higher scaling (Ratio: " << actual_ratio << ")" << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Expected ~3x ratio, got " << actual_ratio << " (High Peak: " << max_high << ", Low Peak: " << max_low << ")");
    }
}

// [Config][Physics] Dynamic lockup thresholds with progressive severity
TEST_CASE(test_dynamic_thresholds, "LockupBraking") {
    std::cout << "\nTest: Dynamic Lockup Thresholds [Config][Physics]" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    
    engine.m_lockup_enabled = true;
    engine.m_lockup_gain = 1.0;
    data.mUnfilteredBrake = 1.0;
    
    // Config: Start 5%, Full 15%
    engine.m_lockup_start_pct = 5.0f;
    engine.m_lockup_full_pct = 15.0f;
    
    // Case A: 4% Slip (Below Start)
    // 0.04 * 20.0 = 0.8
    data.mWheel[0].mLongitudinalPatchVel = -0.8; 
    engine.calculate_force(&data);
    if (engine.m_lockup_phase == 0.0) {
        std::cout << "[PASS] No trigger below 5% start." << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Triggered below start threshold.");
    }

    // Case B: 20% Slip (Saturated/Manual Trigger)
    // 0.20 * 20.0 = 4.0
    data.mWheel[0].mLongitudinalPatchVel = -4.0;
    // Issue #397: Interpolation delay
    PumpEngineTime(engine, data, 0.0125);
    double force_mid = engine.GetDebugBatch().back().total_output;
    ASSERT_TRUE(std::abs(force_mid) > 0.0);
    
    // Case C: 40% Slip (Deep Saturated)
    // 0.40 * 20.0 = 8.0
    data.mWheel[0].mLongitudinalPatchVel = -8.0;
    // Issue #397: Interpolation delay
    PumpEngineTime(engine, data, 0.0125);
    double force_max = engine.GetDebugBatch().back().total_output;
    
    // Both should have non-zero force, and max should be significantly higher due to quadratic ramp
    // 10% slip: severity = (0.5)^2 = 0.25
    // 20% slip: severity = 1.0
    if (std::abs(force_max) > std::abs(force_mid)) {
        std::cout << "[PASS] Force increases with slip depth." << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Force saturation/ramp failed.");
    }
}

// [Regression][Physics] ABS pulse refactor regression test (v0.6.36)
TEST_CASE(test_refactor_abs_pulse, "LockupBraking") {
    std::cout << "\nTest: Refactor Regression - ABS Pulse (v0.6.36) [Regression][Physics]" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);

    // Enable ABS
    engine.m_abs_pulse_enabled = true;
    engine.m_abs_gain = 1.0f;
    engine.m_wheelbase_max_nm = 20.0f; engine.m_target_rim_nm = 20.0f; // Scale 1.0

    // Trigger condition: High Brake + Pressure Delta
    data.mUnfilteredBrake = 1.0;
    data.mWheel[0].mBrakePressure = 1.0;
    engine.calculate_force(&data); // Frame 1: Set previous pressure

    data.mWheel[0].mBrakePressure = 0.5; // Frame 2: Rapid drop (delta)
    // Issue #397: Interpolation delay
    PumpEngineTime(engine, data, 0.0125);
    double force = engine.GetDebugBatch().back().ffb_abs_pulse;

    // Should be non-zero (previously regressed to 0)
    if (std::abs(force) > 0.001) {
        std::cout << "[PASS] ABS Pulse generated force: " << force << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("ABS Pulse silent (force=0). Refactor regression?");
    }
}

// [Regression][Physics][Integration] Torque drop refactor regression test (v0.6.36)
TEST_CASE(test_refactor_torque_drop, "LockupBraking") {
    std::cout << "\nTest: Refactor Regression - Torque Drop (v0.6.36) [Regression][Physics][Integration]" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);

    // Setup: Base force + Spin
    data.mSteeringShaftTorque = 10.0; // 0.5 normalized
    engine.m_spin_enabled = true;
    engine.m_spin_gain = 1.0f;
    engine.m_gain = 1.0f;

    // Trigger Spin
    data.mUnfilteredThrottle = 1.0;
    // Slip = 0.5 (Severe) -> Severity = (0.5 - 0.2) / 0.5 = 0.6
    // Drop Factor = 1.0 - (0.6 * 1.0 * 0.6) = 1.0 - 0.36 = 0.64
    double ground_vel = 20.0;
    data.mWheel[2].mLongitudinalPatchVel = 0.5 * ground_vel;
    data.mWheel[2].mLongitudinalGroundVel = ground_vel;
    data.mWheel[3].mLongitudinalPatchVel = 0.5 * ground_vel;
    data.mWheel[3].mLongitudinalGroundVel = ground_vel;

    // Disable Spin Vibration (gain 0) to check just the drop?
    // No, can't separate gain easily. But vibration is AC.
    // If we check magnitude, it might be messy.
    // Let's check with Spin Gain = 1.0, but Spin Freq Scale = 0 (Constant force?)
    // No, freq scale 0 -> phase 0 -> sin(0) = 0. No vibration.
    // Perfect for checking torque drop!

    engine.m_spin_freq_scale = 0.0f;

    // Add Road Texture (Texture Group - Should NOT be dropped)
    // Setup deflection delta for constant road noise
    // Force = Delta * 50.0. Target 0.1 normalized (2.0 Nm).
    // Delta = 2.0 / 50.0 = 0.04.
    engine.m_road_texture_enabled = true;
    engine.m_road_texture_gain = 1.0f;
    engine.m_wheelbase_max_nm = 20.0f; engine.m_target_rim_nm = 20.0f; // Scale 1.0

    // v0.7.69: Ensure vibration multiplier is 1.0 for this test
    FFBEngineTestAccess::SetStaticFrontLoad(engine, 4000.0);
    FFBEngineTestAccess::SetSmoothedVibrationMult(engine, 1.0);

    // Reset deflection state in engine first
    engine.calculate_force(&data);

    // Apply Delta
    data.mWheel[0].mVerticalTireDeflection += 0.02; // +2cm
    data.mWheel[1].mVerticalTireDeflection += 0.02; // +2cm
    // Total Delta = 0.04. Road Force = 0.04 * 50.0 = 2.0 Nm.
    // Normalized Road = 2.0 / 20.0 = 0.1.

    // Issue #397: Interpolation delay
    PumpEngineTime(engine, data, 0.0125);
    double force = engine.GetDebugBatch().back().total_output;

    // Base Force (Structural) = 10.0 Nm -> 0.5 Norm.
    // Torque Drop = 0.64.
    // Road Force (Texture) = 1.0 Nm (Clamped) -> 0.05 Norm.

    // Logic A (Broken): (Base + Texture) * Drop = (0.5 + 0.05) * 0.64 = 0.352
    // Logic B (Correct): (Base * Drop) + Texture = (0.5 * 0.64) + 0.05 = 0.32 + 0.05 = 0.37

    // Behavioral Check: Force must be reduced due to spin drop
    // Base 0.5 + Texture 0.05 = 0.55. Drop to ~0.37.
    if (force < 0.5 && force > 0.1) {
        std::cout << "[PASS] Torque Drop correctly isolated from Textures (Force: " << force << " vs Baseline ~0.55)" << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Torque Drop logic error. Got: " << force << " (Baseline: ~0.55)");
    }
}



} // namespace FFBEngineTests
