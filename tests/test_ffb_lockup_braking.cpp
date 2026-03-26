#include "test_ffb_common.h"

namespace FFBEngineTests {

// [Texture][Physics] Progressive lockup effect with frequency-based vibration
TEST_CASE(test_progressive_lockup, "LockupBraking") {
    std::cout << "\nTest: Progressive Lockup [Texture][Physics]" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine); // v0.5.12: Initialize with T300 defaults
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    
    engine.m_braking.lockup_enabled = true;
    engine.m_braking.lockup_gain = 1.0;
    engine.m_rear_axle.sop_effect = 0.0;
    engine.m_vibration.slide_enabled = false;
    
    data.mSteeringShaftTorque = 0.0;
    data.mUnfilteredBrake = 1.0;
    
    // Use production defaults: Start 5%, Full 15% (v0.5.13)
    // These are the default values that ship to users
    engine.m_braking.lockup_start_pct = 5.0f;
    engine.m_braking.lockup_full_pct = 15.0f;
    
    // Case 1: High Slip (-0.20 = 20%). 
    // With Full=15%: severity = 1.0
    data.mWheel[WHEEL_FL].mLongitudinalGroundVel = 20.0;
    data.mWheel[WHEEL_FR].mLongitudinalGroundVel = 20.0;
    data.mWheel[WHEEL_FL].mLongitudinalPatchVel = -0.20 * 20.0; // -4.0 m/s
    data.mWheel[WHEEL_FR].mLongitudinalPatchVel = -0.20 * 20.0;
    
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
    
    engine.m_braking.lockup_enabled = true;
    engine.m_braking.lockup_prediction_sens = 50.0f;
    engine.m_braking.lockup_start_pct = 5.0f;
    engine.m_braking.lockup_full_pct = 15.0f; // Default threshold is higher than current slip
    
    data.mUnfilteredBrake = 1.0; // Needs brake input for prediction gating (v0.6.0)
    
    // Force constant rotation history
    PumpEngineTime(engine, data, 0.0125);
    
    // Frame 2: Wheel slows down RAPIDLY (-100 rad/s^2)
    data.mDeltaTime = 0.01;
    // Current rotation for 20m/s is ~66.6. 
    // We set rotation to create a derivative of -100.
    // delta = rotation - prev. so rotation = prev - 1.0.
    double prev_rot = data.mWheel[WHEEL_FL].mRotation;
    data.mWheel[WHEEL_FL].mRotation = prev_rot - 1.0; 
    
    // Slip at 10% (Required now that manual slip is removed)
    data.mWheel[WHEEL_FL].mLongitudinalPatchVel = -2.0; 
    data.mWheel[WHEEL_FL].mRotation = 18.0 / 0.3;
    
    // Car decel is 0 (mLocalAccel.z = 0)
    // Sensitivity threshold is -50. -100 < -50 is TRUE.
    
    // Execute
    PumpEngineTime(engine, data, 0.0125);
    
    // With 10% slip and prediction active, threshold is 5%, so severity is (10-5)/10 = 0.5.
    // Phase should advance.
    
    if (engine.m_lockup_phase > 0.001) {
        std::cout << "[PASS] Predictive trigger activated at 10% slip (Phase: " << engine.m_lockup_phase << ")" << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Predictive trigger failed. Phase: " << engine.m_lockup_phase << " Accel: " << (data.mWheel[WHEEL_FL].mRotation - prev_rot)/0.01);
    }
}

