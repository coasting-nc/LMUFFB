#include "test_ffb_common.h"
#include <iostream>
#include <vector>
#include <cmath>

using namespace FFBEngineTests;

TEST_CASE(test_longitudinal_g_braking, "Physics") {
    std::cout << "[RUNNING] test_longitudinal_g_braking" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);

    // Setup: 100% effect gain
    engine.m_long_load_effect = 1.0f;
    engine.m_long_load_smoothing = 0.0f; // Instant
    engine.m_steering_shaft_gain = 1.0f;
    engine.m_understeer_effect = 0.0f;
    engine.m_long_load_transform = LoadTransform::LINEAR;

    // Scenario: 1G Braking (+Z is rearward/deceleration)
    engine.m_chassis_inertia_smoothing = 1000.0f;
    engine.m_accel_z_smoothed = 9.81;

    TelemInfoV01 data = CreateBasicTestTelemetry(30.0);
    data.mSteeringShaftTorque = 10.0;

    FFBEngineTestAccess::SetStaticFrontLoad(engine, 5000.0);

    engine.calculate_force(&data, "GT3", "Ferrari 296", 0.0f, true, 0.0025);

    auto batch = engine.GetDebugBatch();
    ASSERT_FALSE(batch.empty());
    if (!batch.empty()) {
        float long_force = batch.back().long_load_force;
        float base_force = batch.back().base_force;

        // 1G Linear -> long_load_norm = 1.0.
        // multiplier = 1.0 + 1.0 * 1.0 = 2.0.
        // long_load_force = 10 * (2.0 - 1.0) = 10.

        ASSERT_NEAR(long_force, base_force, 0.1f);
    }
}

TEST_CASE(test_longitudinal_g_high_decel, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);

    engine.m_long_load_effect = 1.0f;
    engine.m_long_load_smoothing = 0.0f;
    engine.m_steering_shaft_gain = 1.0f;
    engine.m_understeer_effect = 0.0f;
    engine.m_long_load_transform = LoadTransform::LINEAR;

    // Scenario: 3G Braking
    engine.m_chassis_inertia_smoothing = 1000.0f;
    engine.m_accel_z_smoothed = 3.0 * 9.81;

    TelemInfoV01 data = CreateBasicTestTelemetry(30.0);
    data.mSteeringShaftTorque = 10.0;

    engine.calculate_force(&data);

    auto snap = engine.GetDebugBatch().back();
    // 3G Linear -> multiplier = 1.0 + 3.0 * 1.0 = 4.0.
    // long_load_force = 10 * (4.0 - 1.0) = 30.
    ASSERT_NEAR(snap.long_load_force, 30.0f, 0.1f);
}

TEST_CASE(test_longitudinal_g_domain_scaling_cubic, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);

    engine.m_long_load_effect = 1.0f;
    engine.m_long_load_smoothing = 0.0f;
    engine.m_steering_shaft_gain = 1.0f;
    engine.m_understeer_effect = 0.0f;
    engine.m_long_load_transform = LoadTransform::CUBIC;

    // Scenario: 0.5G Braking (4.905 m/s2)
    // Domain Scaling: MAX_G_RANGE = 5.0
    // x = 0.5 / 5.0 = 0.1
    // f(0.1) = 1.5(0.1) - 0.5(0.1)^3 = 0.15 - 0.0005 = 0.1495
    // norm = 0.1495 * 5.0 = 0.7475
    // multiplier = 1.0 + 0.7475 * 1.0 = 1.7475
    // long_force = 10 * 0.7475 = 7.475

    engine.m_chassis_inertia_smoothing = 1000.0f;
    engine.m_accel_z_smoothed = 0.5 * 9.81;

    TelemInfoV01 data = CreateBasicTestTelemetry(30.0);
    data.mSteeringShaftTorque = 10.0;

    engine.calculate_force(&data);

    auto snap = engine.GetDebugBatch().back();
    ASSERT_NEAR(snap.long_load_force, 7.475f, 0.01f);
}

TEST_CASE(test_longitudinal_g_aero_independence, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);

    engine.m_long_load_effect = 1.0f;
    engine.m_long_load_smoothing = 0.0f;
    engine.m_steering_shaft_gain = 1.0f;
    engine.m_understeer_effect = 0.0f;

    // Scenario: 0G Acceleration (Constant Speed) but MASSIVE front load (Aero)
    engine.m_chassis_inertia_smoothing = 1000.0f;
    engine.m_accel_z_smoothed = 0.0;

    TelemInfoV01 data = CreateBasicTestTelemetry(80.0);
    data.mWheel[0].mTireLoad = 10000.0;
    data.mWheel[1].mTireLoad = 10000.0;
    data.mSteeringShaftTorque = 10.0;

    engine.calculate_force(&data);

    auto batch = engine.GetDebugBatch();
    ASSERT_FALSE(batch.empty());
    if (!batch.empty()) {
        float long_force = batch.back().long_load_force;
        ASSERT_NEAR(long_force, 0.0f, 0.01f);
    }
}
