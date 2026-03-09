#include "test_ffb_common.h"
#include <vector>
#include <string>
#include <cmath>

using namespace FFBEngineTests;

TEST_CASE_TAGGED(test_sop_load_fallback_hierarchy, "CorePhysics", (std::vector<std::string>{"Physics", "Issue309"})) {
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_lat_load_effect = 1.0f;
    engine.m_sop_scale = 1.0f;
    engine.m_sop_smoothing_factor = 0.0f;
    engine.calculate_force(nullptr, "GT3", "Test Car");

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mLocalAccel.x = 0.0;

    // Trigger missing load warning (need > 20 frames)
    for (int i = 0; i < 30; i++) {
        data.mWheel[0].mTireLoad = 0.0;
        data.mWheel[1].mTireLoad = 0.0;
        data.mWheel[2].mTireLoad = 0.0;
        data.mWheel[3].mTireLoad = 0.0;
        data.mElapsedTime += 0.01f;
        engine.calculate_force(&data);
    }

    // Case 1: mTireLoad missing, but mSuspForce present
    // approximate_load = suspForce + 300
    data.mWheel[0].mSuspForce = 5000.0; // Left
    data.mWheel[1].mSuspForce = 1000.0; // Right
    data.mWheel[2].mSuspForce = 5000.0; // Left
    data.mWheel[3].mSuspForce = 1000.0; // Right

    // total = (5300 * 2) + (1300 * 2) = 10600 + 2600 = 13200
    // L-R = (5300 * 2) - (1300 * 2) = 10600 - 2600 = 8000
    // norm = 8000 / 13200 approx 0.606

    data.mElapsedTime += 0.01f;
    engine.calculate_force(&data);
    auto snap = engine.GetDebugBatch().back();
    std::cout << "[INFO] Hierarchy Case 1 (Approximate): " << snap.lat_load_force << std::endl;
    ASSERT_NEAR(snap.lat_load_force, 0.606f, 0.01f);

    // Case 2: Both missing (SuspForce < MIN_VALID_SUSP_FORCE)
    // fallback to kinematic. At 0 accel, it should be static weight (centered)
    data.mWheel[0].mSuspForce = 0.0;
    data.mWheel[1].mSuspForce = 0.0;
    data.mWheel[2].mSuspForce = 0.0;
    data.mWheel[3].mSuspForce = 0.0;

    data.mElapsedTime += 0.01f;
    engine.calculate_force(&data);
    snap = engine.GetDebugBatch().back();
    std::cout << "[INFO] Hierarchy Case 2 (Kinematic): " << snap.lat_load_force << std::endl;
    // At zero accel, kinematic should be perfectly balanced -> norm = 0
    ASSERT_NEAR(snap.lat_load_force, 0.0f, 0.01f);
}

TEST_CASE_TAGGED(test_sop_load_4wheel_contribution, "CorePhysics", (std::vector<std::string>{"Physics", "Issue309"})) {
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_lat_load_effect = 1.0f;
    engine.m_sop_scale = 1.0f;
    engine.m_sop_smoothing_factor = 0.0f;
    engine.calculate_force(nullptr, "GT3", "Test Car");

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mLocalAccel.x = 0.0;

    // Direct load valid
    data.mWheel[0].mTireLoad = 2000.0; // FL
    data.mWheel[1].mTireLoad = 2000.0; // FR
    data.mWheel[2].mTireLoad = 2000.0; // RL
    data.mWheel[3].mTireLoad = 2000.0; // RR

    data.mElapsedTime += 0.01f;
    engine.calculate_force(&data);
    float force1 = engine.GetDebugBatch().back().lat_load_force;
    ASSERT_NEAR(force1, 0.0f, 0.01f);

    // Change REAR wheel loads only
    data.mWheel[2].mTireLoad = 4000.0; // RL (Left)
    data.mWheel[3].mTireLoad = 1000.0; // RR (Right)

    // total = 2000+2000+4000+1000 = 9000
    // L-R = (2000+4000) - (2000+1000) = 6000 - 3000 = 3000
    // norm = 3000 / 9000 = 0.333

    data.mElapsedTime += 0.01f;
    engine.calculate_force(&data);
    float force2 = engine.GetDebugBatch().back().lat_load_force;
    std::cout << "[INFO] Rear contribution force: " << force2 << std::endl;
    ASSERT_NEAR(force2, 0.333f, 0.01f);
}

TEST_CASE_TAGGED(test_sop_load_sign_convention, "CorePhysics", (std::vector<std::string>{"Physics", "Issue309"})) {
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_sop_effect = 1.0f;
    engine.m_lat_load_effect = 1.0f;
    engine.m_sop_scale = 1.0f;
    engine.m_sop_smoothing_factor = 0.0f;
    engine.calculate_force(nullptr, "GT3", "Test Car");

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);

    // Scenario: Right Turn
    // G-force LEFT (+X)
    data.mLocalAccel.x = 9.81;

    // Outside tires (Left) GAIN load
    data.mWheel[0].mTireLoad = 6000.0; // FL
    data.mWheel[1].mTireLoad = 2000.0; // FR
    data.mWheel[2].mTireLoad = 6000.0; // RL
    data.mWheel[3].mTireLoad = 2000.0; // RR

    data.mElapsedTime += 0.01f;
    engine.calculate_force(&data);
    auto snap = engine.GetDebugBatch().back();

    std::cout << "[INFO] Right Turn - SoP Force (G): " << snap.sop_force << " | Lat Load Force: " << snap.lat_load_force << std::endl;

    // G is +1.0 -> sop_force should be +1.0
    // Load (L-R) = (12000 - 4000) / 16000 = 0.5 -> lat_load_force should be +0.5
    // BOTH SHOULD BE POSITIVE (Additive)
    ASSERT_GT(snap.sop_force, 0.0f);
    ASSERT_GT(snap.lat_load_force, 0.0f);
    ASSERT_NEAR(snap.sop_force, 1.0f, 0.01f);
    ASSERT_NEAR(snap.lat_load_force, 0.5f, 0.01f);
}
