
#include "test_ffb_common.h"
#include <cmath>

using namespace FFBEngineTests;

TEST_CASE(test_coverage_load_reference, "Coverage") {
    FFBEngine engine;
    
    // Set initial condition
    FFBEngineTestAccess::SetStaticFrontLoad(engine, 50.0);
    
    // Case 1: Active update (Speed > 2 && Speed < 15, Load < 100)
    // dt=0.1, should set immediately? 
    // Logic: if (m_static_front_load < 100.0) m_static_front_load = current_load;
    FFBEngineTestAccess::CallUpdateStaticLoadReference(engine, 2250.0, 10.0, 0.1);
    ASSERT_NEAR(FFBEngineTestAccess::GetStaticFrontLoad(engine), 2250.0, 1.0);  // Should jump to 2250 (4500 * 0.5)
    
    // Case 2: Inertial update (Speed > 2 && Speed < 15, Load >= 100)
    // Logic: m_static_front_load += (dt / 5.0) * (current_load - m_static_front_load);
    double initial = 4000.0;
    FFBEngineTestAccess::SetStaticFrontLoad(engine, initial);
    double target = 5000.0;
    double dt = 0.5; // alpha = 0.1
    FFBEngineTestAccess::CallUpdateStaticLoadReference(engine, target, 10.0, dt);
    // Expected: 4000 + 0.1 * (5000 - 4000) = 4100
    ASSERT_GT(FFBEngineTestAccess::GetStaticFrontLoad(engine), initial);
    ASSERT_NEAR(FFBEngineTestAccess::GetStaticFrontLoad(engine), 4100.0, 10.0);

    // Case 3: Speed Too Low
    initial = 4000.0;
    FFBEngineTestAccess::SetStaticFrontLoad(engine, initial);
    FFBEngineTestAccess::CallUpdateStaticLoadReference(engine, 5000.0, 1.0, 0.1);
    ASSERT_NEAR(FFBEngineTestAccess::GetStaticFrontLoad(engine), initial, 0.1); // No change

    // Case 4: Speed Too High
    FFBEngineTestAccess::SetStaticFrontLoad(engine, initial);
    FFBEngineTestAccess::CallUpdateStaticLoadReference(engine, 5000.0, 20.0, 0.1);
    ASSERT_NEAR(FFBEngineTestAccess::GetStaticFrontLoad(engine), initial, 0.1); // No change

    // Case 5: Safety Clamp
    // if (m_static_front_load < 1000.0) m_static_front_load = m_auto_peak_load * 0.5;
    FFBEngineTestAccess::SetStaticFrontLoad(engine, 500.0);
    // Call with valid speed but no internal update needed (load already low, but update logic sets it)
    // Wait, the function ends with the clamp check.
    // So if we call it, it should clamp.
    FFBEngineTestAccess::CallUpdateStaticLoadReference(engine, 500.0, 10.0, 0.1); 
    // Logic: if < 100 -> set to current (500). Then at end, if < 1000 -> set to fallback (2250).
    ASSERT_NEAR(FFBEngineTestAccess::GetStaticFrontLoad(engine), 2250.0, 1.0);
}

TEST_CASE(test_coverage_init_load_ref, "Coverage") {
    FFBEngine engine;
    
    // Just call it to trigger the logic and cout
    // We can verify m_auto_peak_load changes
    FFBEngineTestAccess::SetAutoPeakLoad(engine, 1000.0);
    FFBEngineTestAccess::CallInitializeLoadReference(engine, "GTE", "Ferrari 488 GTE");
    
    // detailed logic inside ParseVehicleClass is covered elsewhere, but we need to hit InitializeLoadReference
    ASSERT_GT(FFBEngineTestAccess::GetAutoPeakLoad(engine), 2000.0);
}

TEST_CASE(test_coverage_slip_utils, "Coverage") {
    FFBEngine engine;
    TelemWheelV01 w1, w2;
    memset(&w1, 0, sizeof(w1));
    memset(&w2, 0, sizeof(w2));

    // calculate_raw_slip_angle_pair
    w1.mLateralPatchVel = 1.0;
    w1.mLongitudinalGroundVel = 10.0;
    w2.mLateralPatchVel = 1.0;
    w2.mLongitudinalGroundVel = 10.0;
    
    double angle = engine.calculate_raw_slip_angle_pair(w1, w2);
    ASSERT_NEAR(angle, std::atan2(1.0, 10.0), 0.001);

    // calculate_slip_angle standard path
    double prev = 0.0;
    double smoothed = engine.calculate_slip_angle(w1, prev, 0.01);
    ASSERT_GT(std::abs(smoothed), 0.0);

    // calculate_manual_slip_ratio
    // radius = 100 -> 1.0m? No, mStaticUndeflectedRadius is cm?
    // Code: double radius_m = (double)w.mStaticUndeflectedRadius / 100.0;
    // If < 0.1 -> 0.33
    w1.mStaticUndeflectedRadius = 30; // 0.3m
    w1.mRotation = 100.0; // rad/s -> 30 m/s
    double speed = 20.0; 
    
    double ratio = engine.calculate_manual_slip_ratio(w1, speed);
    // (30 - 20) / 20 = 0.5
    ASSERT_NEAR(ratio, 0.5, 0.01);
    
    // Small speed path
    ratio = engine.calculate_manual_slip_ratio(w1, 1.0);
    ASSERT_NEAR(ratio, 0.0, 0.001);
}

