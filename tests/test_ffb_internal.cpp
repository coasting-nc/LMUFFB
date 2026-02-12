#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_snapshot_data_integrity, "Internal") {
    std::cout << "\nTest: Snapshot Data Integrity (v0.4.7)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine); // v0.5.12: Initialize with T300 defaults
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));

    // Setup input values
    // Case: Missing Tire Load (0) but Valid Susp Force (1000)
    data.mWheel[0].mTireLoad = 0.0;
    data.mWheel[1].mTireLoad = 0.0;
    data.mWheel[0].mSuspForce = 1000.0;
    data.mWheel[1].mSuspForce = 1000.0;
    
    // Other inputs
    data.mLocalVel.z = 20.0; // Moving
    data.mUnfilteredThrottle = 0.8;
    data.mUnfilteredBrake = 0.2;
    // data.mRideHeight = 0.05; // Removed invalid field
    // Wait, TelemInfoV01 has mWheel[].mRideHeight.
    data.mWheel[0].mRideHeight = 0.03;
    data.mWheel[1].mRideHeight = 0.04; // Min is 0.03

    // Trigger missing load logic
    // Need > 20 frames of missing load
    data.mDeltaTime = 0.01;
    for (int i=0; i<30; i++) {
        engine.calculate_force(&data);
    }

    // Get Snapshot from Missing Load Scenario
    auto batch_load = engine.GetDebugBatch();
    if (!batch_load.empty()) {
        FFBSnapshot snap_load = batch_load.back();
        
        // Test 1: Raw Load should be 0.0 (What the game sent)
        if (std::abs(snap_load.raw_front_tire_load) < 0.001) {
            std::cout << "[PASS] Raw Front Tire Load captured as 0.0." << std::endl;
            g_tests_passed++;
        } else {
            std::cout << "[FAIL] Raw Front Tire Load incorrect: " << snap_load.raw_front_tire_load << std::endl;
            g_tests_failed++;
        }
        
        // Test 2: Calculated Load should be approx 1300 (SuspForce 1000 + 300 offset)
        if (std::abs(snap_load.calc_front_load - 1300.0) < 0.001) {
            std::cout << "[PASS] Calculated Front Load is 1300.0." << std::endl;
            g_tests_passed++;
        } else {
            std::cout << "[FAIL] Calculated Front Load incorrect: " << snap_load.calc_front_load << std::endl;
            g_tests_failed++;
        }
        
        // Test 3: Raw Throttle Input (from initial setup: data.mUnfilteredThrottle = 0.8)
        if (std::abs(snap_load.raw_input_throttle - 0.8) < 0.001) {
            std::cout << "[PASS] Raw Throttle captured." << std::endl;
            g_tests_passed++;
        } else {
            std::cout << "[FAIL] Raw Throttle incorrect: " << snap_load.raw_input_throttle << std::endl;
            g_tests_failed++;
        }
        
        // Test 4: Raw Ride Height (Min of 0.03 and 0.04 -> 0.03)
        if (std::abs(snap_load.raw_front_ride_height - 0.03) < 0.001) {
            std::cout << "[PASS] Raw Ride Height captured (Min)." << std::endl;
            g_tests_passed++;
        } else {
            std::cout << "[FAIL] Raw Ride Height incorrect: " << snap_load.raw_front_ride_height << std::endl;
            g_tests_failed++;
        }
    }

    // New Test Requirement: Distinct Front/Rear Grip
    // Reset data for a clean frame
    std::memset(&data, 0, sizeof(data));
    data.mWheel[0].mGripFract = 1.0; // FL
    data.mWheel[1].mGripFract = 1.0; // FR
    data.mWheel[2].mGripFract = 0.5; // RL
    data.mWheel[3].mGripFract = 0.5; // RR
    
    // Set some valid load so we don't trigger missing load logic
    data.mWheel[0].mTireLoad = 4000.0;
    data.mWheel[1].mTireLoad = 4000.0;
    data.mWheel[2].mTireLoad = 4000.0;
    data.mWheel[3].mTireLoad = 4000.0;
    
    data.mLocalVel.z = 20.0;
    data.mDeltaTime = 0.01;
    
    // Set Deflection for Renaming Test
    data.mWheel[0].mVerticalTireDeflection = 0.05;
    data.mWheel[1].mVerticalTireDeflection = 0.05;

    engine.calculate_force(&data);

    // Get Snapshot
    auto batch = engine.GetDebugBatch();
    if (batch.empty()) {
        std::cout << "[FAIL] No snapshot generated." << std::endl;
        g_tests_failed++;
        return;
    }
    
    FFBSnapshot snap = batch.back();
    
    // Assertions
    
    // 1. Check Front Grip (1.0)
    if (std::abs(snap.calc_front_grip - 1.0) < 0.001) {
        std::cout << "[PASS] Calc Front Grip is 1.0." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Calc Front Grip incorrect: " << snap.calc_front_grip << std::endl;
        g_tests_failed++;
    }
    
    // 2. Check Rear Grip (0.5)
    if (std::abs(snap.calc_rear_grip - 0.5) < 0.001) {
        std::cout << "[PASS] Calc Rear Grip is 0.5." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Calc Rear Grip incorrect: " << snap.calc_rear_grip << std::endl;
        g_tests_failed++;
    }
    
    // 3. Check Renamed Field (raw_front_deflection)
    if (std::abs(snap.raw_front_deflection - 0.05) < 0.001) {
        std::cout << "[PASS] raw_front_deflection captured (Renamed field)." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] raw_front_deflection incorrect: " << snap.raw_front_deflection << std::endl;
        g_tests_failed++;
    }
}