// [Texture][Physics][Regression] ABS pulse detection from brake pressure modulation
TEST_CASE(test_abs_pulse_v060, "LockupBraking") {
    std::cout << "\nTest: ABS Pulse Detection (v0.6.0) [Texture][Physics][Regression]" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0); // Moving car (v0.6.21 FIX)
    
    engine.m_braking.abs_pulse_enabled = true;
    engine.m_braking.abs_gain = 1.0f;
    data.mUnfilteredBrake = 1.0;
    data.mDeltaTime = 0.01;
    
    // Frame 1: Pressure 1.0
    data.mWheel[WHEEL_FL].mBrakePressure = 1.0;
    PumpEngineTime(engine, data, 0.0125);
    
    // Frame 2: Pressure drops to 0.7 (ABS modulation)
    // Delta = -0.3 / 0.01 = -30.0. |Delta| > 2.0.
    data.mWheel[WHEEL_FL].mBrakePressure = 0.7;
    double force = PumpEngineTime(engine, data, 0.0125);
    
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
    engine.m_braking.lockup_enabled = true;
    engine.m_braking.lockup_gain = 1.0;
    engine.m_general.wheelbase_max_nm = 20.0f; engine.m_general.target_rim_nm = 20.0f;
    engine.m_general.gain = 1.0f;
    
    data.mUnfilteredBrake = 1.0; // Braking
    data.mLocalVel.z = 20.0;     // 20 m/s
    data.mDeltaTime = 0.01;      // 10ms step
    
    // Setup Ground Velocity (Reference)
    for (int i = 0; i < NUM_WHEELS; i++) data.mWheel[i].mLongitudinalGroundVel = 20.0;

    // --- PASS 1: Front Lockup Only ---
    TelemInfoV01 data1 = CreateBasicTestTelemetry(20.0);
    data1.mUnfilteredBrake = 1.0;
    // Front Slip -0.5, Rear Slip 0.0
    data1.mWheel[WHEEL_FL].mLongitudinalPatchVel = -0.5 * 20.0; // -10 m/s
    data1.mWheel[WHEEL_FR].mLongitudinalPatchVel = -0.5 * 20.0;
    data1.mWheel[WHEEL_RL].mLongitudinalPatchVel = 0.0;
    data1.mWheel[WHEEL_RR].mLongitudinalPatchVel = 0.0;

    // Seed
    InitializeEngine(engine);
    engine.m_braking.lockup_enabled = true;
    engine.m_braking.lockup_gain = 1.0;

    // Issue #397: Flush and Measure
    PumpEngineTime(engine, data1, 0.05);

    auto accumulate_phase = [&](FFBEngine& eng, TelemInfoV01& d, double duration) {
        double total_p = 0.0;
        int ticks = (int)std::ceil(duration / 0.0025);
        for(int i=0; i<ticks; i++) {
            double prev_p = eng.m_lockup_phase;
            if (i % 4 == 0) { d.mElapsedTime += 0.01; d.mDeltaTime = 0.01; }
            eng.calculate_force(&d, nullptr, nullptr, 0.0f, true, 0.0025);
            double curr_p = eng.m_lockup_phase;
            double diff = curr_p - prev_p;
            if (diff < 0) diff += TWO_PI; // Handle wrap
            total_p += diff;
        }
        return total_p;
    };

    double phase_delta_front = accumulate_phase(engine, data1, 0.1);

    // Verify Front triggered
    if (phase_delta_front > 1.0) {
        std::cout << "[PASS] Front lockup triggered. Phase delta: " << phase_delta_front << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Front lockup silent or too slow. Delta: " << phase_delta_front);
    }

    // --- PASS 2: Rear Lockup Only ---
    TelemInfoV01 data2 = CreateBasicTestTelemetry(20.0);
    data2.mUnfilteredBrake = 1.0;
    // Reset Engine State (interpolators and phases)
    InitializeEngine(engine); // Proper reset
    engine.m_braking.lockup_enabled = true;
    engine.m_braking.lockup_gain = 1.0;
    engine.m_lockup_phase = 0.0;
    
    // Front Slip 0.0, Rear Slip -0.5
    data2.mWheel[WHEEL_FL].mLongitudinalPatchVel = 0.0;
    data2.mWheel[WHEEL_FR].mLongitudinalPatchVel = 0.0;
    data2.mWheel[WHEEL_RL].mLongitudinalPatchVel = -0.5 * 20.0;
    data2.mWheel[WHEEL_RR].mLongitudinalPatchVel = -0.5 * 20.0;

    // Issue #397: Flush and Measure
    PumpEngineTime(engine, data2, 0.05);
    double phase_delta_rear = accumulate_phase(engine, data2, 0.1);

    // Verify Rear triggered (Fixes the bug)
    if (phase_delta_rear > 0.5) {
        std::cout << "[PASS] Rear lockup triggered. Phase delta: " << phase_delta_rear << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Rear lockup silent (Bug not fixed). Delta: " << phase_delta_rear);
    }

    // Rear frequency is lower (Ratio 0.3 per FFBEngine.h)
    double ratio = phase_delta_rear / phase_delta_front;
    
    // Issue #397: Now that we use Flush and Measure, the ratio should be more accurate
    if (std::abs(ratio - 0.3) < 0.1) {
        std::cout << "[PASS] Rear frequency is lower (Ratio: " << ratio << " vs expected 0.3)." << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Frequency differentiation failed. Ratio: " << ratio << " (Expected ~0.3)");
    }
}

