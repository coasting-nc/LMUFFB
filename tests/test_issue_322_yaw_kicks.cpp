#include "test_ffb_common.h"
#include "../src/FFBEngine.h"
#include "../src/Config.h"
#include <cmath>

using namespace FFBEngineTests;

TEST_CASE(test_unloaded_yaw_kick_activation, "YawKicks") {
    FFBEngine engine;
    InitializeEngine(engine);

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    // Initialize car metadata
    engine.calculate_force(&data, "GT3", "TestCar", 0.0f, true, 0.01);

    // Force manual seed of static loads AFTER metadata init
    FFBEngineTestAccess::SetStaticFrontLoad(engine, 5000.0);
    FFBEngineTestAccess::SetStaticRearLoad(engine, 5000.0);
    FFBEngineTestAccess::SetStaticLoadLatched(engine, true);

    Preset p;
    // Enable Unloaded Yaw Kick, disable general
    p.SetUnloadedYawKick(1.0f, 0.05f, 1.0f, 1.0f, 0.0f);
    p.SetSoPYaw(0.0f);
    p.SetSpeedGate(0.0f, 0.0f); // Disable speed gate for test
    p.Apply(engine);
    // Simulate low rear load (2500N vs 5000N static) -> 0.5 drop
    // early rear load calc uses mSuspForce + 300.0
    data.mWheel[2].mSuspForce = 2200.0;
    data.mWheel[3].mSuspForce = 2200.0;

    // Frame 1: Steady
    data.mLocalRot.y = 0.0;
    data.mElapsedTime = 1.0;
    engine.calculate_force(&data, "GT3", "TestCar", 0.0f, true, 0.01);

    // Frame 2: Sudden Yaw Acceleration (1. rad/s^2)
    data.mLocalRot.y = 0.01; // (0.01 - 0.0) / 0.01 = 1.0 rad/s^2
    data.mElapsedTime = 1.01;
    engine.calculate_force(&data, "GT3", "TestCar", 0.0f, true, 0.01);

    // Frame 3: Continue to allow smoothing to settle (dt=0.01, tau=0.015 -> alpha = 0.01 / 0.025 = 0.4)
    data.mLocalRot.y = 0.02;
    data.mElapsedTime = 1.02;
    engine.calculate_force(&data, "GT3", "TestCar", 0.0f, true, 0.01);

    // Frame 10-50: Let it settle
    for (int i=3; i<50; ++i) {
        data.mLocalRot.y += 0.01;
        data.mElapsedTime += 0.01;
        engine.calculate_force(&data, "GT3", "TestCar", 0.0f, true, 0.01);
    }

    // Get results from snapshot
    auto snapshots = engine.GetDebugBatch();
    ASSERT_GT(snapshots.size(), 0);
    float yaw_force = snapshots.back().ffb_yaw_kick;

    // Unloaded Kick should be active.
    // drop = 0.5. threshold = 0.05. accel = 1.0. processed = 0.95.
    // force_nm = -1.0 * 0.95 * 1.0 (gain) * 5.0 (BASE) * 0.5 (drop) = -2.375
    ASSERT_NEAR(yaw_force, -2.375f, 0.5);
}

