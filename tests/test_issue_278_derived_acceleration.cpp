#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_issue_278_road_texture_spike_rejection, "DerivedAccel") {
    std::cout << "\nTest: Issue #278 Road Texture Spike Rejection" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);

    // Setup: Isolate Road Texture (Fallback Mode)
    engine.m_vibration.road_enabled = true;
    engine.m_vibration.road_gain = 1.0f;
    engine.m_vibration.vibration_gain = 1.0f;
    engine.m_general.gain = 1.0f;
    engine.m_general.wheelbase_max_nm = 20.0f;
    engine.m_general.target_rim_nm = 20.0f;

    // Disable other effects
    engine.m_front_axle.understeer_effect = 0.0f;
    engine.m_rear_axle.sop_effect = 0.0f;
    engine.m_load_forces.lat_load_effect = 0.0f;
    engine.m_gyro_gain = 0.0f;
    engine.m_rear_axle.rear_align_effect = 0.0f;
    engine.m_braking.lockup_enabled = false;
    engine.m_vibration.spin_enabled = false;
    engine.m_vibration.slide_enabled = false;
    engine.m_vibration.bottoming_enabled = false;
    engine.m_braking.abs_pulse_enabled = false;

    TelemInfoV01 data = CreateBasicTestTelemetry(10.0); // Speed = 10.0 (> 5.0 cutoff)
    data.mDeltaTime = 0.01; // 100Hz game tick

    // Ensure deflection is zero to force fallback to vertical acceleration
    for(int i=0; i<4; i++) data.mWheel[i].mVerticalTireDeflection = 0.0;

    // Step 1: Steady state
    data.mLocalVel.y = 0.0;
    data.mLocalAccel.y = 0.0;
    FFBEngineTestAccess::SetDerivativesSeeded(engine, false);
    engine.calculate_force(&data); // Seed 1 (Seeding gate)

    data.mElapsedTime += 0.01;
    engine.calculate_force(&data); // Seed 2 (Establish baseline)

    // Step 2: Inject a massive raw acceleration spike but NO velocity change
    data.mElapsedTime += 0.01;
    data.mLocalAccel.y = 500.0; // 50G spike!
    // Set vehicle class to ensure InitializeLoadReference is not called
    // and upsampling/derived logic remains focused on our manual inputs.
    double force_with_spike = engine.calculate_force(&data, "GT3", "911");

    // In old code, this would produce a massive jerk delta: (500 - 0) * 0.05 * 50 = 1250 Nm -> massive clip.
    // In new code, derived accel is (0 - 0) / 0.01 = 0.
    // So road texture should remain near 0.
    ASSERT_NEAR(force_with_spike, 0.0, 0.01);
    std::cout << "[PASS] Massive raw acceleration spike correctly ignored (force: " << force_with_spike << ")" << std::endl;
}

TEST_CASE(test_issue_278_road_texture_velocity_response, "DerivedAccel") {
    std::cout << "\nTest: Issue #278 Road Texture Velocity Response" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_vibration.road_enabled = true;
    engine.m_vibration.road_gain = 1.0f;
    engine.m_vibration.vibration_gain = 1.0f;
    engine.m_general.gain = 1.0f;
    engine.m_invert_force = false;

    TelemInfoV01 data = CreateBasicTestTelemetry(10.0);
    data.mDeltaTime = 0.01;
    for(int i=0; i<4; i++) data.mWheel[i].mVerticalTireDeflection = 0.0;

    // Step 1: Seed
    data.mLocalVel.y = 0.0;
    FFBEngineTestAccess::SetDerivativesSeeded(engine, false);
    engine.calculate_force(&data, "GT3", "911"); // Seed 1 (Seeding gate)

    data.mElapsedTime += 0.01;
    engine.calculate_force(&data, "GT3", "911"); // Seed 2 (Baseline)

    // Step 2: Constant vertical velocity change (constant acceleration)
    // dv = 0.1 m/s over 0.01s -> accel = 10 m/s^2 (~1G)
    data.mElapsedTime += 0.01;
    data.mLocalVel.y = 0.1;
    engine.calculate_force(&data, "GT3", "911"); // Derived accel = 10. prev_vert_accel = 10.

    // Step 3: Acceleration changes (Jerk)
    // dv = 0.3 m/s over 0.01s -> accel = 20 m/s^2 (increase from 10 to 20)
    // jerk = 10 m/s^2 / 0.01s ? No, Jerk in code is delta_accel = 20 - 10 = 10.
    // Road noise = 10 * 0.05 * 50 = 25 Nm.
    // 25 Nm / 20 Nm (base) = 1.25 -> Clipped to 1.0.
    data.mElapsedTime += 0.01;
    data.mLocalVel.y = 0.3;
    double force = engine.calculate_force(&data, "GT3", "911");

    ASSERT_GT(std::abs(force), 0.1);
    std::cout << "[PASS] Road texture responds to velocity-derived acceleration changes (force: " << force << ")" << std::endl;
}

TEST_CASE(test_issue_278_lockup_continuity, "DerivedAccel") {
    std::cout << "\nTest: Issue #278 Lockup Continuity" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);

    engine.m_braking.lockup_enabled = true;
    engine.m_braking.lockup_gain = 1.0f;
    engine.m_vibration.vibration_gain = 1.0f;
    engine.m_general.gain = 1.0f;
    engine.m_braking.lockup_prediction_sens = 10.0f;

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = 0.01;
    data.mUnfilteredBrake = 1.0;
    for(int i=0; i<4; i++) data.mWheel[i].mSuspForce = 1000.0;

    // Step 1: Seed
    data.mLocalVel.z = 20.0;
    FFBEngineTestAccess::SetDerivativesSeeded(engine, false);
    engine.calculate_force(&data); // Seed 1 (Seeding gate)
    data.mElapsedTime += 0.01;
    engine.calculate_force(&data); // Seed 2 (Baseline)

    // Step 2: Decelerate chassis
    // dv_z = -0.5 m/s -> accel = -50 m/s^2
    data.mLocalVel.z = 19.5;
    for(int i=0; i<4; i++) data.mWheel[i].mLongitudinalGroundVel = 19.5;

    // Decelerate wheel more to trigger predictive lockup
    // Initial rotation (at v=20) = 20 / 0.33 = 60.60 rad/s
    // Target rotation = 55.54 rad/s (significant deceleration)
    data.mWheel[0].mRotation = 55.54;

    // Set slip velocity to exceed start threshold (5%)
    // slip = mLongitudinalPatchVel / v_long.
    // Target slip = 0.07 -> mLongitudinalPatchVel = 0.07 * 19.5 = 1.365
    // We use negative for braking slip
    data.mWheel[0].mLongitudinalPatchVel = -1.4;

    // Inject raw long accel noise in opposite direction (acceleration spike)
    // If we relied on this, car_dec_ang would be positive/small, and predictive trigger would fail.
    data.mLocalAccel.z = 100.0;

    // Calculate force
    engine.calculate_force(&data);

    // Verify lockup rumble is active despite raw accel noise
    // The rumble is updated in the NEXT frame's calculate_force call, or we can check m_debug_buffer
    // For simplicity in unit test, let's call calculate_force one more time with same data.
    engine.calculate_force(&data);

    std::vector<FFBSnapshot> batch = engine.GetDebugBatch();
    bool found_rumble = false;
    for(const auto& snap : batch) {
        if (std::abs(snap.texture_lockup) > 0.0001) found_rumble = true;
    }

    ASSERT_TRUE(found_rumble);
    std::cout << "[PASS] Predictive lockup active despite raw longitudinal acceleration noise" << std::endl;
}

} // namespace FFBEngineTests
