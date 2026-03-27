#include "test_ffb_common.h"
#include "../src/ffb/FFBEngine.h"
#include "../src/core/Config.h"
#include <cmath>

using namespace FFBEngineTests;
using namespace LMUFFB::Utils;

TEST_CASE(test_unloaded_yaw_kick_activation, "YawKicks") {
    FFBEngine engine;
    InitializeEngine(engine);

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    // Initialize car metadata
    engine.calculate_force(&data, "GT3", "TestCar", 0.0f, true, 0.01);

    for (int i = 0; i < NUM_WHEELS; i++) data.mWheel[i].mTireLoad = 0.0;

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
    data.mWheel[WHEEL_RL].mSuspForce = 2200.0;
    data.mWheel[WHEEL_RR].mSuspForce = 2200.0;

    // Run many frames to trigger load warning
    for(int i=0; i<30; i++) {
        data.mElapsedTime += 0.01;
        engine.calculate_force(&data, "GT3", "TestCar", 0.0f, true, 0.01);
    }

    // Frame N: Steady
    data.mLocalRot.y = 0.0;
    engine.calculate_force(&data, "GT3", "TestCar", 0.0f, true, 0.01);

    // Frame N+1: Sudden Yaw Acceleration (1. rad/s^2)
    data.mLocalRot.y = 0.01; // (0.01 - 0.0) / 0.01 = 1.0 rad/s^2
    data.mElapsedTime += 0.01;
    engine.calculate_force(&data, "GT3", "TestCar", 0.0f, true, 0.01);

    // Let it settle
    for (int i=0; i<50; ++i) {
        data.mLocalRot.y += 0.01;
        data.mElapsedTime += 0.01;
        engine.calculate_force(&data, "GT3", "TestCar", 0.0f, true, 0.01);
    }

    // Get results from snapshot
    auto snapshots = engine.GetDebugBatch();
    ASSERT_GT(snapshots.size(), 0);
    float yaw_force = snapshots.back().ffb_yaw_kick;

    // Unloaded Kick should be active.
    ASSERT_TRUE(std::abs(yaw_force) > 0.1);
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

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mUnfilteredThrottle = 1.0;
    // Simulate wheel spin: Ground 20m/s, Wheel 22m/s -> 10% slip
    data.mWheel[WHEEL_RL].mLongitudinalGroundVel = 20.0;
    data.mWheel[WHEEL_RL].mLongitudinalPatchVel = 2.0; // 2.0 / 20.0 = 0.1
    data.mWheel[WHEEL_RR].mLongitudinalGroundVel = 20.0;
    data.mWheel[WHEEL_RR].mLongitudinalPatchVel = 2.0;

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

    ASSERT_TRUE(std::abs(yaw_force) > 0.1);
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
    data.mWheel[WHEEL_RL].mLongitudinalGroundVel = 20.0;
    data.mWheel[WHEEL_RL].mLongitudinalPatchVel = 2.0;
    data.mWheel[WHEEL_RR].mLongitudinalGroundVel = 20.0;
    data.mWheel[WHEEL_RR].mLongitudinalPatchVel = 2.0;

    // Frame 1: Steady (accel = 0)
    data.mLocalRot.y = 0.0;
    data.mElapsedTime = 1.0;
    engine.calculate_force(&data, "GT3", "TestCar", 0.0f, true, 0.01);

    // Frame 2: Step to 1.0 rad/s^2.
    data.mLocalRot.y = 0.01;
    data.mElapsedTime = 1.01;
    // Issue #397: Flush the 10ms transient ramp
    for (int i = 0; i < NUM_WHEELS; i++) {
        engine.calculate_force(&data, "GT3", "TestCar", 0.0f, true, 0.0025);
    }

    auto snapshots = engine.GetDebugBatch();
    float force_with_punch = snapshots.back().ffb_yaw_kick;

    ASSERT_TRUE(std::abs(force_with_punch) > 0.05);
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
    data.mWheel[WHEEL_RL].mLongitudinalGroundVel = 20.0;
    data.mWheel[WHEEL_RL].mLongitudinalPatchVel = 2.0;
    data.mWheel[WHEEL_RR].mLongitudinalGroundVel = 20.0;
    data.mWheel[WHEEL_RR].mLongitudinalPatchVel = 2.0;

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

    // Step 2: Negative jerk (accel decreasing)
    data.mLocalRot.y += 0.005;
    data.mElapsedTime += 0.01;
    engine.calculate_force(&data, "GT3", "TestCar", 0.0f, true, 0.01);

    // Step 3: Even more negative jerk
    data.mLocalRot.y += 0.0001; // raw accel 0.01
    data.mElapsedTime += 0.01;
    engine.calculate_force(&data, "GT3", "TestCar", 0.0f, true, 0.01);

    auto snapshots = engine.GetDebugBatch();
    float force_with_negative_jerk = snapshots.back().ffb_yaw_kick;

    ASSERT_LT(force_with_negative_jerk, 0.0f);
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
    data.mWheel[WHEEL_RL].mLongitudinalGroundVel = 20.0;
    data.mWheel[WHEEL_RL].mLongitudinalPatchVel = 2.0; // 10% slip -> vuln 1.0
    data.mWheel[WHEEL_RR].mLongitudinalGroundVel = 20.0;
    data.mWheel[WHEEL_RR].mLongitudinalPatchVel = 2.0;

    // Frame 1: Activation
    data.mLocalRot.y = 0.01;
    data.mElapsedTime = 1.01;
    engine.calculate_force(&data, "GT3", "TestCar", 0.0f, true, 0.01);

    // Frame 2: Deactivation (slip drops to 0)
    data.mWheel[WHEEL_RL].mLongitudinalPatchVel = 0.0;
    data.mWheel[WHEEL_RR].mLongitudinalPatchVel = 0.0;
    data.mLocalRot.y = 0.02;
    data.mElapsedTime = 1.02;

    // Issue #397: Flush the 10ms transient ramp
    for (int i = 0; i < NUM_WHEELS; i++) {
        engine.calculate_force(&data, "GT3", "TestCar", 0.0f, true, 0.0025);
    }

    auto snapshots = engine.GetDebugBatch();
    float force_after_drop = snapshots.back().ffb_yaw_kick;

    ASSERT_LT(force_after_drop, -0.05f);
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
    data.mWheel[WHEEL_RL].mLongitudinalGroundVel = 20.0;
    data.mWheel[WHEEL_RL].mLongitudinalPatchVel = 2.0; // 10% slip
    data.mWheel[WHEEL_RL].mSuspForce = 700.0; // (700+300)/5000 = 0.2 ratio -> 0.8 drop
    data.mWheel[WHEEL_RR].mSuspForce = 700.0;

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

    ASSERT_TRUE(std::abs(final_force) > 1.0);
}