TEST_CASE(test_zero_effects_leakage, "Internal") {
    std::cout << "\nTest: Zero Effects Leakage (No Ghost Forces)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine); // v0.5.12: Initialize with T300 defaults
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));

    // 1. Load "Test: No Effects" Preset configuration
    // (Gain 1.0, everything else 0.0)
    engine.m_gain = 1.0f;
    engine.m_min_force = 0.0f;
    engine.m_understeer_effect = 0.0f;
    engine.m_sop_effect = 0.0f;
    engine.m_oversteer_boost = 0.0f;
    engine.m_rear_align_effect = 0.0f;
    engine.m_lockup_enabled = false;
    engine.m_spin_enabled = false;
    engine.m_slide_texture_enabled = false;
    engine.m_road_texture_enabled = false;
    engine.m_bottoming_enabled = false;
    engine.m_scrub_drag_gain = 0.0f;
    
    // 2. Set Inputs that WOULD trigger forces if effects were on
    
    // Base Force: 0.0 (We want to verify generated effects, not pass-through)
    data.mSteeringShaftTorque = 0.0;
    
    // SoP Trigger: 1G Lateral
    data.mLocalAccel.x = 9.81; 
    
    // Rear Align Trigger: Lat Force + Slip
    data.mWheel[2].mLateralForce = 0.0; // Simulate missing force (workaround trigger)
    data.mWheel[3].mLateralForce = 0.0;
    data.mWheel[2].mTireLoad = 3000.0; // Load
    data.mWheel[3].mTireLoad = 3000.0;
    data.mWheel[2].mGripFract = 0.0; // Trigger approx
    data.mWheel[3].mGripFract = 0.0;
    data.mWheel[2].mLateralPatchVel = 5.0; // Slip
    data.mWheel[3].mLateralPatchVel = 5.0;
    data.mWheel[2].mLongitudinalGroundVel = 20.0;
    data.mWheel[3].mLongitudinalGroundVel = 20.0;
    
    // Bottoming Trigger: Ride Height
    data.mWheel[0].mRideHeight = 0.001; // Scraping
    data.mWheel[1].mRideHeight = 0.001;
    
    // Textures Trigger:
    data.mWheel[0].mLateralPatchVel = 5.0; // Slide
    data.mWheel[1].mLateralPatchVel = 5.0;
    
    data.mDeltaTime = 0.01;
    data.mLocalVel.z = 20.0;
    
    // Run Calculation
    double force = engine.calculate_force(&data);
    
    // Assert: Total Output must be exactly 0.0
    if (std::abs(force) < 0.000001) {
        std::cout << "[PASS] Zero leakage verified (Force = 0.0)." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Ghost Force detected! Output: " << force << std::endl;
        // Debug components
        auto batch = engine.GetDebugBatch();
        if (!batch.empty()) {
            FFBSnapshot s = batch.back();
            std::cout << "Debug: SoP=" << s.sop_force 
                      << " RearT=" << s.ffb_rear_torque 
                      << " Slide=" << s.texture_slide 
                      << " Bot=" << s.texture_bottoming << std::endl;
        }
        g_tests_failed++;
    }
}

