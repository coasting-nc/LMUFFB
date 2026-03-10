#include "test_ffb_common.h"
#include <iostream>
#include <vector>
#include <cmath>

using namespace FFBEngineTests;

TEST_CASE(test_longitudinal_g_braking, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);

    // Setup: 100% effect gain
    engine.m_long_load_effect = 1.0f;
    engine.m_long_load_smoothing = 0.0f; // Instant
    engine.m_steering_shaft_gain = 1.0f;
    engine.m_understeer_effect = 0.0f;

    // Scenario: 1G Braking (+Z is rearward/deceleration)
    // To make sure calculate_force doesn't overwrite it, we set a high smoothing
    engine.m_chassis_inertia_smoothing = 1000.0f;
    engine.m_accel_z_smoothed = 9.81;

    TelemInfoV01 data = CreateBasicTestTelemetry(30.0);
    // 10Nm base torque
    data.mSteeringShaftTorque = 10.0;

    FFBEngineTestAccess::SetStaticFrontLoad(engine, 5000.0);

    engine.calculate_force(&data, "GT3", "Ferrari 296", 0.0f, true, 0.0025);

    auto batch = engine.GetDebugBatch();
    ASSERT_FALSE(batch.empty());
    if (!batch.empty()) {
        float long_force = batch.back().long_load_force;
        float base_force = batch.back().base_force;

        // long_g = 9.81 / 9.81 = 1.0.
        // multiplier = 1.0 + 1.0 * 1.0 = 2.0.
        // long_load_force = base_steer_force * (2.0 - 1.0) = base_steer_force.

        std::cout << "[DEBUG] Base Force: " << base_force << " Long Force: " << long_force << std::endl;

        // Use a slightly larger epsilon because of alpha_chassis (even with 1000s tau, it's not 0)
        ASSERT_NEAR(long_force, base_force, 0.2f);
    }
}

TEST_CASE(test_longitudinal_g_acceleration, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);

    // Setup: 50% effect gain
    engine.m_long_load_effect = 0.5f;
    engine.m_long_load_smoothing = 0.0f; // Instant
    engine.m_steering_shaft_gain = 1.0f;
    engine.m_understeer_effect = 0.0f;

    // Scenario: 1G Acceleration (-Z is forward)
    engine.m_chassis_inertia_smoothing = 1000.0f;
    engine.m_accel_z_smoothed = -9.81;

    TelemInfoV01 data = CreateBasicTestTelemetry(30.0);
    data.mSteeringShaftTorque = 10.0;

    FFBEngineTestAccess::SetStaticFrontLoad(engine, 5000.0);

    engine.calculate_force(&data, "GT3", "Ferrari 296", 0.0f, true, 0.0025);

    auto batch = engine.GetDebugBatch();
    ASSERT_FALSE(batch.empty());
    if (!batch.empty()) {
        float long_force = batch.back().long_load_force;
        float base_force = batch.back().base_force;

        // long_g = -1.0.
        // multiplier = 1.0 + (-1.0) * 0.5 = 0.5.
        // long_load_force = base_steer_force * (0.5 - 1.0) = -0.5 * base_steer_force.

        std::cout << "[DEBUG] Base Force: " << base_force << " Long Force: " << long_force << std::endl;

        ASSERT_NEAR(long_force, -0.5f * base_force, 0.1f);
    }
}

TEST_CASE(test_longitudinal_g_aero_independence, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);

    // Setup: 100% effect gain
    engine.m_long_load_effect = 1.0f;
    engine.m_long_load_smoothing = 0.0f; // Instant
    engine.m_steering_shaft_gain = 1.0f;
    engine.m_understeer_effect = 0.0f;

    // Scenario: 0G Acceleration (Constant Speed) but MASSIVE front load (Aero)
    engine.m_chassis_inertia_smoothing = 1000.0f;
    engine.m_accel_z_smoothed = 0.0;

    TelemInfoV01 data = CreateBasicTestTelemetry(80.0); // High speed
    data.mWheel[0].mTireLoad = 10000.0; // Massive load (2x static)
    data.mWheel[1].mTireLoad = 10000.0;
    data.mSteeringShaftTorque = 10.0;

    FFBEngineTestAccess::SetStaticFrontLoad(engine, 5000.0);

    engine.calculate_force(&data, "GT3", "Ferrari 296", 0.0f, true, 0.0025);

    auto batch = engine.GetDebugBatch();
    ASSERT_FALSE(batch.empty());
    if (!batch.empty()) {
        float long_force = batch.back().long_load_force;
        // long_g = 0.0.
        // multiplier = 1.0 + 0.0 * 1.0 = 1.0.
        // long_load_force = base_steer_force * (1.0 - 1.0) = 0.0.

        ASSERT_NEAR(long_force, 0.0f, 0.01f);
    }
}
