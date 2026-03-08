#include "test_ffb_common.h"
#include <vector>
#include <string>

using namespace FFBEngineTests;

TEST_CASE_TAGGED(test_issue_213_lateral_load_additive, "CorePhysics", (std::vector<std::string>{"Physics", "Issue213"})) {
    FFBEngine engine;
    InitializeEngine(engine);

    // Setup both effects
    engine.m_sop_effect = 1.0f;       // Lateral G
    engine.m_lat_load_effect = 1.0f;  // Lateral Load
    engine.m_sop_scale = 1.0f;
    engine.m_wheelbase_max_nm = 20.0f;
    engine.m_target_rim_nm = 20.0f;

    // Force a car class to initialize load reference
    engine.calculate_force(nullptr, "GT3", "Test Car");

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);

    // Simulate Right Turn
    data.mLocalAccel.x = 9.81; // 1G Left (Right Turn)
    data.mWheel[0].mTireLoad = 6000.0; // FL
    data.mWheel[1].mTireLoad = 2000.0; // FR

    // Run several frames to overcome smoothing
    for (int i = 0; i < 50; i++) {
        engine.calculate_force(&data);
    }

    auto snapshots = engine.GetDebugBatch();
    ASSERT_FALSE(snapshots.empty());
    if (!snapshots.empty()) {
        // lat_g_accel = 1.0
        // lat_load_norm = (2000-6000)/8000 = -0.5 (Inverted in Issue #282)
        // sop_force (G-only) = 1.0 * 1.0 * 1.0 = 1.0 Nm
        // lat_load_force = -0.5 * 1.0 * 1.0 = -0.5 Nm
        std::cout << "[INFO] SoP Force (G): " << snapshots.back().sop_force << " | Lat Load: " << snapshots.back().lat_load_force << std::endl;
        ASSERT_NEAR(snapshots.back().sop_force, 1.0f, 0.1f);
        ASSERT_NEAR(snapshots.back().lat_load_force, -0.5f, 0.1f);
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
    engine.m_sop_effect = 1.0f;
    engine.m_lat_load_effect = 0.0f;
    for (int i = 0; i < 50; i++) engine.calculate_force(&data);
    float force_g = engine.GetDebugBatch().back().sop_force;

    // Case 2: ONLY Lateral Load
    engine.m_sop_effect = 0.0f;
    engine.m_lat_load_effect = 1.0f;
    for (int i = 0; i < 50; i++) engine.calculate_force(&data);
    auto snap = engine.GetDebugBatch().back();
    float force_g_none = snap.sop_force;
    float force_load = snap.lat_load_force;

    std::cout << "[INFO] Force G: " << force_g << " | Force Load: " << force_load << std::endl;

    ASSERT_NEAR(force_g, 1.0f, 0.1f);
    ASSERT_NEAR(force_g_none, 0.0f, 0.1f);
    ASSERT_NEAR(force_load, -0.5f, 0.1f); // Inverted in Issue #282
}

TEST_CASE_TAGGED(test_issue_213_lateral_load_kinematic, "CorePhysics", (std::vector<std::string>{"Physics", "Issue213"})) {
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_sop_effect = 0.0f;
    engine.m_lat_load_effect = 1.0f;
    engine.m_sop_scale = 1.0f;
    engine.calculate_force(nullptr, "GT3", "Test Car");

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);

    // Set direct load to zero to trigger fallback
    data.mWheel[0].mTireLoad = 0.0;
    data.mWheel[1].mTireLoad = 0.0;

    // Set 1G Left acceleration (Right Turn)
    data.mLocalAccel.x = 9.81;

    // Run enough frames to trigger MISSING_LOAD_WARN_THRESHOLD (20) and overcome smoothing
    for (int i = 0; i < 60; i++) {
        engine.calculate_force(&data);
    }

    auto snapshots = engine.GetDebugBatch();
    ASSERT_FALSE(snapshots.empty());
    if (!snapshots.empty()) {
        // With 1G Left acceleration, kinematic load should show FL > FR
        // lat_load_norm = (FR - FL) / total -> should be negative
        std::cout << "[INFO] Lat Load Force (Kinematic Fallback): " << snapshots.back().lat_load_force << std::endl;
        ASSERT_LT(snapshots.back().lat_load_force, -0.01f);
    }
}

TEST_CASE_TAGGED(test_issue_213_orientation_matrix, "CorePhysics", (std::vector<std::string>{"Physics", "Issue213", "Matrix"})) {
    std::cout << "\nTest: Orientation Matrix - Lateral Load & G-Force (Issue #213)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);

    // Enable both effects to ensure they pull together
    engine.m_sop_effect = 1.0f;
    engine.m_lat_load_effect = 1.0f;
    engine.m_sop_scale = 1.0f;
    engine.m_gain = 1.0f;
    engine.m_invert_force = true; // Match app default (Pull away from centripetal)

    // Scenario 1: Right Turn (Centrifugal force LEFT, Load shift LEFT)
    // Game: +X = Left (Centrifugal), FL > FR (Load)
    // Expected SoP (G): Positive (Internal)
    // Expected Lat Load: Negative (Internal, Inverted in #282)
    // Total FFB: Should still pull LEFT (Negative) because G (1.0) > Load (0.5)
    OrientationScenario right_turn = { 9.81, 6000.0, 2000.0, "Right Turn (1G Left Body Force)" };
    VerifyOrientation(engine, right_turn, 1.0f, -1.0f);

    // Scenario 2: Left Turn (Centrifugal force RIGHT, Load shift RIGHT)
    // Game: -X = Right (Centrifugal), FR > FL (Load)
    // Expected SoP (G): Negative (Internal)
    // Expected Lat Load: Positive (Internal, Inverted in #282)
    // Total FFB: Should still pull RIGHT (Positive)
    OrientationScenario left_turn = { -9.81, 2000.0, 6000.0, "Left Turn (1G Right Body Force)" };
    VerifyOrientation(engine, left_turn, -1.0f, 1.0f);
}