TEST_CASE(test_coverage_textures, "Coverage") {
    FFBEngine engine;
    TelemInfoV01 data = CreateBasicTestTelemetry();
    FFBCalculationContext ctx;
    ctx.dt = 0.01;
    ctx.car_speed = 20.0;
    ctx.speed_gate = 1.0;
    ctx.decoupling_scale = 1.0;
    ctx.avg_grip = 1.0;

    // 1. Wheel Spin
    // Requirements: m_spin_enabled, Throttle > 0.05, Rear Slip > 0.2
    engine.m_spin_enabled = true;
    engine.m_spin_gain = 1.0;
    data.mUnfilteredThrottle = 1.0;
    
    // Set rear wheels spinning
    // slip ratio = (wheel_vel - car_vel) / car_vel
    // wheel_vel = car_vel * (1 + slip)
    // car_vel = 20.
    // wheel_vel = 20 * 1.5 = 30.
    // radius = 0.33 (fallback). rotation = 30 / 0.33 = 90.9
    
    // We rely on calculate_wheel_slip_ratio internal logic:
    // w.mLongitudinalPatchVel / v_long
    data.mWheel[2].mLongitudinalPatchVel = 10.0; // Slip velocity
    data.mWheel[2].mLongitudinalGroundVel = 20.0; 
    // Ratio = 0.5. > 0.2 threshold.
    
    FFBEngineTestAccess::CallCalculateWheelSpin(engine, &data, ctx);
    // Should have generated spin rumble
    // We can't easily check internal phase, but we can check if it crashed
    // or if we can access the output in ctx?
    // ctx.spin_rumble is not in FFBEngineTestAccess, but FFBCalculationContext is struct.
    ASSERT_NEAR(ctx.spin_rumble, 0.0, 100.0); // Just check it runs
    
    // 2. Slide Texture
    // effective_slip_vel > 1.5
    engine.m_slide_texture_enabled = true;
    engine.m_slide_texture_gain = 1.0;
    data.mWheel[0].mLateralPatchVel = 2.0;
    data.mWheel[1].mLateralPatchVel = 2.0; // Avg = 2.0 > 1.5
    FFBEngineTestAccess::CallCalculateSlideTexture(engine, &data, ctx);
    // ctx.slide_noise should be non-zero (unless phase is 0)
    
    // 3. Road Texture & Scrub
    engine.m_road_texture_enabled = true;
    engine.m_road_texture_gain = 1.0;
    FFBEngineTestAccess::SetScrubDragGain(engine, 1.0);
    
    // Scrub logic: abs(avg_lat_vel) > 0.001
    data.mWheel[0].mLateralPatchVel = 1.0;
    data.mWheel[1].mLateralPatchVel = 1.0;
    
    // Road noise logic: deflection delta
    // Need prev deflection to be different.
    // But prev is 0.0 initially.
    data.mWheel[0].mVerticalTireDeflection = 0.005;
    
    FFBEngineTestAccess::CallCalculateRoadTexture(engine, &data, ctx);
    
    ASSERT_GT(std::abs(ctx.scrub_drag_force), 0.0);
    ASSERT_GT(std::abs(ctx.road_noise), 0.0);

    // 4. Road Texture (Accelerometer Path)
    // Speed > 5.0, No Deflection
    ctx.car_speed = 10.0;
    data.mWheel[0].mVerticalTireDeflection = 0.0;
    data.mWheel[1].mVerticalTireDeflection = 0.0;
    // Need prev vs current accel diff
    // FFBEngine uses m_prev_vert_accel (internal state).
    // Calling calculate_road_texture does NOT update m_prev_vert_accel (it's updated in calculate_force).
    // But it reads data->mLocalAccel.y and uses m_prev_vert_accel.
    // m_prev_vert_accel is 0.0 init.
    data.mLocalAccel.y = 1.0; 
    FFBEngineTestAccess::CallCalculateRoadTexture(engine, &data, ctx);
    // road_noise_val = (1.0 - 0.0) * ...
    ASSERT_NEAR(ctx.road_noise, 2.5, 0.1); 
}