TEST_CASE(test_snapshot_data_v049, "Internal") {
    std::cout << "\nTest: Snapshot Data v0.4.9 (Rear Physics)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine); // v0.5.12: Initialize with T300 defaults
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));

    // Setup input values
    data.mLocalVel.z = 20.0;
    data.mDeltaTime = 0.01;
    
    // Front Wheels
    data.mWheel[0].mLongitudinalPatchVel = 1.0;
    data.mWheel[1].mLongitudinalPatchVel = 1.0;
    
    // Rear Wheels (Sliding Lat + Long)
    data.mWheel[2].mLateralPatchVel = 2.0;
    data.mWheel[3].mLateralPatchVel = 2.0;
    data.mWheel[2].mLongitudinalPatchVel = 3.0;
    data.mWheel[3].mLongitudinalPatchVel = 3.0;
    data.mWheel[2].mLongitudinalGroundVel = 20.0;
    data.mWheel[3].mLongitudinalGroundVel = 20.0;

    // Run Engine
    engine.calculate_force(&data);

    // Verify Snapshot
    auto batch = engine.GetDebugBatch();
    if (batch.empty()) {
        std::cout << "[FAIL] No snapshot." << std::endl;
        g_tests_failed++;
        return;
    }
    
    FFBSnapshot snap = batch.back();
    
    // Check Front Long Patch Vel
    // Avg(1.0, 1.0) = 1.0
    if (std::abs(snap.raw_front_long_patch_vel - 1.0) < 0.001) {
        std::cout << "[PASS] raw_front_long_patch_vel correct." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] raw_front_long_patch_vel: " << snap.raw_front_long_patch_vel << std::endl;
        g_tests_failed++;
    }
    
    // Check Rear Lat Patch Vel
    // Avg(abs(2.0), abs(2.0)) = 2.0
    if (std::abs(snap.raw_rear_lat_patch_vel - 2.0) < 0.001) {
        std::cout << "[PASS] raw_rear_lat_patch_vel correct." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] raw_rear_lat_patch_vel: " << snap.raw_rear_lat_patch_vel << std::endl;
        g_tests_failed++;
    }
    
    // Check Rear Long Patch Vel
    // Avg(3.0, 3.0) = 3.0
    if (std::abs(snap.raw_rear_long_patch_vel - 3.0) < 0.001) {
        std::cout << "[PASS] raw_rear_long_patch_vel correct." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] raw_rear_long_patch_vel: " << snap.raw_rear_long_patch_vel << std::endl;
        g_tests_failed++;
    }
    
    // Check Rear Slip Angle Raw
    // atan2(2, 20) = ~0.0996 rad
    // snap.raw_rear_slip_angle
    if (std::abs(snap.raw_rear_slip_angle - 0.0996) < 0.01) {
        std::cout << "[PASS] raw_rear_slip_angle correct." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] raw_rear_slip_angle: " << snap.raw_rear_slip_angle << std::endl;
        g_tests_failed++;
    }
}

TEST_CASE(test_refactor_snapshot_sop, "Internal") {
    std::cout << "\nTest: Refactor Regression - Snapshot SoP (v0.6.36)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);

    // Setup SoP + Boost
    engine.m_sop_effect = 1.0f;
    engine.m_oversteer_boost = 1.0f;
    engine.m_sop_smoothing_factor = 1.0f; // Instant
    engine.m_sop_scale = 10.0f; // 1G -> 1.0 unboosted (normalized 20Nm)

    data.mLocalAccel.x = 9.81; // 1G Lat

    // Trigger Boost: Rear Grip Loss
    data.mWheel[0].mGripFract = 1.0;
    data.mWheel[1].mGripFract = 1.0;
    data.mWheel[2].mGripFract = 0.5;
    data.mWheel[3].mGripFract = 0.5;
    // Delta = 0.5. Boost = 1.0 + (0.5 * 1.0 * 2.0) = 2.0x.

    // Expected:
    // SoP Base (Unboosted) = 1.0 * 1.0 * 10 = 10.0 Nm
    // SoP Total (Boosted) = 10.0 * 2.0 = 20.0 Nm
    // Snapshot SoP Force = 10.0 (Unboosted Nm)
    // Snapshot Boost = 20.0 - 10.0 = 10.0 (Nm)

    engine.calculate_force(&data);

    auto batch = engine.GetDebugBatch();
    if (!batch.empty()) {
        FFBSnapshot snap = batch.back();

        bool sop_ok = (std::abs(snap.sop_force - 10.0) < 0.01);
        bool boost_ok = (std::abs(snap.oversteer_boost - 10.0) < 0.01);

        if (sop_ok && boost_ok) {
            std::cout << "[PASS] Snapshot values correct (SoP: " << snap.sop_force << ", Boost: " << snap.oversteer_boost << ")" << std::endl;
            g_tests_passed++;
        } else {
            std::cout << "[FAIL] Snapshot logic error. SoP: " << snap.sop_force << " (Exp: 10.0) Boost: " << snap.oversteer_boost << " (Exp: 10.0)" << std::endl;
            g_tests_failed++;
        }
    } else {
        std::cout << "[FAIL] No snapshot." << std::endl;
        g_tests_failed++;
    }
}

// --- Unit Tests for Private Helper Methods (v0.6.36) ---