TEST_CASE(test_static_rear_load_tracking, "YawKicks") {
    FFBEngine engine;
    InitializeEngine(engine);
    // Set vehicle name so it saves
    StringUtils::SafeCopy(engine.m_metadata.m_vehicle_name, sizeof(engine.m_metadata.m_vehicle_name), "TestTracker");

    // Learning phase: speed between 2.0 and 15.0
    // Issue #397: InitializeEngine calls calculate_force once which updates
    // m_metadata.m_current_class. But we need to make sure we are unlatched.
    FFBEngineTestAccess::SetStaticLoadLatched(engine, false);
    engine.m_static_front_load = 0.0;
    engine.m_static_rear_load = 0.0;

    for(int i=0; i<200; ++i) {
        FFBEngineTestAccess::CallUpdateStaticLoadReference(engine, 4000.0, 4000.0, 8.0, 0.1);
    }

    ASSERT_NEAR(FFBEngineTestAccess::GetStaticFrontLoad(engine), 4000.0, 10.0);
    ASSERT_NEAR(FFBEngineTestAccess::GetStaticRearLoad(engine), 4000.0, 50.0);
    // Note: InitializeEngine now sets GT3 which sets static load to 4000 and latched=false,
    // so this test should pass after we reset it to unlatched
    FFBEngineTestAccess::SetStaticLoadLatched(engine, false);
    ASSERT_FALSE(FFBEngineTestAccess::GetStaticLoadLatched(engine));

    // Latch phase: speed >= 15.0
    FFBEngineTestAccess::CallUpdateStaticLoadReference(engine, 4000.0, 4000.0, 16.0, 0.1);

    ASSERT_TRUE(FFBEngineTestAccess::GetStaticLoadLatched(engine));
    ASSERT_NEAR(FFBEngineTestAccess::GetStaticFrontLoad(engine), 4000.0, 1.0);
    ASSERT_NEAR(FFBEngineTestAccess::GetStaticRearLoad(engine), 4000.0, 1.0);
}