// [Config][Physics] Split load caps for braking vs texture effects
TEST_CASE(test_split_load_caps, "LockupBraking") {
    std::cout << "\nTest: Split Load Caps (Brake vs Texture) [Config][Physics]" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);

    // Setup High Load (12000N = 3.0x Load Factor)
    for (int i = 0; i < NUM_WHEELS; i++) data.mWheel[i].mTireLoad = 12000.0;

    // Config: Texture Cap = 1.0x, Brake Cap = 3.0x
    engine.m_vibration.texture_load_cap = 1.0f;
    engine.m_braking.brake_load_cap = 3.0f;
    engine.m_braking.abs_pulse_enabled = false; // Disable ABS to isolate lockup (v0.6.0)
    
    // ===================================================================
    // PART 1: Test Road Texture (Should be clamped to 1.0x)
    // ===================================================================
    engine.m_vibration.road_enabled = true;
    engine.m_vibration.road_gain = 1.0;
    engine.m_braking.lockup_enabled = false;
    data.mWheel[WHEEL_FL].mVerticalTireDeflection = 0.01; // Bump FL
    data.mWheel[WHEEL_FR].mVerticalTireDeflection = 0.01; // Bump FR
    
    // Road Texture Baseline: Delta * Sum * 50.0
    // Bump 0.01 -> Delta Sum = 0.02. 0.02 * 50.0 = 1.0 Nm.
    // 1.0 Nm * Texture Load Cap (1.0) = 1.0 Nm.
    // Normalized by 20 Nm (Default decoupling baseline) = 0.05.

    // Issue #397: Measure Road Texture DURING the 10ms interpolation ramp
    // Reset state to ensure clean stimulus
    data.mWheel[WHEEL_FL].mVerticalTireDeflection = 0.0;
    data.mWheel[WHEEL_FR].mVerticalTireDeflection = 0.0;
    PumpEngineSteadyState(engine, data);
    engine.GetDebugBatch();

    data.mWheel[WHEEL_FL].mVerticalTireDeflection = 0.01;
    data.mWheel[WHEEL_FR].mVerticalTireDeflection = 0.01;
    data.mElapsedTime += 0.01; // New frame

    double force_road = 0.0;
    for (int i = 0; i < NUM_WHEELS; i++) {
        engine.calculate_force(&data, nullptr, nullptr, 0.0f, true, 0.0025);
        auto b = engine.GetDebugBatch();
        if (!b.empty()) force_road = std::max(force_road, std::abs((double)b.back().texture_road));
    }

    // v0.7.200 (Issue #402): Normalized for TDI. 400Hz is now 4x stronger (matches 100Hz tuning)
    // With 10ms delay, 0.01 step over 4 ticks (2.5ms each) gives 0.0025 delta/tick (1.0 m/s velocity).
    // Road Force = (1.0 + 1.0) * 50.0 * 0.01 = 1.0 Nm.
    // Note: snap.texture_road is in Nm.
    if (std::abs(force_road - 1.0) < 0.05) {
        std::cout << "[PASS] Road texture correctly clamped to 1.0x (Force: " << force_road << ")" << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Road texture clamping failed. Expected ~1.0 Nm, got " << force_road);
        return;
    }

    // ===================================================================
    // PART 2: Test Lockup (Should use Brake Load Cap 3.0x)
    // ===================================================================
    engine.m_vibration.road_enabled = false;
    engine.m_braking.lockup_enabled = true;
    engine.m_braking.lockup_gain = 1.0;
    data.mUnfilteredBrake = 1.0;
    data.mWheel[WHEEL_FL].mLongitudinalPatchVel = -10.0; // Slip
    data.mWheel[WHEEL_FR].mLongitudinalPatchVel = -10.0; // Slip (both wheels for consistency)
    
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

    engine_low.m_braking.brake_load_cap = 1.0f;
    engine_low.m_braking.lockup_enabled = true;
    engine_low.m_braking.lockup_gain = 1.0;
    engine_low.m_braking.abs_pulse_enabled = false; // Disable ABS (v0.6.0)
    engine_low.m_vibration.road_enabled = false; // Disable Road (v0.6.0)
    
    // Reset phase and flush transients
    engine.m_lockup_phase = 0.0;
    engine_low.m_lockup_phase = 0.0;
    
    // Issue #397: Flush the 10ms transient ramp
    PumpEngineTime(engine, data, 0.015);
    PumpEngineTime(engine_low, data, 0.015);

    // Discard transients
    engine.GetDebugBatch();
    engine_low.GetDebugBatch();

    // Measure steady state
    PumpEngineTime(engine, data, 0.05);
    PumpEngineTime(engine_low, data, 0.05);

    auto batch_high = engine.GetDebugBatch();
    auto batch_low = engine_low.GetDebugBatch();

    double max_high = 0.0, max_low = 0.0;
    for(const auto& s : batch_high) max_high = std::max(max_high, std::abs((double)s.texture_lockup));
    for(const auto& s : batch_low) max_low = std::max(max_low, std::abs((double)s.texture_lockup));

    // Verify the 3x ratio more precisely
    // Expected: max_high â‰ˆ 3.0 * max_low
    double expected_ratio = 3.0;
    double actual_ratio = max_high / (max_low + 0.0001);
    
    if (std::abs(actual_ratio - expected_ratio) < 0.2) {
        std::cout << "[PASS] Brake load cap applies 3x scaling (Ratio: " << actual_ratio << ", High: " << max_high << ", Low: " << max_low << ")" << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Expected ~3x ratio, got " << actual_ratio << " (High: " << max_high << ", Low: " << max_low << ")");
    }
}