TEST_CASE(test_power_yaw_kick_activation, "YawKicks") {
    FFBEngine engine;
    InitializeEngine(engine);
    Preset p;
    // Enable Power Yaw Kick, disable general
    p.SetPowerYawKick(1.0f, 0.05f, 0.10f, 1.0f, 0.0f);
    p.SetSoPYaw(0.0f);
    p.SetSpeedGate(0.0f, 0.0f);
    p.Apply(engine);

    FFBCalculationContext ctx;
    ctx.car_speed = 20.0;
    ctx.dt = 0.01;

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mUnfilteredThrottle = 1.0;
    // Simulate wheel spin: Ground 20m/s, Wheel 22m/s -> 10% slip
    data.mWheel[2].mLongitudinalGroundVel = 20.0;
    data.mWheel[2].mLongitudinalPatchVel = 2.0; // 2.0 / 20.0 = 0.1
    data.mWheel[3].mLongitudinalGroundVel = 20.0;
    data.mWheel[3].mLongitudinalPatchVel = 2.0;

    // Frame 1: Steady
    data.mLocalRot.y = 0.0;
    data.mElapsedTime = 1.0;
    engine.calculate_force(&data, "GT3", "TestCar", 0.0f, true, 0.01);

    // Frame 2: Sudden Yaw Acceleration (1.0 rad/s^2)
    data.mLocalRot.y = 0.01;
    data.mElapsedTime = 1.01;
    engine.calculate_force(&data, "GT3", "TestCar", 0.0f, true, 0.01);

    // Frame 10-50: Let it settle
    for (int i=2; i<50; ++i) {
        data.mLocalRot.y += 0.01;
        data.mElapsedTime += 0.01;
        engine.calculate_force(&data, "GT3", "TestCar", 0.0f, true, 0.01);
    }

    auto snapshots = engine.GetDebugBatch();
    ASSERT_GT(snapshots.size(), 0);
    float yaw_force = snapshots.back().ffb_yaw_kick;

    // Power Kick should be active.
    // slip = 0.1. target = 0.1. vulnerability = 1.0.
    // accel = 1.0. threshold = 0.05. processed = 0.95.
    // force_nm = -1.0 * 0.95 * 1.0 (gain) * 5.0 (BASE) * 1.0 (vuln) = -4.75
    ASSERT_NEAR(yaw_force, -4.75f, 0.5);
}

TEST_CASE(test_yaw_jerk_punch, "YawKicks") {
    FFBEngine engine;
    InitializeEngine(engine);
    Preset p;
    // Enable Power Yaw Kick with Punch, disable general
    p.SetPowerYawKick(1.0f, 0.0f, 0.10f, 1.0f, 0.1f); // punch = 0.1
    p.SetSoPYaw(0.0f);
    p.SetSpeedGate(0.0f, 0.0f);
    p.Apply(engine);

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mUnfilteredThrottle = 1.0;
    data.mWheel[2].mLongitudinalGroundVel = 20.0;
    data.mWheel[2].mLongitudinalPatchVel = 2.0;
    data.mWheel[3].mLongitudinalGroundVel = 20.0;
    data.mWheel[3].mLongitudinalPatchVel = 2.0;

    // Frame 1: Steady (accel = 0)
    data.mLocalRot.y = 0.0;
    data.mElapsedTime = 1.0;
    engine.calculate_force(&data, "GT3", "TestCar", 0.0f, true, 0.01);

    // Frame 2: Step to 1.0 rad/s^2.
    // raw derived accel = 1.0.
    // fast smoothed accel = 0.4 * 1.0 = 0.4.
    // prev fast smoothed accel = 0.
    // jerk = (0.4 - 0) / 0.01 = 40.0.
    // punch_addition = 40.0 * 0.1 = 4.0.
    // punchy_yaw = 0.4 + 4.0 = 4.4.
    data.mLocalRot.y = 0.01;
    data.mElapsedTime = 1.01;
    engine.calculate_force(&data, "GT3", "TestCar", 0.0f, true, 0.01);

    auto snapshots = engine.GetDebugBatch();
    float force_with_punch = snapshots.back().ffb_yaw_kick;

    // force_nm = -1.0 * 4.4 * 1.0 * 5.0 = -22.0.
    ASSERT_NEAR(force_with_punch, -22.0f, 1.0);
}