void FFBEngineTestAccess::test_unit_sop_lateral() {
    std::cout << "\nTest Unit: calculate_sop_lateral" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    FFBCalculationContext ctx;
    ctx.dt = 0.01;
    ctx.car_speed = 20.0;
    ctx.avg_load = 4000.0;

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mLocalAccel.x = 9.81; // 1G
    engine.m_sop_effect = 1.0;
    engine.m_sop_scale = 10.0;
    engine.m_sop_smoothing_factor = 1.0; // Instant

    engine.calculate_sop_lateral(&data, ctx);

    if (std::abs(ctx.sop_base_force - 10.0) < 0.01) {
        std::cout << "[PASS] calculate_sop_lateral base logic." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] calculate_sop_lateral failed. Got " << ctx.sop_base_force << std::endl;
        g_tests_failed++;
    }
}

void FFBEngineTestAccess::test_unit_gyro_damping() {
    std::cout << "\nTest Unit: calculate_gyro_damping" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    FFBCalculationContext ctx;
    ctx.dt = 0.01;
    ctx.car_speed = 10.0;

    TelemInfoV01 data = CreateBasicTestTelemetry(10.0);
    data.mUnfilteredSteering = 0.1; 
    engine.m_prev_steering_angle = 0.0;

    engine.m_gyro_gain = 1.0;
    engine.m_gyro_smoothing = 0.0001f; 

    engine.calculate_gyro_damping(&data, ctx);

    if (ctx.gyro_force < -40.0) {
        std::cout << "[PASS] calculate_gyro_damping logic." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] calculate_gyro_damping failed. Got " << ctx.gyro_force << std::endl;
        g_tests_failed++;
    }
}

void FFBEngineTestAccess::test_unit_abs_pulse() {
    std::cout << "\nTest Unit: calculate_abs_pulse" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    FFBCalculationContext ctx;
    ctx.dt = 0.01;

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mUnfilteredBrake = 1.0;
    data.mWheel[0].mBrakePressure = 0.5;
    engine.m_prev_brake_pressure[0] = 1.0; 

    engine.m_abs_pulse_enabled = true;
    engine.m_abs_gain = 1.0;

    engine.calculate_abs_pulse(&data, ctx);

    if (std::abs(ctx.abs_pulse_force) > 0.0001 || engine.m_abs_phase > 0.0) {
        std::cout << "[PASS] calculate_abs_pulse triggered." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] calculate_abs_pulse failed." << std::endl;
        g_tests_failed++;
    }
}

TEST_CASE(test_refactor_units, "Internal") {
    FFBEngineTestAccess::test_unit_sop_lateral();
    FFBEngineTestAccess::test_unit_gyro_damping();
    FFBEngineTestAccess::test_unit_abs_pulse();
}

TEST_CASE(test_wheel_slip_ratio_helper, "Internal") {
    std::cout << "\nTest: calculate_wheel_slip_ratio Helper (v0.6.36)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemWheelV01 wheel;
    std::memset(&wheel, 0, sizeof(wheel));
    wheel.mLongitudinalGroundVel = 20.0;
    wheel.mLongitudinalPatchVel = 4.0;
    double slip = engine.calculate_wheel_slip_ratio(wheel);
    ASSERT_NEAR(slip, 0.2, 0.001);
}

TEST_CASE(test_signal_conditioning_helper, "Internal") {
    std::cout << "\nTest: apply_signal_conditioning Helper (v0.6.36)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    FFBCalculationContext ctx;
    ctx.dt = 0.01;
    ctx.car_speed = 20.0;
    double result = engine.apply_signal_conditioning(10.0, &data, ctx);
    ASSERT_NEAR(result, 10.0, 0.01);
}

TEST_CASE(test_unconditional_vert_accel_update, "Internal") {
    std::cout << "\nTest: Unconditional m_prev_vert_accel Update (v0.6.36)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    
    // Disable road texture effect
    engine.m_road_texture_enabled = false;
    
    // Set a known vertical acceleration
    data.mLocalAccel.y = 5.5;
    
    // Reset the engine state
    engine.m_prev_vert_accel = 0.0;
    
    // Run calculate_force
    engine.calculate_force(&data);
    
    // Check that m_prev_vert_accel was updated EVEN THOUGH road_texture is disabled
    if (std::abs(engine.m_prev_vert_accel - 5.5) < 0.01) {
        std::cout << "[PASS] m_prev_vert_accel updated unconditionally: " << engine.m_prev_vert_accel << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] m_prev_vert_accel not updated. Got: " << engine.m_prev_vert_accel << " Expected: 5.5" << std::endl;
        g_tests_failed++;
    }
    
    // Verify the value changes on next frame
    data.mLocalAccel.y = -3.2;
    engine.calculate_force(&data);
    
    if (std::abs(engine.m_prev_vert_accel - (-3.2)) < 0.01) {
        std::cout << "[PASS] m_prev_vert_accel tracks changes: " << engine.m_prev_vert_accel << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] m_prev_vert_accel not tracking. Got: " << engine.m_prev_vert_accel << " Expected: -3.2" << std::endl;
        g_tests_failed++;
    }
}



} // namespace FFBEngineTests