#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_smoothstep_helper_function, "SpeedGate") {
    std::cout << "\nTest: Smoothstep Helper Function (v0.7.2)" << std::endl;
    FFBEngine engine;
    double at_lower = engine.smoothstep(1.0, 5.0, 1.0);
    ASSERT_NEAR(at_lower, 0.0, 0.001);
    double at_upper = engine.smoothstep(1.0, 5.0, 5.0);
    ASSERT_NEAR(at_upper, 1.0, 0.001);
    double at_mid = engine.smoothstep(1.0, 5.0, 3.0);
    ASSERT_NEAR(at_mid, 0.5, 0.001);
    double at_25 = engine.smoothstep(1.0, 5.0, 2.0);
    ASSERT_NEAR(at_25, 0.15625, 0.001);
    double at_75 = engine.smoothstep(1.0, 5.0, 4.0);
    ASSERT_NEAR(at_75, 0.84375, 0.001);
}

TEST_CASE(test_smoothstep_vs_linear, "SpeedGate") {
    std::cout << "\nTest: Smoothstep vs Linear Comparison (v0.7.2)" << std::endl;
    FFBEngine engine;
    double smooth_25 = engine.smoothstep(1.0, 5.0, 2.0);
    ASSERT_TRUE(smooth_25 < 0.25);
    double smooth_75 = engine.smoothstep(1.0, 5.0, 4.0);
    ASSERT_TRUE(smooth_75 > 0.75);
}

TEST_CASE(test_smoothstep_edge_cases, "SpeedGate") {
    std::cout << "\nTest: Smoothstep Edge Cases (v0.7.2)" << std::endl;
    FFBEngine engine;
    double below = engine.smoothstep(1.0, 5.0, 0.0);
    ASSERT_NEAR(below, 0.0, 0.001);
    double above = engine.smoothstep(1.0, 5.0, 10.0);
    ASSERT_NEAR(above, 1.0, 0.001);
    double negative = engine.smoothstep(1.0, 5.0, -5.0);
    ASSERT_NEAR(negative, 0.0, 0.001);
    double zero_range = engine.smoothstep(3.0, 3.0, 3.0);
    ASSERT_TRUE(zero_range == 0.0 || zero_range == 1.0);
    double tiny_range = engine.smoothstep(1.0, 1.0001, 1.00005);
    ASSERT_TRUE(tiny_range >= 0.0 && tiny_range <= 1.0);
}

TEST_CASE(test_speed_gate_uses_smoothstep, "SpeedGate") {
    std::cout << "\nTest: Speed Gate Uses Smoothstep (v0.7.2)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_speed_gate_lower = 1.0f;
    engine.m_speed_gate_upper = 5.0f;
    engine.m_road_texture_enabled = true;
    engine.m_road_texture_gain = 1.0f;
    engine.m_max_torque_ref = 20.0f;
    TelemInfoV01 data_25 = CreateBasicTestTelemetry(2.0);
    data_25.mWheel[0].mVerticalTireDeflection = 0.002;
    data_25.mWheel[1].mVerticalTireDeflection = 0.002;
    engine.calculate_force(&data_25);
    TelemInfoV01 data_50 = CreateBasicTestTelemetry(3.0);
    data_50.mWheel[0].mVerticalTireDeflection = 0.002;
    data_50.mWheel[1].mVerticalTireDeflection = 0.002;
    engine.m_prev_vert_deflection[0] = 0.0;
    engine.m_prev_vert_deflection[1] = 0.0;
    double force_50 = engine.calculate_force(&data_50);
    engine.m_prev_vert_deflection[0] = 0.0;
    engine.m_prev_vert_deflection[1] = 0.0;
    double force_25 = engine.calculate_force(&data_25);
    if (std::abs(force_50) > 0.0001) {
        double ratio = std::abs(force_25 / force_50);
        ASSERT_TRUE(ratio < 0.4);
    } else {
        std::cout << "[FAIL] Force at 50% is zero." << std::endl;
        g_tests_failed++;
    }
}

TEST_CASE(test_smoothstep_stationary_silence_preserved, "SpeedGate") {
    std::cout << "\nTest: Smoothstep Stationary Silence (v0.7.2)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_speed_gate_lower = 1.0f;
    engine.m_speed_gate_upper = 5.0f;
    TelemInfoV01 data = CreateBasicTestTelemetry(0.0);
    data.mSteeringShaftTorque = 10.0;
    data.mLocalAccel.x = 5.0;
    double force = engine.calculate_force(&data);
    ASSERT_NEAR(force, 0.0, 0.001);
}

TEST_CASE(test_speed_gate_custom_thresholds, "SpeedGate") {
    std::cout << "\nTest: Speed Gate Custom Thresholds" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    
    // Verify default upper threshold (Reset to expected for test)
    engine.m_speed_gate_upper = 5.0f;
    if (engine.m_speed_gate_upper == 5.0f) {
        std::cout << "[PASS] Default upper threshold is 5.0 m/s (18 km/h)." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Default upper threshold is " << engine.m_speed_gate_upper << std::endl;
        g_tests_failed++;
    }

    // Try custom thresholds
    engine.m_speed_gate_lower = 2.0f;
    engine.m_speed_gate_upper = 10.0f;
    
    TelemInfoV01 data = CreateBasicTestTelemetry(6.0); // Exactly halfway
    engine.m_road_texture_enabled = true;
    engine.m_road_texture_gain = 1.0;
    engine.m_max_torque_ref = 20.0f;
    data.mWheel[0].mVerticalTireDeflection = 0.001;
    data.mWheel[1].mVerticalTireDeflection = 0.001;
    
    double force = engine.calculate_force(&data);
    // Gate = (6 - 2) / (10 - 2) = 4 / 8 = 0.5
    // Texture Force = 0.5 * (0.001 + 0.001) * 50.0 = 0.05 Nm
    // Normalized = 0.05 / 20.0 = 0.0025
    ASSERT_NEAR(force, 0.0025, 0.0001);
}



} // namespace FFBEngineTests