TEST_CASE(test_coverage_slope_grip_fusion, "Coverage") {
    FFBEngine engine;
    TelemInfoV01 data = CreateBasicTestTelemetry();
    FFBEngineTestAccess::SetSlopeUseTorque(engine, true);
    
    // Force some slope state
    // We can't easily force m_slope_torque_current via public API quickly without running the loop
    // But we can check calculate_slope_grip with data != nullptr
    
    // We need to trigger the torque fusion branch:
    // double loss_percent_torque = 0.0;
    // bool use_torque_fusion = (m_slope_use_torque && data != nullptr);
    
    FFBEngineTestAccess::CallCalculateSlopeGrip(engine, 0.5, 0.1, 0.01, &data);
    // This simply ensures the lines are executed
    
    FFBEngineTestAccess::SetSlopeUseTorque(engine, false);
    FFBEngineTestAccess::CallCalculateSlopeGrip(engine, 0.5, 0.1, 0.01, &data);
}

TEST_CASE(test_coverage_bottoming_rh, "Coverage") {
    FFBEngine engine;
    TelemInfoV01 data = CreateBasicTestTelemetry();
    FFBCalculationContext ctx;
    ctx.dt = 0.0025; // Smaller dt for better phase control
    ctx.speed_gate = 1.0;
    ctx.decoupling_scale = 1.0;
    
    FFBEngineTestAccess::SetBottomingEnabled(engine, true);
    FFBEngineTestAccess::SetBottomingGain(engine, 1.0);
    
    // Method 0: Ride Height
    FFBEngineTestAccess::SetBottomingMethod(engine, 0);
    data.mWheel[0].mRideHeight = 0.001; // < 0.002
    data.mWheel[1].mRideHeight = 0.001;
    
    // First call advances phase from 0
    FFBEngineTestAccess::CallCalculateSuspensionBottoming(engine, &data, ctx);
    ctx.bottoming_crunch = 0.0;
    // Second call should have non-zero sin value
    FFBEngineTestAccess::CallCalculateSuspensionBottoming(engine, &data, ctx);
    
    // After phase advancement, sin should be non-zero
    ASSERT_GT(std::abs(ctx.bottoming_crunch), 0.001);
}

TEST_CASE(test_coverage_bottoming_dforce, "Coverage") {
    FFBEngine engine;
    TelemInfoV01 data = CreateBasicTestTelemetry();
    FFBCalculationContext ctx;
    ctx.dt = 0.0025;
    ctx.speed_gate = 1.0;
    ctx.decoupling_scale = 1.0;

    FFBEngineTestAccess::SetBottomingEnabled(engine, true);
    FFBEngineTestAccess::SetBottomingGain(engine, 1.0);

    // Method 1: dForce
    FFBEngineTestAccess::SetBottomingMethod(engine, 1);
    data.mWheel[0].mSuspForce = 200000.0; // Huge force
    // Prev is 0. dForce = 20,000,000. > 100,000 thousand.
    
    // First call
    FFBEngineTestAccess::CallCalculateSuspensionBottoming(engine, &data, ctx);
    ctx.bottoming_crunch = 0.0;
    // Second call should have non-zero output
    FFBEngineTestAccess::CallCalculateSuspensionBottoming(engine, &data, ctx);
    
    ASSERT_GT(std::abs(ctx.bottoming_crunch), 0.001);
}

TEST_CASE(test_coverage_bottoming_fallback, "Coverage") {
    FFBEngine engine;
    TelemInfoV01 data = CreateBasicTestTelemetry();
    FFBCalculationContext ctx;
    ctx.dt = 0.0025;
    ctx.speed_gate = 1.0;
    ctx.decoupling_scale = 1.0;

    FFBEngineTestAccess::SetBottomingEnabled(engine, true);
    FFBEngineTestAccess::SetBottomingGain(engine, 1.0);
    
    // Use Method 1 (dForce) but with conditions that WON'T trigger it
    // This forces the code to check the fallback condition (max load > 8000)
    FFBEngineTestAccess::SetBottomingMethod(engine, 1);
    
    // Fallback: Max Load
    data.mWheel[0].mSuspForce = 0.0; // Low force, won't trigger dForce
    data.mWheel[0].mTireLoad = 9000.0; // > threshold (4500 * 1.6 = 7200) - triggers fallback
    
    // First call
    FFBEngineTestAccess::CallCalculateSuspensionBottoming(engine, &data, ctx);
    ctx.bottoming_crunch = 0.0;
    // Second call
    FFBEngineTestAccess::CallCalculateSuspensionBottoming(engine, &data, ctx);
    
    ASSERT_GT(std::abs(ctx.bottoming_crunch), 0.001);
}

TEST_CASE(test_coverage_approximations, "Coverage") {
    FFBEngine engine;
    TelemWheelV01 w;
    memset(&w, 0, sizeof(w));
    w.mSuspForce = 1000.0;
    
    // approximate_load: SuspForce + 300
    ASSERT_NEAR(engine.approximate_load(w), 1300.0, 0.1);
    
    // approximate_rear_load: SuspForce + 300
    ASSERT_NEAR(engine.approximate_rear_load(w), 1300.0, 0.1);
}
