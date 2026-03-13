#include "test_ffb_common.h"
#include <vector>
#include <string>
#include <cmath>

using namespace FFBEngineTests;

TEST_CASE_TAGGED(test_issue_306_4_wheel_lateral_load, "CorePhysics", (std::vector<std::string>{"Physics", "Issue306"})) {
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_sop_effect = 0.0f;
    engine.m_lat_load_effect = 1.0f;
    engine.m_sop_scale = 1.0f;
    engine.m_sop_smoothing_factor = 0.0f;

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mLocalAccel.x = 0.0;

    // Baseline: All wheels equal load
    for(int i=0; i<4; i++) data.mWheel[i].mTireLoad = 2000.0;

    engine.calculate_force(&data);
    auto snap = engine.GetDebugBatch().back();
    ASSERT_NEAR(snap.lat_load_force, 0.0f, 0.001f);

    // Test 1: Only front wheels asymmetric
    // Left = 3000 (FL), Right = 1000 (FR). Rears = 2000.
    // Total = 8000. Left = 3000 + 2000 = 5000. Right = 1000 + 2000 = 3000.
    // lat_load_norm = (3000 - 5000) / 8000 = -0.25
    data.mWheel[0].mTireLoad = 3000.0; // FL
    data.mWheel[1].mTireLoad = 1000.0; // FR
    data.mWheel[2].mTireLoad = 2000.0; // RL
    data.mWheel[3].mTireLoad = 2000.0; // RR

    engine.calculate_force(&data);
    snap = engine.GetDebugBatch().back();
    ASSERT_NEAR(snap.lat_load_force, -0.25f, 0.01f);

    // Test 2: Only rear wheels asymmetric (Proves Issue #306 fix)
    // Fronts = 2000. Left = 2000 + 3000 = 5000. Right = 2000 + 1000 = 3000.
    // lat_load_norm = (3000 - 5000) / 8000 = -0.25
    data.mWheel[0].mTireLoad = 2000.0;
    data.mWheel[1].mTireLoad = 2000.0;
    data.mWheel[2].mTireLoad = 3000.0; // RL
    data.mWheel[3].mTireLoad = 1000.0; // RR

    engine.calculate_force(&data);
    snap = engine.GetDebugBatch().back();
    ASSERT_NEAR(snap.lat_load_force, -0.25f, 0.01f);

    // Test 3: All wheels asymmetric
    // Left = 3000 + 3000 = 6000. Right = 1000 + 1000 = 2000. Total = 8000.
    // lat_load_norm = (2000 - 6000) / 8000 = -0.5
    data.mWheel[0].mTireLoad = 3000.0;
    data.mWheel[1].mTireLoad = 1000.0;
    data.mWheel[2].mTireLoad = 3000.0;
    data.mWheel[3].mTireLoad = 1000.0;

    engine.calculate_force(&data);
    snap = engine.GetDebugBatch().back();
    ASSERT_NEAR(snap.lat_load_force, -0.5f, 0.01f);
}

TEST_CASE_TAGGED(test_issue_306_sign_convention, "CorePhysics", (std::vector<std::string>{"Physics", "Issue306"})) {
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_sop_effect = 1.0f;
    engine.m_lat_load_effect = 1.0f;
    engine.m_sop_scale = 1.0f;
    engine.m_sop_smoothing_factor = 0.0f;

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);

    // Right Turn: Centrifugal force LEFT (+X), Load shift LEFT (FL > FR, RL > RR)
    data.mLocalAccel.x = 9.81;
    data.mWheel[0].mTireLoad = 3000.0; // FL
    data.mWheel[1].mTireLoad = 1000.0; // FR
    data.mWheel[2].mTireLoad = 3000.0; // RL
    data.mWheel[3].mTireLoad = 1000.0; // RR

    engine.calculate_force(&data);
    auto snap = engine.GetDebugBatch().back();

    // Lat G accel = 1.0 -> sop_force (G-based) = 1.0
    // Lat Load norm = (2000 - 6000) / 8000 = -0.5 -> lat_load_force = -0.5
    // They should now have OPPOSITE signs (Issue #306: fixed inverted feel).
    std::cout << "[INFO] SoP Force (G): " << snap.sop_force << " | Lat Load Force: " << snap.lat_load_force << std::endl;
    ASSERT_GT(snap.sop_force, 0.0f);
    ASSERT_LT(snap.lat_load_force, 0.0f);
}

