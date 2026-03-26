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
    engine.m_load_forces.long_load_effect = 1.0f;
    engine.m_load_forces.long_load_smoothing = 0.0f; // Instant
    engine.m_front_axle.steering_shaft_gain = 1.0f;
    engine.m_front_axle.understeer_effect = 0.0f;
    engine.m_load_forces.long_load_transform = (int)LoadTransform::LINEAR;
    engine.m_grip_estimation.chassis_inertia_smoothing = 0.0f; // Reduce LPF latency

    // Scenario: 1G Braking (+Z is rearward/deceleration)
    TelemInfoV01 data = CreateBasicTestTelemetry(30.0);
    data.mSteeringShaftTorque = 10.0;
    data.mLocalAccel.z = 9.81; // 1G Braking
    data.mLocalAccel.y = 0.0;

    FFBEngineTestAccess::SetStaticFrontLoad(engine, 5000.0);

    // Step 1: Warmup call to trigger seeding gate
    FFBEngineTestAccess::SetDerivativesSeeded(engine, false);
    engine.calculate_force(&data, "GT3", "Ferrari 296");

    // Step 2: Establish baseline (1G braking via velocity change) over several frames
    // Issue #397: Use FFB loop ticks to advance time correctly through interpolator
    for (int i = 0; i < 60; i++) {
        if (i % 4 == 0) {
            data.mLocalVel.z += (9.81 * 0.01);
            data.mElapsedTime += 0.01;
        }
        engine.calculate_force(&data, "GT3", "Ferrari 296", 0.0f, true, 0.0025);
    }

    auto batch = engine.GetDebugBatch();
    ASSERT_FALSE(batch.empty());
    auto snap = batch.back();
    // 1G Linear -> multiplier = 1.0 + 1.0 * 1.0 = 2.0.
    // long_load_force = 10 * (2.0 - 1.0) = 10.
    // Issue #397: Relax tolerance slightly
    ASSERT_NEAR(snap.long_load_force, 10.0f, 1.0f);

}

TEST_CASE(test_longitudinal_g_high_decel, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);

    engine.m_load_forces.long_load_effect = 1.0f;
    engine.m_load_forces.long_load_smoothing = 0.0f;
    engine.m_front_axle.steering_shaft_gain = 1.0f;
    engine.m_front_axle.understeer_effect = 0.0f;
    engine.m_load_forces.long_load_transform = (int)LoadTransform::LINEAR;
    engine.m_grip_estimation.chassis_inertia_smoothing = 0.0f; // Reduce LPF latency

    // Scenario: 3G Braking
    TelemInfoV01 data = CreateBasicTestTelemetry(30.0);
    data.mSteeringShaftTorque = 10.0;
    data.mLocalAccel.z = 3.0 * 9.81;

    // Seeding logic
    FFBEngineTestAccess::SetDerivativesSeeded(engine, false);
    engine.calculate_force(&data, "GT3", "Ferrari 296");

    // Establishing 3G
    // Issue #397: Use FFB loop ticks
    for (int i = 0; i < 60; i++) {
        if (i % 4 == 0) {
            data.mLocalVel.z += (3.0 * 9.81 * 0.01);
            data.mElapsedTime += 0.01;
        }
        engine.calculate_force(&data, "GT3", "Ferrari 296", 0.0f, true, 0.0025);
    }

    auto snap = engine.GetDebugBatch().back();
    // 3G Linear -> multiplier = 1.0 + 3.0 * 1.0 = 4.0.
    // long_load_force = 10 * (4.0 - 1.0) = 30.
    // Issue #397: Relax tolerance
    ASSERT_NEAR(snap.long_load_force, 30.0f, 3.0f);
}

TEST_CASE(test_longitudinal_g_domain_scaling_cubic, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);

    engine.m_load_forces.long_load_effect = 1.0f;
    engine.m_load_forces.long_load_smoothing = 0.0f;
    engine.m_front_axle.steering_shaft_gain = 1.0f;
    engine.m_front_axle.understeer_effect = 0.0f;
    engine.m_load_forces.long_load_transform = (int)LoadTransform::CUBIC;
    engine.m_grip_estimation.chassis_inertia_smoothing = 0.0f; // Reduce LPF latency

    // Scenario: 0.5G Braking (4.905 m/s2)
    // Domain Scaling: MAX_G_RANGE = 5.0
    // x = 0.5 / 5.0 = 0.1
    // f(0.1) = 1.5(0.1) - 0.5(0.1)^3 = 0.15 - 0.0005 = 0.1495
    // norm = 0.1495 * 5.0 = 0.7475
    // multiplier = 1.0 + 0.7475 * 1.0 = 1.7475
    // long_force = 10 * 0.7475 = 7.475

    TelemInfoV01 data = CreateBasicTestTelemetry(30.0);
    data.mSteeringShaftTorque = 10.0;
    data.mLocalAccel.z = 0.5 * 9.81;

    // Warmup
    FFBEngineTestAccess::SetDerivativesSeeded(engine, false);
    engine.calculate_force(&data, "GT3", "Ferrari 296");

    // Establish 0.5G
    // Issue #397: Use FFB loop ticks
    for (int i = 0; i < 60; i++) {
        if (i % 4 == 0) {
            data.mLocalVel.z += (0.5 * 9.81 * 0.01);
            data.mElapsedTime += 0.01;
        }
        engine.calculate_force(&data, "GT3", "Ferrari 296", 0.0f, true, 0.0025);
    }

    auto snap = engine.GetDebugBatch().back();
    // Issue #397: Relax tolerance
    ASSERT_NEAR(snap.long_load_force, 7.475f, 0.6f);
}

TEST_CASE(test_longitudinal_g_aero_independence, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);

    engine.m_load_forces.long_load_effect = 1.0f;
    engine.m_load_forces.long_load_smoothing = 0.0f;
    engine.m_front_axle.steering_shaft_gain = 1.0f;
    engine.m_front_axle.understeer_effect = 0.0f;

    // Scenario: 0G Acceleration (Constant Speed) but MASSIVE front load (Aero)
    engine.m_grip_estimation.chassis_inertia_smoothing = 1000.0f;
    engine.m_accel_z_smoothed = 0.0;

    TelemInfoV01 data = CreateBasicTestTelemetry(80.0);
    data.mWheel[WHEEL_FL].mTireLoad = 10000.0;
    data.mWheel[WHEEL_FR].mTireLoad = 10000.0;
    data.mSteeringShaftTorque = 10.0;

    // Issue #461: Predictive filter needs time to settle the Working Copy load/torque
    PumpEngineSteadyState(engine, data);

    auto batch = engine.GetDebugBatch();
    ASSERT_FALSE(batch.empty());
    if (!batch.empty()) {
        float long_force = batch.back().long_load_force;
        ASSERT_NEAR(long_force, 0.0f, 0.1f);
    }
}

