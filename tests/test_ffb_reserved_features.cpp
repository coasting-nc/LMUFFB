#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_understeer_affects_sop, "ReservedFeatures") {
    std::cout << "\nTest: Understeer Affects SoP (v0.7.18)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);

    // Setup: Enable SoP and Understeer Effect
    engine.m_sop_effect = 1.0f;
    engine.m_sop_scale = 1.0f;
    engine.m_understeer_effect = 1.0f;
    engine.m_gain = 1.0f;
    engine.m_max_torque_ref = 20.0f;

    // Disable other effects to isolate SoP
    engine.m_rear_align_effect = 0.0f;
    engine.m_oversteer_boost = 0.0f;
    engine.m_sop_yaw_gain = 0.0f;
    engine.m_gyro_gain = 0.0f;
    engine.m_steering_shaft_gain = 0.0f; // Mute base force

    // Input: High Lateral G (1G) and Zero Grip
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.5); // High slip angle
    data.mLocalAccel.x = 9.81; // 1G
    data.mWheel[0].mGripFract = 0.0; // Zero grip
    data.mWheel[1].mGripFract = 0.0;

    // Case 1: Feature Disabled (Default)
    engine.m_understeer_affects_sop = false;
    double force_disabled = 0.0;
    for (int i = 0; i < FILTER_SETTLING_FRAMES; i++) force_disabled = engine.calculate_force(&data);

    // Case 2: Feature Enabled
    // Reset engine to clear smoothing history for clean comparison
    FFBEngine engine2;
    InitializeEngine(engine2);
    engine2.m_sop_effect = 1.0f;
    engine2.m_sop_scale = 1.0f;
    engine2.m_understeer_effect = 1.0f;
    engine2.m_gain = 1.0f;
    engine2.m_max_torque_ref = 20.0f;
    engine2.m_steering_shaft_gain = 0.0f;
    engine2.m_understeer_affects_sop = true;
    double force_enabled = 0.0;
    for (int i = 0; i < FILTER_SETTLING_FRAMES; i++) force_enabled = engine2.calculate_force(&data);

    std::cout << "Force Disabled: " << force_disabled << ", Force Enabled: " << force_enabled << std::endl;

    // Grip factor should be ~0.2 (floor) when grip is 0 and understeer effect is 1.0
    // So SoP should be reduced significantly
    ASSERT_TRUE(std::abs(force_enabled) < std::abs(force_disabled) * 0.5f);
}

TEST_CASE(test_road_fallback_scale, "ReservedFeatures") {
    std::cout << "\nTest: Road Fallback Scale (v0.7.18)" << std::endl;

    // Setup for fallback: fast speed (>5.0) and zero deflection change
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    for (int i = 0; i < 4; i++) data.mWheel[i].mVerticalTireDeflection = 0.1f;
    // Initial mLocalAccel.y
    data.mLocalAccel.y = 10.0f;

    // Use high Max Torque Ref and LOW gain to avoid clipping at 1.0
    const float TEST_MAX_TORQUE = 100.0f;
    const float TEST_ROAD_GAIN = 0.1f;

    // Case 1: Low Sensitivity
    FFBEngine engine1;
    InitializeEngine(engine1);
    engine1.m_road_texture_enabled = true;
    engine1.m_road_texture_gain = TEST_ROAD_GAIN;
    engine1.m_gain = 1.0f;
    engine1.m_max_torque_ref = TEST_MAX_TORQUE;
    engine1.m_road_fallback_scale = 0.05f; // Default

    // Call once to settle deflection and acceleration state
    engine1.calculate_force(&data);

    // Now trigger acceleration change with NO deflection change
    data.mLocalAccel.y = 15.0f; // +5.0 delta
    double force_low = engine1.calculate_force(&data);

    // Case 2: High Sensitivity
    FFBEngine engine2;
    InitializeEngine(engine2);
    engine2.m_road_texture_enabled = true;
    engine2.m_road_texture_gain = TEST_ROAD_GAIN;
    engine2.m_gain = 1.0f;
    engine2.m_max_torque_ref = TEST_MAX_TORQUE;
    engine2.m_road_fallback_scale = 0.20f;

    // Call once to settle
    data.mLocalAccel.y = 10.0f;
    engine2.calculate_force(&data);

    // Trigger same acceleration change
    data.mLocalAccel.y = 15.0f; // +5.0 delta
    double force_high = engine2.calculate_force(&data);

    std::cout << "Force Low (0.05): " << force_low << ", Force High (0.20): " << force_high << std::endl;

    // Higher scale should produce higher force
    // 0.20 is 4x 0.05, so force should be roughly 4x
    ASSERT_TRUE(std::abs(force_high) > std::abs(force_low) * 2.0f);
    ASSERT_TRUE(std::abs(force_high) < 0.95f); // Ensure no clipping occurred
}

} // namespace FFBEngineTests