TEST_CASE(test_yaw_jerk_attack_phase_gate, "YawKicks") {
    FFBEngine engine;
    InitializeEngine(engine);
    Preset p;
    // Enable Power Yaw Kick with Punch
    p.SetPowerYawKick(1.0f, 0.0f, 0.10f, 1.0f, 1.0f); // high punch
    p.SetSoPYaw(0.0f);
    p.SetSpeedGate(0.0f, 0.0f);
    p.Apply(engine);

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mUnfilteredThrottle = 1.0;
    data.mWheel[2].mLongitudinalGroundVel = 20.0;
    data.mWheel[2].mLongitudinalPatchVel = 2.0;
    data.mWheel[3].mLongitudinalGroundVel = 20.0;
    data.mWheel[3].mLongitudinalPatchVel = 2.0;

    // Frame 0: Seed yaw rate
    data.mLocalRot.y = 0.0;
    data.mElapsedTime = 1.0;
    engine.calculate_force(&data, "GT3", "TestCar", 0.0f, true, 0.01);

    // Step 1: Initial positive acceleration, let it settle
    for (int i=0; i<50; ++i) {
        data.mLocalRot.y += 0.01; // raw accel 1.0
        data.mElapsedTime += 0.01;
        engine.calculate_force(&data, "GT3", "TestCar", 0.0f, true, 0.01);
    }
    // m_fast_yaw_accel_smoothed should be ~1.0

    // Step 2: Negative jerk (accel decreasing)
    // raw accel drops to 0.5
    data.mLocalRot.y += 0.005;
    data.mElapsedTime += 0.01;
    engine.calculate_force(&data, "GT3", "TestCar", 0.0f, true, 0.01);
    // jerk will be negative.

    // Step 3: Even more negative jerk
    data.mLocalRot.y += 0.0001; // raw accel 0.01
    data.mElapsedTime += 0.01;
    engine.calculate_force(&data, "GT3", "TestCar", 0.0f, true, 0.01);
    // Negative Jerk!
    // Since jerk * smoothed_accel = negative * positive < 0, punch should be gated.

    auto snapshots = engine.GetDebugBatch();
    float force_with_negative_jerk = snapshots.back().ffb_yaw_kick;

    // Expected force: -1.0 * smoothed_accel * gain * BASE * vuln.
    // Smoothed accel is positive, so force should be negative.
    // If punch WAS applied, it would be a huge negative addition to accel,
    // resulting in a large POSITIVE force.

    ASSERT_LT(force_with_negative_jerk, 0.0f);
    // Relaxed check if it's still near zero
    ASSERT_GT(force_with_negative_jerk, -20.0f);
}

TEST_CASE(test_vulnerability_asymmetric_smoothing, "YawKicks") {
    FFBEngine engine;
    InitializeEngine(engine);
    Preset p;
    p.SetPowerYawKick(1.0f, 0.0f, 0.10f, 1.0f, 0.0f);
    p.SetSoPYaw(0.0f);
    p.SetSpeedGate(0.0f, 0.0f);
    p.Apply(engine);

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mUnfilteredThrottle = 1.0;
    data.mWheel[2].mLongitudinalGroundVel = 20.0;
    data.mWheel[2].mLongitudinalPatchVel = 2.0; // 10% slip -> vuln 1.0
    data.mWheel[3].mLongitudinalGroundVel = 20.0;
    data.mWheel[3].mLongitudinalPatchVel = 2.0;

    // Frame 1: Activation
    data.mLocalRot.y = 0.01;
    data.mElapsedTime = 1.01;
    engine.calculate_force(&data, "GT3", "TestCar", 0.0f, true, 0.01);

    // Alpha attack (2ms) = 0.01 / 0.012 = 0.833.
    // Vulnerability = 0.833 * 1.0 = 0.833.

    // Frame 2: Deactivation (slip drops to 0)
    data.mWheel[2].mLongitudinalPatchVel = 0.0;
    data.mWheel[3].mLongitudinalPatchVel = 0.0;
    data.mLocalRot.y = 0.02;
    data.mElapsedTime = 1.02;
    engine.calculate_force(&data, "GT3", "TestCar", 0.0f, true, 0.01);

    // Alpha decay (50ms) = 0.01 / 0.06 = 0.166.
    // Vulnerability = 0.833 + 0.166 * (0.0 - 0.833) = 0.833 * (1 - 0.166) = 0.694.

    auto snapshots = engine.GetDebugBatch();
    float force_after_drop = snapshots.back().ffb_yaw_kick;

    // accel = 1.0. smoothed accel settled a bit more.
    // force = -1.0 * accel_smoothed * gain * BASE * vuln.
    // If it was instant decay, force would be 0.
    // Since it's slow decay, force should be around -1.0 * 1.0 * 5.0 * 0.694 = -3.47.
    ASSERT_LT(force_after_drop, -1.0f);
}

