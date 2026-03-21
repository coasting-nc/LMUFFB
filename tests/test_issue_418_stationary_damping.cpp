#include "test_ffb_common.h"

using namespace FFBEngineTests;

TEST_CASE(test_stationary_damping_default, "Physics") {
    FFBEngine engine;
    ASSERT_NEAR(engine.m_advanced.stationary_damping, 1.0f, 0.0001f);

    Preset p;
    ASSERT_NEAR(p.advanced.stationary_damping, 1.0f, 0.0001f);
}

TEST_CASE(test_stationary_damping_at_zero_speed, "Physics") {
    FFBEngine engine;
    TelemInfoV01 data = CreateBasicTestTelemetry(0.0); // 0 km/h
    InitializeEngine(engine);

    engine.m_advanced.stationary_damping = 1.0f;
    engine.m_advanced.gyro_gain = 0.0f;
    engine.m_advanced.speed_gate_lower = 1.0f; // 3.6 km/h
    engine.m_advanced.speed_gate_upper = 5.0f; // 18.0 km/h

    // Setup steering velocity: 1.0 rad/s
    // steering_velocity = (steer_angle - m_prev_steering_angle) / dt
    // steering_angle = data->mUnfilteredSteering * (range / 2.0)
    // range defaults to ~9.42 rad
    // So steer_angle = data->mUnfilteredSteering * 4.71
    // For 1.0 rad/s velocity with dt=0.0025: delta_angle = 0.0025 rad
    // delta_unfiltered = 0.0025 / 4.71 = 0.00053

    data.mUnfilteredSteering = 0.0;
    engine.calculate_force(&data, "GT3", "Ferrari 296 GT3", 0.0f, true); // First frame to seed

    // Pump with changing steering to create velocity
    for (int i = 0; i < 10; ++i) {
        data.mUnfilteredSteering += 0.001;
        data.mElapsedTime += 0.01;
        engine.calculate_force(&data, "GT3", "Ferrari 296 GT3", 0.0f, true, 0.0025);
    }

    auto snapshots = engine.GetDebugBatch();
    ASSERT_FALSE(snapshots.empty());
    float last_stationary_damping = snapshots.back().ffb_stationary_damping;

    // At 0 speed, stationary damping should be active
    // With positive steering velocity, damping force should be negative (opposing)
    ASSERT_LT(last_stationary_damping, 0.0f);

    // Gyro damping should be 0 at 0 speed
    ASSERT_NEAR(snapshots.back().ffb_gyro_damping, 0.0f, 0.0001f);
}

TEST_CASE(test_stationary_damping_fades_at_speed, "Physics") {
    FFBEngine engine;
    TelemInfoV01 data = CreateBasicTestTelemetry(50.0); // 50 m/s (~180 km/h)
    InitializeEngine(engine);

    engine.m_advanced.stationary_damping = 1.0f;
    engine.m_advanced.speed_gate_lower = 1.0f;
    engine.m_advanced.speed_gate_upper = 5.0f;

    data.mUnfilteredSteering = 0.0;
    engine.calculate_force(&data, "GT3", "Ferrari 296 GT3", 0.0f, true);

    // Pump with changing steering to create velocity
    for (int i = 0; i < 10; ++i) {
        data.mUnfilteredSteering += 0.001;
        data.mElapsedTime += 0.01;
        engine.calculate_force(&data, "GT3", "Ferrari 296 GT3", 0.0f, true, 0.0025);
    }

    auto snapshots = engine.GetDebugBatch();
    ASSERT_FALSE(snapshots.empty());
    float last_stationary_damping = snapshots.back().ffb_stationary_damping;

    // At 50 m/s, speed_gate is 1.0, so 1.0 - speed_gate = 0.0
    ASSERT_NEAR(last_stationary_damping, 0.0f, 0.0001f);
}

TEST_CASE(test_stationary_damping_active_in_menus, "Physics") {
    FFBEngine engine;
    TelemInfoV01 data = CreateBasicTestTelemetry(0.0); // 0 km/h
    InitializeEngine(engine);

    engine.m_advanced.stationary_damping = 1.0f;
    engine.m_general.gain = 1.0f;
    engine.m_general.wheelbase_max_nm = 10.0f;
    engine.m_general.target_rim_nm = 10.0f;
    engine.m_advanced.speed_gate_lower = 1.0f; // 3.6 km/h
    engine.m_advanced.speed_gate_upper = 5.0f; // 18.0 km/h

    data.mUnfilteredSteering = 0.0;
    // First frame seeds
    engine.calculate_force(&data, "GT3", "Ferrari 296 GT3", 0.0f, true);

    // Move into menu
    bool allowed = false;

    // Pump engine time with allowed=false and changing steering
    for (int i = 0; i < 40; ++i) {
        data.mUnfilteredSteering += 0.01; // Faster steering for more velocity
        data.mElapsedTime += 0.0025;
        engine.calculate_force(&data, "GT3", "Ferrari 296 GT3", 0.0f, allowed, 0.0025);
    }

    auto snapshots = engine.GetDebugBatch();
    ASSERT_FALSE(snapshots.empty());
    float last_total = snapshots.back().total_output;
    float last_stationary_damping = snapshots.back().ffb_stationary_damping;

    // Total output should be non-zero in menus because of stationary damping
    ASSERT_NE(last_total, 0.0f);

    // Account for dynamic normalization scaling: force * mult * (target/max) * gain
    double structural_mult = FFBEngineTestAccess::GetSmoothedStructuralMult(engine);
    float expected_total = last_stationary_damping * (float)structural_mult * (engine.m_general.target_rim_nm / engine.m_general.wheelbase_max_nm) * engine.m_general.gain;

    ASSERT_NEAR(last_total, expected_total, 0.01f);
}