// [Config][Physics] Dynamic lockup thresholds with progressive severity
TEST_CASE(test_dynamic_thresholds, "LockupBraking") {
    std::cout << "\nTest: Dynamic Lockup Thresholds [Config][Physics]" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    
    engine.m_braking.lockup_enabled = true;
    engine.m_braking.lockup_gain = 1.0;
    data.mUnfilteredBrake = 1.0;
    
    // Config: Start 5%, Full 15%
    engine.m_braking.lockup_start_pct = 5.0f;
    engine.m_braking.lockup_full_pct = 15.0f;
    
    // Case A: 4% Slip (Below Start)
    // 0.04 * 20.0 = 0.8
    data.mWheel[WHEEL_FL].mLongitudinalPatchVel = -0.8; 
    engine.calculate_force(&data);
    if (engine.m_lockup_phase == 0.0) {
        std::cout << "[PASS] No trigger below 5% start." << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Triggered below start threshold.");
    }

    // Case B: 20% Slip (Saturated/Manual Trigger)
    // 0.20 * 20.0 = 4.0
    data.mWheel[WHEEL_FL].mLongitudinalPatchVel = -4.0;
    double force_mid = PumpEngineTime(engine, data, 0.0125);
    ASSERT_TRUE(std::abs(force_mid) > 0.0);
    
    // Case C: 40% Slip (Deep Saturated)
    // 0.40 * 20.0 = 8.0
    data.mWheel[WHEEL_FL].mLongitudinalPatchVel = -8.0;
    double force_max = PumpEngineTime(engine, data, 0.0125);
    
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
    engine.m_braking.abs_pulse_enabled = true;
    engine.m_braking.abs_gain = 1.0f;
    engine.m_general.wheelbase_max_nm = 20.0f; engine.m_general.target_rim_nm = 20.0f; // Scale 1.0

    // Trigger condition: High Brake + Pressure Delta
    data.mUnfilteredBrake = 1.0;
    data.mWheel[WHEEL_FL].mBrakePressure = 1.0;
    PumpEngineTime(engine, data, 0.0125); // Frame 1: Set previous pressure

    data.mWheel[WHEEL_FL].mBrakePressure = 0.5; // Frame 2: Rapid drop (delta)
    double force = PumpEngineTime(engine, data, 0.0125);

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
    engine.m_vibration.spin_enabled = true;
    engine.m_vibration.spin_gain = 1.0f;
    engine.m_general.gain = 1.0f;

    // Trigger Spin
    data.mUnfilteredThrottle = 1.0;
    // Slip = 0.5 (Severe) -> Severity = (0.5 - 0.2) / 0.5 = 0.6
    // Drop Factor = 1.0 - (0.6 * 1.0 * 0.6) = 1.0 - 0.36 = 0.64
    double ground_vel = 20.0;
    data.mWheel[WHEEL_RL].mLongitudinalPatchVel = 0.5 * ground_vel;
    data.mWheel[WHEEL_RL].mLongitudinalGroundVel = ground_vel;
    data.mWheel[WHEEL_RR].mLongitudinalPatchVel = 0.5 * ground_vel;
    data.mWheel[WHEEL_RR].mLongitudinalGroundVel = ground_vel;

    // Disable Spin Vibration (gain 0) to check just the drop?
    // No, can't separate gain easily. But vibration is AC.
    // If we check magnitude, it might be messy.
    // Let's check with Spin Gain = 1.0, but Spin Freq Scale = 0 (Constant force?)
    // No, freq scale 0 -> phase 0 -> sin(0) = 0. No vibration.
    // Perfect for checking torque drop!

    engine.m_vibration.spin_freq_scale = 0.0f;

    // Add Road Texture (Texture Group - Should NOT be dropped)
    // Setup deflection delta for constant road noise
    // Force = Delta * 50.0. Target 0.1 normalized (2.0 Nm).
    // Delta = 2.0 / 50.0 = 0.04.
    engine.m_vibration.road_enabled = true;
    engine.m_vibration.road_gain = 1.0f;
    engine.m_general.wheelbase_max_nm = 20.0f; engine.m_general.target_rim_nm = 20.0f; // Scale 1.0

    // v0.7.69: Ensure vibration multiplier is 1.0 for this test
    FFBEngineTestAccess::SetStaticFrontLoad(engine, 4000.0);
    FFBEngineTestAccess::SetSmoothedVibrationMult(engine, 1.0);

    // Reset deflection state in engine first
    PumpEngineTime(engine, data, 0.0125);

    // Apply Delta
    data.mWheel[WHEEL_FL].mVerticalTireDeflection += 0.02; // +2cm
    data.mWheel[WHEEL_FR].mVerticalTireDeflection += 0.02; // +2cm
    // Total Delta = 0.04. Road Force = 0.04 * 50.0 = 2.0 Nm.
    // Normalized Road = 2.0 / 20.0 = 0.1.

    double force = PumpEngineTime(engine, data, 0.0125);

    // Base Force (Structural) = 10.0 Nm -> 0.5 Norm.
    // Torque Drop = 0.64.
    // Road Force (Texture) = 1.0 Nm (Clamped) -> 0.05 Norm.

    // Logic A (Broken): (Base + Texture) * Drop = (0.5 + 0.05) * 0.64 = 0.352
    // Logic B (Correct): (Base * Drop) + Texture = (0.5 * 0.64) + 0.05 = 0.32 + 0.05 = 0.37

    if (std::abs(force - 0.37) < 0.03) {
        std::cout << "[PASS] Torque Drop correctly isolated from Textures (Force: " << force << " Expected: 0.37)" << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Torque Drop logic error. Got: " << force << " Expected: 0.37 (Broken: 0.352)");
    }
}



} // namespace FFBEngineTests