TEST_CASE(test_yaw_kick_blending, "YawKicks") {
    FFBEngine engine;
    InitializeEngine(engine);
    Preset p;
    // Configure all three kicks
    p.SetSoPYaw(0.5f); // General gain
    p.SetYawKickThreshold(0.1f);
    p.SetUnloadedYawKick(1.0f, 0.1f, 1.0f, 1.0f, 0.0f);
    p.SetPowerYawKick(2.0f, 0.1f, 0.1f, 1.0f, 0.0f);
    p.SetSpeedGate(0.0f, 0.0f);
    p.Apply(engine);

    FFBEngineTestAccess::SetStaticFrontLoad(engine, 5000.0);
    FFBEngineTestAccess::SetStaticRearLoad(engine, 5000.0);
    FFBEngineTestAccess::SetStaticLoadLatched(engine, true);

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    // Open all gates
    data.mUnfilteredThrottle = 1.0;
    data.mWheel[2].mLongitudinalGroundVel = 20.0;
    data.mWheel[2].mLongitudinalPatchVel = 2.0; // 10% slip
    data.mWheel[2].mSuspForce = 700.0; // (700+300)/5000 = 0.2 ratio -> 0.8 drop
    data.mWheel[3].mSuspForce = 700.0;

    data.mLocalRot.y = 0.0;
    data.mElapsedTime = 1.0;
    engine.calculate_force(&data, "GT3", "TestCar", 0.0f, true, 0.01);

    data.mLocalRot.y = 0.011; // 1.1 rad/s^2. processed = 1.0
    data.mElapsedTime = 1.01;
    engine.calculate_force(&data, "GT3", "TestCar", 0.0f, true, 0.01);

    // Let it settle
    for (int i=0; i<50; ++i) {
        data.mLocalRot.y += 0.011;
        data.mElapsedTime += 0.01;
        engine.calculate_force(&data, "GT3", "TestCar", 0.0f, true, 0.01);
    }

    auto snapshots = engine.GetDebugBatch();
    float final_force = snapshots.back().ffb_yaw_kick;

    // General Nm: -1.0 * 0.5 * 5.0 = -2.5
    // Unloaded Nm: -1.0 * 1.0 * 5.0 * 0.8 (drop) = -4.0
    // Power Nm: -1.0 * 2.0 * 5.0 * 1.0 (slip) = -10.0
    // Max abs is Power: -10.0 Nm
    ASSERT_NEAR(final_force, -10.0f, 1.0);
}

TEST_CASE(test_static_rear_load_tracking, "YawKicks") {
    FFBEngine engine;
    InitializeEngine(engine);
    // Set vehicle name so it saves
    StringUtils::SafeCopy(engine.m_vehicle_name, sizeof(engine.m_vehicle_name), "TestTracker");

    // Learning phase: speed between 2.0 and 15.0
    // Call 10 times to settle (dt=0.1, tau=5.0 -> alpha=0.02)
    for(int i=0; i<50; ++i) {
        FFBEngineTestAccess::CallUpdateStaticLoadReference(engine, 4000.0, 8.0, 0.1);
    }

    // Should be close to 4000.0 but not latched yet
    ASSERT_NEAR(FFBEngineTestAccess::GetStaticFrontLoad(engine), 4000.0, 10.0);
    ASSERT_NEAR(FFBEngineTestAccess::GetStaticRearLoad(engine), 4000.0, 10.0);
    ASSERT_FALSE(FFBEngineTestAccess::GetStaticLoadLatched(engine));

    // Latch phase: speed >= 15.0
    FFBEngineTestAccess::CallUpdateStaticLoadReference(engine, 4000.0, 16.0, 0.1);

    ASSERT_TRUE(FFBEngineTestAccess::GetStaticLoadLatched(engine));
    ASSERT_NEAR(FFBEngineTestAccess::GetStaticFrontLoad(engine), 4000.0, 1.0);
    ASSERT_NEAR(FFBEngineTestAccess::GetStaticRearLoad(engine), 4000.0, 1.0);
}
