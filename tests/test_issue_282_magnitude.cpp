#include "test_ffb_common.h"
#include <vector>
#include <string>

using namespace FFBEngineTests;

TEST_CASE_TAGGED(test_issue_282_aero_fade_resistance, "CorePhysics", (std::vector<std::string>{"Physics", "Issue282"})) {
    FFBEngine engine;
    InitializeEngine(engine);

    // Setup: Pure Lateral Load effect
    engine.m_sop_effect = 0.0f;
    engine.m_lat_load_effect = 1.0f;
    engine.m_sop_scale = 1.0f;

    // Use LMP2 Restricted (7500N axle basis)
    TelemInfoV01 init_data = CreateBasicTestTelemetry(10.0, 0.0);
    engine.calculate_force(&init_data, "LMP2", "Oreca 07");

    // Set 1G Left acceleration (Right Turn)
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mLocalAccel.x = 9.81;

    // Encrypted telemetry simulation: zero tire loads
    for(int i=0; i<4; i++) data.mWheel[i].mTireLoad = 0.0;

    // 1. Low Speed Case (20 m/s = 72 km/h)
    for (int i = 0; i < 60; i++) engine.calculate_force(&data);
    float force_low_speed = engine.GetDebugBatch().back().sop_force;

    // 2. High Speed Case (83.33 m/s = 300 km/h)
    data.mLocalVel.z = -83.33;
    // Run enough frames to settle upsamplers and smoothing
    for (int i = 0; i < 100; i++) engine.calculate_force(&data);
    float force_high_speed = engine.GetDebugBatch().back().sop_force;

    std::cout << "[INFO] Low Speed Force (72 km/h): " << force_low_speed << std::endl;
    std::cout << "[INFO] High Speed Force (300 km/h): " << force_high_speed << std::endl;

    // With fixed static normalization, the force should NOT drop at high speed.
    // In fact, it might increase slightly due to kinematic coupling or upsampling transients,
    // but definitely should not shrink to 25% of low-speed value.
    ASSERT_NEAR(force_low_speed, force_high_speed, 0.05f);
    ASSERT_GT(force_high_speed, 0.5f); // 2400 / 7500 * 2.0 = 0.64
}

TEST_CASE_TAGGED(test_issue_282_magnitude_parity, "CorePhysics", (std::vector<std::string>{"Physics", "Issue282"})) {
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 init_data = CreateBasicTestTelemetry(10.0, 0.0);
    // GT3 -> 4800N axle basis. Static front load per wheel = 2400N.
    engine.calculate_force(&init_data, "GT3", "Test GT3");

    // Create 1G turn scenario
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mLocalAccel.x = 9.81; // 1G
    // Simulate 1G weight transfer: 100% shift (L=4800, R=0) -> L-R = 4800
    data.mWheel[0].mTireLoad = 4800.0;
    data.mWheel[1].mTireLoad = 0.0;

    // 1. Lateral G Magnitude
    engine.m_sop_effect = 1.0f;
    engine.m_lat_load_effect = 0.0f;
    for (int i = 0; i < 60; i++) engine.calculate_force(&data);
    float force_g = engine.GetDebugBatch().back().sop_force;

    // 2. Lateral Load Magnitude (with 2.0x boost)
    engine.m_sop_effect = 0.0f;
    engine.m_lat_load_effect = 1.0f;
    // New Math: (4800-0) / 4800 * 2.0 boost = 2.0 Nm
    // Lateral G: 1.0G * 1.0 effect = 1.0 Nm
    // With 2.0x boost, Load is now STRONGER than G at 100% transfer.
    for (int i = 0; i < 60; i++) engine.calculate_force(&data);
    float force_load = engine.GetDebugBatch().back().sop_force;

    std::cout << "[INFO] G Magnitude: " << force_g << " | Load Magnitude: " << force_load << std::endl;

    // Verify parity: force_load should be exactly 2.0
    ASSERT_NEAR(force_load, 2.0f, 0.1f);
}

TEST_CASE_TAGGED(test_issue_282_immediate_fallback, "CorePhysics", (std::vector<std::string>{"Physics", "Issue282"})) {
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_lat_load_effect = 1.0f;

    // Simulate car moving at speed with 0 tire load (encrypted signature)
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mLocalAccel.x = 9.81;
    for(int i=0; i<4; i++) data.mWheel[i].mTireLoad = 0.0;

    // First frame
    engine.calculate_force(&data, "LMP2", "Oreca 07");

    // Second frame - should already have non-zero force due to aggressive pivot
    engine.calculate_force(&data);
    float force = engine.GetDebugBatch().back().sop_force;

    std::cout << "[INFO] Immediate Fallback Force: " << force << std::endl;
    ASSERT_GT(force, 0.1f);
}