TEST_CASE_TAGGED(test_issue_306_scrub_drag_scaling, "CorePhysics", (std::vector<std::string>{"Physics", "Issue306"})) {
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_scrub_drag_gain = 1.0f;
    engine.m_auto_load_normalization_enabled = false;
    engine.m_static_front_load = 4000.0; // Static front load (per axle)

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mWheel[0].mLateralPatchVel = 1.0; // Scrubbing
    data.mWheel[1].mLateralPatchVel = 1.0;

    // Case 1: Standard load (4000N per axle) -> texture_load_factor approx 1.0
    data.mWheel[0].mTireLoad = 2000.0;
    data.mWheel[1].mTireLoad = 2000.0;

    engine.calculate_force(&data);
    float scrub_1 = engine.GetDebugBatch().back().ffb_scrub_drag;

    // Case 2: High load (8000N per axle) -> texture_load_factor > 1.0
    data.mWheel[0].mTireLoad = 4000.0;
    data.mWheel[1].mTireLoad = 4000.0;

    engine.calculate_force(&data);
    float scrub_2 = engine.GetDebugBatch().back().ffb_scrub_drag;

    std::cout << "[INFO] Scrub Drag (Low Load): " << scrub_1 << " | Scrub Drag (High Load): " << scrub_2 << std::endl;
    ASSERT_GT(std::abs(scrub_2), std::abs(scrub_1));
}

TEST_CASE_TAGGED(test_issue_306_wheel_spin_scaling, "CorePhysics", (std::vector<std::string>{"Physics", "Issue306"})) {
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_spin_enabled = true;
    engine.m_spin_gain = 1.0f;
    engine.m_static_front_load = 4000.0;

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mUnfilteredThrottle = 1.0;
    data.mWheel[2].mLongitudinalPatchVel = 10.0; // RL Spinning
    data.mWheel[3].mLongitudinalPatchVel = 10.0; // RR Spinning

    // Case 1: Low rear load
    data.mWheel[2].mTireLoad = 500.0;
    data.mWheel[3].mTireLoad = 500.0;

    engine.calculate_force(&data);

    float max_spin_1 = 0.0f;
    for(int i=0; i<20; i++) {
        data.mElapsedTime += 0.0025;
        engine.calculate_force(&data);
        max_spin_1 = std::max(max_spin_1, std::abs(engine.GetDebugBatch().back().texture_spin));
    }

    // Case 2: High rear load
    data.mWheel[2].mTireLoad = 4000.0;
    data.mWheel[3].mTireLoad = 4000.0;

    float max_spin_2 = 0.0f;
    for(int i=0; i<20; i++) {
        data.mElapsedTime += 0.0025;
        engine.calculate_force(&data);
        max_spin_2 = std::max(max_spin_2, std::abs(engine.GetDebugBatch().back().texture_spin));
    }

    std::cout << "[INFO] Max Spin Vibration (Low Load): " << max_spin_1 << " | Max Spin Vibration (High Load): " << max_spin_2 << std::endl;
    ASSERT_GT(max_spin_2, max_spin_1);
}

TEST_CASE_TAGGED(test_issue_306_suspension_fallback_4_wheel, "CorePhysics", (std::vector<std::string>{"Physics", "Issue306"})) {
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_sop_effect = 0.0f;
    engine.m_lat_load_effect = 1.0f;
    engine.m_sop_scale = 1.0f;
    engine.m_sop_smoothing_factor = 0.0f;

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    // Force missing load warning
    for(int i=0; i<4; i++) data.mWheel[i].mTireLoad = 0.0;

    // Set asymmetric suspension forces
    data.mWheel[0].mSuspForce = 5000.0; // FL
    data.mWheel[1].mSuspForce = 1000.0; // FR
    data.mWheel[2].mSuspForce = 4000.0; // RL
    data.mWheel[3].mSuspForce = 2000.0; // RR

    // We need to trigger the warning threshold
    for(int i=0; i<60; i++) {
        data.mElapsedTime += 0.01;
        engine.calculate_force(&data);
    }

    auto snap = engine.GetDebugBatch().back();
    ASSERT_TRUE(snap.warn_load);
    ASSERT_GT(std::abs(snap.lat_load_force), 0.01f);

    // centrifugal left -> left gain load -> positive Internal -> Negative output (Right-Left)
    ASSERT_LT(snap.lat_load_force, 0.0f);
}
