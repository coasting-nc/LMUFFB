#include "test_ffb_common.h"

namespace FFBEngineTests {

/**
 * @brief Verification test for Issue #290.
 * Verifies that ABS Pulse and Lockup Vibration ARE working even when Vibration Strength is 0.
 */
TEST_CASE(test_issue_290_fix_verification, "Issue290") {
    FFBEngine engine;
    InitializeEngine(engine);

    // Set global vibration gain to zero
    engine.m_vibration_gain = 0.0f;

    // 1. Test ABS Pulse
    engine.m_braking.abs_pulse_enabled = true;
    engine.m_braking.abs_gain = 1.0f;

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = 0.01f;
    data.mElapsedTime = 0.01;
    data.mUnfilteredBrake = 1.0f;
    for(int i=0; i<4; i++) {
        data.mWheel[i].mBrakePressure = 1.0f;
    }

    // First call to prime previous state
    engine.calculate_force(&data);

    // Second call to trigger ABS (high delta)
    data.mElapsedTime += 0.01;
    for(int i=0; i<4; i++) data.mWheel[i].mBrakePressure = 0.7f;

    // Set other gains to 0 to isolate textures
    engine.m_general.gain = 1.0f;
    engine.m_front_axle.steering_shaft_gain = 0.0f;
    engine.m_rear_axle.sop_effect = 0.0f;
    engine.m_rear_axle.sop_yaw_gain = 0.0f;
    engine.m_rear_axle.rear_align_effect = 0.0f;
    engine.m_gyro_gain = 0.0f;
    engine.m_scrub_drag_gain = 0.0f;
    engine.m_soft_lock_enabled = false;
    engine.m_general.wheelbase_max_nm = 20.0f; engine.m_general.target_rim_nm = 20.0f;

    // Issue #397: Flush the 10ms transient ramp
    double force = 0.0;
    for(int i=0; i<4; i++) {
        force = engine.calculate_force(&data, nullptr, nullptr, 0.0f, true, 0.0025);
    }
    auto batch = engine.GetDebugBatch();

    ASSERT_GT(std::abs(batch.back().ffb_abs_pulse), 0.0f);
    ASSERT_GT(std::abs(batch.back().total_output), 0.0f);
    ASSERT_NEAR(batch.back().total_output, batch.back().ffb_abs_pulse / 20.0f, 0.001f);

    // 2. Test Lockup Vibration
    engine.m_braking.lockup_enabled = true;
    engine.m_braking.lockup_gain = 1.0f;
    engine.m_braking.lockup_start_pct = 5.0f;
    engine.m_braking.lockup_full_pct = 15.0f;

    // Trigger lockup: car speed 20, wheel slip high
    data.mLocalVel.z = 20.0f;
    data.mUnfilteredBrake = 1.0f;
    for(int i=0; i<4; i++) {
        data.mWheel[i].mLongitudinalGroundVel = 20.0;
        data.mWheel[i].mLongitudinalPatchVel = -10.0; // 50% slip
        data.mWheel[i].mSuspForce = 1000.0f; // Grounded
    }
    data.mElapsedTime += 0.01;
    // Issue #397: Flush the 10ms transient ramp
    for(int i=0; i<4; i++) {
        force = engine.calculate_force(&data, nullptr, nullptr, 0.0f, true, 0.0025);
    }
    batch = engine.GetDebugBatch();

    ASSERT_GT(std::abs(batch.back().texture_lockup), 0.0f);
    ASSERT_GT(std::abs(batch.back().total_output), 0.0f);

    // 3. Verify Road Texture is still muted
    engine.m_road_texture_enabled = true;
    engine.m_road_texture_gain = 1.0f;
    engine.m_braking.abs_pulse_enabled = false;
    engine.m_braking.lockup_enabled = false;

    data.mWheel[0].mVerticalTireDeflection += 0.02;
    data.mWheel[1].mVerticalTireDeflection += 0.02;
    data.mElapsedTime += 0.01;

    // Issue #397: Flush the 10ms transient ramp
    for(int i=0; i<4; i++) {
        force = engine.calculate_force(&data, nullptr, nullptr, 0.0f, true, 0.0025);
    }
    batch = engine.GetDebugBatch();

    ASSERT_GT(std::abs(batch.back().texture_road), 0.0f);
    ASSERT_EQ(batch.back().total_output, 0.0f);
}

} // namespace FFBEngineTests
