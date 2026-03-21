#include "test_ffb_common.h"
#include <vector>
#include <string>

using namespace FFBEngineTests;

TEST_CASE_TAGGED(test_issue_213_lateral_load_additive, "CorePhysics", (std::vector<std::string>{"Physics", "Issue213"})) {
    FFBEngine engine;
    InitializeEngine(engine);

    // Setup both effects
    engine.m_rear_axle.sop_effect = 1.0f;       // Lateral G
    engine.m_lat_load_effect = 1.0f;  // Lateral Load
    engine.m_rear_axle.sop_scale = 1.0f;
    engine.m_general.wheelbase_max_nm = 20.0f;
    engine.m_general.target_rim_nm = 20.0f;

    // Force a car class to initialize load reference
    engine.calculate_force(nullptr, "GT3", "Test Car");

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);

    // Simulate Right Turn
    data.mLocalAccel.x = 9.81; // 1G Left (Right Turn)
    data.mWheel[0].mTireLoad = 6000.0; // FL
    data.mWheel[1].mTireLoad = 2000.0; // FR
    data.mWheel[2].mTireLoad = 4000.0; // RL
    data.mWheel[3].mTireLoad = 4000.0; // RR

    // Run several frames to overcome smoothing
    for (int i = 0; i < 50; i++) {
        engine.calculate_force(&data);
    }

    auto snapshots = engine.GetDebugBatch();
    ASSERT_FALSE(snapshots.empty());
    if (!snapshots.empty()) {
        // lat_g_accel = 1.0
        // total_load = 16000. left_load = 6000+4000 = 10000. right_load = 2000+4000 = 6000.
        // lat_load_norm = (right_load - left_load)/total_load = (6000-10000)/16000 = -0.25 (Issue #306: Right - Left)
        // sop_force (G-only) = 1.0 * 1.0 * 1.0 = 1.0 Nm
        // lat_load_force = -0.25 * 1.0 * 1.0 = -0.25 Nm
        std::cout << "[INFO] SoP Force (G): " << snapshots.back().sop_force << " | Lat Load: " << snapshots.back().lat_load_force << std::endl;
        ASSERT_NEAR(snapshots.back().sop_force, 1.0f, 0.1f);
        ASSERT_NEAR(snapshots.back().lat_load_force, -0.25f, 0.1f);
    }
}

TEST_CASE_TAGGED(test_issue_213_lateral_load_isolation, "CorePhysics", (std::vector<std::string>{"Physics", "Issue213"})) {
    FFBEngine engine;
    InitializeEngine(engine);
    engine.calculate_force(nullptr, "GT3", "Test Car");

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mLocalAccel.x = 9.81;
    data.mWheel[0].mTireLoad = 6000.0;
    data.mWheel[1].mTireLoad = 2000.0;

    // Case 1: ONLY Lateral G
    engine.m_rear_axle.sop_effect = 1.0f;
    engine.m_lat_load_effect = 0.0f;
    for (int i = 0; i < 50; i++) engine.calculate_force(&data);
    float force_g = engine.GetDebugBatch().back().sop_force;

    // Case 2: ONLY Lateral Load
    engine.m_rear_axle.sop_effect = 0.0f;
    engine.m_lat_load_effect = 1.0f;
    data.mWheel[2].mTireLoad = 4000.0;
    data.mWheel[3].mTireLoad = 4000.0;
    for (int i = 0; i < 50; i++) engine.calculate_force(&data);
    auto snap = engine.GetDebugBatch().back();
    float force_g_none = snap.sop_force;
    float force_load = snap.lat_load_force;

    std::cout << "[INFO] Force G: " << force_g << " | Force Load: " << force_load << std::endl;

    ASSERT_NEAR(force_g, 1.0f, 0.1f);
    ASSERT_NEAR(force_g_none, 0.0f, 0.1f);
    ASSERT_NEAR(force_load, -0.25f, 0.1f); // Issue #306: (6000-10000)/16000 = -0.25
}

TEST_CASE_TAGGED(test_issue_213_lateral_load_suspension_fallback, "CorePhysics", (std::vector<std::string>{"Physics", "Issue213"})) {
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_rear_axle.sop_effect = 0.0f;
    engine.m_lat_load_effect = 1.0f;
    engine.m_rear_axle.sop_scale = 1.0f;
    engine.calculate_force(nullptr, "GT3", "Test Car");

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);

    // Set direct load to zero to trigger fallback
    for(int i=0; i<4; i++) data.mWheel[i].mTireLoad = 0.0;

    // Set asymmetric suspension force to simulate load transfer
    // Right Turn: Centrifugal pushed Left. Left gains load.
    data.mWheel[0].mSuspForce = 5000.0; // FL
    data.mWheel[1].mSuspForce = 1000.0; // FR
    data.mWheel[2].mSuspForce = 4000.0; // RL
    data.mWheel[3].mSuspForce = 2000.0; // RR

    // Run enough frames to trigger MISSING_LOAD_WARN_THRESHOLD (20) and overcome smoothing
    for (int i = 0; i < 60; i++) {
        data.mElapsedTime += 0.01;
        engine.calculate_force(&data);
    }

    auto snapshots = engine.GetDebugBatch();
    ASSERT_FALSE(snapshots.empty());
    if (!snapshots.empty()) {
        // Loads: FL=5300, FR=1300, RL=4300, RR=2300.
        // Left=9600, Right=3600. Total=13200.
        // norm = (3600 - 9600) / 13200 = -6000 / 13200 approx -0.45
        std::cout << "[INFO] Lat Load Force (Suspension Fallback): " << snapshots.back().lat_load_force << std::endl;
        ASSERT_LT(snapshots.back().lat_load_force, -0.1f);
    }
}

TEST_CASE_TAGGED(test_issue_213_orientation_matrix, "CorePhysics", (std::vector<std::string>{"Physics", "Issue213", "Matrix"})) {
    std::cout << "\nTest: Orientation Matrix - Lateral Load & G-Force (Issue #213)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);

    // Enable both effects to ensure they pull together
    engine.m_rear_axle.sop_effect = 1.0f;
    engine.m_lat_load_effect = 1.0f;
    engine.m_rear_axle.sop_scale = 1.0f;
    engine.m_general.gain = 1.0f;
    engine.m_invert_force = true; // Match app default (Pull away from centripetal)

    // Scenario 1: Right Turn (Centrifugal force LEFT, Load shift LEFT)
    // Game: +X = Left (Centrifugal), Left side > Right side (Load)
    // Expected SoP (G): Positive (Internal)
    // Expected Lat Load: Negative (Internal, Issue #306: Right - Left)
    // Total FFB: With m_invert_force=true, positive structural sum becomes NEGATIVE output.
    OrientationScenario right_turn = { 9.81, 6000.0, 2000.0, "Right Turn (1G Left Body Force)" };
    VerifyOrientation(engine, right_turn, 1.0f, -1.0f);

    // Scenario 2: Left Turn (Centrifugal force RIGHT, Load shift RIGHT)
    // Game: -X = Right (Centrifugal), Right side > Left side (Load)
    // Expected SoP (G): Negative (Internal)
    // Expected Lat Load: Positive (Internal)
    // Total FFB: With m_invert_force=true, negative structural sum becomes POSITIVE output.
    OrientationScenario left_turn = { -9.81, 2000.0, 6000.0, "Left Turn (1G Right Body Force)" };
    VerifyOrientation(engine, left_turn, -1.0f, 1.0f);
}
